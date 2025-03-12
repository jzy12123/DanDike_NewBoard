/*
 * File: FFTImplementationCallback.c
 *
 * MATLAB Coder version            : 23.2
 * C/C++ source code generated on  : 31-May-2024 16:43:36
 */

/* Include Files */
#include "FFTImplementationCallback.h"
#include "FFT2C_emxutil.h"
#include "FFT2C_types.h"
#include "rt_nonfinite.h"
#include <math.h>
#include <string.h>

/* Function Declarations */
static void c_FFTImplementationCallback_doH(const emxArray_real_T *x,
                                            emxArray_creal_T *y,
                                            int unsigned_nRows,
                                            const emxArray_real_T *costab,
                                            const emxArray_real_T *sintab);

static void c_FFTImplementationCallback_get(const emxArray_real_T *costab,
                                            const emxArray_real_T *sintab,
                                            emxArray_real_T *hcostab,
                                            emxArray_real_T *hsintab);

static void d_FFTImplementationCallback_doH(
    const emxArray_real_T *x, emxArray_creal_T *y, int nrowsx, int nRows,
    int nfft, const emxArray_creal_T *wwc, const emxArray_real_T *costab,
    const emxArray_real_T *sintab, const emxArray_real_T *costabinv,
    const emxArray_real_T *sintabinv);

static void d_FFTImplementationCallback_gen(int nRows, emxArray_real_T *costab,
                                            emxArray_real_T *sintab,
                                            emxArray_real_T *sintabinv);

static void d_FFTImplementationCallback_get(
    const emxArray_real_T *costab, const emxArray_real_T *sintab,
    const emxArray_real_T *costabinv, const emxArray_real_T *sintabinv,
    emxArray_real_T *hcostab, emxArray_real_T *hsintab,
    emxArray_real_T *hcostabinv, emxArray_real_T *hsintabinv);

static void d_FFTImplementationCallback_r2b(const emxArray_creal_T *x,
                                            int unsigned_nRows,
                                            const emxArray_real_T *costab,
                                            const emxArray_real_T *sintab,
                                            emxArray_creal_T *y);

static void e_FFTImplementationCallback_get(emxArray_creal_T *y,
                                            const emxArray_creal_T *reconVar1,
                                            const emxArray_creal_T *reconVar2,
                                            const int wrapIndex_data[],
                                            int hnRows);

static void e_FFTImplementationCallback_r2b(const emxArray_creal_T *x,
                                            int n1_unsigned,
                                            const emxArray_real_T *costab,
                                            const emxArray_real_T *sintab,
                                            emxArray_creal_T *y);

static void f_FFTImplementationCallback_r2b(const emxArray_creal_T *x,
                                            int n1_unsigned,
                                            const emxArray_real_T *costab,
                                            const emxArray_real_T *sintab,
                                            emxArray_creal_T *y);

/* Function Definitions */
/*
 * Arguments    : const emxArray_real_T *x
 *                emxArray_creal_T *y
 *                int unsigned_nRows
 *                const emxArray_real_T *costab
 *                const emxArray_real_T *sintab
 * Return Type  : void
 */
static void c_FFTImplementationCallback_doH(const emxArray_real_T *x,
                                            emxArray_creal_T *y,
                                            int unsigned_nRows,
                                            const emxArray_real_T *costab,
                                            const emxArray_real_T *sintab)
{
  emxArray_creal_T *b_y;
  emxArray_creal_T *reconVar1;
  emxArray_creal_T *reconVar2;
  emxArray_real_T *hcostab;
  emxArray_real_T *hsintab;
  creal_T *reconVar1_data;
  creal_T *reconVar2_data;
  creal_T *y_data;
  const double *costab_data;
  const double *sintab_data;
  const double *x_data;
  double temp_im;
  double temp_re;
  double temp_re_tmp;
  double twid_re;
  double z;
  double *hcostab_data;
  double *hsintab_data;
  int bitrevIndex_data[15360];
  int wrapIndex_data[15360];
  int i;
  int iDelta2;
  int iheight;
  int istart;
  int iy;
  int ju;
  int k;
  int nRows;
  int nRowsD2;
  int u0;
  boolean_T tst;
  sintab_data = sintab->data;
  costab_data = costab->data;
  y_data = y->data;
  x_data = x->data;
  nRows = (int)((unsigned int)unsigned_nRows >> 1);
  u0 = y->size[0];
  if (u0 > nRows) {
    u0 = nRows;
  }
  iDelta2 = nRows - 2;
  nRowsD2 = nRows / 2;
  k = nRowsD2 / 2;
  emxInit_real_T(&hcostab, 2);
  emxInit_real_T(&hsintab, 2);
  c_FFTImplementationCallback_get(costab, sintab, hcostab, hsintab);
  hsintab_data = hsintab->data;
  hcostab_data = hcostab->data;
  emxInit_creal_T(&reconVar1);
  ju = reconVar1->size[0];
  reconVar1->size[0] = nRows;
  emxEnsureCapacity_creal_T(reconVar1, ju);
  reconVar1_data = reconVar1->data;
  emxInit_creal_T(&reconVar2);
  ju = reconVar2->size[0];
  reconVar2->size[0] = nRows;
  emxEnsureCapacity_creal_T(reconVar2, ju);
  reconVar2_data = reconVar2->data;
  ju = (unsigned short)nRows;
  for (i = 0; i < ju; i++) {
    temp_re = sintab_data[i];
    temp_im = costab_data[i];
    reconVar1_data[i].re = temp_re + 1.0;
    reconVar1_data[i].im = -temp_im;
    reconVar2_data[i].re = 1.0 - temp_re;
    reconVar2_data[i].im = temp_im;
  }
  for (i = 0; i < ju; i++) {
    if (i + 1 != 1) {
      wrapIndex_data[i] = (nRows - i) + 1;
    } else {
      wrapIndex_data[0] = 1;
    }
  }
  z = (double)unsigned_nRows / 2.0;
  ju = 0;
  iy = 1;
  istart = (int)z;
  if (istart - 1 >= 0) {
    memset(&bitrevIndex_data[0], 0, (unsigned int)istart * sizeof(int));
  }
  for (iheight = 0; iheight <= u0 - 2; iheight++) {
    bitrevIndex_data[iheight] = iy;
    istart = (int)z;
    tst = true;
    while (tst) {
      istart >>= 1;
      ju ^= istart;
      tst = ((ju & istart) == 0);
    }
    iy = ju + 1;
  }
  bitrevIndex_data[u0 - 1] = iy;
  if ((x->size[0] & 1) == 0) {
    tst = true;
    u0 = x->size[0];
  } else if (x->size[0] >= unsigned_nRows) {
    tst = true;
    u0 = unsigned_nRows;
  } else {
    tst = false;
    u0 = x->size[0] - 1;
  }
  if (u0 > unsigned_nRows) {
    u0 = unsigned_nRows;
  }
  temp_re = (double)u0 / 2.0;
  ju = (unsigned short)(int)temp_re;
  for (i = 0; i < ju; i++) {
    istart = i << 1;
    iy = bitrevIndex_data[i];
    y_data[iy - 1].re = x_data[istart];
    y_data[iy - 1].im = x_data[istart + 1];
  }
  if (!tst) {
    ju = bitrevIndex_data[(int)temp_re] - 1;
    if ((unsigned short)(int)temp_re - 1 < 0) {
      u0 = 0;
    } else {
      u0 = (unsigned short)(int)temp_re << 1;
    }
    y_data[ju].re = x_data[u0];
    y_data[ju].im = 0.0;
  }
  emxInit_creal_T(&b_y);
  ju = b_y->size[0];
  b_y->size[0] = y->size[0];
  emxEnsureCapacity_creal_T(b_y, ju);
  reconVar1_data = b_y->data;
  istart = y->size[0];
  for (ju = 0; ju < istart; ju++) {
    reconVar1_data[ju] = y_data[ju];
  }
  if (nRows > 1) {
    for (i = 0; i <= iDelta2; i += 2) {
      temp_re_tmp = reconVar1_data[i + 1].re;
      temp_re = reconVar1_data[i + 1].im;
      temp_im = reconVar1_data[i].re;
      twid_re = reconVar1_data[i].im;
      reconVar1_data[i + 1].re = temp_im - temp_re_tmp;
      reconVar1_data[i + 1].im = twid_re - temp_re;
      reconVar1_data[i].re = temp_im + temp_re_tmp;
      reconVar1_data[i].im = twid_re + temp_re;
    }
  }
  nRows = 2;
  iDelta2 = 4;
  iheight = ((k - 1) << 2) + 1;
  while (k > 0) {
    for (i = 0; i < iheight; i += iDelta2) {
      istart = i + nRows;
      temp_re = reconVar1_data[istart].re;
      temp_im = reconVar1_data[istart].im;
      reconVar1_data[istart].re = reconVar1_data[i].re - temp_re;
      reconVar1_data[istart].im = reconVar1_data[i].im - temp_im;
      reconVar1_data[i].re += temp_re;
      reconVar1_data[i].im += temp_im;
    }
    istart = 1;
    for (iy = k; iy < nRowsD2; iy += k) {
      double twid_im;
      twid_re = hcostab_data[iy];
      twid_im = hsintab_data[iy];
      i = istart;
      ju = istart + iheight;
      while (i < ju) {
        u0 = i + nRows;
        temp_re_tmp = reconVar1_data[u0].im;
        temp_im = reconVar1_data[u0].re;
        temp_re = twid_re * temp_im - twid_im * temp_re_tmp;
        temp_im = twid_re * temp_re_tmp + twid_im * temp_im;
        reconVar1_data[u0].re = reconVar1_data[i].re - temp_re;
        reconVar1_data[u0].im = reconVar1_data[i].im - temp_im;
        reconVar1_data[i].re += temp_re;
        reconVar1_data[i].im += temp_im;
        i += iDelta2;
      }
      istart++;
    }
    k /= 2;
    nRows = iDelta2;
    iDelta2 += iDelta2;
    iheight -= nRows;
  }
  emxFree_real_T(&hsintab);
  emxFree_real_T(&hcostab);
  ju = y->size[0];
  y->size[0] = b_y->size[0];
  emxEnsureCapacity_creal_T(y, ju);
  y_data = y->data;
  istart = b_y->size[0];
  for (ju = 0; ju < istart; ju++) {
    y_data[ju] = reconVar1_data[ju];
  }
  emxFree_creal_T(&b_y);
  e_FFTImplementationCallback_get(y, reconVar1, reconVar2, wrapIndex_data,
                                  (int)z);
  emxFree_creal_T(&reconVar2);
  emxFree_creal_T(&reconVar1);
}

