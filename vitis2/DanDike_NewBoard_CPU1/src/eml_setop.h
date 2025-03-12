/*
 * File: eml_setop.h
 *
 * MATLAB Coder version            : 23.2
 * C/C++ source code generated on  : 31-May-2024 16:43:36
 */

#ifndef EML_SETOP_H
#define EML_SETOP_H

/* Include Files */
#include "FFT2C_types.h"
#include "rtwtypes.h"
#include <stddef.h>
#include <stdlib.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Function Declarations */
int do_vectors(const int a_data[], int a_size, const int b_data[], int b_size,
               emxArray_int32_T *c, int ia_data[], int ib_data[], int *ib_size);

#ifdef __cplusplus
}
#endif

#endif
/*
 * File trailer for eml_setop.h
 *
 * [EOF]
 */
