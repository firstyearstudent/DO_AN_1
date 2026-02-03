#ifndef SENSOR_CONFIG_H
#define SENSOR_CONFIG_H

#include <stdint.h>

// Định nghĩa kích thước gói tin
#define MAX_SAMPLES_PER_BLOCK  32

// Định nghĩa các loại cảm biến
typedef enum {
    SENSOR_ECG = 0,
    SENSOR_PPG,
    SENSOR_PCG // <--- Đảm bảo dòng này có mặt
} sensor_type_t;

// Cấu trúc dữ liệu riêng cho PPG
typedef struct {
    uint32_t red[MAX_SAMPLES_PER_BLOCK]; 
    uint32_t ir[MAX_SAMPLES_PER_BLOCK];  
} ppg_data_t;

// Cấu trúc gói tin tổng
typedef struct {
    sensor_type_t type;     
    uint32_t sample_id;     
    uint16_t count;         
    
    union {
        int16_t ecg[MAX_SAMPLES_PER_BLOCK]; 
        ppg_data_t ppg;                     
        int32_t pcg[MAX_SAMPLES_PER_BLOCK]; 
    };
} sensor_block_t;

#endif // SENSOR_CONFIG_H