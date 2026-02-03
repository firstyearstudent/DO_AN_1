#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "AD8232.h"
#include "MAX30102.h"
#include "INMP441.h"   
#include "logger.h"
#include "sensor_config.h"

static const char *TAG = "MAIN";

// Biến toàn cục để điều khiển việc dừng đo
volatile bool g_is_measuring = true;

// Cấu hình
#define I2C_PORT_NUM I2C_NUM_0 
#define MEASUREMENT_DURATION_MS 10000 // 10 giây

// =======================
// TASK 1: PPG (MAX30102)
// =======================
void PPG_Task(void *pvParam) {
    // Init MAX30102
    if (max30102_init(I2C_PORT_NUM) != ESP_OK) {
        vTaskDelete(NULL);
    }

    uint32_t red_val, ir_val;
    uint32_t packet_id = 0;

    // Chỉ chạy khi g_is_measuring == true
    while(g_is_measuring) {
        if (max30102_read_fifo(&red_val, &ir_val) == ESP_OK) {
            sensor_block_t ppg_packet;
            ppg_packet.type = SENSOR_PPG;
            ppg_packet.sample_id = packet_id++;
            ppg_packet.count = 1;
            ppg_packet.ppg.red[0] = red_val;
            ppg_packet.ppg.ir[0]  = ir_val;

            if (logger_queue != NULL) xQueueSend(logger_queue, &ppg_packet, 0);
        }
        vTaskDelay(pdMS_TO_TICKS(10));
    }
    vTaskDelete(NULL);
}

// =======================
// TASK 2: ECG (AD8232)
// =======================
void ECG_Task(void *pvParam) {
    ESP_ERROR_CHECK(ad8232_init());
    
    int16_t batch_buffer[MAX_SAMPLES_PER_BLOCK];
    int batch_index = 0;
    uint32_t packet_counter = 0;

    TickType_t xLastWakeTime = xTaskGetTickCount();
    const TickType_t xFrequency = pdMS_TO_TICKS(10); // 100Hz

    while (g_is_measuring) {
        int16_t raw_val;
        if (ad8232_read_sample(&raw_val) == ESP_OK) {
            batch_buffer[batch_index++] = raw_val;

            if (batch_index >= MAX_SAMPLES_PER_BLOCK) {
                sensor_block_t tx_packet;
                tx_packet.type = SENSOR_ECG;
                tx_packet.sample_id = packet_counter++;
                tx_packet.count = MAX_SAMPLES_PER_BLOCK;
                memcpy(tx_packet.ecg, batch_buffer, sizeof(batch_buffer));

                if (logger_queue != NULL) xQueueSend(logger_queue, &tx_packet, 0);
                batch_index = 0;
            }
        }
        vTaskDelayUntil(&xLastWakeTime, xFrequency);
    }
    vTaskDelete(NULL);
}

// =======================
// TASK 3: PCG (INMP441)
// =======================
void PCG_Task(void *pvParam) {
    ESP_ERROR_CHECK(inmp441_init());

    int32_t raw_buffer[MAX_SAMPLES_PER_BLOCK];
    uint32_t packet_id = 0;

    while(g_is_measuring) {
        // Hàm này blocking, nhưng vẫn check được g_is_measuring ở vòng lặp sau
        size_t samples_read = inmp441_read_block(raw_buffer, MAX_SAMPLES_PER_BLOCK);

        if (samples_read == MAX_SAMPLES_PER_BLOCK) {
            sensor_block_t pcg_packet;
            pcg_packet.type = SENSOR_PCG;
            pcg_packet.sample_id = packet_id++;
            pcg_packet.count = MAX_SAMPLES_PER_BLOCK;

            for (int i=0; i<MAX_SAMPLES_PER_BLOCK; i++) {
                pcg_packet.pcg[i] = raw_buffer[i] >> 14; 
            }

            if (logger_queue != NULL) xQueueSend(logger_queue, &pcg_packet, 0);
        }
        // Không delay vì Audio cần tốc độ cao
    }
    vTaskDelete(NULL);
}

// =======================
// APP MAIN
// =======================
void app_main(void) {
    ESP_LOGI(TAG, "STARTING MEASUREMENT (10 Seconds)...");

    // 1. Init
    ESP_ERROR_CHECK(Logger_init());
    
    // 2. Start Tasks
    xTaskCreate(Logger_task, "Logger", 4096, NULL, 5, &logger_task_handle);
    xTaskCreate(ECG_Task, "ECG", 4096, NULL, 5, NULL);
    xTaskCreate(PPG_Task, "PPG", 4096, NULL, 5, NULL);
    xTaskCreate(PCG_Task, "PCG", 4096, NULL, 5, NULL);

    // 3. Đếm ngược 10 giây
    vTaskDelay(pdMS_TO_TICKS(MEASUREMENT_DURATION_MS));

    // 4. Dừng mọi thứ
    g_is_measuring = false;
    ESP_LOGW(TAG, "STOPPING ALL SENSORS...");
    
    // Chờ một chút cho các task thoát
    vTaskDelay(pdMS_TO_TICKS(1000));
    
    // In ký hiệu kết thúc đặc biệt để Python nhận biết
    printf("MEASUREMENT_DONE\n");
    ESP_LOGI(TAG, "DONE!");
}