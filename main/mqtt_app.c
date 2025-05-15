#include "mqtt_app.h"
#include "esp_log.h"
#include "mqtt_client.h"

static const char *TAG = "MQTT_APP";
static esp_mqtt_client_handle_t client = NULL;

// ปรับให้ตรงกับที่คุณให้มา
#define MQTT_BROKER_URI "mqtt://192.168.1.67"
#define MQTT_USER "raspi5nr"
#define MQTT_PASS "12345"
#define MQTT_TOPIC "ESP32S3_N16R8"

static void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data) {
    // ป้องกัน warning ตัวแปรไม่ได้ใช้
    (void)handler_args;
    (void)base;
    (void)event_data;

    switch ((esp_mqtt_event_id_t)event_id) {
        case MQTT_EVENT_CONNECTED:
            ESP_LOGI(TAG, "Connected to MQTT broker");
            break;
        case MQTT_EVENT_DISCONNECTED:
            ESP_LOGI(TAG, "Disconnected from MQTT broker");
            break;
        default:
            break;
    }
}

void mqtt_app_start(void) {
    esp_mqtt_client_config_t mqtt_cfg = {
        .broker.address.uri = MQTT_BROKER_URI,
        .credentials.username = MQTT_USER,
        .credentials.authentication.password = MQTT_PASS,
    };

    client = esp_mqtt_client_init(&mqtt_cfg);
    esp_mqtt_client_register_event(client, ESP_EVENT_ANY_ID, mqtt_event_handler, NULL);
    esp_mqtt_client_start(client);
}

void mqtt_send_image(const uint8_t *image_data, size_t image_len) {
    if (client) {
        int msg_id = esp_mqtt_client_publish(client, MQTT_TOPIC, (const char *)image_data, image_len, 0, 0);
        ESP_LOGI(TAG, "Image published with msg_id=%d", msg_id);
    }
}
