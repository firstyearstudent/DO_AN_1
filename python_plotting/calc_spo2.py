import pandas as pd
import numpy as np
import scipy.signal as signal
import matplotlib.pyplot as plt
import os

# --- CẤU HÌNH ---
OUTPUT_FOLDER = 'result'
DATA_FILE = os.path.join(OUTPUT_FOLDER, 'ppg.csv')
IMAGE_FILE = os.path.join(OUTPUT_FOLDER, 'spo2_result.png') # Tên file ảnh đầu ra

FS = 1000  # Tần số lấy mẫu

def calculate_vital_signs():
    if not os.path.exists(DATA_FILE):
        print(f"Lỗi: Không tìm thấy file {DATA_FILE}")
        return

    try:
        # 1. Đọc dữ liệu
        df = pd.read_csv(DATA_FILE)
        
        red = df['Red'].values
        ir = df['IR'].values
        
        # 2. Xử lý nhịp tim (Dùng IR)
        ir_inverted = -1 * ir
        b, a = signal.butter(2, 5 / (FS / 2), 'low') 
        ir_filtered = signal.filtfilt(b, a, ir_inverted)
        
        # Tìm đỉnh (Prominence thấp để bắt nhạy hơn)
        peaks, _ = signal.find_peaks(ir_filtered, distance=FS/3, prominence=10)
        
        bpm = 0
        if len(peaks) >= 2:
            diffs = np.diff(peaks)
            mean_dist = np.mean(diffs)
            bpm = (FS * 60) / mean_dist

        # 3. Tính SpO2 (AC/DC Ratio)
        ac_red = np.std(red)
        dc_red = np.mean(red)
        ac_ir = np.std(ir)
        dc_ir = np.mean(ir)
        
        spo2 = 0
        if dc_red != 0 and dc_ir != 0:
            R = (ac_red / dc_red) / (ac_ir / dc_ir)
            spo2 = 104 - 17 * R
            # Giới hạn giá trị
            if spo2 > 100: spo2 = 99.9
            if spo2 < 50: spo2 = 50.0 

        # 4. Hiển thị kết quả Text
        print("="*30)
        print(f"KẾT QUẢ PHÂN TÍCH (FS={FS}Hz)")
        print("="*30)
        print(f"Số nhịp phát hiện : {len(peaks)}")
        print(f"NHỊP TIM (HR)     : {bpm:.2f} BPM")
        print(f"NỒNG ĐỘ OXY (SpO2): {spo2:.2f} %")
        print("="*30)
        
        # 5. VẼ VÀ LƯU ẢNH
        plt.figure(figsize=(10, 5))
        plt.plot(ir_filtered, label='IR Signal (Filtered)')
        plt.plot(peaks, ir_filtered[peaks], 'rx', label='Heart Peaks')
        
        plt.title(f"SpO2 Analysis: {spo2:.1f}% | HR: {bpm:.1f} BPM")
        plt.xlabel("Sample Index")
        plt.ylabel("Amplitude")
        plt.legend()
        plt.grid(True)
        plt.tight_layout()

        # Lưu ảnh trước khi show
        print(f"Đang lưu biểu đồ vào: {IMAGE_FILE}")
        plt.savefig(IMAGE_FILE, dpi=300)
        print("Lưu thành công!")
        
        plt.show()

    except Exception as e:
        print(f"Lỗi tính toán: {e}")

if __name__ == "__main__":
    calculate_vital_signs()