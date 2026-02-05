#ifndef SENSOR_CONFIG_H
#define SENSOR_CONFIG_H

#include <stdint.h>

// Chỉ giữ lại định nghĩa cho PPG
typedef enum {
    SENSOR_PPG = 1
} sensor_type_t;

#define MAX_SAMPLES_PER_BLOCK 16 

typedef struct {
    sensor_type_t type;
    uint32_t sample_id;
    uint16_t count;
    
    // Chỉ giữ lại struct của PPG
    struct {
        uint32_t red[MAX_SAMPLES_PER_BLOCK];
        uint32_t ir[MAX_SAMPLES_PER_BLOCK];
    } ppg;
    
} sensor_block_t;

#endif