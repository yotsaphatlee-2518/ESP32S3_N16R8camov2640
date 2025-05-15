#ifndef MQTT_APP_H
#define MQTT_APP_H

#include <stdint.h>  // สำหรับ uint8_t
#include <stddef.h>  // สำหรับ size_t

void mqtt_app_start(void);
void mqtt_send_image(const uint8_t *image_data, size_t image_len);

#endif