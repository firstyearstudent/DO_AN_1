import serial
import time
import os
import pandas as pd
import threading

# --- CẤU HÌNH ---
SERIAL_PORT = '/dev/ttyUSB0'  # Kiểm tra lại cổng USB của bạn (vd: /dev/ttyACM0)
BAUD_RATE = 115200
OUTPUT_FOLDER = 'result'

# Biến lưu dữ liệu
data_ecg = []
data_ppg = []
data_pcg = []

def parse_line(line):
    line = line.strip()
    if not line: return None
    
    # Tách loại dữ liệu và giá trị (Ví dụ: "E,1234")
    parts = line.split(',')
    if len(parts) < 2: return None
    
    tag = parts[0]
    try:
        # Nếu dòng có nhiều giá trị (VD: P,Red,IR)
        if tag == 'P' and len(parts) >= 3:
            return ('P', int(parts[1]), int(parts[2]))
        
        # Nếu dòng có 1 giá trị (E hoặc A)
        val = int(parts[1])
        return (tag, val)
    except ValueError:
        return None

def save_data():
    if not os.path.exists(OUTPUT_FOLDER):
        os.makedirs(OUTPUT_FOLDER)
        print(f"Created folder: {OUTPUT_FOLDER}")

    # Lưu ECG
    df_ecg = pd.DataFrame(data_ecg, columns=['Value'])
    df_ecg.to_csv(f'{OUTPUT_FOLDER}/ecg.csv', index_label='Index')
    
    # Lưu PPG (Red, IR)
    df_ppg = pd.DataFrame(data_ppg, columns=['Red', 'IR'])
    df_ppg.to_csv(f'{OUTPUT_FOLDER}/ppg.csv', index_label='Index')

    # Lưu PCG
    df_pcg = pd.DataFrame(data_pcg, columns=['Value'])
    df_pcg.to_csv(f'{OUTPUT_FOLDER}/pcg.csv', index_label='Index')

    print(f"\nSaved {len(data_ecg)} ECG samples")
    print(f"Saved {len(data_ppg)} PPG samples")
    print(f"Saved {len(data_pcg)} PCG samples")
    print(f"Files saved in '{OUTPUT_FOLDER}/' directory.")

def main():
    try:
        ser = serial.Serial(SERIAL_PORT, BAUD_RATE, timeout=1)
        print(f"Connected to {SERIAL_PORT}. Waiting for data...")
        print("Press Ctrl+C to force stop.")
        
        # Reset ESP32 để bắt đầu đo lại từ đầu
        ser.setDTR(False)
        time.sleep(0.05)
        ser.setDTR(True)

        start_time = time.time()
        
        while True:
            try:
                # Đọc từng dòng từ ESP32
                line = ser.readline().decode('utf-8', errors='ignore')
                
                # Kiểm tra tín hiệu dừng từ Firmware
                if "MEASUREMENT_DONE" in line:
                    print("\nReceived STOP signal from ESP32.")
                    break
                
                parsed = parse_line(line)
                if parsed:
                    tag = parsed[0]
                    if tag == 'E':
                        data_ecg.append(parsed[1])
                    elif tag == 'P':
                        data_ppg.append((parsed[1], parsed[2]))
                    elif tag == 'A':
                        data_pcg.append(parsed[1])
                
                # In trạng thái mỗi giây
                if time.time() - start_time > 1:
                    print(f"\rCollecting... E:{len(data_ecg)} | P:{len(data_ppg)} | A:{len(data_pcg)}", end="")
                    start_time = time.time()

            except UnicodeDecodeError:
                continue

    except KeyboardInterrupt:
        print("\nForce stopping...")
    except Exception as e:
        print(f"Error: {e}")
    finally:
        if 'ser' in locals() and ser.is_open:
            ser.close()
        save_data()

if __name__ == "__main__":
    main()