/*
 * Arguments    : const emxArray_real_T *costab
 *                const emxArray_real_T *sintab
 *                emxArray_real_T *hcostab
 *                emxArray_real_T *hsintab
 * Return Type  : void
 */
static void c_FFTImplementationCallback_get(const emxArray_real_T *costab,
                                            const emxArray_real_T *sintab,
                                            emxArray_real_T *hcostab,
                                            emxArray_real_T *hsintab)
{
  const double *costab_data;
  const double *sintab_data;
  double *hcostab_data;
  double *hsintab_data;
  int hcostab_tmp;
  int hszCostab;
  int i;
  sintab_data = sintab->data;
  costab_data = costab->data;
  hszCostab = (int)((unsigned int)costab->size[1] >> 1);
  hcostab_tmp = hcostab->size[0] * hcostab->size[1];
  hcostab->size[0] = 1;
  hcostab->size[1] = hszCostab;
  emxEnsureCapacity_real_T(hcostab, hcostab_tmp);
  hcostab_data = hcostab->data;
  hcostab_tmp = hsintab->size[0] * hsintab->size[1];
  hsintab->size[0] = 1;
  hsintab->size[1] = hszCostab;
  emxEnsureCapacity_real_T(hsintab, hcostab_tmp);
  hsintab_data = hsintab->data;
  for (i = 0; i < hszCostab; i++) {
    hcostab_tmp = ((i + 1) << 1) - 2;
    hcostab_data[i] = costab_data[hcostab_tmp];
    hsintab_data[i] = sintab_data[hcostab_tmp];
  }
}

/*
 * Arguments    : const emxArray_real_T *x
 *                emxArray_creal_T *y
 *                int nrowsx
 *                int nRows
 *                int nfft
 *                const emxArray_creal_T *wwc
 *                const emxArray_real_T *costab
 *                const emxArray_real_T *sintab
 *                const emxArray_real_T *costabinv
 *                const emxArray_real_T *sintabinv
 * Return Type  : void
 */
