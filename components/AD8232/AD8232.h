#ifndef AD8232_H
#define AD8232_H

#ifdef __cplusplus
extern "C" {
#endif

#include "esp_err.h"
#include <stdint.h>
#include <stdbool.h>

// --- Cấu hình phần cứng (Có thể sửa đổi tại đây) ---
#define AD8232_GPIO_LO_PLUS     GPIO_NUM_27  // Chân LO+ nối vào GPIO 27
#define AD8232_GPIO_LO_MINUS    GPIO_NUM_26  // Chân LO- nối vào GPIO 26

// Cấu hình ADS1115
#define AD8232_ADS1115_ADDR     0x48         // Địa chỉ I2C mặc định (GND)
#define AD8232_ADS_CHANNEL      0            // Kênh A0 của ADS1115 nối với Output AD8232

/**
 * @brief Khởi tạo driver AD8232
 * * Hàm này sẽ:
 * 1. Cấu hình GPIO cho chân LO+ và LO- (Input)
 * 2. Khởi tạo/Kiểm tra kết nối tới ADS1115
 * * @return esp_err_t ESP_OK nếu thành công
 */
esp_err_t ad8232_init(void);

/**
 * @brief Đọc một mẫu tín hiệu ECG
 * * Hàm này sẽ:
 * 1. Kiểm tra xem dây điện cực có bị tuột không (Leads Off Check).
 * 2. Nếu dây tuột: Trả về 0 và log cảnh báo.
 * 3. Nếu dây tốt: Đọc giá trị Analog từ ADS1115.
 * * @param[out] val Pointer để lưu giá trị đọc được (nếu thành công)
 * @return esp_err_t ESP_OK nếu đọc thành công, ESP_FAIL nếu lỗi I2C hoặc tuột dây
 */
esp_err_t ad8232_read_sample(int16_t *val);

/**
 * @brief Kiểm tra nhanh trạng thái kết nối điện cực
 * * @return true Nếu bị tuột dây (Leads Off)
 * @return false Nếu dây đang kết nối tốt
 */
bool ad8232_is_leads_off(void);

#ifdef __cplusplus
}
#endif

#endif // AD8232_H