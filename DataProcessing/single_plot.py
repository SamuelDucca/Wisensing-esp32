import matplotlib.pyplot as plt
from datetime import datetime
import os
import glob
import pandas as pd
import numpy as np
import sys

def plot_csi(csi_matrix):

    path_to_dir = os.path.dirname(os.path.abspath(__file__))
    raw_data_folder = "1_raw_data"
    raw_folder_path = os.path.join(path_to_dir, raw_data_folder)
    raw_data_csv_files = glob.glob(os.path.join(raw_folder_path, "*.csv"))

    tem_time_stamp = 0

    preprocessed_file_name =("-".join(FILE_NAME.split("-")[1:])).split(".")[0]

    for file in raw_data_csv_files:
        raw_file_name = os.path.basename(file).split(".")[0]
        if(raw_file_name == preprocessed_file_name):
            data = pd.read_csv(file, header=0)
            if(len(data.columns) == 27):
                tem_time_stamp = 1
                time_stamps = data.iloc[:, -1].to_numpy()
                break
    def handleEvent(event):
        if event.xdata is not None:
            show_time_stamp = datetime.fromtimestamp(time_stamps[round(event.xdata)])
            text_box.set_text(f"Timestamp: {show_time_stamp}")  
            fig.canvas.draw_idle() 
            print(time_stamps[round(event.xdata)])

    fig, ax = plt.subplots()

    if(tem_time_stamp):
        text_box = ax.text(0.95, 0.05, "Timestamp: undefined", fontsize=8,bbox=dict(facecolor='white', edgecolor='black', boxstyle='round,pad=0.2'),transform=ax.transAxes, ha="right", va="bottom")
        fig.canvas.mpl_connect("button_press_event", handleEvent)

    csi_matrix = np.transpose(csi_matrix)
    xlim = csi_matrix.shape[1]
    x_label = "Frame No."
    limits = [0, xlim, 1, csi_matrix.shape[0]] 
    im = ax.imshow(csi_matrix, cmap="jet", extent=limits, aspect="auto")
    cbar = ax.figure.colorbar(im, ax=ax)
    cbar.ax.set_ylabel("Amplitude (dB)")

    plt.xlabel(x_label)
    plt.ylabel("Subcarrier Index")
    plt.title("CSI Amplitude Heatmap Plot")
    plt.show()

# Getting the path to all folders and files we need
path_to_dir = os.path.dirname(os.path.abspath(__file__))
data_folder = "3_preprocessed_data"

FILE_NAME = sys.argv[1]

file_path = os.path.join(path_to_dir, data_folder, FILE_NAME)
print("Reading data from: " + file_path)
data_parquet = pd.read_parquet(file_path)
plot_csi(data_parquet)
