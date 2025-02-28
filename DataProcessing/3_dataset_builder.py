import matplotlib.pyplot as plt
import sys
from pathlib import Path
import os
import glob
import pandas as pd
import numpy as np
from enum import Enum

class Partition(Enum):
    TRAIN, TEST = range(2)

# Include your classes here
class Label(Enum):
    NOISE, PERSON = range(2)


#Use dataset_source.txt as argument


# Getting the path to all folders and files we need
path_to_dir = os.path.dirname(os.path.abspath(__file__))
sliced_data_folder = "4_sliced_data"
labeled_data_folder ="5_datasets"


labeled_file_folder_path = os.path.join(path_to_dir, labeled_data_folder)

#Create folder if none exists
if not os.path.exists(labeled_file_folder_path):
    os.makedirs(labeled_file_folder_path)


args = sys.argv[1:]

# Check for missing args
if len(args) < 2:
    print("Argument error: please enter the source file and dataset name as arguments")
    exit()

source_file_name = args[0]
dataset_name = args[1]

print(f'Reading data from {source_file_name} \n')

df_source = pd.read_csv(source_file_name, skipinitialspace=True)
df_files = pd.DataFrame(columns=['Type', 'Partition', "FileList"])
files_to_label_list = []

# Iterate over every row of the source instruction file
for row in df_source.itertuples(index=True, name='Pandas'):

    file_name_root = row.FILE
    file_type = row.TYPE
    
    train_file_type_path = os.path.join(path_to_dir, sliced_data_folder, file_type, Partition.TRAIN.name.lower(), row.FILE)
    test_file_type_path = os.path.join(path_to_dir, sliced_data_folder, file_type, Partition.TEST.name.lower(), row.FILE)

    print("Train path")
    print(train_file_type_path)
    print("Test path")
    print(test_file_type_path)
    print(" ")

    # use glob to get files
    # https://pynative.com/python-glob/
    train_subfiles_path_list = glob.glob(train_file_type_path + "*")
    test_subfiles_path_list = glob.glob(test_file_type_path + "*")
    print("Train subfiles: " + str(len(train_subfiles_path_list)))
    print("Test subfiles: " + str(len(test_subfiles_path_list)))
    print(" ")

    files_to_label_list.append([row.TYPE, Partition.TRAIN.name.lower(), train_subfiles_path_list])
    files_to_label_list.append([row.TYPE, Partition.TEST.name.lower(), test_subfiles_path_list])


df_files = pd.DataFrame(files_to_label_list, columns=["Type", "Partition", "FileList"])
print(df_files)
print("")

# Creating base (empty) dataframe
# Label (0 for nothing, 1 for person, etc) is in first position
slice_size = 100
subcarrier_size = 52
headers = ["{}x{}".format(frame, sub) for frame in range(1, slice_size+1) for sub in range (1, subcarrier_size+1)]
headers.insert(0, "label")


def flatten_data(df_data):

    # We aggregate the data in a Python list and later pass them to a Dataframe
    # for performance reasons
    train_list = []
    test_list = []

    for row in df_data.itertuples():

        print("Type is " + row.Type + " and label is " + str(Label[row.Type.upper()]))
        print("This is " + row.Partition + " data \n")

        for file in row.FileList:
            print("Reading data from: " + file)
            data = pd.read_parquet(file)

            # Flatten and label data
            flattened_array = data.to_numpy()
            flattened_array = flattened_array.flatten()
            labeled_array = np.concatenate(([Label[row.Type.upper()].value], flattened_array)) # Adds label based on the Label enum
            labeled_array = labeled_array.reshape(1,-1) # array is in column shape, we need to change to row

            if Partition[row.Partition.upper()] is Partition.TEST:
                test_list.append(labeled_array[0].tolist())
            elif Partition[row.Partition.upper()] is Partition.TRAIN:
                train_list.append(labeled_array[0].tolist())
            else:
                print("ERROR: Couldn't derive partition from the data!")
                exit()

        print("Finished reading files from this row! \n")

    labeled_train_df = pd.DataFrame(train_list, columns=headers)
    labeled_test_df = pd.DataFrame(test_list, columns=headers)

    print("Finished flattening data!")

    print("Final TRAIN dataframe:")
    print(labeled_train_df)
    print("Final TEST dataframe:")
    print(labeled_test_df)

    train_dataset_file_name = Partition.TRAIN.name +"_"+ dataset_name + ".parquet"
    train_dataset_file_path = os.path.join(labeled_file_folder_path, train_dataset_file_name)
    labeled_train_df.to_parquet(os.path.join(train_dataset_file_path), index=False)

    test_dataset_file_name = Partition.TEST.name +"_"+ dataset_name + ".parquet"
    test_dataset_file_path = os.path.join(labeled_file_folder_path, test_dataset_file_name)
    labeled_test_df.to_parquet(os.path.join(test_dataset_file_path), index=False)

flatten_data(df_files)

print("Done labeling!")

