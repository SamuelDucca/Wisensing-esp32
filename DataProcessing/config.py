# Global variables for window slicing and sliding window

#Size of each sample window
WINDOW_SIZE = 100

# How many frames should we slide each sample for the sliding window procedure.
WINDOW_SLIDE = 10

# How many times should we slide to create samples on each side of the window
# Total number of slices will be  2*WINDOW_SAMPLE_AMOUNT + 1
WINDOW_SAMPLE_AMOUNT = 3

# Parameter for running mean filter
RUNNING_MEAN_PARAM = 10
