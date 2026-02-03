#include <stdio.h>
#include "AD8232.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "ADS1115.h" // Gọi thư viện ADS1115

static const char *TAG = "AD8232_DRIVER";

// --- CẤU HÌNH CHÂN I2C CHO ADS1115 ---
#define I2C_MASTER_SDA_IO 21  // Chân SDA của ESP32 (Thường là GPIO 21)
#define I2C_MASTER_SCL_IO 22  // Chân SCL của ESP32 (Thường là GPIO 22)
#define I2C_MASTER_NUM    I2C_NUM_0 // Cổng I2C số 0

esp_err_t ad8232_init(void) {
    ESP_LOGI(TAG, "Initializing AD8232...");

    // 1. Cấu hình GPIO cho Leads Off Detection (Lo+ / Lo-)
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << AD8232_GPIO_LO_PLUS) | (1ULL << AD8232_GPIO_LO_MINUS),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE, 
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };
    
    esp_err_t err = gpio_config(&io_conf);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to config GPIO for Leads Off detection");
        return err;
    }

    // 2. --- QUAN TRỌNG: KHỞI TẠO I2C DRIVER ---
    // Gọi hàm từ thư viện ADS1115 để cài đặt I2C
    ESP_LOGI(TAG, "Initializing I2C for ADS1115 (SDA=%d, SCL=%d)...", I2C_MASTER_SDA_IO, I2C_MASTER_SCL_IO);
    err = ads1115_init_i2c(I2C_MASTER_NUM, I2C_MASTER_SDA_IO, I2C_MASTER_SCL_IO);
    
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to install I2C Driver: %s", esp_err_to_name(err));
        return err;
    }
    
    ESP_LOGI(TAG, "AD8232 Initialized & I2C Ready");
    return ESP_OK;
}

bool ad8232_is_leads_off(void) {
    int lo_plus = gpio_get_level(AD8232_GPIO_LO_PLUS);
    int lo_minus = gpio_get_level(AD8232_GPIO_LO_MINUS);
    return (lo_plus == 1) || (lo_minus == 1);
}

esp_err_t ad8232_read_sample(int16_t *val) {
    // 1. Kiểm tra tuột dây
    if (ad8232_is_leads_off()) {
        *val = 0; 
        // Trả về OK nhưng giá trị là 0 để đồ thị không bị gãy, 
        // hoặc trả về ESP_FAIL tùy logic của bạn.
        // Ở đây tôi trả về OK để Logger vẫn chạy, nhưng giá trị bằng 0.
        return ESP_OK; 
    }

    // 2. Đọc từ ADS1115 (Qua I2C)
    // Sử dụng hàm ads1115_get_raw đã viết
    *val = ads1115_get_raw(ADS1115_MUX_SINGLE_0, 
                           ADS1115_GAIN_4_096V, 
                           ADS1115_DR_860SPS);
    
    // Kiểm tra nếu giá trị trả về là 0 liên tục mà không phải do tuột dây
    // thì có thể do lỗi I2C đọc không được, nhưng hàm get_raw hiện tại chưa trả mã lỗi.
    
    return ESP_OK;
}