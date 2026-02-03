#include "INMP441.h"
#include "driver/i2s.h"
#include "esp_log.h"

static const char *TAG = "INMP441";
#define I2S_PORT_NUM I2S_NUM_0

esp_err_t inmp441_init(void) {
    // 1. Cấu hình I2S
    i2s_config_t i2s_config = {
        .mode = I2S_MODE_MASTER | I2S_MODE_RX, // Master, chỉ Nhận (RX)
        .sample_rate = SAMPLE_RATE,
        .bits_per_sample = I2S_BITS_PER_SAMPLE_32BIT, // INMP441 trả về 24-bit trong gói 32-bit
        .channel_format = I2S_CHANNEL_FMT_ONLY_LEFT,  // Vì chân L/R nối GND -> Kênh Trái
        .communication_format = I2S_COMM_FORMAT_STAND_I2S,
        .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
        .dma_buf_count = 4,      // Số lượng buffer DMA
        .dma_buf_len = 64,       // Độ dài mỗi buffer (mẫu)
        .use_apll = false,
        .tx_desc_auto_clear = false,
        .fixed_mclk = 0
    };

    // 2. Cấu hình Chân (Pin)
    i2s_pin_config_t pin_config = {
        .bck_io_num = I2S_BCK_IO,   // SCK
        .ws_io_num = I2S_WS_IO,     // WS
        .data_out_num = I2S_PIN_NO_CHANGE, // Không dùng loa
        .data_in_num = I2S_DO_IO    // SD
    };

    // 3. Cài đặt Driver
    esp_err_t err = i2s_driver_install(I2S_PORT_NUM, &i2s_config, 0, NULL);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed installing I2S driver: %d", err);
        return err;
    }

    err = i2s_set_pin(I2S_PORT_NUM, &pin_config);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed setting I2S pins: %d", err);
        return err;
    }

    ESP_LOGI(TAG, "INMP441 Initialized at %d Hz", SAMPLE_RATE);
    return ESP_OK;
}

size_t inmp441_read_block(int32_t *buffer, size_t max_samples) {
    size_t bytes_read = 0;
    
    // Đọc dữ liệu thô từ DMA (Đơn vị là Bytes)
    esp_err_t result = i2s_read(I2S_PORT_NUM, 
                                (void*)buffer, 
                                max_samples * sizeof(int32_t), 
                                &bytes_read, 
                                100 / portTICK_PERIOD_MS); // Timeout 100ms
    
    if (result == ESP_OK) {
        // Trả về số lượng MẪU đã đọc được (Bytes / 4)
        return bytes_read / sizeof(int32_t);
    }
    return 0;
}