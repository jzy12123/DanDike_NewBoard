/*
 * File: eml_setop.c
 *
 * MATLAB Coder version            : 23.2
 * C/C++ source code generated on  : 31-May-2024 16:43:36
 */

/* Include Files */
#include "eml_setop.h"
#include "FFT2C_emxutil.h"
#include "FFT2C_types.h"
#include "rt_nonfinite.h"

/* Function Definitions */
/*
 * Arguments    : const int a_data[]
 *                int a_size
 *                const int b_data[]
 *                int b_size
 *                emxArray_int32_T *c
 *                int ia_data[]
 *                int ib_data[]
 *                int *ib_size
 * Return Type  : int
 */
int do_vectors(const int a_data[], int a_size, const int b_data[], int b_size,
               emxArray_int32_T *c, int ia_data[], int ib_data[], int *ib_size)
{
  int b_ialast;
  int b_iblast;
  int ia_size;
  int iafirst;
  int ialast;
  int ibfirst;
  int iblast;
  int nc;
  int ncmax;
  int nia;
  int nib;
  int *c_data;
  ncmax = a_size + b_size;
  nia = c->size[0];
  c->size[0] = ncmax;
  emxEnsureCapacity_int32_T(c, nia);
  c_data = c->data;
  ia_size = a_size;
  *ib_size = b_size;
  nc = -1;
  nia = -1;
  nib = 0;
  iafirst = 1;
  ialast = 0;
  ibfirst = 0;
  iblast = 0;
  while ((ialast + 1 <= a_size) && (iblast + 1 <= b_size)) {
    int ak;
    int bk;
    b_ialast = ialast + 1;
    ak = a_data[ialast];
    while ((b_ialast < a_size) && (a_data[b_ialast] == ak)) {
      b_ialast++;
    }
    ialast = b_ialast - 1;
    b_iblast = iblast + 1;
    bk = b_data[iblast];
    while ((b_iblast < b_size) && (b_data[b_iblast] == bk)) {
      b_iblast++;
    }
    iblast = b_iblast - 1;
    if (ak == bk) {
      nc++;
      c_data[nc] = ak;
      nia++;
      ia_data[nia] = iafirst;
      ialast = b_ialast;
      iafirst = b_ialast + 1;
      iblast = b_iblast;
      ibfirst = b_iblast;
    } else if (ak < bk) {
      nc++;
      nia++;
      c_data[nc] = ak;
      ia_data[nia] = iafirst;
      ialast = b_ialast;
      iafirst = b_ialast + 1;
    } else {
      nc++;
      nib++;
      c_data[nc] = bk;
      ib_data[nib - 1] = ibfirst + 1;
      iblast = b_iblast;
      ibfirst = b_iblast;
    }
  }
  while (ialast + 1 <= a_size) {
    b_ialast = ialast + 1;
    while ((b_ialast < a_size) && (a_data[b_ialast] == a_data[ialast])) {
      b_ialast++;
    }
    nc++;
    nia++;
    c_data[nc] = a_data[ialast];
    ia_data[nia] = iafirst;
    ialast = b_ialast;
    iafirst = b_ialast + 1;
  }
  while (iblast + 1 <= b_size) {
    b_iblast = iblast + 1;
    while ((b_iblast < b_size) && (b_data[b_iblast] == b_data[iblast])) {
      b_iblast++;
    }
    nc++;
    nib++;
    c_data[nc] = b_data[iblast];
    ib_data[nib - 1] = ibfirst + 1;
    iblast = b_iblast;
    ibfirst = b_iblast;
  }
  if (a_size > 0) {
    if (nia + 1 < 1) {
      ia_size = 0;
    } else {
      ia_size = nia + 1;
    }
  }
  if (b_size > 0) {
    if (nib < 1) {
      *ib_size = 0;
    } else {
      *ib_size = nib;
    }
  }
  if (ncmax > 0) {
    nia = c->size[0];
    if (nc + 1 < 1) {
      c->size[0] = 0;
    } else {
      c->size[0] = nc + 1;
    }
    emxEnsureCapacity_int32_T(c, nia);
  }
  return ia_size;
}

/*
 * File trailer for eml_setop.c
 *
 * [EOF]
 */
