#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "driver/i2c.h" 
#include "MAX30102.h"
#include "logger.h"
#include "sensor_config.h"

static const char *TAG = "MAIN";

volatile bool g_is_measuring = true;
TaskHandle_t ppg_task_handle = NULL;

// Cấu hình I2C
#define I2C_PORT_NUM I2C_NUM_0 
#define I2C_SDA_PIN 21
#define I2C_SCL_PIN 22

// Thời gian đo
#define MEASUREMENT_DURATION_MS 10000 
#define PPG_READ_INTERVAL_MS 20       

static esp_err_t i2c_master_init(void) {
    i2c_config_t conf = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = I2C_SDA_PIN,
        .scl_io_num = I2C_SCL_PIN,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .master.clk_speed = 400000, 
    };
    esp_err_t err = i2c_param_config(I2C_PORT_NUM, &conf);
    if (err != ESP_OK) return err;
    return i2c_driver_install(I2C_PORT_NUM, conf.mode, 0, 0, 0);
}

void PPG_Task(void *pvParam) {
    uint32_t red_val, ir_val;
    uint32_t packet_id = 0;
    const TickType_t xFrequency = pdMS_TO_TICKS(PPG_READ_INTERVAL_MS); 
    TickType_t xLastWakeTime = xTaskGetTickCount();

    ESP_LOGI("PPG_TASK", "Started reading MAX30102 at 1000Hz...");

    while(g_is_measuring) {
        while (g_is_measuring && max30102_read_fifo(&red_val, &ir_val) == ESP_OK) {
            sensor_block_t ppg_packet;
            ppg_packet.type = SENSOR_PPG;
            ppg_packet.sample_id = packet_id++;
            ppg_packet.count = 1;
            ppg_packet.ppg.red[0] = red_val;
            ppg_packet.ppg.ir[0]  = ir_val;

            if (logger_queue != NULL) xQueueSend(logger_queue, &ppg_packet, 0);
        }
        vTaskDelayUntil(&xLastWakeTime, xFrequency);
    }
    ESP_LOGI("PPG_TASK", "Task Finished.");
    
    // Task tự kết thúc an toàn. 
    // Handle ppg_task_handle sẽ trở thành rác, Main KHÔNG ĐƯỢC đụng vào nữa.
    vTaskDelete(NULL); 
}

void app_main(void) {
    vTaskPrioritySet(NULL, 20); 
    ESP_ERROR_CHECK(Logger_init());
    
    // 1. Cài đặt I2C
    if (i2c_master_init() != ESP_OK) {
        ESP_LOGE(TAG, "I2C Init Failed! Reset board.");
        return;
    }

    // 2. Khởi tạo MAX30102
    ESP_LOGI(TAG, "Initializing MAX30102...");
    // Thử init nhiều lần cho chắc
    int retry = 3;
    while (max30102_init(I2C_PORT_NUM) != ESP_OK && retry > 0) {
        ESP_LOGE(TAG, "MAX30102 Not Found! Retrying...");
        vTaskDelay(pdMS_TO_TICKS(500));
        retry--;
    }
    
    if (retry == 0) {
        ESP_LOGE(TAG, "GIVE UP: MAX30102 Connection Lost!");
    } else {
        ESP_LOGI(TAG, "MAX30102 Init Success! (Configured for 1000Hz)");
    }

    // 3. Tạo Task
    xTaskCreate(Logger_task, "Logger", 4096, NULL, 6, &logger_task_handle);
    xTaskCreate(PPG_Task, "PPG", 4096, NULL, 5, &ppg_task_handle);

    // 4. Đo trong 5 giây
    ESP_LOGI(TAG, "Measuring for %d ms...", MEASUREMENT_DURATION_MS);
    vTaskDelay(pdMS_TO_TICKS(MEASUREMENT_DURATION_MS));

    // 5. Dừng hệ thống
    g_is_measuring = false;
    printf("MEASUREMENT_DONE\n");
    
    // Chờ 1 chút cho PPG Task kịp chạy hết vòng lặp và tự hủy
    vTaskDelay(pdMS_TO_TICKS(1000));

    // --- SỬA LỖI CRASH TẠI ĐÂY ---
    // KHÔNG gọi vTaskDelete(ppg_task_handle) vì nó đã tự xóa rồi!
    
    // Logger Task là vòng lặp vô tận (while(1)) nên ta phải xóa thủ công
    if (logger_task_handle) {
        vTaskDelete(logger_task_handle);
        logger_task_handle = NULL;
    }
    
    ESP_LOGI(TAG, "Done.");
    // Main Loop vô tận để giữ CPU không bị reset
    while(1) vTaskDelay(1000);
}