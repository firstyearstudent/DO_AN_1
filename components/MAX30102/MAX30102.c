#include "MAX30102.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "MAX30102";
static i2c_port_t g_i2c_num;

#define MAX30102_ADDR 0x57

static esp_err_t write_reg(uint8_t reg, uint8_t data) {
    uint8_t write_buf[2] = {reg, data};
    return i2c_master_write_to_device(g_i2c_num, MAX30102_ADDR, write_buf, 2, 1000 / portTICK_PERIOD_MS);
}

static esp_err_t read_reg(uint8_t reg, uint8_t *data) {
    return i2c_master_write_read_device(g_i2c_num, MAX30102_ADDR, &reg, 1, data, 1, 1000 / portTICK_PERIOD_MS);
}

esp_err_t max30102_init(i2c_port_t i2c_num) {
    g_i2c_num = i2c_num;
    esp_err_t err;

    // 1. Kiểm tra kết nối
    uint8_t part_id;
    err = read_reg(0xFF, &part_id);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "I2C Error or Device Not Found");
        return err;
    }
    ESP_LOGI(TAG, "Found MAX30102 with Part ID: 0x%02X", part_id);

    // 2. Reset
    write_reg(0x09, 0x40);
    vTaskDelay(pdMS_TO_TICKS(100));

    // 3. Cấu hình FIFO
    // SMP_AVE = 000 (1 sample averaging - Tốc độ cao nhất, không gộp mẫu)
    // FIFO_ROLLOVER_EN = 1 (Cho phép ghi đè khi đầy, tránh treo)
    // FIFO_A_FULL = 14 (Báo đầy khi còn trống 14 mẫu)
    write_reg(0x08, 0x1F); 

    // 4. Cấu hình Mode
    // Mode = 0x03 (SpO2 Mode: Red + IR)
    write_reg(0x09, 0x03); 

    // 5. Cấu hình Tốc độ (SpO2 Configuration) - SỬA Ở ĐÂY CHO 1000HZ
    // SPO2_ADC_RGE = 01 (4096nA)
    // SPO2_SR      = 101 (1000 SPS - 1000Hz)
    // LED_PW       = 10 (215 us - Độ rộng xung này hoạt động tốt ở 1000Hz)
    // Value: 0b0 01 101 10 = 0x36
    write_reg(0x0A, 0x36); 

    // 6. Cấu hình dòng LED (Chỉnh độ sáng)
    // Mức 0x24 (~7.2mA)
    write_reg(0x0C, 0x24); 
    write_reg(0x0D, 0x24); 

    // 7. Xóa sạch Pointer để bắt đầu mới
    write_reg(0x04, 0x00);
    write_reg(0x05, 0x00);
    write_reg(0x06, 0x00);

    ESP_LOGI(TAG, "MAX30102 Configured: 1000 SPS, Raw Mode");
    return ESP_OK;
}

esp_err_t max30102_read_fifo(uint32_t *red, uint32_t *ir) {
    uint8_t reg_ptr = 0x06; // FIFO Read Pointer
    uint8_t wr_ptr = 0x04;  // FIFO Write Pointer
    uint8_t r_ptr_val, w_ptr_val;

    // Kiểm tra xem có dữ liệu mới không
    read_reg(reg_ptr, &r_ptr_val);
    read_reg(wr_ptr, &w_ptr_val);

    if (r_ptr_val == w_ptr_val) {
        return ESP_FAIL; // Không có dữ liệu
    }

    // Đọc 1 mẫu (6 bytes: 3 byte Red, 3 byte IR)
    uint8_t data_reg = 0x07; // FIFO Data Register
    uint8_t raw_data[6];
    
    esp_err_t err = i2c_master_write_read_device(g_i2c_num, MAX30102_ADDR, &data_reg, 1, raw_data, 6, 100 / portTICK_PERIOD_MS);
    
    if (err == ESP_OK) {
        // Hợp nhất 3 byte thành số 18-bit (bỏ 2 bit đầu rác của thanh ghi MSB nếu cần)
        *red = ((uint32_t)raw_data[0] << 16 | (uint32_t)raw_data[1] << 8 | raw_data[2]) & 0x03FFFF;
        *ir  = ((uint32_t)raw_data[3] << 16 | (uint32_t)raw_data[4] << 8 | raw_data[5]) & 0x03FFFF;
        return ESP_OK;
    }
    
    return ESP_FAIL;
}