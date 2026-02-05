import pandas as pd
import numpy as np
import scipy.signal as signal
import matplotlib.pyplot as plt
import os

# --- CẤU HÌNH ---
OUTPUT_FOLDER = 'result'
DATA_FILE = os.path.join(OUTPUT_FOLDER, 'ppg.csv')
IMAGE_FILE = os.path.join(OUTPUT_FOLDER, 'hr_result.png') # Tên file ảnh đầu ra

FS = 1000  # Tần số lấy mẫu

def calculate_heart_rate():
    if not os.path.exists(DATA_FILE):
        print(f"Lỗi: Không tìm thấy file {DATA_FILE}")
        return

    # 1. Đọc dữ liệu
    df = pd.read_csv(DATA_FILE)
    ir_raw = df['IR'].values

    # 2. Xử lý tín hiệu
    ir_inverted = -1 * ir_raw
    ir_centered = ir_inverted - np.mean(ir_inverted)

    # Lọc thông dải (Bandpass: 0.5Hz - 4Hz)
    low_cut = 0.5
    high_cut = 4.0
    nyquist = 0.5 * FS
    b, a = signal.butter(2, [low_cut/nyquist, high_cut/nyquist], btype='band')
    ir_filtered = signal.filtfilt(b, a, ir_centered)

    # 3. Tìm đỉnh (Prominence=10 để nhạy hơn)
    peaks, _ = signal.find_peaks(ir_filtered, distance=FS/3, prominence=10)

    # 4. Tính toán BPM
    bpm = 0
    if len(peaks) >= 2:
        diffs = np.diff(peaks)
        mean_distance = np.mean(diffs)
        bpm = (FS * 60) / mean_distance

    # --- HIỂN THỊ KẾT QUẢ ---
    print("="*40)
    print(f"KẾT QUẢ PHÂN TÍCH NHỊP TIM")
    print("="*40)
    print(f"Số nhịp phát hiện   : {len(peaks)}")
    print(f"NHỊP TIM ĐO ĐƯỢC    : {bpm:.2f} BPM")
    print("="*40)

    # --- VẼ VÀ LƯU ẢNH ---
    plt.figure(figsize=(12, 6))
    
    plt.plot(ir_filtered, label='Filtered PPG Signal', color='blue', linewidth=1)
    plt.plot(peaks, ir_filtered[peaks], 'rx', markersize=10, label='Detected Beats')
    
    plt.title(f"Heart Rate Analysis: {bpm:.1f} BPM ({len(peaks)} beats detected)")
    plt.xlabel('Samples')
    plt.ylabel('Amplitude')
    plt.legend()
    plt.grid(True)
    plt.tight_layout()

    # Lưu ảnh
    print(f"Đang lưu biểu đồ vào: {IMAGE_FILE}")
    plt.savefig(IMAGE_FILE, dpi=300)
    print("Lưu thành công!")

    plt.show()

if __name__ == "__main__":
    calculate_heart_rate()