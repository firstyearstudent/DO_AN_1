#ifndef INMP441_H
#define INMP441_H

#include "esp_err.h"

// Cấu hình chân (Theo sơ đồ bạn đã nối)
#define I2S_WS_IO       15
#define I2S_BCK_IO      14
#define I2S_DO_IO       32

// Tần số lấy mẫu: 4000Hz (Đủ cho tiếng tim, nhẹ gánh cho ESP32)
#define SAMPLE_RATE     4000 

// Khởi tạo I2S cho INMP441
esp_err_t inmp441_init(void);

// Đọc một mẫu âm thanh (Blocking mode)
esp_err_t inmp441_read_sample(int32_t *sample);

// Đọc nhiều mẫu vào buffer (Hiệu quả hơn)
size_t inmp441_read_block(int32_t *buffer, size_t max_samples);

#endif