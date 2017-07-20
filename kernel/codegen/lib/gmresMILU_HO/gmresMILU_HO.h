#ifndef GMRESMILU_HO_H
#define GMRESMILU_HO_H
#include <math.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include "rtwtypes.h"
#include "gmresMILU_HO_types.h"

extern void gmresMILU_HO(const struct0_T *A, const emxArray_real_T *b, const
  struct1_T *prec, int restart, double rtol, int maxit, const emxArray_real_T
  *x0, int verbose, int nthreads, const struct1_T *param, const emxArray_real_T *
  rowscal, const emxArray_real_T *colscal, emxArray_real_T *x, int *flag, int
  *iter, emxArray_real_T *resids);
extern void gmresMILU_HO_initialize(void);
extern void gmresMILU_HO_terminate(void);

#endif