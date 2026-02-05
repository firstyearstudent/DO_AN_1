import pandas as pd
import matplotlib.pyplot as plt
import os

# --- CẤU HÌNH ---
OUTPUT_FOLDER = 'result'
DATA_FILE = os.path.join(OUTPUT_FOLDER, 'ppg.csv')
IMAGE_FILE = os.path.join(OUTPUT_FOLDER, 'ppg_result.png') # Tên file ảnh đầu ra

EXPECTED_FS = 1000  

def main():
    print(f"Đang đọc file: {DATA_FILE}...")
    
    if not os.path.exists(DATA_FILE):
        print(f"Lỗi: Không tìm thấy file {DATA_FILE}")
        return

    try:
        df = pd.read_csv(DATA_FILE)
        
        if 'SampleID' not in df.columns:
            print("Lỗi: File CSV thiếu cột SampleID.")
            return

        # Phân tích mất mẫu
        df['ID_Diff'] = df['SampleID'].diff().fillna(1)
        drops = df[df['ID_Diff'] > 1]
        total_lost = int((drops['ID_Diff'] - 1).sum())
        
        # Tạo trục thời gian
        df['Time'] = (df['SampleID'] - df['SampleID'].iloc[0]) / EXPECTED_FS

        # --- VẼ ĐỒ THỊ ---
        fig, (ax1, ax2) = plt.subplots(2, 1, figsize=(12, 8), sharex=True, gridspec_kw={'height_ratios': [3, 1]})

        # Đồ thị 1: Tín hiệu
        ax1.plot(df['Time'], df['Red'], label='Red (Oxy)', color='red', linewidth=1)
        ax1.plot(df['Time'], df['IR'], label='IR (Tim)', color='blue', linewidth=1, alpha=0.7)
        ax1.set_title(f'PPG Signal (Fs={EXPECTED_FS}Hz) | Samples: {len(df)} | Lost: {total_lost}')
        ax1.set_ylabel('ADC Value')
        ax1.legend()
        ax1.grid(True, linestyle='--', alpha=0.5)

        # Đồ thị 2: Integrity
        ax2.plot(df['Time'], df['ID_Diff'], color='green', label='Step Size (Ideal=1)')
        if not drops.empty:
            ax2.scatter(drops['Time'], drops['ID_Diff'], color='red', label='Lost Packets', zorder=5)
        ax2.set_ylabel('Diff')
        ax2.set_xlabel('Time (s)')
        ax2.set_ylim(0, max(5, df['ID_Diff'].max() + 1))
        ax2.legend()
        ax2.grid(True)

        plt.tight_layout()

        # --- LƯU ẢNH (QUAN TRỌNG) ---
        # Lưu ảnh trước khi show()
        print(f"Đang lưu biểu đồ vào: {IMAGE_FILE}")
        plt.savefig(IMAGE_FILE, dpi=300, bbox_inches='tight')
        print("Lưu thành công!")

        # Hiển thị
        plt.show()

    except Exception as e:
        print(f"Lỗi: {e}")

if __name__ == "__main__":
    main()