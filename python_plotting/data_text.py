import serial
import time
import os
import pandas as pd

# --- CẤU HÌNH ---
SERIAL_PORT = '/dev/ttyUSB0'  # Sửa đúng cổng COM/TTY
BAUD_RATE = 921600            
OUTPUT_FOLDER = 'result'      # Tên thư mục kết quả

# CẤU HÌNH KIỂM TRA (Khớp với Firmware)
EXPECTED_FS = 1000            
TOLERANCE = 0.1               

data_ppg = []

def save_data():
    # 1. Tự động tạo thư mục nếu chưa có
    if not os.path.exists(OUTPUT_FOLDER):
        os.makedirs(OUTPUT_FOLDER)
        print(f"[INFO] Đã tạo thư mục mới: {OUTPUT_FOLDER}")
    
    # 2. Lưu file CSV vào trong thư mục đó
    file_path = os.path.join(OUTPUT_FOLDER, 'ppg.csv')
    
    df = pd.DataFrame(data_ppg, columns=['SampleID', 'Red', 'IR'])
    df.to_csv(file_path, index=False)
    print(f"\n[DONE] Đã lưu {len(df)} mẫu vào file: {file_path}")

def main():
    print(f"--- KẾT NỐI {SERIAL_PORT} @ {BAUD_RATE} ---")
    print(f"--- MỤC TIÊU: {EXPECTED_FS} Hz (Lưu vào '{OUTPUT_FOLDER}') ---")

    try:
        ser = serial.Serial(SERIAL_PORT, BAUD_RATE, timeout=1)
        ser.setDTR(False); time.sleep(0.05); ser.setDTR(True) # Reset ESP32
    except Exception as e:
        print(f"Lỗi mở cổng Serial: {e}")
        return

    print("Đang đợi dữ liệu...")
    
    start_time = time.time()
    last_check_time = start_time
    total_samples = 0
    samples_in_last_sec = 0
    last_sample_id = -1
    lost_packets = 0

    try:
        while True:
            line = ser.readline().decode('utf-8', errors='ignore').strip()
            
            if "MEASUREMENT_DONE" in line:
                print("\n[STOP] Nhận tín hiệu dừng từ ESP32.")
                break

            if not line: continue

            parts = line.split(',')
            
            # Format: P, Sample_ID, Red, IR
            if len(parts) == 4 and parts[0] == 'P':
                try:
                    s_id = int(parts[1])
                    red = int(parts[2])
                    ir  = int(parts[3])
                    
                    if last_sample_id != -1:
                        diff = s_id - last_sample_id
                        if diff > 1: lost_packets += (diff - 1)
                    
                    last_sample_id = s_id
                    data_ppg.append((s_id, red, ir))
                    total_samples += 1
                    samples_in_last_sec += 1

                except ValueError: pass

            # Monitor mỗi giây
            current_time = time.time()
            if current_time - last_check_time >= 1.0:
                fps = samples_in_last_sec / (current_time - last_check_time)
                quality = "OK" if abs(fps - EXPECTED_FS) <= (EXPECTED_FS * TOLERANCE) else "BAD"
                print(f"Time: {int(current_time - start_time)}s | FPS: {int(fps)} | Lost: {lost_packets} | -> {quality}")
                samples_in_last_sec = 0
                last_check_time = current_time

    except KeyboardInterrupt:
        print("\n[USER STOP]")
    finally:
        if 'ser' in locals() and ser.is_open: ser.close()
        save_data()

if __name__ == "__main__":
    main()