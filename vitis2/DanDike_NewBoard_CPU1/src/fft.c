/*
 * File: fft.c
 *
 * MATLAB Coder version            : 23.2
 * C/C++ source code generated on  : 31-May-2024 16:43:36
 */

/* Include Files */
#include "fft.h"
#include "FFT2C_emxutil.h"
#include "FFT2C_types.h"
#include "FFTImplementationCallback.h"
#include "rt_nonfinite.h"

/* Function Definitions */
/*
 * Arguments    : const emxArray_real_T *x
 *                emxArray_creal_T *y
 * Return Type  : void
 */
void fft(const emxArray_real_T *x, emxArray_creal_T *y)
{
  emxArray_real_T *costab;
  emxArray_real_T *sintab;
  emxArray_real_T *sintabinv;
  if (x->size[0] == 0) {
    y->size[0] = 0;
  } else {
    int N2blue;
    int pmax;
    boolean_T useRadix2;
    useRadix2 = ((x->size[0] & (x->size[0] - 1)) == 0);
    N2blue = 1;
    if (useRadix2) {
      pmax = x->size[0];
    } else {
      N2blue = (x->size[0] + x->size[0]) - 1;
      pmax = 31;
      if (N2blue <= 1) {
        pmax = 0;
      } else {
        int pmin;
        boolean_T exitg1;
        pmin = 0;
        exitg1 = false;
        while ((!exitg1) && (pmax - pmin > 1)) {
          int k;
          int pow2p;
          k = (pmin + pmax) >> 1;
          pow2p = 1 << k;
          if (pow2p == N2blue) {
            pmax = k;
            exitg1 = true;
          } else if (pow2p > N2blue) {
            pmax = k;
          } else {
            pmin = k;
          }
        }
      }
      N2blue = 1 << pmax;
      pmax = N2blue;
    }
    emxInit_real_T(&costab, 2);
    emxInit_real_T(&sintab, 2);
    emxInit_real_T(&sintabinv, 2);
    c_FFTImplementationCallback_gen(pmax, useRadix2, costab, sintab, sintabinv);
    if (useRadix2) {
      c_FFTImplementationCallback_r2b(x, x->size[0], costab, sintab, y);
    } else {
      c_FFTImplementationCallback_dob(x, N2blue, x->size[0], costab, sintab,
                                      sintabinv, y);
    }
    emxFree_real_T(&sintabinv);
    emxFree_real_T(&sintab);
    emxFree_real_T(&costab);
  }
}

/*
 * File trailer for fft.c
 *
 * [EOF]
 */
