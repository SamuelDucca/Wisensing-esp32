import os
import pandas as pd  
import sys
from enum import Enum
from pathlib import Path
import config


class Partition(Enum):
    TRAIN, TEST = range(2)


def slice_data(slice_list, partition:Partition, data, sliced_file_main_folder, sliced_file_name):

    # Slice window size, number of subslices on each side and amount of slide between subslices
    # Obs: subslice is a slice containing the same event, but slightly skewed left or right for data augmentation purposes
    # Change this variables to your liking at config.py

    slice_size = config.WINDOW_SIZE
    slice_amount = config.WINDOW_SAMPLE_AMOUNT # actual number of slices is 2*slice_amount + 1
    slice_range = range(-slice_amount, slice_amount+1)
    slice_slide = config.WINDOW_SLIDE

    print(f' ----- Slicing data for {partition.name} -----')

    #Checks if subfolder for partition already exists
    subfolder_path = os.path.join(sliced_file_main_folder, partition.name.lower())
    if not os.path.exists(subfolder_path):
        print(f'Creating partition subfolder at: {subfolder_path}')
        os.makedirs(subfolder_path)

    # Iterates over list of events
    for slice_center in slice_list:
        slice_start = int(int(slice_center) - slice_size/2) #casting to int because of iloc
        slice_end = int(int(slice_center) + slice_size/2)

        print(f'Iterating over frame # {slice_center}. Start: {slice_start}   End: {slice_end}')

        # Creates subslices for each event
        for slice_number in slice_range:
            sliced_data = data.iloc[slice_start+(slice_slide*slice_number):slice_end+(slice_slide*slice_number)]
            # Path is like: sliced/train/sliced_file_name_3500_2.parquet
            path = os.path.join(subfolder_path, sliced_file_name) + "_" + str(slice_center) +"_"+ str(slice_number) + ".parquet"
            sliced_data.to_parquet(path)

    print(f'Slicing done for {sliced_file_name} in the {partition.name} partition.')
    return

# Getting the path to all folders and files we need
path_to_dir = os.path.dirname(os.path.abspath(__file__))

preprocessed_data_folder = "3_preprocessed_data"
sliced_data_folder = "4_sliced_data"

args = sys.argv[1:]

# Check for missing args
if len(args) < 1:
    print("Argument error: missing source file!")
    exit()

source_file_name = args[0]

print(f'Reading data from {source_file_name}')

df_source = pd.read_csv(source_file_name, skipinitialspace=True)

#Create folder if none exists
if not os.path.exists(sliced_data_folder):
    os.makedirs(sliced_data_folder)

# Iterate over every row of source.txt
for row in df_source.itertuples(index=True, name='Pandas'):

    slice_center_list =  row.FRAMES.split(" ")

    print("Slicing around the following frames:")
    print(slice_center_list)

    # We need to remove the "preprocessed-" and file extension to get the raw file name
    raw_file_name = Path(row.IN).stem.split("-")[1] 
    preprocessed_file_name =  row.IN
    sliced_file_name = "sliced-" + raw_file_name # .parquet is added later
    
    preprocessed_file_path = os.path.join(path_to_dir, preprocessed_data_folder, preprocessed_file_name)
    destination_folder_name = row.OUT
    sliced_file_main_folder = os.path.join(path_to_dir, sliced_data_folder, destination_folder_name)

    #Separate between train and test partitions
    train_perc = 0.8
    test_perc = 1 - train_perc
    list_len = len(slice_center_list)
    index_split = int(list_len*train_perc)

    print("---------- TRAIN AND TEST SPLIT -----------")
    print(f'Index split: {index_split}')

    slice_train_list = slice_center_list[:index_split]
    slice_test_list =  slice_center_list[index_split:]

    print(f'Train list has len {len(slice_train_list)}:')
    print(slice_train_list)
    print(f'Test list has len {len(slice_test_list)}:')
    print(slice_test_list)

    # Read preprocessed file
    data = pd.read_parquet(preprocessed_file_path)

    #Slicing train data
    slice_data(slice_train_list, Partition.TRAIN, data, sliced_file_main_folder, sliced_file_name)
    #Slicing test data
    slice_data(slice_test_list, Partition.TEST, data, sliced_file_main_folder, sliced_file_name)

    print("Completed all slicing!")





