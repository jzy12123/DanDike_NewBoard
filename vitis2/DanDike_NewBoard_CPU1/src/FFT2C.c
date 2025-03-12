/*
 * File: FFT2C.c
 *
 * MATLAB Coder version            : 23.2
 * C/C++ source code generated on  : 31-May-2024 16:43:36
 */

/* Include Files */
#include "FFT2C.h"
#include "FFT2C_emxutil.h"
#include "FFT2C_types.h"
#include "MostLike_FindFrequence.h"
#include "fft.h"
#include "findpeaks.h"
#include "rt_nonfinite.h"
#include "rt_nonfinite.h"
#include <math.h>
#include <string.h>

/* Function Declarations */
static double rt_hypotd_snf(double u0, double u1);

/* Function Definitions */
/*
 * Arguments    : double u0
 *                double u1
 * Return Type  : double
 */
static double rt_hypotd_snf(double u0, double u1)
{
  double a;
  double b;
  double y;
  a = fabs(u0);
  b = fabs(u1);
  if (a < b) {
    a /= b;
    y = b * sqrt(a * a + 1.0);
  } else if (a > b) {
    b /= a;
    y = a * sqrt(b * b + 1.0);
  } else if (rtIsNaN(b)) {
    y = rtNaN;
  } else {
    y = a * 1.4142135623730951;
  }
  return y;
}

/*
 * Arguments    : const emxArray_real_T *var1
 *                double *fundamental_frequency
 *                double *fundamental_value
 *                double harmonic_info_data[]
 *                int harmonic_info_size[2]
 * Return Type  : void
 */
