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

The Onboard Processing module requires the ESP-IDF framework v5.4 for programming the ESP32 boards, as well as the esp-tflite-micro library. We discuss in more detail how to install these requirements at the relevant section.

We have tested our tool in the following operating systems: 
- Windows 11
- Fedora 38 (Linux kernel v6.8.9)
- Lubuntu 24.04.2

**Hardware requirements**

To run the Onboard Processing example, you will need at least two ESP32 boards: one acting as a Wi-Fi Acess Point (AP), and the other as a client station (STA) connected to AP in order to collect CSI data and run real-time Wi-Fi sensing with a deep learning model.

## Environment Setup

We recommend using [conda](https://docs.conda.io/projects/conda/en/stable/index.html) to install the dependencies:

```
conda env create -f environment.yml
````
And then running the following to activate the environment:
```
conda activate wisensing-esp32
```

If you don't want to use conda, you can use pip as well:

```
pip install -r requirements.txt
```


## First Module: Data Processing

The scripts, inputs and outputs for the data processing stage are located in the ```DataProcessing``` folder. We do all processing in a non-destructive way, so new files are created in every step.



### 1. Importing raw data

Raw Wi-Fi CSI files should be placed in the ```1_raw_data``` folder, where they will be automatically read by the data processing scripts. Our data processing scripts require RSSI as well as CSI data, so we recommend using [ESP32-CSI-TOOL](https://github.com/StevenMHernandez/ESP32-CSI-Tool) for collecting timestamped CSI data in a format that is compatible with our tool.

We provide three raw CSI files representing a person walking between two ESP32 boards at distances of 140, 150 and 200 cm. In the following sections, we will use this data to demonstrate Wisensing-ESP32's capabilities and build a dataset suitable for training machine learning models using CSI data.

### 2. Data processing

Let's start by running:

```
python 1_format_and_preprocess.py 
```

This script will format the raw CSI data, calculate its amplitude normalized by the RSSI and apply a running mean filter to reduce noise. The running mean window size can be configured in the ```config.py``` file.

The script will create two new folders with the transformed data: ```2_formatted_data``` (for the raw formatted data) and ```3_preprocessed_data``` (for the extracted and filtered amplitudes). We save the preprocessed files using the ```.parquet``` format to reduce memory usage and improve read/write speed.

### 3. Data visualization

Now, we can visualize the CSI amplitudes (located in the ```3_preprocessed_data``` folder) using a heatmap plot, by running the ```single_plot.py``` script with the desired file as the argument:
```
python single_plot.py preprocessed-person_150cm_1.parquet
```

A matplotlib window showing the amplitude heatmap will appear. You can zoom in using the loupe in the lower left corner, as well as adjust the scale as needed to better visualize the data.

<img width="500" alt="matplotlib_csi" src="https://github.com/user-attachments/assets/8139feea-4767-47d8-968c-7539603fa4b6" />

The narrow blue strips denote the CSI amplitude drop when a person walks in between the ESP32 boards. Simply close the window when you finish analyzing the data.


### 4. Data annotation

In order to use this data to train a machine learning model, we first need to annotate it, indicating what class corresponds to the events in a certain window of frames. To do this, we use the ```data_annotation.py``` script, with the source file name and the class we want to annotate as arguments: ``python data_annotation.py source_file.parquet class_name``

For example, let's annotate the "person" class in the experiment with the boards placed 150 cm apart:

```
python data_annotation.py preprocessed-person_150cm_1.parquet person
```

Once the matplotlib window is open, let's zoom in between frames 0 and 2000 to better visualize the data. Now, we can annotate the data by *right clicking* the narrow amplitude drops where a person interfered with the signal. 

<img width="500" alt="image" src="https://github.com/user-attachments/assets/b626e3c3-e536-400b-a1e8-baa6fdab77e3" />


A blue dot with a blue horizontal bar will appear, showing you the size of the window you are annotating for that sample. Avoid any juxtaposition between samples, as that could lead to data contamination and reduced performance due to duplicated samples in the dataset.

In case you make a mistake, use ``CTRL+Z`` to undo the last annotation.

When you are done, simply close the window. Your annotated frames will be automatically saved in the ``slicing_source.txt`` file, in a format suitable for input to our next scripts. We provide a ``slicing_source_demo.txt`` file with all samples from the three experiments already annotated, so you don't have to manually annotate anything to proceed.


### 5. Data slicing

Now, we will now run a script to slice our annotated windows to create separate samples from each one of them applying a sliding window procedure. You can customize the window size and amount of sliding in the ``config.py`` file, but we recommend maintaining the default values for this demonstration.

Let's use the provided ``slicing_source_demo.txt`` file for slicing the data:

```
python 2_slicing.py slicing_source_demo.txt
```

This will create a new ``4_sliced_data`` folder containing our separated train and test samples (80/20 split), ready for assembling our dataset in the next step.


### 6. Assembling the dataset

We have all of our data ready, now we just need to assemple our dataset! We will use the ``3_dataset_builder.py`` script, with arguments including our dataset source file and the dataset name we wish to create.

The ``dataset_source.txt`` file is provided as an example. You can edit it to remove or add data from any experiment or class.

Run:

```
python 3_dataset_builder.py  dataset_source.txt my_dataset
```

A new ``5_datasets`` folder will be created, containing both train and test datasets in ``.parquet`` format. We can now use this dataset to train our deep learning models!


## Second Module: Model training and quantization

The second module is provided in the ``ModelTraining`` folder. It consists of a Jupyer Notebook contanining all necessary code and documentation for:

1. Data importing and feature engineering;
2. Model training using Tensorflow;
3. Model quantization using LiteRT;
4. Model convertion to C data array.

It enables you to run the code step-by-step, alongside instructions and explanations.

To open the Jupyter Notebook, navigate to the ``ModelTraining`` folder using the terminal. Then, run:

```
jupyter notebook
```

This will open a new tab with the Jupyter Notebook interface in your default browser. Click the ``model_training_and_quantization.ipynb`` file to open the Notebook and run the code. If you are not familiar with Jupyter Notebooks, we recommend [reading the documentation](https://jupyter-notebook.readthedocs.io/en/latest/) before proceeding.

This module will generate four files as output: ``preprocessing.c``, ``preprocessing.h``, ``model.c``, ``preprocessing.h``. Copy the files to the ``OnboardProcessing/esp-wisense/examples/person-detection/main`` to use your model and feature extraction process in the next module using the ESP32.


## Third Module: Onboard processing using the ESP32

The onboard processing module constitutes a component that can be included as a dependency for ESP-IDF projects. Its source code and examples are located under the ```OnboardProcessing/esp-wisense```.

### Install and setup

The ESP32 toolchain and ESP-IDF scripts are required for proper use of this module. Follow the Espressif [Get Started guide](https://docs.espressif.com/projects/esp-idf/en/v5.4/esp32/get-started/index.html) to install the essential software. In case of Linux or macOS setup, make sure to install tools for your desired chip targets when running the install script.

The component and examples were developed using ESP-IDF v5.4 stable, but the project dependencies only require >= v4.2.0.

The `.c`, `.cc` and `.h` files generated by the [Model Training Module](./ModelTraining/) must be informed to the build system on the `CMakeLists.txt` inside the `main` directory of the project. The example projects expect these files directly on the `main` directory.

### Configuration

There are currently two available example projects: [Person Detection](./OnboardProcessing/esp-wisense/examples/person-detection/) and [SoftAP](./OnboardProcessing/esp-wisense/examples/softAP/).

Before compiling a project, define the compile target:
```
idf.py set-target <chip_name>
```
Where ```<chip_name>``` indicates the board's ESP32 SoC (e.g. esp32, esp32s3, esp32c3). Additionally, in order to establish Wi-Fi connections, the SSID and password must be provided. Open the configuration menu (```idf.py menuconfig```) and navigate to ```Component config > WiSense``` to view the [available options](./OnboardProcessing/esp-wisense/README.md#config-options).

### Building

```
idf.py build
```
The initial build may take several minutes depending on the configuration of the host machine. Further modifications to the ```menuconfig``` will also require longer builds.

### Flashing and monitoring

Flash the built image into the board and monitor its serial output.
```
idf.py [-p <PORT>] flash monitor
```
The argument ```-p <PORT>``` is optional and denotes the serial port of the connected ESP32 board. Not providing this specifier will trigger an iterative test on the available ports.
(To exit the serial monitor, type ```Ctrl-]```)


Refer to [esp-wisense](./OnboardProcessing/esp-wisense/README.md) for further information.

