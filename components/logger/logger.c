#include "logger.h"
#include "esp_log.h"
#include <stdlib.h>

static const char *TAG = "LOGGER";

QueueHandle_t logger_queue = NULL;
TaskHandle_t logger_task_handle = NULL;

esp_err_t Logger_init(void){
    // Tăng kích thước Queue lên để chứa được cả ECG và PPG
    logger_queue = xQueueCreate(60, sizeof(sensor_block_t)); 
    if(logger_queue == NULL) {
        ESP_LOGE(TAG, "Failed to create Queue");
        return ESP_FAIL;
    }
    ESP_LOGI(TAG, "Logger Initialized");
    return ESP_OK;
}

void Logger_task(void *pvParameter){
    sensor_block_t block;

    ESP_LOGI(TAG, "Logger Task Started. Mode: ASYNC PRINT (ECG & PPG)");

    // In tiêu đề cho Serial Plotter dễ hiểu (tùy chọn)
    // Lưu ý: Khi chạy song song, dữ liệu sẽ xen kẽ nhau.
    // Dạng ECG: "E, giá_trị"
    // Dạng PPG: "P, đỏ, hồng_ngoại"

    while(1){
        // Chờ dữ liệu từ Queue
        if(xQueueReceive(logger_queue, &block, portMAX_DELAY) == pdTRUE){
            
            switch(block.type){
                // -------------------------------------------------
                // TRƯỜNG HỢP 1: GÓI TIN ECG (AD8232)
                // -------------------------------------------------
                case SENSOR_ECG:
                    for(uint16_t i = 0; i < block.count; i++){
                        // In ra dạng: "E, giá_trị"
                        // Thêm prefix 'E' để phân biệt với PPG
                        // Hoặc chỉ in số nếu bạn chỉ muốn vẽ 1 đồ thị
                        printf("E,%d\n", block.ecg[i]);
                    }
                    break;

                // -------------------------------------------------
                // TRƯỜNG HỢP 2: GÓI TIN PPG (MAX30102)
                // -------------------------------------------------
                case SENSOR_PPG:
                    for(uint16_t i = 0; i < block.count; i++){
                        // In ra dạng: "P, Red, IR"
                        // Lưu ý: Giá trị Red/IR rất lớn (hàng chục/trăm nghìn)
                        printf("P,%lu,%lu\n", block.ppg.red[i], block.ppg.ir[i]);
                    }
                    break;

                case SENSOR_PCG:
                    for(uint16_t i = 0; i < block.count; i++){
                    // In ra dạng: "A, giá_trị" (A là Audio)
                        printf("A,%ld\n", block.pcg[i]);
                    }
                    break;

                default:
                    break;
            }
        }
    }
}