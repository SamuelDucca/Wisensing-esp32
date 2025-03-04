#pragma once

#define SUBCARRIER_COUNT 52
#define FRAME_WINDOW_SIZE 100
#define PCA_COMPONENTS 42

typedef struct {
  float mean;
  float std;
} standard_scaler;

extern const standard_scaler scaler[FRAME_WINDOW_SIZE][SUBCARRIER_COUNT];
extern const float pca_means[FRAME_WINDOW_SIZE][SUBCARRIER_COUNT];
extern const float pca_matrix[PCA_COMPONENTS]
                             [SUBCARRIER_COUNT * FRAME_WINDOW_SIZE];
