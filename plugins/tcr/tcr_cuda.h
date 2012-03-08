#ifndef TCR_CUDA_H
#define TCR_CUDA_H

extern "C" bool IterateTCR( float alpha, float beta, float step_size, int iterations, float* estimate, float* gradient, float* estimate_ch, float* sensitivity, float* mask, float* meas, int rows, int cols, int channels, int sets, int phases );

#endif