static void d_FFTImplementationCallback_doH(
    const emxArray_real_T *x, emxArray_creal_T *y, int nrowsx, int nRows,
    int nfft, const emxArray_creal_T *wwc, const emxArray_real_T *costab,
    const emxArray_real_T *sintab, const emxArray_real_T *costabinv,
    const emxArray_real_T *sintabinv)
{
  emxArray_creal_T *fv;
  emxArray_creal_T *r;
  emxArray_creal_T *reconVar1;
  emxArray_creal_T *reconVar2;
  emxArray_creal_T *ytmp;
  emxArray_real_T *a__1;
  emxArray_real_T *costable;
  emxArray_real_T *hcostab;
  emxArray_real_T *hcostabinv;
  emxArray_real_T *hsintab;
  emxArray_real_T *hsintabinv;
  emxArray_real_T *sintable;
  const creal_T *wwc_data;
  creal_T *fv_data;
  creal_T *r1;
  creal_T *reconVar1_data;
  creal_T *reconVar2_data;
  creal_T *y_data;
  creal_T *ytmp_data;
  cuint8_T b_y_data[30720];
  const double *x_data;
  double a_im;
  double a_re;
  double b_im;
  double b_re;
  double z_tmp;
  double *costable_data;
  double *sintable_data;
  int wrapIndex_data[15360];
  int b_i;
  int hnRows;
  int i;
  int i1;
  int k1;
  int u0;
  boolean_T nxeven;
  wwc_data = wwc->data;
  y_data = y->data;
  x_data = x->data;
  hnRows = (int)((unsigned int)nRows >> 1);
  if (hnRows > nrowsx) {
    if (hnRows - 1 >= 0) {
      memset(&b_y_data[0], 0, (unsigned int)hnRows * sizeof(cuint8_T));
    }
  }
  emxInit_creal_T(&ytmp);
  i = ytmp->size[0];
  ytmp->size[0] = hnRows;
  emxEnsureCapacity_creal_T(ytmp, i);
  ytmp_data = ytmp->data;
  for (i = 0; i < hnRows; i++) {
    ytmp_data[i].re = 0.0;
    ytmp_data[i].im = b_y_data[i].im;
  }
  if ((x->size[0] & 1) == 0) {
    nxeven = true;
    u0 = x->size[0];
  } else if (x->size[0] >= nRows) {
    nxeven = true;
    u0 = nRows;
  } else {
    nxeven = false;
    u0 = x->size[0] - 1;
  }
  if (u0 > nRows) {
    u0 = nRows;
  }
  emxInit_real_T(&a__1, 2);
  emxInit_real_T(&costable, 2);
  emxInit_real_T(&sintable, 2);
  d_FFTImplementationCallback_gen(nRows << 1, costable, sintable, a__1);
  sintable_data = sintable->data;
  costable_data = costable->data;
  emxFree_real_T(&a__1);
  emxInit_real_T(&hcostab, 2);
  emxInit_real_T(&hsintab, 2);
  emxInit_real_T(&hcostabinv, 2);
  emxInit_real_T(&hsintabinv, 2);
  d_FFTImplementationCallback_get(costab, sintab, costabinv, sintabinv, hcostab,
                                  hsintab, hcostabinv, hsintabinv);
  emxInit_creal_T(&reconVar1);
  i = reconVar1->size[0];
  reconVar1->size[0] = hnRows;
  emxEnsureCapacity_creal_T(reconVar1, i);
  reconVar1_data = reconVar1->data;
  emxInit_creal_T(&reconVar2);
  i = reconVar2->size[0];
  reconVar2->size[0] = hnRows;
  emxEnsureCapacity_creal_T(reconVar2, i);
  reconVar2_data = reconVar2->data;
  i = (unsigned short)hnRows;
  for (b_i = 0; b_i < i; b_i++) {
    i1 = b_i << 1;
    z_tmp = sintable_data[i1];
    a_re = costable_data[i1];
    reconVar1_data[b_i].re = z_tmp + 1.0;
    reconVar1_data[b_i].im = -a_re;
    reconVar2_data[b_i].re = 1.0 - z_tmp;
    reconVar2_data[b_i].im = a_re;
  }
  emxFree_real_T(&sintable);
  emxFree_real_T(&costable);
  for (b_i = 0; b_i < i; b_i++) {
    if (b_i + 1 != 1) {
      wrapIndex_data[b_i] = (hnRows - b_i) + 1;
    } else {
      wrapIndex_data[0] = 1;
    }
  }
  z_tmp = (double)u0 / 2.0;
  i1 = (unsigned short)(int)z_tmp;
  for (k1 = 0; k1 < i1; k1++) {
    a_re = wwc_data[(hnRows + k1) - 1].re;
    a_im = wwc_data[(hnRows + k1) - 1].im;
    b_i = k1 << 1;
    b_re = x_data[b_i];
    b_im = x_data[b_i + 1];
    ytmp_data[k1].re = a_re * b_re + a_im * b_im;
    ytmp_data[k1].im = a_re * b_im - a_im * b_re;
  }
  if (!nxeven) {
    a_re = wwc_data[(hnRows + (int)z_tmp) - 1].re;
    a_im = wwc_data[(hnRows + (int)z_tmp) - 1].im;
    if ((unsigned short)(int)z_tmp - 1 < 0) {
      i1 = 0;
    } else {
      i1 = (unsigned short)(int)z_tmp << 1;
    }
    b_re = x_data[i1];
    ytmp_data[(int)((double)u0 / 2.0)].re = a_re * b_re + a_im * 0.0;
    ytmp_data[(int)((double)u0 / 2.0)].im = a_re * 0.0 - a_im * b_re;
    if ((int)z_tmp + 2 <= hnRows) {
      i1 = (int)z_tmp + 2;
      for (b_i = i1; b_i <= hnRows; b_i++) {
        ytmp_data[b_i - 1].re = 0.0;
        ytmp_data[b_i - 1].im = 0.0;
      }
    }
  } else if ((int)z_tmp + 1 <= hnRows) {
    i1 = (int)z_tmp + 1;
    for (b_i = i1; b_i <= hnRows; b_i++) {
      ytmp_data[b_i - 1].re = 0.0;
      ytmp_data[b_i - 1].im = 0.0;
    }
  }
  b_i = (int)((double)nfft / 2.0);
  emxInit_creal_T(&fv);
  d_FFTImplementationCallback_r2b(ytmp, b_i, hcostab, hsintab, fv);
  fv_data = fv->data;
  emxInit_creal_T(&r);
  e_FFTImplementationCallback_r2b(wwc, b_i, hcostab, hsintab, r);
  r1 = r->data;
  emxFree_real_T(&hsintab);
  emxFree_real_T(&hcostab);
  u0 = fv->size[0];
  for (i1 = 0; i1 < u0; i1++) {
    z_tmp = fv_data[i1].re;
    a_re = r1[i1].im;
    a_im = fv_data[i1].im;
    b_re = r1[i1].re;
    fv_data[i1].re = z_tmp * b_re - a_im * a_re;
    fv_data[i1].im = z_tmp * a_re + a_im * b_re;
  }
  f_FFTImplementationCallback_r2b(fv, b_i, hcostabinv, hsintabinv, r);
  r1 = r->data;
  emxFree_real_T(&hsintabinv);
  emxFree_real_T(&hcostabinv);
  i1 = fv->size[0];
  fv->size[0] = r->size[0];
  emxEnsureCapacity_creal_T(fv, i1);
  fv_data = fv->data;
  u0 = r->size[0];
  for (i1 = 0; i1 < u0; i1++) {
    fv_data[i1] = r1[i1];
  }
  emxFree_creal_T(&r);
  i1 = wwc->size[0];
  for (b_i = hnRows; b_i <= i1; b_i++) {
    z_tmp = wwc_data[b_i - 1].re;
    a_re = fv_data[b_i - 1].im;
    a_im = wwc_data[b_i - 1].im;
    b_re = fv_data[b_i - 1].re;
    ytmp_data[b_i - hnRows].re = z_tmp * b_re + a_im * a_re;
    ytmp_data[b_i - hnRows].im = z_tmp * a_re - a_im * b_re;
  }
  emxFree_creal_T(&fv);
  for (b_i = 0; b_i < i; b_i++) {
    double b_ytmp_re_tmp;
    double ytmp_im_tmp;
    double ytmp_re_tmp;
    i1 = wrapIndex_data[b_i];
    z_tmp = ytmp_data[b_i].re;
    a_re = reconVar1_data[b_i].im;
    a_im = ytmp_data[b_i].im;
    b_re = reconVar1_data[b_i].re;
    b_im = ytmp_data[i1 - 1].re;
    ytmp_im_tmp = -ytmp_data[i1 - 1].im;
    ytmp_re_tmp = reconVar2_data[b_i].im;
    b_ytmp_re_tmp = reconVar2_data[b_i].re;
    y_data[b_i].re = 0.5 * ((z_tmp * b_re - a_im * a_re) +
                            (b_im * b_ytmp_re_tmp - ytmp_im_tmp * ytmp_re_tmp));
    y_data[b_i].im = 0.5 * ((z_tmp * a_re + a_im * b_re) +
                            (b_im * ytmp_re_tmp + ytmp_im_tmp * b_ytmp_re_tmp));
    y_data[hnRows + b_i].re =
        0.5 * ((z_tmp * b_ytmp_re_tmp - a_im * ytmp_re_tmp) +
               (b_im * b_re - ytmp_im_tmp * a_re));
    y_data[hnRows + b_i].im =
        0.5 * ((z_tmp * ytmp_re_tmp + a_im * b_ytmp_re_tmp) +
               (b_im * a_re + ytmp_im_tmp * b_re));
  }
  emxFree_creal_T(&reconVar2);
  emxFree_creal_T(&reconVar1);
  emxFree_creal_T(&ytmp);
}

/*
 * Arguments    : int nRows
 *                emxArray_real_T *costab
 *                emxArray_real_T *sintab
 *                emxArray_real_T *sintabinv
 * Return Type  : void
 */
