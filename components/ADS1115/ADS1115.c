#include <stdio.h>
#include "ADS1115.h"
#include "driver/i2c.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "ADS1115";

// Biến lưu cấu hình I2C port (mặc định I2C_NUM_0)
static i2c_port_t g_i2c_port = I2C_NUM_0;
static uint8_t g_i2c_addr = ADS1115_ADDR_GND; // 0x48

#define ADS1115_REG_CONVERSION  0x00
#define ADS1115_REG_CONFIG      0x01

esp_err_t ads1115_init_i2c(i2c_port_t i2c_num, int sda_pin, int scl_pin) {
    g_i2c_port = i2c_num;

    i2c_config_t conf = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = sda_pin,
        .scl_io_num = scl_pin,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .master.clk_speed = 400000, // 400kHz Fast Mode
    };

    ESP_LOGI(TAG, "Initializing I2C Master on Port %d...", i2c_num);
    esp_err_t err = i2c_param_config(i2c_num, &conf);
    if (err != ESP_OK) return err;

    return i2c_driver_install(i2c_num, conf.mode, 0, 0, 0);
}

// Hàm ghi vào thanh ghi 16-bit
static esp_err_t write_register(uint8_t reg, uint16_t val) {
    uint8_t data[3];
    data[0] = reg;
    data[1] = (val >> 8) & 0xFF; // MSB
    data[2] = val & 0xFF;        // LSB

    return i2c_master_write_to_device(g_i2c_port, g_i2c_addr, data, 3, pdMS_TO_TICKS(100));
}

// Hàm đọc thanh ghi 16-bit
static int16_t read_register(uint8_t reg) {
    uint8_t data[2] = {0, 0};
    
    // Bước 1: Ghi địa chỉ thanh ghi cần đọc (Write Pointer)
    esp_err_t err = i2c_master_write_to_device(g_i2c_port, g_i2c_addr, &reg, 1, pdMS_TO_TICKS(100));
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "I2C Write Pointer Error");
        return 0;
    }

    // Bước 2: Đọc 2 byte dữ liệu
    err = i2c_master_read_from_device(g_i2c_port, g_i2c_addr, data, 2, pdMS_TO_TICKS(100));
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "I2C Read Data Error");
        return 0;
    }

    // Ghép MSB và LSB thành 16-bit signed
    return (int16_t)((data[0] << 8) | data[1]);
}

int16_t ads1115_get_raw(ads_mux_t mux, ads_gain_t gain, ads_sps_t sps) {
    // Cấu hình thanh ghi Config (16-bit)
    // Bit [15]: OS = 1 (Start Single-conversion)
    // Bit [14:12]: MUX (Kênh input)
    // Bit [11:9]: PGA (Độ lợi)
    // Bit [8]: MODE = 1 (Single-shot mode)
    // Bit [7:5]: DR (Data Rate)
    // Bit [4:0]: COMP (Comparator - Disable)
    
    uint16_t config = 0x8103; // Base config: OS=1, Mode=1, Disable Comp
    config |= mux;
    config |= gain;
    config |= sps;

    // 1. Gửi lệnh cấu hình để bắt đầu đo
    esp_err_t err = write_register(ADS1115_REG_CONFIG, config);
    if (err != ESP_OK) return 0;

    // 2. Chờ chuyển đổi xong
    // Thời gian chờ phụ thuộc vào SPS. 
    // Ví dụ 860SPS -> ~1.2ms. 128SPS -> ~8ms.
    // Để an toàn, ta chờ khoảng 2ms (nếu dùng tốc độ cao) hoặc đọc bit OS để biết xong chưa.
    // Ở đây dùng delay cứng cho đơn giản (phù hợp với task FreeRTOS).
    
    // Nếu dùng 860SPS cho ECG:
    vTaskDelay(pdMS_TO_TICKS(2)); 
    
    // Nếu dùng 128SPS (Mặc định):
    // vTaskDelay(pdMS_TO_TICKS(8));

    // 3. Đọc kết quả từ thanh ghi Conversion (0x00)
    return read_register(ADS1115_REG_CONVERSION);
}