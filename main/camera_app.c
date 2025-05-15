#include "esp_camera.h"
#include "esp_log.h"
#include "esp_system.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "camera_app.h"
#include "esp_http_server.h"

#ifdef CONFIG_CAMERA_SENSOR_OV5640
#include "ov5640.h"
#elif defined(CONFIG_CAMERA_SENSOR_OV2640)
#include "ov2640.h"
#endif

static const char *TAG = "CAMERA_APP";
static httpd_handle_t stream_httpd = NULL;

// ตัวแปรเก็บสถานะการพลิกภาพ
static bool vflip_enabled = false;
static bool hmirror_enabled = false;

// ตั้งค่ากล้องสำหรับ ESP32-S3-EYE
static camera_config_t camera_config = {
    .pin_pwdn = -1,
    .pin_reset = -1,
    .pin_xclk = 15,
    .pin_sscb_sda = 4,
    .pin_sscb_scl = 5,
    .pin_d7 = 16,
    .pin_d6 = 17,
    .pin_d5 = 18,
    .pin_d4 = 12,
    .pin_d3 = 10,
    .pin_d2 = 8,
    .pin_d1 = 9,
    .pin_d0 = 11,
    .pin_vsync = 6,
    .pin_href = 7,
    .pin_pclk = 13,
    .xclk_freq_hz = 20000000,
    .ledc_timer = LEDC_TIMER_0,
    .ledc_channel = LEDC_CHANNEL_0,
    .pixel_format = PIXFORMAT_JPEG,
    .frame_size = FRAMESIZE_VGA,
    .jpeg_quality = 12,
    .fb_count = 2
};

static const char* _STREAM_CONTENT_TYPE = "multipart/x-mixed-replace;boundary=frame";
static const char* _STREAM_BOUNDARY = "\r\n--frame\r\n";
static const char* _STREAM_PART = "Content-Type: image/jpeg\r\nContent-Length: %u\r\n\r\n";

// ฟังก์ชันสำหรับสตรีมภาพ
esp_err_t stream_handler(httpd_req_t *req) {
    camera_fb_t *fb = NULL;
    esp_err_t res = ESP_OK;
    size_t _jpg_buf_len = 0;
    uint8_t *_jpg_buf = NULL;
    char part_buf[64];

    res = httpd_resp_set_type(req, _STREAM_CONTENT_TYPE);
    if(res != ESP_OK){
        return res;
    }

    while(true){
        fb = esp_camera_fb_get();
        if (!fb) {
            ESP_LOGE(TAG, "Camera capture failed");
            res = ESP_FAIL;
        } else {
            if(fb->format != PIXFORMAT_JPEG){
                bool jpeg_converted = frame2jpg(fb, 80, &_jpg_buf, &_jpg_buf_len);
                if(!jpeg_converted){
                    ESP_LOGE(TAG, "JPEG compression failed");
                    esp_camera_fb_return(fb);
                    res = ESP_FAIL;
                }
            } else {
                _jpg_buf_len = fb->len;
                _jpg_buf = fb->buf;
            }
        }
        if(res == ESP_OK){
            res = httpd_resp_send_chunk(req, _STREAM_BOUNDARY, strlen(_STREAM_BOUNDARY));
        }
        if(res == ESP_OK){
            size_t hlen = snprintf((char *)part_buf, 64, _STREAM_PART, _jpg_buf_len);
            res = httpd_resp_send_chunk(req, (const char *)part_buf, hlen);
        }
        if(res == ESP_OK){
            res = httpd_resp_send_chunk(req, (const char *)_jpg_buf, _jpg_buf_len);
        }
        if(fb->format != PIXFORMAT_JPEG && _jpg_buf){
            free(_jpg_buf);
        }
        esp_camera_fb_return(fb);
        if(res != ESP_OK){
            break;
        }
    }
    return res;
}