void FFT2C(const emxArray_real_T *var1, double *fundamental_frequency,
           double *fundamental_value, double harmonic_info_data[],
           int harmonic_info_size[2])
{
  emxArray_creal_T *var2;
  emxArray_real_T *P1;
  emxArray_real_T *P2;
  emxArray_real_T *f;
  emxArray_real_T *locs;
  emxArray_real_T *pks;
  emxArray_real_T *x;
  emxArray_real_T *y;
  creal_T *var2_data;
  double harmonic_info[30];
  double peak_indices[10];
  const double *var1_data;
  double b;
  double im;
  double percentage_of_fundamental;
  double *P1_data;
  double *P2_data;
  double *locs_data;
  double *x_data;
  double *y_data;
  int b_i;
  int harmonic_count;
  int i;
  int loop_ub;
  int peak_count;
  int x_size;
  boolean_T exitg1;
  var1_data = var1->data;
  /* 找频率 */
  b = MostLike_FindFrequence(var1) * 1024.0;
  /*  1024*50采样频率 */
  /*  FFT变换 */
  /*  数据点数 */
  loop_ub = var1->size[0];
  emxInit_real_T(&P2, 1);
  i = P2->size[0];
  P2->size[0] = var1->size[0];
  emxEnsureCapacity_real_T(P2, i);
  P2_data = P2->data;
  for (i = 0; i < loop_ub; i++) {
    P2_data[i] = var1_data[i];
  }
  emxInit_creal_T(&var2);
  fft(P2, var2);
  var2_data = var2->data;
  harmonic_count = var1->size[0];
  loop_ub = var2->size[0];
  for (i = 0; i < loop_ub; i++) {
    double ai;
    im = var2_data[i].re;
    ai = var2_data[i].im;
    if (ai == 0.0) {
      percentage_of_fundamental = im / (double)harmonic_count;
      im = 0.0;
    } else if (im == 0.0) {
      percentage_of_fundamental = 0.0;
      im = ai / (double)harmonic_count;
    } else {
      percentage_of_fundamental = im / (double)harmonic_count;
      im = ai / (double)harmonic_count;
    }
    var2_data[i].re = percentage_of_fundamental;
    var2_data[i].im = im;
  }
  i = var2->size[0];
  x_size = P2->size[0];
  P2->size[0] = var2->size[0];
  emxEnsureCapacity_real_T(P2, x_size);
  P2_data = P2->data;
  for (harmonic_count = 0; harmonic_count < i; harmonic_count++) {
    P2_data[harmonic_count] = rt_hypotd_snf(var2_data[harmonic_count].re,
                                            var2_data[harmonic_count].im);
  }
  emxFree_creal_T(&var2);
  im = (double)var1->size[0] / 2.0;
  loop_ub = (int)floor(im + 1.0) - 1;
  emxInit_real_T(&P1, 1);
  i = P1->size[0];
  P1->size[0] = loop_ub + 1;
  emxEnsureCapacity_real_T(P1, i);
  P1_data = P1->data;
  for (i = 0; i <= loop_ub; i++) {
    P1_data[i] = P2_data[i];
  }
  if (loop_ub < 2) {
    i = 0;
    x_size = 0;
    loop_ub = 0;
  } else {
    i = 1;
    x_size = 1;
  }
  loop_ub -= x_size;
  for (x_size = 0; x_size < loop_ub; x_size++) {
    harmonic_count = i + x_size;
    P1_data[harmonic_count] = 2.0 * P2_data[harmonic_count];
  }
  emxFree_real_T(&P2);
  emxInit_real_T(&f, 2);
  i = f->size[0] * f->size[1];
  f->size[0] = 1;
  f->size[1] = (int)im + 1;
  emxEnsureCapacity_real_T(f, i);
  P2_data = f->data;
  loop_ub = (int)im;
  for (i = 0; i <= loop_ub; i++) {
    P2_data[i] = (double)i * b / (double)var1->size[0];
  }
  /*  其中Fs/N为频率分辨率 */
  /*  找到频谱的前几个最大值及其对应的频率 */
  emxInit_real_T(&pks, 1);
  emxInit_real_T(&locs, 1);
  findpeaks(P1, pks, locs);
  locs_data = locs->data;
  emxFree_real_T(&pks);
  /*  自动调整要找到的峰值数量，最大不超过20个 */
  /*  保证找到的点两边没有别的幅度高的点 */
  memset(&peak_indices[0], 0, 10U * sizeof(double));
  peak_count = 0;
  b_i = 0;
  emxInit_real_T(&y, 1);
  emxInit_real_T(&x, 1);
  while ((b_i <= locs->size[0] - 1) && (peak_count < 10)) {
    boolean_T guard1;
    guard1 = false;
    if (b_i + 1 == 1) {
      guard1 = true;
    } else {
      boolean_T b_x_data[15360];
      boolean_T b_y;
      i = x->size[0];
      x->size[0] = b_i;
      emxEnsureCapacity_real_T(x, i);
      x_data = x->data;
      for (i = 0; i < b_i; i++) {
        x_data[i] = locs_data[b_i] - locs_data[i];
      }
      i = x->size[0];
      x_size = y->size[0];
      y->size[0] = x->size[0];
      emxEnsureCapacity_real_T(y, x_size);
      y_data = y->data;
      for (harmonic_count = 0; harmonic_count < i; harmonic_count++) {
        y_data[harmonic_count] = fabs(x_data[harmonic_count]);
      }
      x_size = y->size[0];
      loop_ub = y->size[0];
      for (i = 0; i < loop_ub; i++) {
        b_x_data[i] = (y_data[i] > 1.0);
      }
      b_y = true;
      harmonic_count = 1;
      exitg1 = false;
      while ((!exitg1) && (harmonic_count <= x_size)) {
        if (!b_x_data[harmonic_count - 1]) {
          b_y = false;
          exitg1 = true;
        } else {
          harmonic_count++;
        }
      }
      if (b_y) {
        guard1 = true;
      }
    }
    if (guard1) {
      /*  保证当前峰值点与之前的峰值点不相邻 */
      peak_count++;
      peak_indices[peak_count - 1] = locs_data[b_i];
    }
    b_i++;
  }
  emxFree_real_T(&x);
  emxFree_real_T(&y);
  emxFree_real_T(&locs);
  /*  基波 */
  *fundamental_value = P1_data[(int)peak_indices[0] - 1];
  *fundamental_frequency = P2_data[(int)peak_indices[0] - 1];
  /*  预分配谐波信息矩阵 */
  memset(&harmonic_info[0], 0, 30U * sizeof(double));
  /*  每行存储频率、幅值和百分比 */
  /*  设置阈值 */
  /*  计数器 */
  harmonic_count = -1;
  /*  计算谐波信息 */
  b_i = 1;
  exitg1 = false;
  while ((!exitg1) && (b_i - 1 < 9)) {
    x_size = (int)peak_indices[b_i] - 1;
    im = P1_data[x_size];
    percentage_of_fundamental = im / *fundamental_value * 100.0;
    if (percentage_of_fundamental < 0.1) {
      exitg1 = true;
    } else {
      harmonic_count++;
      harmonic_info[harmonic_count] = P2_data[x_size];
      harmonic_info[harmonic_count + 10] = im;
      harmonic_info[harmonic_count + 20] = percentage_of_fundamental;
      b_i++;
    }
  }
  emxFree_real_T(&f);
  emxFree_real_T(&P1);
  /*  裁剪矩阵以去除未使用的行 */
  if (harmonic_count + 1 < 1) {
    loop_ub = -1;
  } else {
    loop_ub = harmonic_count;
  }
  harmonic_info_size[0] = loop_ub + 1;
  harmonic_info_size[1] = 3;
  for (i = 0; i < 3; i++) {
    for (x_size = 0; x_size <= loop_ub; x_size++) {
      harmonic_info_data[x_size + (loop_ub + 1) * i] =
          harmonic_info[x_size + 10 * i];
    }
  }
}

/*
 * File trailer for FFT2C.c
 *
 * [EOF]
 */
