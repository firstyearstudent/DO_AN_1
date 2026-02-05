#include "logger.h"
#include "esp_log.h"
#include <stdlib.h>

static const char *TAG = "LOGGER";

QueueHandle_t logger_queue = NULL;
TaskHandle_t logger_task_handle = NULL;

esp_err_t Logger_init(void){
    // Queue nhỏ hơn vì chỉ còn 1 cảm biến
    logger_queue = xQueueCreate(100, sizeof(sensor_block_t)); 
    if(logger_queue == NULL) return ESP_FAIL;
    
    ESP_LOGI(TAG, "Logger Initialized (PPG Only)");
    return ESP_OK;
}

void Logger_task(void *pvParameter){
    sensor_block_t block;

    while(1){
        if(xQueueReceive(logger_queue, &block, portMAX_DELAY) == pdTRUE){
            // Chỉ xử lý đúng 1 loại tin nhắn là PPG
            if (block.type == SENSOR_PPG) {
                for(uint16_t i = 0; i < block.count; i++){
                    // Format: P, Red, IR
                    printf("P,%lu,%lu,%lu\n", block.sample_id, block.ppg.red[i], block.ppg.ir[i]);
                }
            }
        }
    }
}