static void d_FFTImplementationCallback_gen(int nRows, emxArray_real_T *costab,
                                            emxArray_real_T *sintab,
                                            emxArray_real_T *sintabinv)
{
  emxArray_real_T *b_costab;
  emxArray_real_T *b_sintab;
  emxArray_real_T *b_sintabinv;
  emxArray_real_T *costab1q;
  double e;
  double *b_costab_data;
  double *costab_data;
  double *sintab_data;
  double *sintabinv_data;
  int i;
  int k;
  int n;
  int nd2;
  e = 6.2831853071795862 / (double)nRows;
  n = (int)((unsigned int)nRows >> 1) >> 1;
  emxInit_real_T(&costab1q, 2);
  i = costab1q->size[0] * costab1q->size[1];
  costab1q->size[0] = 1;
  costab1q->size[1] = n + 1;
  emxEnsureCapacity_real_T(costab1q, i);
  costab_data = costab1q->data;
  costab_data[0] = 1.0;
  nd2 = ((unsigned short)n >> 1) - 1;
  for (k = 0; k <= nd2; k++) {
    costab_data[k + 1] = cos(e * ((double)k + 1.0));
  }
  i = nd2 + 2;
  nd2 = n - 1;
  for (k = i; k <= nd2; k++) {
    costab_data[k] = sin(e * (double)(n - k));
  }
  costab_data[n] = 0.0;
  n = costab1q->size[1] - 1;
  nd2 = (costab1q->size[1] - 1) << 1;
  emxInit_real_T(&b_costab, 2);
  i = b_costab->size[0] * b_costab->size[1];
  b_costab->size[0] = 1;
  b_costab->size[1] = nd2 + 1;
  emxEnsureCapacity_real_T(b_costab, i);
  b_costab_data = b_costab->data;
  emxInit_real_T(&b_sintab, 2);
  i = b_sintab->size[0] * b_sintab->size[1];
  b_sintab->size[0] = 1;
  b_sintab->size[1] = nd2 + 1;
  emxEnsureCapacity_real_T(b_sintab, i);
  sintab_data = b_sintab->data;
  b_costab_data[0] = 1.0;
  sintab_data[0] = 0.0;
  emxInit_real_T(&b_sintabinv, 2);
  i = b_sintabinv->size[0] * b_sintabinv->size[1];
  b_sintabinv->size[0] = 1;
  b_sintabinv->size[1] = nd2 + 1;
  emxEnsureCapacity_real_T(b_sintabinv, i);
  sintabinv_data = b_sintabinv->data;
  for (k = 0; k < n; k++) {
    sintabinv_data[k + 1] = costab_data[(n - k) - 1];
  }
  i = costab1q->size[1];
  for (k = i; k <= nd2; k++) {
    sintabinv_data[k] = costab_data[k - n];
  }
  for (k = 0; k < n; k++) {
    b_costab_data[k + 1] = costab_data[k + 1];
    sintab_data[k + 1] = -costab_data[(n - k) - 1];
  }
  i = costab1q->size[1];
  for (k = i; k <= nd2; k++) {
    b_costab_data[k] = -costab_data[nd2 - k];
    sintab_data[k] = -costab_data[k - n];
  }
  emxFree_real_T(&costab1q);
  i = costab->size[0] * costab->size[1];
  costab->size[0] = 1;
  costab->size[1] = b_costab->size[1];
  emxEnsureCapacity_real_T(costab, i);
  costab_data = costab->data;
  nd2 = b_costab->size[1];
  for (i = 0; i < nd2; i++) {
    costab_data[i] = b_costab_data[i];
  }
  emxFree_real_T(&b_costab);
  i = sintab->size[0] * sintab->size[1];
  sintab->size[0] = 1;
  sintab->size[1] = b_sintab->size[1];
  emxEnsureCapacity_real_T(sintab, i);
  costab_data = sintab->data;
  nd2 = b_sintab->size[1];
  for (i = 0; i < nd2; i++) {
    costab_data[i] = sintab_data[i];
  }
  emxFree_real_T(&b_sintab);
  i = sintabinv->size[0] * sintabinv->size[1];
  sintabinv->size[0] = 1;
  sintabinv->size[1] = b_sintabinv->size[1];
  emxEnsureCapacity_real_T(sintabinv, i);
  costab_data = sintabinv->data;
  nd2 = b_sintabinv->size[1];
  for (i = 0; i < nd2; i++) {
    costab_data[i] = sintabinv_data[i];
  }
  emxFree_real_T(&b_sintabinv);
}

/*
 * Arguments    : const emxArray_real_T *costab
 *                const emxArray_real_T *sintab
 *                const emxArray_real_T *costabinv
 *                const emxArray_real_T *sintabinv
 *                emxArray_real_T *hcostab
 *                emxArray_real_T *hsintab
 *                emxArray_real_T *hcostabinv
 *                emxArray_real_T *hsintabinv
 * Return Type  : void
 */
static void d_FFTImplementationCallback_get(
    const emxArray_real_T *costab, const emxArray_real_T *sintab,
    const emxArray_real_T *costabinv, const emxArray_real_T *sintabinv,
    emxArray_real_T *hcostab, emxArray_real_T *hsintab,
    emxArray_real_T *hcostabinv, emxArray_real_T *hsintabinv)
{
  const double *costab_data;
  const double *costabinv_data;
  const double *sintab_data;
  const double *sintabinv_data;
  double *hcostab_data;
  double *hcostabinv_data;
  double *hsintab_data;
  double *hsintabinv_data;
  int hcostab_tmp;
  int hszCostab;
  int i;
  sintabinv_data = sintabinv->data;
  costabinv_data = costabinv->data;
  sintab_data = sintab->data;
  costab_data = costab->data;
  hszCostab = (int)((unsigned int)costab->size[1] >> 1);
  hcostab_tmp = hcostab->size[0] * hcostab->size[1];
  hcostab->size[0] = 1;
  hcostab->size[1] = hszCostab;
  emxEnsureCapacity_real_T(hcostab, hcostab_tmp);
  hcostab_data = hcostab->data;
  hcostab_tmp = hsintab->size[0] * hsintab->size[1];
  hsintab->size[0] = 1;
  hsintab->size[1] = hszCostab;
  emxEnsureCapacity_real_T(hsintab, hcostab_tmp);
  hsintab_data = hsintab->data;
  hcostab_tmp = hcostabinv->size[0] * hcostabinv->size[1];
  hcostabinv->size[0] = 1;
  hcostabinv->size[1] = hszCostab;
  emxEnsureCapacity_real_T(hcostabinv, hcostab_tmp);
  hcostabinv_data = hcostabinv->data;
  hcostab_tmp = hsintabinv->size[0] * hsintabinv->size[1];
  hsintabinv->size[0] = 1;
  hsintabinv->size[1] = hszCostab;
  emxEnsureCapacity_real_T(hsintabinv, hcostab_tmp);
  hsintabinv_data = hsintabinv->data;
  for (i = 0; i < hszCostab; i++) {
    hcostab_tmp = ((i + 1) << 1) - 2;
    hcostab_data[i] = costab_data[hcostab_tmp];
    hsintab_data[i] = sintab_data[hcostab_tmp];
    hcostabinv_data[i] = costabinv_data[hcostab_tmp];
    hsintabinv_data[i] = sintabinv_data[hcostab_tmp];
  }
}

/*
 * Arguments    : const emxArray_creal_T *x
 *                int unsigned_nRows
 *                const emxArray_real_T *costab
 *                const emxArray_real_T *sintab
 *                emxArray_creal_T *y
 * Return Type  : void
 */
