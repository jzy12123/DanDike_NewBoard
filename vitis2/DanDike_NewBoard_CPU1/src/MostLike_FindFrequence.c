/*
 * File: MostLike_FindFrequence.c
 *
 * MATLAB Coder version            : 23.2
 * C/C++ source code generated on  : 31-May-2024 16:43:36
 */

/* Include Files */
#include "MostLike_FindFrequence.h"
#include "FFT2C_types.h"
#include "rt_nonfinite.h"
#include "rt_nonfinite.h"
#include <string.h>


double MostLike_FindFrequence(const emxArray_real_T *WaveData)
{
  double likelihood[1138];
  double x_segment[780];
  const double *WaveData_data;
  double b_y;
  int d;
  int i;
  int k;
  WaveData_data = WaveData->data;

  for (i = 0; i < 780; i++) {
    x_segment[i] = WaveData_data[i];
  }

  memset(&likelihood[0], 0, 1138U * sizeof(double));

  for (d = 0; d < 1138; d++) {
    double y[780];

    for (k = 0; k < 780; k++) {
      b_y = WaveData_data[(k + d) + 1] - x_segment[k];
      y[k] = b_y * b_y;
    }
    b_y = y[0];
    for (k = 0; k < 779; k++) {
      b_y += y[k + 1];
    }
    likelihood[d] = -b_y;

  }
  if (!rtIsNaN(likelihood[0])) {
    d = 1;
  } else {
    boolean_T exitg1;
    d = 0;
    k = 2;
    exitg1 = false;
    while ((!exitg1) && (k < 1139)) {
      if (!rtIsNaN(likelihood[k - 1])) {
        d = k;
        exitg1 = true;
      } else {
        k++;
      }
    }
  }
  if (d == 0) {
    d = 1;
  } else {
    double ex;
    ex = likelihood[d - 1];
    i = d + 1;
    for (k = i; k < 1139; k++) {
      b_y = likelihood[k - 1];
      if (ex < b_y) {
        ex = b_y;
        d = k;
      }
    }
  }

  return 51200.0 / (((double)d - 1.0) + 1.0);
}

/*
 * File trailer for MostLike_FindFrequence.c
 *
 * [EOF]
 */
