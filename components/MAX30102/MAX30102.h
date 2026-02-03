#ifndef MAX30102_H
#define MAX30102_H

#include "esp_err.h"
#include "driver/i2c.h"

// Địa chỉ I2C mặc định
#define MAX30102_ADDR 0x57

// Cấu hình Init
esp_err_t max30102_init(i2c_port_t i2c_num);

// Đọc giá trị Red và IR từ FIFO
// Trả về ESP_OK nếu đọc được, ESP_FAIL nếu chưa có dữ liệu
esp_err_t max30102_read_fifo(uint32_t *red, uint32_t *ir);

#endif