static void d_FFTImplementationCallback_r2b(const emxArray_creal_T *x,
                                            int unsigned_nRows,
                                            const emxArray_real_T *costab,
                                            const emxArray_real_T *sintab,
                                            emxArray_creal_T *y)
{
  emxArray_creal_T *b_y;
  const creal_T *x_data;
  creal_T *b_y_data;
  creal_T *y_data;
  const double *costab_data;
  const double *sintab_data;
  double temp_im;
  double temp_re;
  double temp_re_tmp;
  double twid_re;
  int i;
  int iDelta;
  int iDelta2;
  int iheight;
  int iy;
  int j;
  int ju;
  int k;
  int nRowsD2;
  int nRowsM2;
  sintab_data = sintab->data;
  costab_data = costab->data;
  x_data = x->data;
  emxInit_creal_T(&b_y);
  ju = b_y->size[0];
  b_y->size[0] = unsigned_nRows;
  emxEnsureCapacity_creal_T(b_y, ju);
  y_data = b_y->data;
  if (unsigned_nRows > x->size[0]) {
    ju = b_y->size[0];
    b_y->size[0] = unsigned_nRows;
    emxEnsureCapacity_creal_T(b_y, ju);
    y_data = b_y->data;
    for (ju = 0; ju < unsigned_nRows; ju++) {
      y_data[ju].re = 0.0;
      y_data[ju].im = 0.0;
    }
  }
  ju = y->size[0];
  y->size[0] = b_y->size[0];
  emxEnsureCapacity_creal_T(y, ju);
  b_y_data = y->data;
  iy = b_y->size[0];
  for (ju = 0; ju < iy; ju++) {
    b_y_data[ju] = y_data[ju];
  }
  j = x->size[0];
  if (j > unsigned_nRows) {
    j = unsigned_nRows;
  }
  nRowsM2 = unsigned_nRows - 2;
  nRowsD2 = (int)((unsigned int)unsigned_nRows >> 1);
  k = nRowsD2 / 2;
  iy = 0;
  ju = 0;
  for (i = 0; i <= j - 2; i++) {
    boolean_T tst;
    b_y_data[iy] = x_data[i];
    iy = unsigned_nRows;
    tst = true;
    while (tst) {
      iy >>= 1;
      ju ^= iy;
      tst = ((ju & iy) == 0);
    }
    iy = ju;
  }
  if (j - 2 < 0) {
    j = 0;
  } else {
    j--;
  }
  b_y_data[iy] = x_data[j];
  ju = b_y->size[0];
  b_y->size[0] = y->size[0];
  emxEnsureCapacity_creal_T(b_y, ju);
  y_data = b_y->data;
  iy = y->size[0];
  for (ju = 0; ju < iy; ju++) {
    y_data[ju] = b_y_data[ju];
  }
  if (unsigned_nRows > 1) {
    for (i = 0; i <= nRowsM2; i += 2) {
      temp_re_tmp = y_data[i + 1].re;
      temp_im = y_data[i + 1].im;
      temp_re = y_data[i].re;
      twid_re = y_data[i].im;
      y_data[i + 1].re = temp_re - temp_re_tmp;
      y_data[i + 1].im = twid_re - temp_im;
      y_data[i].re = temp_re + temp_re_tmp;
      y_data[i].im = twid_re + temp_im;
    }
  }
  iDelta = 2;
  iDelta2 = 4;
  iheight = ((k - 1) << 2) + 1;
  while (k > 0) {
    for (i = 0; i < iheight; i += iDelta2) {
      iy = i + iDelta;
      temp_re = y_data[iy].re;
      temp_im = y_data[iy].im;
      y_data[iy].re = y_data[i].re - temp_re;
      y_data[iy].im = y_data[i].im - temp_im;
      y_data[i].re += temp_re;
      y_data[i].im += temp_im;
    }
    iy = 1;
    for (j = k; j < nRowsD2; j += k) {
      double twid_im;
      twid_re = costab_data[j];
      twid_im = sintab_data[j];
      i = iy;
      ju = iy + iheight;
      while (i < ju) {
        nRowsM2 = i + iDelta;
        temp_re_tmp = y_data[nRowsM2].im;
        temp_im = y_data[nRowsM2].re;
        temp_re = twid_re * temp_im - twid_im * temp_re_tmp;
        temp_im = twid_re * temp_re_tmp + twid_im * temp_im;
        y_data[nRowsM2].re = y_data[i].re - temp_re;
        y_data[nRowsM2].im = y_data[i].im - temp_im;
        y_data[i].re += temp_re;
        y_data[i].im += temp_im;
        i += iDelta2;
      }
      iy++;
    }
    k /= 2;
    iDelta = iDelta2;
    iDelta2 += iDelta2;
    iheight -= iDelta;
  }
  ju = y->size[0];
  y->size[0] = b_y->size[0];
  emxEnsureCapacity_creal_T(y, ju);
  b_y_data = y->data;
  iy = b_y->size[0];
  for (ju = 0; ju < iy; ju++) {
    b_y_data[ju] = y_data[ju];
  }
  emxFree_creal_T(&b_y);
}

/*
 * Arguments    : emxArray_creal_T *y
 *                const emxArray_creal_T *reconVar1
 *                const emxArray_creal_T *reconVar2
 *                const int wrapIndex_data[]
 *                int hnRows
 * Return Type  : void
 */
static void e_FFTImplementationCallback_get(emxArray_creal_T *y,
                                            const emxArray_creal_T *reconVar1,
                                            const emxArray_creal_T *reconVar2,
                                            const int wrapIndex_data[],
                                            int hnRows)
{
  const creal_T *reconVar1_data;
  const creal_T *reconVar2_data;
  creal_T *y_data;
  double b_temp1_re_tmp;
  double b_temp2_re_tmp;
  double b_y_re_tmp;
  double c_y_re_tmp;
  double d_y_re_tmp;
  double temp1_im_tmp;
  double temp1_re_tmp;
  double temp1_re_tmp_tmp;
  double y_im_tmp;
  double y_re_tmp;
  int i;
  int iterVar;
  reconVar2_data = reconVar2->data;
  reconVar1_data = reconVar1->data;
  y_data = y->data;
  iterVar = (int)((unsigned int)hnRows >> 1);
  temp1_re_tmp_tmp = y_data[0].re;
  temp1_im_tmp = y_data[0].im;
  y_re_tmp = temp1_re_tmp_tmp * reconVar1_data[0].re;
  y_im_tmp = temp1_re_tmp_tmp * reconVar1_data[0].im;
  b_y_re_tmp = temp1_re_tmp_tmp * reconVar2_data[0].re;
  temp1_re_tmp_tmp *= reconVar2_data[0].im;
  y_data[0].re = 0.5 * ((y_re_tmp - temp1_im_tmp * reconVar1_data[0].im) +
                        (b_y_re_tmp - -temp1_im_tmp * reconVar2_data[0].im));
  y_data[0].im =
      0.5 * ((y_im_tmp + temp1_im_tmp * reconVar1_data[0].re) +
             (temp1_re_tmp_tmp + -temp1_im_tmp * reconVar2_data[0].re));
  y_data[hnRows].re =
      0.5 * ((b_y_re_tmp - temp1_im_tmp * reconVar2_data[0].im) +
             (y_re_tmp - -temp1_im_tmp * reconVar1_data[0].im));
  y_data[hnRows].im =
      0.5 * ((temp1_re_tmp_tmp + temp1_im_tmp * reconVar2_data[0].re) +
             (y_im_tmp + -temp1_im_tmp * reconVar1_data[0].re));
  for (i = 2; i <= iterVar; i++) {
    double temp2_im_tmp;
    double temp2_re_tmp;
    int b_i;
    int i1;
    temp1_re_tmp = y_data[i - 1].re;
    temp1_im_tmp = y_data[i - 1].im;
    b_i = wrapIndex_data[i - 1];
    temp2_re_tmp = y_data[b_i - 1].re;
    temp2_im_tmp = y_data[b_i - 1].im;
    y_re_tmp = reconVar1_data[i - 1].im;
    b_y_re_tmp = reconVar1_data[i - 1].re;
    c_y_re_tmp = reconVar2_data[i - 1].im;
    d_y_re_tmp = reconVar2_data[i - 1].re;
    y_data[i - 1].re =
        0.5 * ((temp1_re_tmp * b_y_re_tmp - temp1_im_tmp * y_re_tmp) +
               (temp2_re_tmp * d_y_re_tmp - -temp2_im_tmp * c_y_re_tmp));
    y_data[i - 1].im =
        0.5 * ((temp1_re_tmp * y_re_tmp + temp1_im_tmp * b_y_re_tmp) +
               (temp2_re_tmp * c_y_re_tmp + -temp2_im_tmp * d_y_re_tmp));
    i1 = (hnRows + i) - 1;
    y_data[i1].re =
        0.5 * ((temp1_re_tmp * d_y_re_tmp - temp1_im_tmp * c_y_re_tmp) +
               (temp2_re_tmp * b_y_re_tmp - -temp2_im_tmp * y_re_tmp));
    y_data[i1].im =
        0.5 * ((temp1_re_tmp * c_y_re_tmp + temp1_im_tmp * d_y_re_tmp) +
               (temp2_re_tmp * y_re_tmp + -temp2_im_tmp * b_y_re_tmp));
    temp1_re_tmp_tmp = reconVar1_data[b_i - 1].im;
    b_temp2_re_tmp = reconVar1_data[b_i - 1].re;
    b_temp1_re_tmp = reconVar2_data[b_i - 1].im;
    y_im_tmp = reconVar2_data[b_i - 1].re;
    y_data[b_i - 1].re =
        0.5 *
        ((temp2_re_tmp * b_temp2_re_tmp - temp2_im_tmp * temp1_re_tmp_tmp) +
         (temp1_re_tmp * y_im_tmp - -temp1_im_tmp * b_temp1_re_tmp));
    y_data[b_i - 1].im =
        0.5 *
        ((temp2_re_tmp * temp1_re_tmp_tmp + temp2_im_tmp * b_temp2_re_tmp) +
         (temp1_re_tmp * b_temp1_re_tmp + -temp1_im_tmp * y_im_tmp));
    b_i = (b_i + hnRows) - 1;
    y_data[b_i].re =
        0.5 *
        ((temp2_re_tmp * y_im_tmp - temp2_im_tmp * b_temp1_re_tmp) +
         (temp1_re_tmp * b_temp2_re_tmp - -temp1_im_tmp * temp1_re_tmp_tmp));
    y_data[b_i].im =
        0.5 *
        ((temp2_re_tmp * b_temp1_re_tmp + temp2_im_tmp * y_im_tmp) +
         (temp1_re_tmp * temp1_re_tmp_tmp + -temp1_im_tmp * b_temp2_re_tmp));
  }
  if (iterVar != 0) {
    temp1_re_tmp = y_data[iterVar].re;
    temp1_im_tmp = y_data[iterVar].im;
    y_re_tmp = reconVar1_data[iterVar].im;
    b_y_re_tmp = reconVar1_data[iterVar].re;
    c_y_re_tmp = temp1_re_tmp * b_y_re_tmp;
    y_im_tmp = temp1_re_tmp * y_re_tmp;
    d_y_re_tmp = reconVar2_data[iterVar].im;
    b_temp2_re_tmp = reconVar2_data[iterVar].re;
    b_temp1_re_tmp = temp1_re_tmp * b_temp2_re_tmp;
    temp1_re_tmp_tmp = temp1_re_tmp * d_y_re_tmp;
    y_data[iterVar].re = 0.5 * ((c_y_re_tmp - temp1_im_tmp * y_re_tmp) +
                                (b_temp1_re_tmp - -temp1_im_tmp * d_y_re_tmp));
    y_data[iterVar].im =
        0.5 * ((y_im_tmp + temp1_im_tmp * b_y_re_tmp) +
               (temp1_re_tmp_tmp + -temp1_im_tmp * b_temp2_re_tmp));
    y_data[hnRows + iterVar].re =
        0.5 * ((b_temp1_re_tmp - temp1_im_tmp * d_y_re_tmp) +
               (c_y_re_tmp - -temp1_im_tmp * y_re_tmp));
    y_data[hnRows + iterVar].im =
        0.5 * ((temp1_re_tmp_tmp + temp1_im_tmp * b_temp2_re_tmp) +
               (y_im_tmp + -temp1_im_tmp * b_y_re_tmp));
  }
}

