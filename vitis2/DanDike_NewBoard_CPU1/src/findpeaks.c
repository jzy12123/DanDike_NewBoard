/*
 * File: findpeaks.c
 *
 * MATLAB Coder version            : 23.2
 * C/C++ source code generated on  : 31-May-2024 16:43:36
 */

/* Include Files */
#include "findpeaks.h"
#include "FFT2C_emxutil.h"
#include "FFT2C_types.h"
#include "eml_setop.h"
#include "rt_nonfinite.h"
#include "rt_nonfinite.h"
#include <math.h>

/* Function Declarations */
static void c_findPeaksSeparatedByMoreThanM(const emxArray_real_T *y,
                                            const emxArray_int32_T *iPk,
                                            emxArray_int32_T *idx);

/* Function Definitions */
/*
 * Arguments    : const emxArray_real_T *y
 *                const emxArray_int32_T *iPk
 *                emxArray_int32_T *idx
 * Return Type  : void
 */
static void c_findPeaksSeparatedByMoreThanM(const emxArray_real_T *y,
                                            const emxArray_int32_T *iPk,
                                            emxArray_int32_T *idx)
{
  emxArray_int32_T *iwork;
  emxArray_int32_T *s;
  const double *y_data;
  const int *iPk_data;
  int i;
  int i2;
  int k;
  int n;
  int pEnd;
  int qEnd;
  int yk;
  int *idx_data;
  int *iwork_data;
  int *s_data;
  short b_y_data[30722];
  iPk_data = iPk->data;
  y_data = y->data;
  n = iPk->size[0];
  i2 = iPk->size[0];
  if (iPk->size[0] > 0) {
    b_y_data[0] = 1;
    yk = 1;
    for (k = 2; k <= n; k++) {
      yk++;
      b_y_data[k - 1] = (short)yk;
    }
  }
  i = idx->size[0];
  idx->size[0] = i2;
  emxEnsureCapacity_int32_T(idx, i);
  idx_data = idx->data;
  for (i = 0; i < i2; i++) {
    idx_data[i] = b_y_data[i];
  }
  if (idx->size[0] != 0) {
    double d;
    emxInit_int32_T(&s);
    i = s->size[0];
    s->size[0] = idx->size[0];
    emxEnsureCapacity_int32_T(s, i);
    s_data = s->data;
    yk = idx->size[0];
    for (i = 0; i < yk; i++) {
      s_data[i] = 0;
    }
    n = idx->size[0] + 1;
    emxInit_int32_T(&iwork);
    i = iwork->size[0];
    iwork->size[0] = idx->size[0];
    emxEnsureCapacity_int32_T(iwork, i);
    iwork_data = iwork->data;
    i = idx->size[0] - 1;
    for (k = 1; k <= i; k += 2) {
      d = y_data[iPk_data[idx_data[k - 1] - 1] - 1];
      if ((d >= y_data[iPk_data[idx_data[k] - 1] - 1]) || rtIsNaN(d)) {
        s_data[k - 1] = k;
        s_data[k] = k + 1;
      } else {
        s_data[k - 1] = k + 1;
        s_data[k] = k;
      }
    }
    if ((idx->size[0] & 1) != 0) {
      s_data[idx->size[0] - 1] = idx->size[0];
    }
    yk = 2;
    while (yk < n - 1) {
      int j;
      i2 = yk << 1;
      j = 1;
      for (pEnd = yk + 1; pEnd < n; pEnd = qEnd + yk) {
        int kEnd;
        int p;
        int q;
        p = j - 1;
        q = pEnd;
        qEnd = j + i2;
        if (qEnd > n) {
          qEnd = n;
        }
        k = 0;
        kEnd = qEnd - j;
        while (k + 1 <= kEnd) {
          d = y_data[iPk_data[idx_data[s_data[p] - 1] - 1] - 1];
          i = s_data[q - 1];
          if ((d >= y_data[iPk_data[idx_data[i - 1] - 1] - 1]) || rtIsNaN(d)) {
            iwork_data[k] = s_data[p];
            p++;
            if (p + 1 == pEnd) {
              while (q < qEnd) {
                k++;
                iwork_data[k] = s_data[q - 1];
                q++;
              }
            }
          } else {
            iwork_data[k] = i;
            q++;
            if (q == qEnd) {
              while (p + 1 < pEnd) {
                k++;
                iwork_data[k] = s_data[p];
                p++;
              }
            }
          }
          k++;
        }
        for (k = 0; k < kEnd; k++) {
          s_data[(j + k) - 1] = iwork_data[k];
        }
        j = qEnd;
      }
      yk = i2;
    }
    i = iwork->size[0];
    iwork->size[0] = s->size[0];
    emxEnsureCapacity_int32_T(iwork, i);
    iwork_data = iwork->data;
    yk = s->size[0];
    for (i = 0; i < yk; i++) {
      iwork_data[i] = idx_data[s_data[i] - 1];
    }
    emxFree_int32_T(&s);
    i = idx->size[0];
    idx->size[0] = iwork->size[0];
    emxEnsureCapacity_int32_T(idx, i);
    idx_data = idx->data;
    yk = iwork->size[0];
    for (i = 0; i < yk; i++) {
      idx_data[i] = iwork_data[i];
    }
    emxFree_int32_T(&iwork);
  }
}

