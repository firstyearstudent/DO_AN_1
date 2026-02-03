#ifndef INC_LOGGER_H_
#define INC_LOGGER_H_

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"
#include "esp_err.h"

// Include file cấu hình chung (Bắt buộc)
#include "sensor_config.h"

// Extern Handles để main sử dụng
extern QueueHandle_t logger_queue;
extern TaskHandle_t logger_task_handle;

// Hàm khởi tạo
esp_err_t Logger_init(void);

// Task chính
void Logger_task(void *pvParameter);

#ifdef __cplusplus
}
#endif

#endif /* INC_LOGGER_H_ */