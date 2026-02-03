import pandas as pd
import matplotlib.pyplot as plt
import os

# --- CẤU HÌNH ---
DATA_FOLDER = 'result'  # Tên thư mục chứa file csv
FILE_ECG = os.path.join(DATA_FOLDER, 'ecg.csv')
FILE_PPG = os.path.join(DATA_FOLDER, 'ppg.csv')
FILE_PCG = os.path.join(DATA_FOLDER, 'pcg.csv')

# Cấu hình tần số lấy mẫu (để tính trục thời gian - giây)
# Nếu bạn muốn xem theo số mẫu (Sample Index), hãy đặt USE_TIME_AXIS = False
USE_TIME_AXIS = True
FS_ECG = 100    # 100 Hz
FS_PPG = 100    # 100 Hz
FS_PCG = 4000   # 4000 Hz

def load_data(filepath):
    if not os.path.exists(filepath):
        print(f"Cảnh báo: Không tìm thấy file '{filepath}'")
        return pd.DataFrame()
    return pd.read_csv(filepath)

def plot_separate_files():
    print("Đang đọc dữ liệu từ các file CSV...")
    
    # 1. Đọc dữ liệu
    df_ecg = load_data(FILE_ECG)
    df_ppg = load_data(FILE_PPG)
    df_pcg = load_data(FILE_PCG)

    if df_ecg.empty and df_ppg.empty and df_pcg.empty:
        print("Lỗi: Không có dữ liệu nào để vẽ.")
        return

    # 2. Tạo trục thời gian (nếu cần)
    if USE_TIME_AXIS:
        if not df_ecg.empty: df_ecg['Time'] = df_ecg['Index'] / FS_ECG
        if not df_ppg.empty: df_ppg['Time'] = df_ppg['Index'] / FS_PPG
        if not df_pcg.empty: df_pcg['Time'] = df_pcg['Index'] / FS_PCG
        xlabel = 'Time (seconds)'
        x_col_ecg = 'Time'
        x_col_ppg = 'Time'
        x_col_pcg = 'Time'
    else:
        xlabel = 'Sample Index'
        x_col_ecg = 'Index'
        x_col_ppg = 'Index'
        x_col_pcg = 'Index'

    # 3. Khởi tạo đồ thị
    fig, (ax1, ax2, ax3) = plt.subplots(3, 1, figsize=(12, 10), sharex=False)
    # sharex=False vì số lượng mẫu và thời gian có thể lệch nhau nếu start không đồng bộ tuyệt đối, 
    # nhưng thường ta muốn zoom riêng nên để False linh hoạt hơn.

    fig.suptitle('Kết quả đo: ECG - PPG - PCG (3 Files)', fontsize=16, fontweight='bold')

    # --- Đồ thị 1: ECG ---
    if not df_ecg.empty:
        ax1.plot(df_ecg[x_col_ecg], df_ecg['Value'], color='#d62728', linewidth=1.2, label='ECG')
        ax1.set_ylabel('Amplitude', fontsize=10)
        ax1.set_title(f'ECG (Sample Rate: ~{FS_ECG}Hz)', loc='left', fontweight='bold')
        ax1.legend(loc='upper right')
        ax1.grid(True, linestyle='--', alpha=0.5)
    else:
        ax1.text(0.5, 0.5, 'No ECG Data', horizontalalignment='center', verticalalignment='center')

    # --- Đồ thị 2: PPG ---
    if not df_ppg.empty:
        ax2.plot(df_ppg[x_col_ppg], df_ppg['Red'], color='#ff7f0e', linewidth=1.0, label='Red (Oxy)')
        ax2.plot(df_ppg[x_col_ppg], df_ppg['IR'], color='#1f77b4', linewidth=1.0, label='IR (Heartbeat)', alpha=0.8)
        ax2.set_ylabel('Light Intensity', fontsize=10)
        ax2.set_title(f'PPG (Sample Rate: ~{FS_PPG}Hz)', loc='left', fontweight='bold')
        ax2.legend(loc='upper right')
        ax2.grid(True, linestyle='--', alpha=0.5)
    else:
        ax2.text(0.5, 0.5, 'No PPG Data', horizontalalignment='center', verticalalignment='center')

    # --- Đồ thị 3: PCG ---
    if not df_pcg.empty:
        # PCG tần số cao, vẽ nét mảnh hơn
        ax3.plot(df_pcg[x_col_pcg], df_pcg['Value'], color='#2ca02c', linewidth=0.5, alpha=0.8, label='Audio')
        ax3.set_ylabel('Amplitude', fontsize=10)
        ax3.set_xlabel(xlabel, fontsize=12)
        ax3.set_title(f'PCG (Sample Rate: ~{FS_PCG}Hz)', loc='left', fontweight='bold')
        ax3.legend(loc='upper right')
        ax3.grid(True, linestyle='--', alpha=0.5)
    else:
        ax3.text(0.5, 0.5, 'No PCG Data', horizontalalignment='center', verticalalignment='center')

    plt.tight_layout()
    print("Đang hiển thị biểu đồ...")
    print("Đang lưu biểu đồ vào file 'ket_qua.png'...")
    plt.savefig('ket_qua.png', dpi=300) # Lưu ảnh độ nét cao
    print("Đã xong! Hãy mở file ket_qua.png để xem.")

if __name__ == "__main__":
    plot_separate_files()