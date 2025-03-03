# Wisensing-ESP32

Wisensing-ESP32 provides an all-in-one solution for Wi-Fi sensing development using ESP32 devices, including:
1. Scripts for CSI amplitude extraction and preprocessing;
2. An interface for easy data annotation and dataset creation;
3. A Jupyter Notebook for training, evaluating and quantizing machine learning models optimized for ESP32 use.
4. A template project for collecting CSI data and executing deep learning models in real time using ESP32 boards.


As a demonstration of Wisensing-ESP32 capabilities, in this README we provide a guide for building a Wi-Fi sensing person detection application on the ESP32 from start to finish.

## Prerequisites

**Software and dependencies**

This project uses Python 3.11. All required packages and libraries for the Data Processing and Model Training modules are listed under ```requirements.txt```. In case you use [conda](https://docs.conda.io/projects/conda/en/stable/index.html), we also provide an environment file for easy setup.

The Onboard Processing module requires the ESP-IDF framework for programming the ESP32 boards, as well as the esp-tflite-micro library. We discuss in more detail how to install these requirements at the relevant section.

We have tested our tool in the following operating systems: 
- Windows 11
- Fedora 38 (Linux kernel v6.8.9)

**Hardware requirements**

To run the Onboard Processing example, you will need at least two ESP32 boards: one acting as a Wi-Fi Acess Point (AP), and the other as a client station (STA) connected to AP in order to collect CSI data and run real-time Wi-Fi sensing with a deep learning model.

## Environment Setup

We recommend using pip:

```
pip install -r requirements.txt
```


## First Module: Data Processing

The scripts, inputs and outputs for the data processing stage are located in the ```DataProcessing``` folder. We do all processing in a non-destructive way, so new files are created in every step.



### 1. Importing raw data

Raw Wi-Fi CSI files should be placed in the ```1_raw_data``` folder, where they will be automatically read by the data processing scripts. Our data processing scripts require RSSI as well as CSI data, so we recommend using [ESP32-CSI-TOOL](https://github.com/StevenMHernandez/ESP32-CSI-Tool) for collecting timestamped CSI data in a format that is compatible with our tool.

We provide three raw CSI files representing a person walking between two ESP32 boards at distances of 140, 150 and 200 cm.

### 2. Data processing

Let's start by running:

```
python 1_format_and_preprocessing.py
```

This script will format the raw CSI data, calculate its amplitude normalized by the RSSI and apply a running mean filter to reduce noise. The running mean window size can be configured in the ```config.py``` file.

The script will create two new folders with the transformed data: ```2_formatted_data``` (for the raw formatted data) and ```3_preprocessed_data``` (for the extracted and filtered amplitudes). We save the preprocessed files using the ```.parquet``` format to reduce memory usage and speed up read/wr

### 3. Data visualization

Now, we can visualize the CSI amplitudes (located in the ```3_preprocessed_data``` folder) using a heatmap plot, by running the ```single_plot.py``` script with the desired file as the argument:
```
python single_plot.py preprocessed-person_150cm_1.parquet
```

A matplotlib window showing the amplitude heatmap will appear. You can zoom in and adjust the scale as needed to better visualize the data.

<img width="500" alt="matplotlib_csi" src="https://github.com/user-attachments/assets/8139feea-4767-47d8-968c-7539603fa4b6" />

The narrow blue strips denote the CSI amplitude drop when a person walks in between the ESP32 boards.


### 4. Data annotation

In order to use this data to train a machine learning model, we first need to annotate it, indicating what class corresponds to the events in a certain window of frames. To do this, we use the ```data_annotation.py``` script, with the source file name and the class we want to annotate as arguments:

```
python data_annotation.py preprocessed-person_150cm_1.parquet person
```

Once the matplotlib window is open, we can annotate the data by right clicking. In case a wrong



### 5. Data slicing
### 6. Assembling the dataset


## Second Module: Model training and quantization

## Third Module: Onboard processing using the ESP32