/*
 * Arguments    : const emxArray_creal_T *x
 *                int n1_unsigned
 *                const emxArray_real_T *costab
 *                const emxArray_real_T *sintab
 *                emxArray_creal_T *y
 * Return Type  : void
 */
static void e_FFTImplementationCallback_r2b(const emxArray_creal_T *x,
                                            int n1_unsigned,
                                            const emxArray_real_T *costab,
                                            const emxArray_real_T *sintab,
                                            emxArray_creal_T *y)
{
  const creal_T *x_data;
  creal_T *y_data;
  const double *costab_data;
  const double *sintab_data;
  double temp_im;
  double temp_re;
  double temp_re_tmp;
  double twid_re;
  int i;
  int iDelta;
  int iDelta2;
  int iheight;
  int ihi;
  int iy;
  int j;
  int ju;
  int k;
  int nRowsD2;
  sintab_data = sintab->data;
  costab_data = costab->data;
  x_data = x->data;
  iy = y->size[0];
  y->size[0] = n1_unsigned;
  emxEnsureCapacity_creal_T(y, iy);
  y_data = y->data;
  if (n1_unsigned > x->size[0]) {
    iy = y->size[0];
    y->size[0] = n1_unsigned;
    emxEnsureCapacity_creal_T(y, iy);
    y_data = y->data;
    for (iy = 0; iy < n1_unsigned; iy++) {
      y_data[iy].re = 0.0;
      y_data[iy].im = 0.0;
    }
  }
  j = x->size[0];
  if (j > n1_unsigned) {
    j = n1_unsigned;
  }
  ihi = n1_unsigned - 2;
  nRowsD2 = (int)((unsigned int)n1_unsigned >> 1);
  k = nRowsD2 / 2;
  iy = 0;
  ju = 0;
  for (i = 0; i <= j - 2; i++) {
    boolean_T tst;
    y_data[iy] = x_data[i];
    iy = n1_unsigned;
    tst = true;
    while (tst) {
      iy >>= 1;
      ju ^= iy;
      tst = ((ju & iy) == 0);
    }
    iy = ju;
  }
  if (j - 2 < 0) {
    j = 0;
  } else {
    j--;
  }
  y_data[iy] = x_data[j];
  if (n1_unsigned > 1) {
    for (i = 0; i <= ihi; i += 2) {
      temp_re_tmp = y_data[i + 1].re;
      temp_im = y_data[i + 1].im;
      temp_re = y_data[i].re;
      twid_re = y_data[i].im;
      y_data[i + 1].re = temp_re - temp_re_tmp;
      y_data[i + 1].im = twid_re - temp_im;
      y_data[i].re = temp_re + temp_re_tmp;
      y_data[i].im = twid_re + temp_im;
    }
  }
  iDelta = 2;
  iDelta2 = 4;
  iheight = ((k - 1) << 2) + 1;
  while (k > 0) {
    for (i = 0; i < iheight; i += iDelta2) {
      iy = i + iDelta;
      temp_re = y_data[iy].re;
      temp_im = y_data[iy].im;
      y_data[iy].re = y_data[i].re - temp_re;
      y_data[iy].im = y_data[i].im - temp_im;
      y_data[i].re += temp_re;
      y_data[i].im += temp_im;
    }
    iy = 1;
    for (j = k; j < nRowsD2; j += k) {
      double twid_im;
      twid_re = costab_data[j];
      twid_im = sintab_data[j];
      i = iy;
      ihi = iy + iheight;
      while (i < ihi) {
        ju = i + iDelta;
        temp_re_tmp = y_data[ju].im;
        temp_im = y_data[ju].re;
        temp_re = twid_re * temp_im - twid_im * temp_re_tmp;
        temp_im = twid_re * temp_re_tmp + twid_im * temp_im;
        y_data[ju].re = y_data[i].re - temp_re;
        y_data[ju].im = y_data[i].im - temp_im;
        y_data[i].re += temp_re;
        y_data[i].im += temp_im;
        i += iDelta2;
      }
      iy++;
    }
    k /= 2;
    iDelta = iDelta2;
    iDelta2 += iDelta2;
    iheight -= iDelta;
  }
}

/*
 * Arguments    : const emxArray_creal_T *x
 *                int n1_unsigned
 *                const emxArray_real_T *costab
 *                const emxArray_real_T *sintab
 *                emxArray_creal_T *y
 * Return Type  : void
 */
