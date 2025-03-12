/*
 * File: FFT2C.h
 *
 * MATLAB Coder version            : 23.2
 * C/C++ source code generated on  : 31-May-2024 16:43:36
 */

#ifndef FFT2C_H
#define FFT2C_H

/* Include Files */
#include "FFT2C_types.h"
#include "rtwtypes.h"
#include <stddef.h>
#include <stdlib.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Function Declarations */
extern void FFT2C(const emxArray_real_T *var1, double *fundamental_frequency,
                  double *fundamental_value, double harmonic_info_data[],
                  int harmonic_info_size[2]);

#ifdef __cplusplus
}
#endif

#endif
/*
 * File trailer for FFT2C.h
 *
 * [EOF]
 */
