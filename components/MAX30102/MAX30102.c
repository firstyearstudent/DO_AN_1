#include "MAX30102.h"
#include "esp_log.h"

static const char *TAG = "MAX30102";
static i2c_port_t g_i2c_num;

// Địa chỉ chuẩn 0x57
#define MAX30102_ADDR 0x57

static esp_err_t write_reg(uint8_t reg, uint8_t data) {
    uint8_t write_buf[2] = {reg, data};
    return i2c_master_write_to_device(g_i2c_num, MAX30102_ADDR, write_buf, 2, 1000 / portTICK_PERIOD_MS);
}

static esp_err_t read_reg(uint8_t reg, uint8_t *data) {
    return i2c_master_write_read_device(g_i2c_num, MAX30102_ADDR, &reg, 1, data, 1, 1000 / portTICK_PERIOD_MS);
}

static esp_err_t read_fifo_burst(uint8_t *buffer, size_t len) {
    uint8_t reg = 0x07; 
    return i2c_master_write_read_device(g_i2c_num, MAX30102_ADDR, &reg, 1, buffer, len, 1000 / portTICK_PERIOD_MS);
}

// Hàm Dump Register để kiểm tra (Giữ nguyên để debug)
void max30102_dump_regs() {
    uint8_t val;
    ESP_LOGW(TAG, "--- MAX30102 REGISTER DUMP ---");
    read_reg(0x00, &val); ESP_LOGW(TAG, "0x00 (Int Status 1): 0x%02X", val);
    read_reg(0x04, &val); ESP_LOGW(TAG, "0x04 (FIFO WR Ptr) : %d", val);
    read_reg(0x06, &val); ESP_LOGW(TAG, "0x06 (FIFO RD Ptr) : %d", val);
    read_reg(0x08, &val); ESP_LOGW(TAG, "0x08 (FIFO Config) : 0x%02X", val);
    read_reg(0x09, &val); ESP_LOGW(TAG, "0x09 (Mode Config) : 0x%02X", val);
    ESP_LOGW(TAG, "------------------------------");
}

esp_err_t max30102_init(i2c_port_t i2c_num) {
    g_i2c_num = i2c_num;
    uint8_t data;
    
    // 1. Check ID
    esp_err_t err = read_reg(0xFF, &data);
    if (err != ESP_OK) return err;
    ESP_LOGI(TAG, "Found MAX30102 (0x57). ID: 0x%02X", data);

    // 2. Reset chip
    write_reg(0x09, 0x40); 
    vTaskDelay(pdMS_TO_TICKS(500)); // Đợi Reset xong

    // 3. Xóa cờ ngắt nguồn (Power Ready Flag)
    // Đọc thanh ghi 0x00 để xóa cờ này, giúp chip biết đã sẵn sàng
    read_reg(0x00, &data); 

    // 4. Cấu hình FIFO (QUAN TRỌNG)
    // Sửa 0x4F -> 0x0F (SMP_AVE = 0: No Averaging). 
    // Tắt gộp mẫu để dữ liệu ra ngay lập tức.
    write_reg(0x08, 0x0F); 

    // 5. Cấu hình SpO2
    // 0x27: Range 4096nA, 100 SPS, PulseWidth 411us
    write_reg(0x0A, 0x27); 

    // 6. Cấu hình dòng LED (Giữ mức 0x1F là an toàn)
    write_reg(0x0C, 0x1F); 
    write_reg(0x0D, 0x1F); 

    // 7. Xóa sạch Pointer
    write_reg(0x04, 0x00);
    write_reg(0x05, 0x00);
    write_reg(0x06, 0x00);

    // 8. BƯỚC CUỐI CÙNG: Kích hoạt Mode SpO2
    // Đưa lệnh này xuống cuối cùng để "đề nổ" máy
    write_reg(0x09, 0x03); 

    // Kiểm tra lại ngay
    max30102_dump_regs();

    return ESP_OK;
}

esp_err_t max30102_read_fifo(uint32_t *red, uint32_t *ir) {
    uint8_t ptr_wr, ptr_rd, mode_reg;
    
    // Kiểm tra mode (phòng hờ sụt nguồn)
    read_reg(0x09, &mode_reg);
    if (mode_reg & 0x80) { // Nếu bị Shutdown
        write_reg(0x09, 0x03); // Bật lại
        return ESP_FAIL;
    }

    read_reg(0x04, &ptr_wr);
    read_reg(0x06, &ptr_rd);

    // Nếu con trỏ bằng nhau -> Không có dữ liệu
    if (ptr_wr == ptr_rd) return ESP_FAIL;

    uint8_t buf[6];
    if (read_fifo_burst(buf, 6) == ESP_OK) {
        *red = ((uint32_t)buf[0] << 16 | (uint32_t)buf[1] << 8 | buf[2]) & 0x03FFFF;
        *ir  = ((uint32_t)buf[3] << 16 | (uint32_t)buf[4] << 8 | buf[5]) & 0x03FFFF;
        
        // Fix lỗi phần cứng: Đôi khi Red/IR bị tráo đổi trên chip clone
        // Nếu IR luôn nhỏ hơn Red, hãy thử đảo 2 dòng trên
        
        return ESP_OK;
    }
    
    return ESP_FAIL;
}