static void f_FFTImplementationCallback_r2b(const emxArray_creal_T *x,
                                            int n1_unsigned,
                                            const emxArray_real_T *costab,
                                            const emxArray_real_T *sintab,
                                            emxArray_creal_T *y)
{
  const creal_T *x_data;
  creal_T *y_data;
  const double *costab_data;
  const double *sintab_data;
  double temp_im;
  double temp_re;
  double temp_re_tmp;
  double twid_re;
  int i;
  int iDelta;
  int iDelta2;
  int iheight;
  int ihi;
  int iy;
  int j;
  int ju;
  int k;
  int nRowsD2;
  sintab_data = sintab->data;
  costab_data = costab->data;
  x_data = x->data;
  j = y->size[0];
  y->size[0] = n1_unsigned;
  emxEnsureCapacity_creal_T(y, j);
  y_data = y->data;
  if (n1_unsigned > x->size[0]) {
    j = y->size[0];
    y->size[0] = n1_unsigned;
    emxEnsureCapacity_creal_T(y, j);
    y_data = y->data;
    for (j = 0; j < n1_unsigned; j++) {
      y_data[j].re = 0.0;
      y_data[j].im = 0.0;
    }
  }
  j = x->size[0];
  if (j > n1_unsigned) {
    j = n1_unsigned;
  }
  ihi = n1_unsigned - 2;
  nRowsD2 = (int)((unsigned int)n1_unsigned >> 1);
  k = nRowsD2 / 2;
  iy = 0;
  ju = 0;
  for (i = 0; i <= j - 2; i++) {
    boolean_T tst;
    y_data[iy] = x_data[i];
    iy = n1_unsigned;
    tst = true;
    while (tst) {
      iy >>= 1;
      ju ^= iy;
      tst = ((ju & iy) == 0);
    }
    iy = ju;
  }
  if (j - 2 < 0) {
    ju = 0;
  } else {
    ju = j - 1;
  }
  y_data[iy] = x_data[ju];
  if (n1_unsigned > 1) {
    for (i = 0; i <= ihi; i += 2) {
      temp_re_tmp = y_data[i + 1].re;
      temp_im = y_data[i + 1].im;
      temp_re = y_data[i].re;
      twid_re = y_data[i].im;
      y_data[i + 1].re = temp_re - temp_re_tmp;
      y_data[i + 1].im = twid_re - temp_im;
      y_data[i].re = temp_re + temp_re_tmp;
      y_data[i].im = twid_re + temp_im;
    }
  }
  iDelta = 2;
  iDelta2 = 4;
  iheight = ((k - 1) << 2) + 1;
  while (k > 0) {
    for (i = 0; i < iheight; i += iDelta2) {
      iy = i + iDelta;
      temp_re = y_data[iy].re;
      temp_im = y_data[iy].im;
      y_data[iy].re = y_data[i].re - temp_re;
      y_data[iy].im = y_data[i].im - temp_im;
      y_data[i].re += temp_re;
      y_data[i].im += temp_im;
    }
    iy = 1;
    for (j = k; j < nRowsD2; j += k) {
      double twid_im;
      twid_re = costab_data[j];
      twid_im = sintab_data[j];
      i = iy;
      ihi = iy + iheight;
      while (i < ihi) {
        ju = i + iDelta;
        temp_re_tmp = y_data[ju].im;
        temp_im = y_data[ju].re;
        temp_re = twid_re * temp_im - twid_im * temp_re_tmp;
        temp_im = twid_re * temp_re_tmp + twid_im * temp_im;
        y_data[ju].re = y_data[i].re - temp_re;
        y_data[ju].im = y_data[i].im - temp_im;
        y_data[i].re += temp_re;
        y_data[i].im += temp_im;
        i += iDelta2;
      }
      iy++;
    }
    k /= 2;
    iDelta = iDelta2;
    iDelta2 += iDelta2;
    iheight -= iDelta;
  }
  if (y->size[0] > 1) {
    temp_im = 1.0 / (double)y->size[0];
    iy = y->size[0];
    for (j = 0; j < iy; j++) {
      y_data[j].re *= temp_im;
      y_data[j].im *= temp_im;
    }
  }
}

/*
 * Arguments    : const emxArray_real_T *x
 *                int n2blue
 *                int nfft
 *                const emxArray_real_T *costab
 *                const emxArray_real_T *sintab
 *                const emxArray_real_T *sintabinv
 *                emxArray_creal_T *y
 * Return Type  : void
 */
void c_FFTImplementationCallback_dob(const emxArray_real_T *x, int n2blue,
                                     int nfft, const emxArray_real_T *costab,
                                     const emxArray_real_T *sintab,
                                     const emxArray_real_T *sintabinv,
                                     emxArray_creal_T *y)
{
  emxArray_creal_T *b_fv;
  emxArray_creal_T *fv;
  emxArray_creal_T *wwc;
  creal_T *b_fv_data;
  creal_T *fv_data;
  creal_T *wwc_data;
  creal_T *y_data;
  const double *x_data;
  double nt_im;
  int i;
  int k;
  int minNrowsNx;
  int nRows;
  x_data = x->data;
  emxInit_creal_T(&wwc);
  if ((nfft != 1) && ((nfft & 1) == 0)) {
    int nInt2;
    int nInt2m1;
    int rt;
    nRows = (int)((unsigned int)nfft >> 1);
    nInt2m1 = (nRows + nRows) - 1;
    i = wwc->size[0];
    wwc->size[0] = nInt2m1;
    emxEnsureCapacity_creal_T(wwc, i);
    wwc_data = wwc->data;
    rt = 0;
    wwc_data[nRows - 1].re = 1.0;
    wwc_data[nRows - 1].im = 0.0;
    nInt2 = nRows << 1;
    i = (unsigned short)(nRows - 1);
    for (k = 0; k < i; k++) {
      minNrowsNx = ((k + 1) << 1) - 1;
      if (nInt2 - rt <= minNrowsNx) {
        rt += minNrowsNx - nInt2;
      } else {
        rt += minNrowsNx;
      }
      nt_im = -3.1415926535897931 * (double)rt / (double)nRows;
      wwc_data[(nRows - k) - 2].re = cos(nt_im);
      wwc_data[(nRows - k) - 2].im = -sin(nt_im);
    }
    i = nInt2m1 - 1;
    for (k = i; k >= nRows; k--) {
      wwc_data[k] = wwc_data[(nInt2m1 - k) - 1];
    }
  } else {
    int nInt2;
    int nInt2m1;
    int rt;
    nInt2m1 = (nfft + nfft) - 1;
    i = wwc->size[0];
    wwc->size[0] = nInt2m1;
    emxEnsureCapacity_creal_T(wwc, i);
    wwc_data = wwc->data;
    rt = 0;
    wwc_data[nfft - 1].re = 1.0;
    wwc_data[nfft - 1].im = 0.0;
    nInt2 = nfft << 1;
    i = (unsigned short)(nfft - 1);
    for (k = 0; k < i; k++) {
      minNrowsNx = ((k + 1) << 1) - 1;
      if (nInt2 - rt <= minNrowsNx) {
        rt += minNrowsNx - nInt2;
      } else {
        rt += minNrowsNx;
      }
      nt_im = -3.1415926535897931 * (double)rt / (double)nfft;
      minNrowsNx = (nfft - k) - 2;
      wwc_data[minNrowsNx].re = cos(nt_im);
      wwc_data[minNrowsNx].im = -sin(nt_im);
    }
    i = nInt2m1 - 1;
    for (k = i; k >= nfft; k--) {
      wwc_data[k] = wwc_data[(nInt2m1 - k) - 1];
    }
  }
  i = y->size[0];
  y->size[0] = nfft;
  emxEnsureCapacity_creal_T(y, i);
  y_data = y->data;
  if (nfft > x->size[0]) {
    i = y->size[0];
    y->size[0] = nfft;
    emxEnsureCapacity_creal_T(y, i);
    y_data = y->data;
    for (i = 0; i < nfft; i++) {
      y_data[i].re = 0.0;
      y_data[i].im = 0.0;
    }
  }
  emxInit_creal_T(&fv);
  emxInit_creal_T(&b_fv);
  if ((n2blue != 1) && ((nfft & 1) == 0)) {
    d_FFTImplementationCallback_doH(x, y, x->size[0], nfft, n2blue, wwc, costab,
                                    sintab, costab, sintabinv);
  } else {
    double b_re_tmp;
    double c_re_tmp;
    double re_tmp;
    minNrowsNx = x->size[0];
    if (nfft <= minNrowsNx) {
      minNrowsNx = nfft;
    }
    i = (unsigned short)minNrowsNx;
    for (k = 0; k < i; k++) {
      nRows = (nfft + k) - 1;
      y_data[k].re = wwc_data[nRows].re * x_data[k];
      y_data[k].im = wwc_data[nRows].im * -x_data[k];
    }
    i = minNrowsNx + 1;
    for (k = i; k <= nfft; k++) {
      y_data[k - 1].re = 0.0;
      y_data[k - 1].im = 0.0;
    }
    e_FFTImplementationCallback_r2b(y, n2blue, costab, sintab, fv);
    fv_data = fv->data;
    e_FFTImplementationCallback_r2b(wwc, n2blue, costab, sintab, b_fv);
    i = b_fv->size[0];
    b_fv->size[0] = fv->size[0];
    emxEnsureCapacity_creal_T(b_fv, i);
    b_fv_data = b_fv->data;
    minNrowsNx = fv->size[0];
    for (i = 0; i < minNrowsNx; i++) {
      nt_im = fv_data[i].re;
      re_tmp = b_fv_data[i].im;
      b_re_tmp = fv_data[i].im;
      c_re_tmp = b_fv_data[i].re;
      b_fv_data[i].re = nt_im * c_re_tmp - b_re_tmp * re_tmp;
      b_fv_data[i].im = nt_im * re_tmp + b_re_tmp * c_re_tmp;
    }
    f_FFTImplementationCallback_r2b(b_fv, n2blue, costab, sintabinv, fv);
    fv_data = fv->data;
    i = wwc->size[0];
    for (k = nfft; k <= i; k++) {
      nt_im = wwc_data[k - 1].re;
      re_tmp = fv_data[k - 1].im;
      b_re_tmp = wwc_data[k - 1].im;
      c_re_tmp = fv_data[k - 1].re;
      minNrowsNx = k - nfft;
      y_data[minNrowsNx].re = nt_im * c_re_tmp + b_re_tmp * re_tmp;
      y_data[minNrowsNx].im = nt_im * re_tmp - b_re_tmp * c_re_tmp;
    }
  }
  emxFree_creal_T(&b_fv);
  emxFree_creal_T(&fv);
  emxFree_creal_T(&wwc);
}

