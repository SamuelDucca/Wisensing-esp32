import pandas as pd
import os
import glob
import numpy as np
import math
from pathlib import Path
import config

# Implementation of a decibel function applied to values different from zero
def db(x: np.array) -> np.array:
  db_matrix = np.where(x == 0, 1, x)  
  db_matrix = 20*np.log10(db_matrix)  
  return db_matrix

# Implementation of the running mean filter
def running_mean(x: np.array, size: int) -> np.array:
   if(size == 1):
      return x
   rm = []
   media_movel = 0
   for i in range(len(x)):
      start = max(0, i + 1 - size)
      janela = x[start : i+1]
      janela_sem_zero = janela[(janela!=0)]
      if(len(janela_sem_zero)>0):
        media_movel = np.mean(janela_sem_zero)
        rm.append(media_movel)
      else:
        rm.append(0)
   return rm

# Getting the path to all folders and files we need
path_to_dir = os.path.dirname(os.path.abspath(__file__))
raw_data_folder = "1_raw_data"
formatted_data_folder = "2_formatted_data"
preprocessed_data_folder = "3_preprocessed_data"
raw_folder_path = os.path.join(path_to_dir, raw_data_folder)
preprocessed_folder_path = os.path.join(path_to_dir, preprocessed_data_folder)
formatted_folder_path = os.path.join(path_to_dir, formatted_data_folder)

# Getting all files in the "raw_data" folder
raw_data_csv_files = glob.glob(os.path.join(raw_folder_path, "*.csv"))
print("Raw files to be processed:")
print(raw_data_csv_files)

#Create folders if none exists
if not os.path.exists(formatted_data_folder):
    print("Creating formatted data folder")
    os.makedirs(formatted_data_folder)
if not os.path.exists(preprocessed_data_folder):
    print("Creating formatted and preprocessed data folder")
    os.makedirs(preprocessed_data_folder)

# Formats all files in the raw_data folder
for file in raw_data_csv_files:

    print("Reading data from: " + file)
    data = pd.read_csv(file, header=None, index_col=None)

    # We can deduce the presence of a timestamp by the header length
    if(len(data.columns) == 27):
        data.columns = ['type','role','mac','rssi','rate','sig_mode','mcs','bandwidth','smoothing','not_sounding',
        'aggregation','stbc','fec_coding','sgi','noise_floor','ampdu_cnt','channel','secondary_channel','local_timestamp',
        'ant','sig_len','rx_state','real_time_set', 'real_timestamp','len','CSI_DATA', 'added_timestamp']
    
        # Remove any extra timestamps added, we don't need it apart from data annotation
        data.drop('added_timestamp', inplace=True, axis=1)
    elif(len(data.columns) == 26):
        data = pd.read_csv(file, header=0)
    else:
       print("Invalid number of columns")

    file_name = os.path.basename(os.path.normpath(file))
    formatted_file_path = os.path.join(formatted_folder_path, "formatted-" + file_name)
    print("Writing data to " + formatted_file_path)
    data.to_csv(formatted_file_path, index=False)

print("File formatting done! \n")
print("Starting file preprocessing")

# Getting all files in the "fomatted_folder_path" folder
formatted_data_csv_files = glob.glob(os.path.join(formatted_folder_path, "*.csv"))

for file in formatted_data_csv_files:

    arquivo = pd.read_csv(file)
    csi_matrix = np.zeros([arquivo.shape[0],64])

    # Computing and normalizing amplitudes in each frame
    # We use a RSSI normalization method dervied from https://doi.org/10.1109/JIOT.2020.3022573 and the CSIKit library
    for j in range(arquivo.shape[0]):
        rss = arquivo.iloc[j][arquivo.columns[3]]
        stri = arquivo.iloc[j][arquivo.columns[-1]]
        stri = stri[1:len(stri)-2]
        values = stri.split(" ")
        values = [float(v) for v in values]
        real_part = values[0::2]
        real_part = np.array(real_part)
        complex_part = values[1::2]
        complex_part = np.array(complex_part)
        amplitudes = np.sqrt((real_part**2)+(complex_part**2))
        square_sum = np.sum(amplitudes ** 2)
        #csi_normal = square_sum/64.0
        factor_scaling = math.sqrt((10**(rss/10))/square_sum)
        csi_matrix[j] = amplitudes*factor_scaling

    # Removing null or constant subcarriers
    # There is also one null subcarrier in the middle (index 32)
    csi_matrix_removed = np.delete(csi_matrix, [0, 1, 2, 3, 4, 5, 32, 63, 62, 61, 60, 59], 1) 
    
    # Converting values to decibel
    csi_matrix_removed = db(csi_matrix_removed)
    
    # Applying the running mean filter to remove noise
    for x in range(len(csi_matrix_removed)):
        csi_matrix_removed[x] = running_mean(csi_matrix_removed[x], config.RUNNING_MEAN_PARAM)
    
    # Exporting data
    df = pd.DataFrame(csi_matrix_removed)
    df.columns = df.columns.map(str)
    file_name = (Path(os.path.basename(os.path.normpath(file))).stem).split("-")[1]
    preprocessed_file_path = os.path.join(preprocessed_folder_path, "preprocessed-" + file_name +".parquet")
    print("Saving to:")
    print(preprocessed_file_path)
    df.to_parquet(preprocessed_file_path, index=False)

print("File preprocessing done! \n")