/*
 * Arguments    : const emxArray_real_T *Yin
 *                emxArray_real_T *Ypk
 *                emxArray_real_T *Xpk
 * Return Type  : void
 */
void findpeaks(const emxArray_real_T *Yin, emxArray_real_T *Ypk,
               emxArray_real_T *Xpk)
{
  static int fPk_data[15361];
  static int iFinite_data[15361];
  static int iInfinite_data[15361];
  emxArray_int32_T *c;
  emxArray_int32_T *idx;
  const double *Yin_data;
  double ykfirst;
  double *Xpk_data;
  double *Ypk_data;
  int iPk_data[15361];
  int i;
  int k;
  int kfirst;
  int nInf;
  int nPk;
  int ny;
  int *c_data;
  int *idx_data;
  char dir;
  boolean_T isinfykfirst;
  Yin_data = Yin->data;
  ny = Yin->size[0];
  nPk = 0;
  nInf = 0;
  dir = 'n';
  kfirst = 0;
  ykfirst = rtInf;
  isinfykfirst = true;
  for (k = 1; k <= ny; k++) {
    double yk;
    boolean_T isinfyk;
    yk = Yin_data[k - 1];
    if (rtIsNaN(yk)) {
      yk = rtInf;
      isinfyk = true;
    } else if (rtIsInf(yk) && (yk > 0.0)) {
      isinfyk = true;
      nInf++;
      iInfinite_data[nInf - 1] = k;
    } else {
      isinfyk = false;
    }
    if (yk != ykfirst) {
      char previousdir;
      previousdir = dir;
      if (isinfyk || isinfykfirst) {
        dir = 'n';
      } else if (yk < ykfirst) {
        dir = 'd';
        if (previousdir == 'i') {
          nPk++;
          iFinite_data[nPk - 1] = kfirst;
        }
      } else {
        dir = 'i';
      }
      ykfirst = yk;
      kfirst = k;
      isinfykfirst = isinfyk;
    }
  }
  if (nPk < 1) {
    ny = 0;
  } else {
    ny = nPk;
  }
  nPk = 0;
  for (k = 0; k < ny; k++) {
    i = iFinite_data[k];
    ykfirst = Yin_data[i - 1];
    if ((ykfirst > rtMinusInf) &&
        (ykfirst - fmax(Yin_data[i - 2], Yin_data[i]) >= 0.0)) {
      nPk++;
      iPk_data[nPk - 1] = i;
    }
  }
  emxInit_int32_T(&c);
  if (nInf < 1) {
    nInf = 0;
  }
  do_vectors(iPk_data, nPk, iInfinite_data, nInf, c, fPk_data, iFinite_data,
             &ny);
  c_data = c->data;
  emxInit_int32_T(&idx);
  c_findPeaksSeparatedByMoreThanM(Yin, c, idx);
  idx_data = idx->data;
  if (idx->size[0] > Yin->size[0]) {
    kfirst = Yin->size[0];
    i = idx->size[0];
    idx->size[0] = Yin->size[0];
    emxEnsureCapacity_int32_T(idx, i);
    idx_data = idx->data;
  } else {
    kfirst = idx->size[0];
  }
  for (i = 0; i < kfirst; i++) {
    fPk_data[i] = c_data[idx_data[i] - 1];
  }
  emxFree_int32_T(&c);
  emxFree_int32_T(&idx);
  i = Ypk->size[0];
  Ypk->size[0] = kfirst;
  emxEnsureCapacity_real_T(Ypk, i);
  Ypk_data = Ypk->data;
  i = Xpk->size[0];
  Xpk->size[0] = kfirst;
  emxEnsureCapacity_real_T(Xpk, i);
  Xpk_data = Xpk->data;
  for (i = 0; i < kfirst; i++) {
    ny = fPk_data[i];
    Ypk_data[i] = Yin_data[ny - 1];
    Xpk_data[i] = (short)((short)(ny - 1) + 1);
  }
}

/*
 * File trailer for findpeaks.c
 *
 * [EOF]
 */