// ฟังก์ชันเริ่มต้นเว็บเซิร์ฟเวอร์
void start_camera_stream_server() {
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.server_port = 80;
    config.ctrl_port = 80;
    config.max_open_sockets = 5;

    httpd_uri_t stream_uri = {
        .uri = "/stream",
        .method = HTTP_GET,
        .handler = stream_handler,
        .user_ctx = NULL
    };

    if (httpd_start(&stream_httpd, &config) == ESP_OK) {
        httpd_register_uri_handler(stream_httpd, &stream_uri);
        ESP_LOGI(TAG, "Camera stream server started at /stream on port 80");
    } else {
        ESP_LOGE(TAG, "Failed to start camera stream server");
    }
}

// ฟังก์ชันเริ่มต้นกล้อง
esp_err_t camera_init(void) {
    esp_err_t err = esp_camera_init(&camera_config);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Camera init failed: %s", esp_err_to_name(err));
        return err;
    }

    sensor_t *sensor = esp_camera_sensor_get();
    if (sensor == NULL) {
        ESP_LOGE(TAG, "Get camera sensor failed");
        return ESP_FAIL;
    }

    // ตั้งค่าเซ็นเซอร์เริ่มต้น
    if(sensor->id.PID == OV5640_PID) {
        sensor->set_vflip(sensor, vflip_enabled);
        sensor->set_hmirror(sensor, hmirror_enabled);
        sensor->set_brightness(sensor, 1);
        sensor->set_saturation(sensor, 1);
    } else if(sensor->id.PID == OV2640_PID) {
        sensor->set_vflip(sensor, vflip_enabled);
        sensor->set_hmirror(sensor, hmirror_enabled);
        sensor->set_contrast(sensor, 1);
    }

    ESP_LOGI(TAG, "Camera initialized successfully");
    return ESP_OK;
}

// --- เพิ่ม handler สำหรับ /capture (snapshot) ---
static esp_err_t capture_handler(httpd_req_t *req) {
    camera_fb_t *fb = esp_camera_fb_get();
    if (!fb) {
        ESP_LOGE(TAG, "Camera capture failed");
        httpd_resp_send_500(req);
        return ESP_FAIL;
    }
    httpd_resp_set_type(req, "image/jpeg");
    esp_err_t res = httpd_resp_send(req, (const char *)fb->buf, fb->len);
    esp_camera_fb_return(fb);
    return res;
}

// --- เพิ่ม handler สำหรับ /info (metadata) ---
static esp_err_t info_handler(httpd_req_t *req) {
    camera_fb_t *fb = esp_camera_fb_get();
    if (!fb) {
        ESP_LOGE(TAG, "Camera capture failed");
        httpd_resp_send_500(req);
        return ESP_FAIL;
    }
    char json[128];
    snprintf(json, sizeof(json), "{\"size\":%d,\"width\":%d,\"height\":%d,\"timestamp\":%lu}",
        fb->len, fb->width, fb->height, (unsigned long)esp_log_timestamp());
    esp_camera_fb_return(fb);
    httpd_resp_set_type(req, "application/json");
    return httpd_resp_send(req, json, strlen(json));
}

// --- เพิ่มฟังก์ชันเริ่มต้น server สำหรับ /capture และ /info ---
void start_camera_snapshot_server() {
    if (!stream_httpd) return; // ต้องเริ่ม stream server ก่อน
    httpd_uri_t capture_uri = {
        .uri = "/capture",
        .method = HTTP_GET,
        .handler = capture_handler,
        .user_ctx = NULL
    };
    httpd_register_uri_handler(stream_httpd, &capture_uri);
}

void start_camera_info_server() {
    if (!stream_httpd) return;
    httpd_uri_t info_uri = {
        .uri = "/info",
        .method = HTTP_GET,
        .handler = info_handler,
        .user_ctx = NULL
    };
    httpd_register_uri_handler(stream_httpd, &info_uri);
}

// ... (ฟังก์ชันอื่นๆ เหมือนเดิม) ...