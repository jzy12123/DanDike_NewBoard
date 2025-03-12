#include "FFT2C.h"
#include "ADDA.h"
#include "FFT2C_TYPES.h"
#include "FFT2C_emxAPI.h"
#include "FFT2C_terminate.h"
#include "rt_nonfinite.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>

void free_emxArray_real_T(emxArray_real_T *emxArray);
void FFT_Calculate(int chanel);