/*
 * Arguments    : int nRows
 *                boolean_T useRadix2
 *                emxArray_real_T *costab
 *                emxArray_real_T *sintab
 *                emxArray_real_T *sintabinv
 * Return Type  : void
 */
void c_FFTImplementationCallback_gen(int nRows, boolean_T useRadix2,
                                     emxArray_real_T *costab,
                                     emxArray_real_T *sintab,
                                     emxArray_real_T *sintabinv)
{
  emxArray_real_T *costab1q;
  double e;
  double *costab1q_data;
  double *costab_data;
  double *sintab_data;
  double *sintabinv_data;
  int i;
  int k;
  int n;
  int nd2;
  e = 6.2831853071795862 / (double)nRows;
  n = (int)((unsigned int)nRows >> 1) >> 1;
  emxInit_real_T(&costab1q, 2);
  i = costab1q->size[0] * costab1q->size[1];
  costab1q->size[0] = 1;
  costab1q->size[1] = n + 1;
  emxEnsureCapacity_real_T(costab1q, i);
  costab1q_data = costab1q->data;
  costab1q_data[0] = 1.0;
  nd2 = ((unsigned short)n >> 1) - 1;
  for (k = 0; k <= nd2; k++) {
    costab1q_data[k + 1] = cos(e * ((double)k + 1.0));
  }
  i = nd2 + 2;
  nd2 = n - 1;
  for (k = i; k <= nd2; k++) {
    costab1q_data[k] = sin(e * (double)(n - k));
  }
  costab1q_data[n] = 0.0;
  if (!useRadix2) {
    n = costab1q->size[1] - 1;
    nd2 = (costab1q->size[1] - 1) << 1;
    i = costab->size[0] * costab->size[1];
    costab->size[0] = 1;
    costab->size[1] = nd2 + 1;
    emxEnsureCapacity_real_T(costab, i);
    costab_data = costab->data;
    i = sintab->size[0] * sintab->size[1];
    sintab->size[0] = 1;
    sintab->size[1] = nd2 + 1;
    emxEnsureCapacity_real_T(sintab, i);
    sintab_data = sintab->data;
    costab_data[0] = 1.0;
    sintab_data[0] = 0.0;
    i = sintabinv->size[0] * sintabinv->size[1];
    sintabinv->size[0] = 1;
    sintabinv->size[1] = nd2 + 1;
    emxEnsureCapacity_real_T(sintabinv, i);
    sintabinv_data = sintabinv->data;
    for (k = 0; k < n; k++) {
      sintabinv_data[k + 1] = costab1q_data[(n - k) - 1];
    }
    i = costab1q->size[1];
    for (k = i; k <= nd2; k++) {
      sintabinv_data[k] = costab1q_data[k - n];
    }
    for (k = 0; k < n; k++) {
      costab_data[k + 1] = costab1q_data[k + 1];
      sintab_data[k + 1] = -costab1q_data[(n - k) - 1];
    }
    i = costab1q->size[1];
    for (k = i; k <= nd2; k++) {
      costab_data[k] = -costab1q_data[nd2 - k];
      sintab_data[k] = -costab1q_data[k - n];
    }
  } else {
    n = costab1q->size[1] - 1;
    nd2 = (costab1q->size[1] - 1) << 1;
    i = costab->size[0] * costab->size[1];
    costab->size[0] = 1;
    costab->size[1] = nd2 + 1;
    emxEnsureCapacity_real_T(costab, i);
    costab_data = costab->data;
    i = sintab->size[0] * sintab->size[1];
    sintab->size[0] = 1;
    sintab->size[1] = nd2 + 1;
    emxEnsureCapacity_real_T(sintab, i);
    sintab_data = sintab->data;
    costab_data[0] = 1.0;
    sintab_data[0] = 0.0;
    for (k = 0; k < n; k++) {
      costab_data[k + 1] = costab1q_data[k + 1];
      sintab_data[k + 1] = -costab1q_data[(n - k) - 1];
    }
    i = costab1q->size[1];
    for (k = i; k <= nd2; k++) {
      costab_data[k] = -costab1q_data[nd2 - k];
      sintab_data[k] = -costab1q_data[k - n];
    }
    sintabinv->size[0] = 1;
    sintabinv->size[1] = 0;
  }
  emxFree_real_T(&costab1q);
}

/*
 * Arguments    : const emxArray_real_T *x
 *                int n1_unsigned
 *                const emxArray_real_T *costab
 *                const emxArray_real_T *sintab
 *                emxArray_creal_T *y
 * Return Type  : void
 */
void c_FFTImplementationCallback_r2b(const emxArray_real_T *x, int n1_unsigned,
                                     const emxArray_real_T *costab,
                                     const emxArray_real_T *sintab,
                                     emxArray_creal_T *y)
{
  emxArray_creal_T *b_y;
  creal_T *b_y_data;
  creal_T *y_data;
  const double *x_data;
  int i;
  x_data = x->data;
  i = y->size[0];
  y->size[0] = n1_unsigned;
  emxEnsureCapacity_creal_T(y, i);
  y_data = y->data;
  if (n1_unsigned > x->size[0]) {
    i = y->size[0];
    y->size[0] = n1_unsigned;
    emxEnsureCapacity_creal_T(y, i);
    y_data = y->data;
    for (i = 0; i < n1_unsigned; i++) {
      y_data[i].re = 0.0;
      y_data[i].im = 0.0;
    }
  }
  if (n1_unsigned != 1) {
    c_FFTImplementationCallback_doH(x, y, n1_unsigned, costab, sintab);
  } else {
    int loop_ub;
    y_data[0].re = x_data[0];
    y_data[0].im = 0.0;
    emxInit_creal_T(&b_y);
    i = b_y->size[0];
    b_y->size[0] = y->size[0];
    emxEnsureCapacity_creal_T(b_y, i);
    b_y_data = b_y->data;
    loop_ub = y->size[0];
    for (i = 0; i < loop_ub; i++) {
      b_y_data[i] = y_data[i];
    }
    i = y->size[0];
    y->size[0] = b_y->size[0];
    emxEnsureCapacity_creal_T(y, i);
    y_data = y->data;
    loop_ub = b_y->size[0];
    for (i = 0; i < loop_ub; i++) {
      y_data[i] = b_y_data[i];
    }
    emxFree_creal_T(&b_y);
  }
}

/*
 * File trailer for FFTImplementationCallback.c
 *
 * [EOF]
 */
