/*!
*	@file main.c
*
*	@date 2024
* @author Bulut Bekdemir
* 
* @copyright BSD 3-Clause License
* @version 0.1.0+2
*/
#include <stdio.h>
#include <inttypes.h>

#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_chip_info.h"
#include "esp_flash.h"
#include "esp_system.h"
#include "nvs_flash.h"
#include "esp_log.h"
#include "esp_http_server.h"

#include "wifiManager.h"
#include "esp_camera.h"
#include "camera_app.h"
#include "mqtt_app.h"

#define CAMERA_MODEL_AI_THINKER // หรือกำหนดตามบอร์ดของคุณ

// กำหนดขา GPIO สำหรับกล้อง OV2640 (ตัวอย่างสำหรับ AI-THINKER)
#define PWDN_GPIO_NUM     32
#define RESET_GPIO_NUM    -1
#define XCLK_GPIO_NUM      0
#define SIOD_GPIO_NUM     26
#define SIOC_GPIO_NUM     27

#define Y9_GPIO_NUM       35
#define Y8_GPIO_NUM       34
#define Y7_GPIO_NUM       39
#define Y6_GPIO_NUM       36
#define Y5_GPIO_NUM       21
#define Y4_GPIO_NUM       19
#define Y3_GPIO_NUM       18
#define Y2_GPIO_NUM        5
#define VSYNC_GPIO_NUM    25
#define HREF_GPIO_NUM     23
#define PCLK_GPIO_NUM     22

static const char *TAG = "main";

void start_camera()
{
    camera_config_t config = {
        .pin_pwdn  = PWDN_GPIO_NUM,
        .pin_reset = RESET_GPIO_NUM,
        .pin_xclk = XCLK_GPIO_NUM,
        .pin_sscb_sda = SIOD_GPIO_NUM,
        .pin_sscb_scl = SIOC_GPIO_NUM,

        .pin_d7 = Y9_GPIO_NUM,
        .pin_d6 = Y8_GPIO_NUM,
        .pin_d5 = Y7_GPIO_NUM,
        .pin_d4 = Y6_GPIO_NUM,
        .pin_d3 = Y5_GPIO_NUM,
        .pin_d2 = Y4_GPIO_NUM,
        .pin_d1 = Y3_GPIO_NUM,
        .pin_d0 = Y2_GPIO_NUM,
        .pin_vsync = VSYNC_GPIO_NUM,
        .pin_href = HREF_GPIO_NUM,
        .pin_pclk = PCLK_GPIO_NUM,

        .xclk_freq_hz = 20000000,
        .ledc_timer = LEDC_TIMER_0,
        .ledc_channel = LEDC_CHANNEL_0,
        .pixel_format = PIXFORMAT_JPEG,
        .frame_size = FRAMESIZE_QVGA,
        .jpeg_quality = 12,
        .fb_count = 1,
    };

    // เริ่มต้นกล้อง
    esp_err_t err = esp_camera_init(&config);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Camera init failed with error 0x%x", err);
        return;
    }
    ESP_LOGI(TAG, "Camera init success");
}

// MJPEG Streaming Handler
static esp_err_t stream_handler(httpd_req_t *req) {
    camera_fb_t *fb = NULL;
    char buf[64];
    static const char* _STREAM_CONTENT_TYPE = "multipart/x-mixed-replace;boundary=frame";
    static const char* _STREAM_BOUNDARY = "\r\n--frame\r\n";
    static const char* _STREAM_PART = "Content-Type: image/jpeg\r\nContent-Length: %u\r\n\r\n";

    httpd_resp_set_type(req, _STREAM_CONTENT_TYPE);

    while (true) {
        fb = esp_camera_fb_get();
        if (!fb) {
            ESP_LOGE(TAG, "Camera capture failed");
            continue;
        }
        size_t hlen = snprintf(buf, sizeof(buf), _STREAM_PART, fb->len);
        if (httpd_resp_send_chunk(req, _STREAM_BOUNDARY, strlen(_STREAM_BOUNDARY)) != ESP_OK ||
            httpd_resp_send_chunk(req, buf, hlen) != ESP_OK ||
            httpd_resp_send_chunk(req, (const char *)fb->buf, fb->len) != ESP_OK) {
            esp_camera_fb_return(fb);
            break;
        }
        esp_camera_fb_return(fb);
        vTaskDelay(30 / portTICK_PERIOD_MS); // ปรับ frame rate ตามต้องการ
    }
    return ESP_OK;
}

// Start Webserver สำหรับ stream
static void start_stream_webserver(void) {
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.server_port = 8080; // เปลี่ยน port ได้ตามต้องการ
    httpd_handle_t server = NULL;
    if (httpd_start(&server, &config) == ESP_OK) {
        httpd_uri_t stream_uri = {
            .uri       = "/stream",
            .method    = HTTP_GET,
            .handler   = stream_handler,
            .user_ctx  = NULL
        };
        httpd_register_uri_handler(server, &stream_uri);
        ESP_LOGI(TAG, "MJPEG stream ready at http://<ip>:%d/stream", config.server_port);
    } else {
        ESP_LOGE(TAG, "Failed to start webserver");
    }
}

// Task สำหรับส่งภาพผ่าน MQTT ทุก 10 วินาที
void mqtt_image_task(void *pvParameter) {
    while (1) {
        camera_fb_t *fb = esp_camera_fb_get();
        if (fb) {
            mqtt_send_image(fb->buf, fb->len);
            esp_camera_fb_return(fb);
        }
        vTaskDelay(10000 / portTICK_PERIOD_MS); // 10 วินาที
    }
}

void app_main(void)
{
  ///> Initialize NVS
	esp_err_t ret = nvs_flash_init();
	if(ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND)
	{
		ESP_ERROR_CHECK(nvs_flash_erase());
		ret = nvs_flash_init();
	}
	ESP_ERROR_CHECK(ret);

	///> Get the chip information
	esp_chip_info_t chip_info;
	esp_chip_info(&chip_info);	
	ESP_LOGI("MAIN", "ESP32 Chip Rev: %d", chip_info.revision);
	ESP_LOGI("MAIN", "ESP32 Chip Cores: %d", chip_info.cores);
	ESP_LOGI("MAIN", "ESP32 Chip Features: %ld", chip_info.features);
	ESP_LOGI("MAIN", "ESP32 Chip Revision: %d", chip_info.revision);
	ESP_LOGI("MAIN", "ESP32 Chip Cores: %d", chip_info.cores);

	///> Initialize the wifi application
	wifiManager_init();

	if (camera_init() == ESP_OK) {
		start_camera_stream_server();
		mqtt_app_start();
		xTaskCreate(&mqtt_image_task, "mqtt_image_task", 4096, NULL, 5, NULL);
	}
}