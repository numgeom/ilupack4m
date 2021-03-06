/* ========================================================================== */
/* === AMGfactor mexFunction ================================================ */
/* ========================================================================== */

/*
    Usage:

    Return the structure 'options' and preconditioner 'PREC' for ILUPACK V2.3

    Example:

    % for initializing parameters
    [PREC, options, rcomflag,S,tv] = ZSYMilupackfactor(A,options,PRE,tv);



    Authors:

        Matthias Bollhoefer, TU Braunschweig

    Date:

        March 19, 2009. ILUPACK V2.3.

    Notice:

        Copyright (c) 2009 by TU Braunschweig.  All Rights Reserved.

        THIS MATERIAL IS PROVIDED AS IS, WITH ABSOLUTELY NO WARRANTY
        EXPRESSED OR IMPLIED.  ANY USE IS AT YOUR OWN RISK.

    Availability:

        This file is located at

        http://ilupack.tu-bs.de/
*/

/* ========================================================================== */
/* === Include files and prototypes ========================================= */
/* ========================================================================== */

#include "matrix.h"
#include "mex.h"
#include <ilupack.h>
#include <lapack.h>
#include <stdlib.h>
#include <string.h>

#define MAX(A, B) (((A) >= (B)) ? (A) : (B))
#define MAX_FIELDS 100
/* #define PRINT_INFO */

/* ========================================================================== */
/* === mexFunction ========================================================== */
/* ========================================================================== */

void mexFunction(
    /* === Parameters ======================================================= */

    int nlhs,             /* number of left-hand sides */
    mxArray *plhs[],      /* left-hand side matrices */
    int nrhs,             /* number of right--hand sides */
    const mxArray *prhs[] /* right-hand side matrices */
    ) {
  Zmat A, M;
  ZAMGlevelmat *PRE, *current;
  CAMGlevelmat *SPRE, *scurrent;
  ZILUPACKparam *param;
  integer n, nnzU;
  int tv_exists, tv_field, ind_exists, ind_field;

  const char **fnames;
  const char *pnames[] = {
      "n",           "nB",          "L",        "D",       "U",
      "E",           "F",           "rowscal",  "colscal", "p",
      "invq",        "param",       "ptr",      "isreal",  "isdefinite",
      "issymmetric", "ishermitian", "issingle", "A_H",     "errorL",
      "errorU",      "errorS",      "isblock",  "indB",    "LB"};

  const mwSize *dims;
  const mwSize mydims[] = {1, 1};
  mwSize mrows, ncols, buflen, ndim, nnz;
  mxClassID *classIDflags;
  mxArray *PRE_input, *tv_input, *A_input, *options_input, *PRE_output,
      *options_output, *S_output, *tv_output, *tmp, *fout;
  char *pdata, *input_buf, *output_buf;
  int status, ifield, nfields, ierr, i, j, k, l, m, ind_shiftmatrix;
  integer *ibuff, *Ibuff, ii, blocksize, *istack, jj, kk, ll;
  size_t sizebuf;
  double dbuf, *A_valuesR, *A_valuesI, *convert, *sr, *si, *pr, *pi, det, detr,
      deti;
  mwIndex jstruct, *irs, *jcs, *A_ja, /* row indices of input matrix A */
      *A_ia;                          /* column pointers of input matrix A */
  doublecomplex *mytv, *Dbuff;

  if (nrhs != 2 && nrhs != 4)
    mexErrMsgTxt("Two/four input arguments required.");
  else if (nlhs != 2 && nlhs != 5)
    mexErrMsgTxt("wrong number of output arguments.");
  else if (!mxIsStruct(prhs[1]))
    mexErrMsgTxt("Second input must be a structure.");
  else if (!mxIsNumeric(prhs[0]))
    mexErrMsgTxt("First input must be a matrix.");

  /* The first input must be a square matrix.*/
  A_input = (mxArray *)prhs[0];
  /* get size of input matrix A */
  mrows = mxGetM(A_input);
  ncols = mxGetN(A_input);
  nnz = mxGetNzmax(A_input);
  if (mrows != ncols) {
    mexErrMsgTxt("First input must be a square matrix.");
  }
  if (!mxIsSparse(A_input)) {
    mexErrMsgTxt("ILUPACK: input matrix must be in sparse format.");
  }

  /* copy input matrix to sparse row format */
  A.nc = A.nr = mrows;
  A.ia = (integer *)MAlloc((size_t)(A.nc + 1) * sizeof(integer),
                           "ZSYMilupackfactor");
  A.ja = (integer *)MAlloc((size_t)nnz * sizeof(integer), "ZSYMilupackfactor");
  A.a = (doublecomplex *)MAlloc((size_t)nnz * sizeof(doublecomplex),
                                "ZSYMilupackfactor");

  A_ja = (mwIndex *)mxGetIr(A_input);
  A_ia = (mwIndex *)mxGetJc(A_input);
  A_valuesR = (double *)mxGetPr(A_input);
  A_valuesI = (double *)mxGetPi(A_input);

  /* -------------------------------------------------------------------- */
  /* ..  Convert matrix from 0-based C-notation to Fortran 1-based        */
  /*     notation.                                                        */
  /* -------------------------------------------------------------------- */

  /*
  for (i = 0 ; i < ncols ; i++)
    for (j = A_ia[i] ; j < A_ia[i+1] ; j++)
      printf("i=%d ja=%d  A.real=%e A.imag=%e\n", i+1,  A_ja[j]+1, A_valuesR[j],
  A_valuesI[j]);
  */

  A.ia[0] = 1;
  for (i = 0; i < ncols; i++) {
    A.ia[i + 1] = A.ia[i];
    for (j = A_ia[i]; j < A_ia[i + 1]; j++) {
      k = A_ja[j];
      if (k >= i) {
        l = A.ia[i + 1] - 1;
        A.ja[l] = k + 1;
        A.a[l].r = A_valuesR[j];
        A.a[l].i = A_valuesI[j];
        A.ia[i + 1] = l + 2;
      }
    }
  }

  /*
  for (i = 0 ; i < A.nr ; i++)
    for (j = A.ia[i]-1 ; j < A.ia[i+1]-1 ; j++)
        printf("i=%d ja=%d  A.real=%e  A.imag=%e\n", i+1,  A.ja[j], A.a[j].r,
  A.a[j].i);
  */

  param = (ZILUPACKparam *)MAlloc((size_t)sizeof(ZILUPACKparam),
                                  "ZSYMilupackfactor:param");
  ZSYMAMGinit(&A, param);

#ifdef PRINT_INFO
  mexPrintf("ZSYMilupackfactor: copy parameters\n");
  fflush(stdout);
#endif
  /* Get second input arguments */
  options_input = (mxArray *)prhs[1];
  nfields = mxGetNumberOfFields(options_input);

  /* Allocate memory  for storing classIDflags */
  classIDflags =
      (mxClassID *)mxCalloc((size_t)nfields, (size_t)sizeof(mxClassID));

  /* allocate memory  for storing pointers */
  fnames = mxCalloc((size_t)nfields, (size_t)sizeof(*fnames));

  /* Get field name pointers */
  for (ifield = 0; ifield < nfields; ifield++) {
    fnames[ifield] = mxGetFieldNameByNumber(options_input, ifield);
  }

  /* import data */
  tv_exists = 0;
  tv_field = -1;
  ind_exists = 0;
  ind_field = -1;
  for (ifield = 0; ifield < nfields; ifield++) {
    tmp = mxGetFieldByNumber(options_input, 0, ifield);
    classIDflags[ifield] = mxGetClassID(tmp);

    ndim = mxGetNumberOfDimensions(tmp);
    dims = mxGetDimensions(tmp);

    /* Create string/numeric array */
    if (classIDflags[ifield] == mxCHAR_CLASS) {
      /* Get the length of the input string. */
      buflen = (mxGetM(tmp) * mxGetN(tmp)) + 1;

      /* Allocate memory for input and output strings. */
      input_buf = mxCalloc((size_t)buflen, (size_t)sizeof(char));

      /* Copy the string data from tmp into a C string
         input_buf. */
      status = mxGetString(tmp, input_buf, buflen);

      if (!strcmp("amg", fnames[ifield])) {
        if (strcmp(param->amg, input_buf)) {
          param->amg =
              (char *)MAlloc((size_t)buflen * sizeof(char), "ilupackfactor");
          strcpy(param->amg, input_buf);
        }
      } else if (!strcmp("presmoother", fnames[ifield])) {
        if (strcmp(param->presmoother, input_buf)) {
          param->presmoother =
              (char *)MAlloc((size_t)buflen * sizeof(char), "ilupackfactor");
          strcpy(param->presmoother, input_buf);
        }
      } else if (!strcmp("postsmoother", fnames[ifield])) {
        if (strcmp(param->postsmoother, input_buf)) {
          param->postsmoother =
              (char *)MAlloc((size_t)buflen * sizeof(char), "ilupackfactor");
          strcpy(param->postsmoother, input_buf);
        }
      } else if (!strcmp("typecoarse", fnames[ifield])) {
        if (strcmp(param->typecoarse, input_buf)) {
          param->typecoarse =
              (char *)MAlloc((size_t)buflen * sizeof(char), "ilupackfactor");
          strcpy(param->typecoarse, input_buf);
        }
      } else if (!strcmp("typetv", fnames[ifield])) {
        if (strcmp(param->typetv, input_buf)) {
          param->typetv =
              (char *)MAlloc((size_t)buflen * sizeof(char), "ilupackfactor");
          strcpy(param->typetv, input_buf);
        }
        /* 'static' or 'dynamic' test vector */
        if (strcmp("none", input_buf))
          tv_exists = -1;
      } else if (!strcmp("FCpart", fnames[ifield])) {
        if (strcmp(param->FCpart, input_buf)) {
          param->FCpart =
              (char *)MAlloc((size_t)buflen * sizeof(char), "ilupackfactor");
          strcpy(param->FCpart, input_buf);
        }
      } else if (!strcmp("solver", fnames[ifield])) {
        if (strcmp(param->solver, input_buf)) {
          param->solver =
              (char *)MAlloc((size_t)buflen * sizeof(char), "ilupackfactor");
          strcpy(param->solver, input_buf);
        }
      } else if (!strcmp("ordering", fnames[ifield])) {
        if (strcmp(param->ordering, input_buf)) {
          param->ordering =
              (char *)MAlloc((size_t)buflen * sizeof(char), "ilupackfactor");
          strcpy(param->ordering, input_buf);
        }
      } else {
        /* mexPrintf("%s ignored\n",fnames[ifield]);fflush(stdout); */
      }
    } else {
      if (!strcmp("elbow", fnames[ifield])) {
        param->elbow = *mxGetPr(tmp);
      } else if (!strcmp("lfilS", fnames[ifield])) {
        param->lfilS = *mxGetPr(tmp);
      } else if (!strcmp("lfil", fnames[ifield])) {
        param->lfil = *mxGetPr(tmp);
      } else if (!strcmp("maxit", fnames[ifield])) {
        param->maxit = *mxGetPr(tmp);
      } else if (!strcmp("droptolS", fnames[ifield])) {
        param->droptolS = *mxGetPr(tmp);
      } else if (!strcmp("droptolc", fnames[ifield])) {
        param->droptolc = *mxGetPr(tmp);
      } else if (!strcmp("droptol", fnames[ifield])) {
        param->droptol = *mxGetPr(tmp);
      } else if (!strcmp("condest", fnames[ifield])) {
        param->condest = *mxGetPr(tmp);
      } else if (!strcmp("restol", fnames[ifield])) {
        param->restol = *mxGetPr(tmp);
      } else if (!strcmp("npresmoothing", fnames[ifield])) {
        param->npresmoothing = *mxGetPr(tmp);
      } else if (!strcmp("npostmoothing", fnames[ifield])) {
        param->npostsmoothing = *mxGetPr(tmp);
      } else if (!strcmp("ncoarse", fnames[ifield])) {
        param->ncoarse = *mxGetPr(tmp);
      } else if (!strcmp("matching", fnames[ifield])) {
        param->matching = *mxGetPr(tmp);
      } else if (!strcmp("nrestart", fnames[ifield])) {
        param->nrestart = *mxGetPr(tmp);
      } else if (!strcmp("damping", fnames[ifield])) {
        param->damping = *mxGetPr(tmp);
      } else if (!strcmp("contraction", fnames[ifield])) {
        param->contraction = *mxGetPr(tmp);
      } else if (!strcmp("tv", fnames[ifield])) {
        tv_field = ifield;
      } else if (!strcmp("ind", fnames[ifield])) {
        ind_exists = -1;
        ind_field = ifield;
      } else if (!strcmp("mixedprecision", fnames[ifield])) {
        param->mixedprecision = *mxGetPr(tmp);
      } else if (!strcmp("coarsereduce", fnames[ifield])) {
        if (*mxGetPr(tmp) != 0.0)
          param->flags |= COARSE_REDUCE;
        else
          param->flags &= ~COARSE_REDUCE;
      } else if (!strcmp("decoupleconstraints", fnames[ifield])) {
        if (*mxGetPr(tmp) > 0.0)
          param->flags |= DECOUPLE_CONSTRAINTSHH;
        else if (*mxGetPr(tmp) < 0.0)
          param->flags |= DECOUPLE_CONSTRAINTS;
        else
          param->flags &= ~(DECOUPLE_CONSTRAINTS | DECOUPLE_CONSTRAINTSHH);
      } else if (!strcmp("shift0", fnames[ifield])) {
#ifdef PRINT_INFO
        mexPrintf("ZSYMilupackfactor: copy shift0\n");
        fflush(stdout);
#endif
        pr = mxGetPr(tmp);
        pi = mxGetPi(tmp);
        param->shift0.r = *pr;
        if (pi != NULL)
          param->shift0.i = *pi;
        else
          param->shift0.i = 0;
#ifdef PRINT_INFO
        mexPrintf("ZSYMilupackfactor: shift0 copied\n");
        fflush(stdout);
#endif
      } else if (!strcmp("shiftmax", fnames[ifield])) {
#ifdef PRINT_INFO
        mexPrintf("ZSYMilupackfactor: copy shiftmax\n");
        fflush(stdout);
#endif
        pr = mxGetPr(tmp);
        pi = mxGetPi(tmp);
        param->shiftmax.r = *pr;
        if (pi != NULL)
          param->shiftmax.i = *pi;
        else
          param->shiftmax.i = 0;
#ifdef PRINT_INFO
        mexPrintf("ZSYMilupackfactor: shiftmax copied\n");
        fflush(stdout);
#endif
      } else if (!strcmp("shifts", fnames[ifield])) {
#ifdef PRINT_INFO
        mexPrintf("ZSYMilupackfactor: copy shifts\n");
        fflush(stdout);
#endif
        mrows = mxGetM(tmp);
        ncols = mxGetN(tmp);
        param->nshifts = MAX(mrows, ncols);
        param->shifts = (doublecomplex *)MAlloc(
            param->nshifts * sizeof(doublecomplex), "ZSYMilupackfactor");
        pr = mxGetPr(tmp);
        pi = mxGetPi(tmp);
        for (i = 0; i < param->nshifts; i++) {
          param->shifts[i].r = pr[i];
          if (pi != NULL)
            param->shifts[i].i = pi[i];
          else
            param->shifts[i].i = 0;
        }
#ifdef PRINT_INFO
        mexPrintf("ZSYMilupackfactor: shifts copied\n");
        fflush(stdout);
#endif
      } else if (!strcmp("shiftmatrix", fnames[ifield])) {
#ifdef PRINT_INFO
        mexPrintf("ZSYMilupackfactor: copy shift matrix\n");
        fflush(stdout);
#endif
        ind_shiftmatrix = ifield;
        mrows = mxGetM(tmp);
        ncols = mxGetN(tmp);
        nnz = mxGetNzmax(tmp);
        if (mrows != ncols) {
          mexErrMsgTxt("Shift matrix must be a square matrix.");
        }
        if (!mxIsSparse(tmp)) {
          mexErrMsgTxt("ILUPACK: shift matrix must be in sparse format.");
        }

        /* copy input matrix to sparse row format */
        M.nc = M.nr = mrows;
        M.ia = (integer *)MAlloc((size_t)(M.nc + 1) * sizeof(integer),
                                 "ZSYMilupackfactor");
        M.ja = (integer *)MAlloc((size_t)nnz * sizeof(integer),
                                 "ZSYMilupackfactor");
        M.a = (doublecomplex *)MAlloc((size_t)nnz * sizeof(doublecomplex),
                                      "ZSYMilupackfactor");

        A_ja = (mwIndex *)mxGetIr(tmp);
        A_ia = (mwIndex *)mxGetJc(tmp);
        A_valuesR = (double *)mxGetPr(tmp);
        A_valuesI = (double *)mxGetPi(tmp);

        /* --------------------------------------------------------------------
         */
        /* ..  Convert matrix from 0-based C-notation to Fortran 1-based */
        /*     notation. */
        /* --------------------------------------------------------------------
         */

        /*
          for (i = 0 ; i < ncols ; i++)
          for (j = A_ia[i] ; j < A_ia[i+1] ; j++)
          printf("i=%d ja=%d  A.real=%e\n", i+1,  A_ja[j]+1, A_valuesR[j]);
        */

        M.ia[0] = 1;
        for (i = 0; i < ncols; i++) {
          M.ia[i + 1] = M.ia[i];
          for (j = A_ia[i]; j < A_ia[i + 1]; j++) {
            k = A_ja[j];
            if (k >= i) {
              l = M.ia[i + 1] - 1;
              M.ja[l] = k + 1;
              M.a[l].r = A_valuesR[j];
              if (A_valuesI != NULL)
                M.a[l].i = A_valuesI[j];
              else
                M.a[l].i = 0;
              M.ia[i + 1] = l + 2;
            }
          }
        }
        param->shiftmatrix = &M;
#ifdef PRINT_INFO
        mexPrintf("ZSYMilupackfactor: shift matrix copied\n");
        fflush(stdout);
#endif
      } else {
        /* mexPrintf("%s ignored\n",fnames[ifield]);fflush(stdout); */
      }
    }
  }
  if (param->droptolS > 0.125 * param->droptol) {
    mexPrintf("!!! ILUPACK Warning !!!\n");
    mexPrintf("`param.droptolS' is recommended to be one order of magnitude "
              "less than `param.droptol'\n");
  }
#ifdef PRINT_INFO
  mexPrintf("ZSYMilupackfactor: parameters copied\n");
  fflush(stdout);
#endif

  if (tv_exists && tv_field >= 0 &&
      (nrhs == 2 || (nrhs == 4 && mxIsNumeric(prhs[2])))) {
    tmp = mxGetFieldByNumber(options_input, 0, tv_field);
    pr = mxGetPr(tmp);
    mytv = (doublecomplex *)MAlloc((size_t)A.nr * sizeof(doublecomplex),
                                   "ZSYMilupackfactor");
    param->tv = mytv;
    if (!mxIsComplex(tmp)) {
      for (i = 0; i < A.nr; i++) {
        param->tv[i].r = pr[i];
        param->tv[i].i = 0;
      }
    } else {
      pi = mxGetPi(tmp);
      for (i = 0; i < A.nr; i++) {
        param->tv[i].r = pr[i];
        param->tv[i].i = pi[i];
      }
    }
  }
  if (ind_exists && ind_field >= 0) {
    tmp = mxGetFieldByNumber(options_input, 0, ind_field);
    param->ind = (integer *)MAlloc(A.nc * sizeof(integer), "DSYMilupackfactor");
    pr = mxGetPr(tmp);
    for (i = 0; i < A.nc; i++)
      param->ind[i] = pr[i];
  }
  mxFree(fnames);

  PRE =
      (ZAMGlevelmat *)MAlloc((size_t)sizeof(ZAMGlevelmat), "ZSYMilupackfactor");

#ifdef PRINT_INFO
  mexPrintf("ZSYMilupackfactor: factorize matrix\n");
  fflush(stdout);
#endif
  if (nrhs == 4) {

    /* at least the second time that we call this routine */
    if (!mxIsNumeric(prhs[2])) {
      /* import pointer to the preconditioner */
      PRE_input = (mxArray *)prhs[2];
      /* get number of levels of input preconditioner structure `PREC' */
      /* nlev=mxGetN(PRE_input); */
      nfields = mxGetNumberOfFields(PRE_input);
      /* allocate memory  for storing pointers */
      fnames = mxCalloc((size_t)nfields, (size_t)sizeof(*fnames));
      for (ifield = 0; ifield < nfields; ifield++) {
        fnames[ifield] = mxGetFieldNameByNumber(PRE_input, ifield);
        /* check whether `PREC.ptr' exists */
        if (!strcmp("ptr", fnames[ifield])) {
          /* field `ptr' */
          tmp = mxGetFieldByNumber(PRE_input, 0, ifield);
          pdata = mxGetData(tmp);
          memcpy(&PRE, pdata, (size_t)sizeof(size_t));
        } else if (!strcmp("param", fnames[ifield])) {
          /* field `param' */
          tmp = mxGetFieldByNumber(PRE_input, 0, ifield);
          pdata = mxGetData(tmp);
          memcpy(&param, pdata, (size_t)sizeof(size_t));
        }
      }
      mxFree(fnames);

      /* import pointer to the test vector */
      tv_input = (mxArray *)prhs[3];
      /* size of the test vector */
      j = mxGetM(tv_input);
      pr = mxGetPr(tv_input);

      if (!mxIsComplex(tv_input)) {
        for (i = 0; i < j; i++) {
          param->tv[i].r = pr[i];
          param->tv[i].i = 0;
        }
      } else {
        pi = mxGetPi(tv_input);
        for (i = 0; i < j; i++) {
          param->tv[i].r = pr[i];
          param->tv[i].i = pi[i];
        }
      }

      /* remap A if necessary */
      if (A.nr == param->mstack[0].nr) {
        param->mstack[0].ia = A.ia;
        param->mstack[0].ja = A.ja;
        param->mstack[0].a = A.a;
        /* mexPrintf("original matrix remapped\n");fflush(stdout); */
      }
    }
  }

  ierr = ZSYMAMGfactor(&A, PRE, param);
  /* mexPrintf("AMGfactor interrupted\n");fflush(stdout); */
  if (tv_exists && tv_field >= 0 &&
      (nrhs == 2 || (nrhs == 4 && mxIsNumeric(prhs[2]))))
    free(mytv);

#ifdef PRINT_INFO
  mexPrintf("ZSYMilupackfactor: matrix factored\n");
  fflush(stdout);
  if (param->rcomflag != 0) {
    mexPrintf("ZSYMilupackfactor: request for reverse communication\n");
    fflush(stdout);
  }
#endif

  if (nlhs == 5 && ierr == 0) {
    plhs[2] = mxCreateDoubleMatrix((mwSize)1, (mwSize)1, mxREAL);
    pr = mxGetPr(plhs[2]);
    *pr = param->rcomflag;

    /* mexPrintf("rcomflag set\n");fflush(stdout); */

    if (param->rcomflag != 0) {
      nnz = param->A.ia[param->A.nr] - 1;
      /* mexPrintf("extract sparse matrix
       * %ld,%ld,%ld\n",param->A.nr,param->A.nr,nnz);fflush(stdout); */
      plhs[3] = mxCreateSparse((mwSize)param->A.nr, (mwSize)param->A.nc, nnz,
                               mxCOMPLEX);
      S_output = plhs[3];

      sr = (double *)mxGetPr(S_output);
      si = (double *)mxGetPi(S_output);
      irs = (mwIndex *)mxGetIr(S_output);
      jcs = (mwIndex *)mxGetJc(S_output);

      k = 0;
      for (i = 0; i < param->A.nr; i++) {
        jcs[i] = k;
        /* strict lower triangular part */
        for (j = param->A.ia[i] - 1; j < param->A.ia[i + 1] - 1; j++) {
          irs[k] = param->A.ja[j] - 1;
          sr[k] = param->A.a[j].r;
          si[k++] = param->A.a[j].i;
        }
      } /* end for i */
      jcs[i] = k;

      /* mexPrintf("extract test vector\n");fflush(stdout);*/
      /* output test vector */
      plhs[4] = mxCreateDoubleMatrix((mwSize)A.nr, (mwSize)1, mxCOMPLEX);
      tv_output = plhs[4];
      pr = mxGetPr(tv_output);
      pi = mxGetPi(tv_output);
      for (i = 0; i < A.nr; i++) {
        *pr++ = param->tv[i].r;
        *pi++ = param->tv[i].i;
      }
    } else {
      plhs[3] = mxCreateDoubleMatrix((mwSize)0, (mwSize)0, mxREAL);
      plhs[4] = mxCreateDoubleMatrix((mwSize)0, (mwSize)0, mxREAL);
    }
  }

  if (param->shiftmatrix != NULL) {
    free(M.ia);
    free(M.ja);
    free(M.a);
  }
  if (param->shifts != NULL) {
    free(param->shifts);
  }

  if (ierr) {
    nlhs = 0;
    ZSYMAMGdelete(&A, PRE, param);
    free(A.ia);
    free(A.ja);
    free(A.a);

    free(PRE);
    free(param);
  }

  switch (ierr) {
  case 0: /* perfect! */
    break;
  case -1: /* Error. input matrix may be wrong.
              (The elimination process has generated a
              row in L or U whose length is .gt.  n.) */
    mexErrMsgTxt("ILUPACK error, data may be wrong.");
    break;
  case -2: /* The matrix L overflows the array alu */
    mexErrMsgTxt("memory overflow, please increase `options.elbow' and retry");
    break;
  case -3: /* The matrix U overflows the array alu */
    mexErrMsgTxt("memory overflow, please increase `options.elbow' and retry");
    break;
  case -4: /* Illegal value for lfil */
    mexErrMsgTxt("Illegal value for `options.lfil'\n");
    break;
  case -5: /* zero row encountered */
    mexErrMsgTxt("zero row encountered, please reduce `options.droptol'\n");
    break;
  case -6: /* zero column encountered */
    mexErrMsgTxt("zero column encountered, please reduce `options.droptol'\n");
    break;
  case -7: /* buffers too small */
    mexErrMsgTxt("memory overflow, please increase `options.elbow' and retry");
    break;
  default: /* zero pivot encountered at step number ierr */
    mexErrMsgTxt("zero pivot encountered, please reduce `options.droptol'\n");
    break;
  } /* end switch */
  if (ierr) {
    plhs[0] = NULL;
    return;
  }

  /* prepare a struct matrices for output */
  nfields = mxGetNumberOfFields(options_input);

  /* allocate memory  for storing pointers */
  fnames = mxCalloc((size_t)nfields, (size_t)sizeof(*fnames));
  /* Get field name pointers */
  for (ifield = 0; ifield < nfields; ifield++) {
    fnames[ifield] = mxGetFieldNameByNumber(options_input, ifield);
  }

  plhs[1] = mxCreateStructMatrix((mwSize)1, (mwSize)1, nfields, fnames);
  if (plhs[1] == NULL)
    mexErrMsgTxt("Could not create structure mxArray");
  options_output = plhs[1];

#ifdef PRINT_INFO
  mexPrintf("ZSYMilupackfactor: export data\n");
  fflush(stdout);
#endif
  /* export data */
  for (ifield = 0; ifield < nfields; ifield++) {
    tmp = mxGetFieldByNumber(options_input, 0, ifield);
    classIDflags[ifield] = mxGetClassID(tmp);

    ndim = mxGetNumberOfDimensions(tmp);
    dims = mxGetDimensions(tmp);

    /* Create string/numeric array */
    if (classIDflags[ifield] == mxCHAR_CLASS) {
      if (!strcmp("amg", fnames[ifield])) {
        output_buf = (char *)mxCalloc((size_t)strlen(param->amg) + 1,
                                      (size_t)sizeof(char));
        strcpy(output_buf, param->amg);
        fout = mxCreateString(output_buf);
      } else if (!strcmp("presmoother", fnames[ifield])) {
        output_buf = (char *)mxCalloc((size_t)strlen(param->presmoother) + 1,
                                      (size_t)sizeof(char));
        strcpy(output_buf, param->presmoother);
        fout = mxCreateString(output_buf);
      } else if (!strcmp("postsmoother", fnames[ifield])) {
        output_buf = (char *)mxCalloc((size_t)strlen(param->postsmoother) + 1,
                                      (size_t)sizeof(char));
        strcpy(output_buf, param->postsmoother);
        fout = mxCreateString(output_buf);
      } else if (!strcmp("typecoarse", fnames[ifield])) {
        output_buf = (char *)mxCalloc((size_t)strlen(param->typecoarse) + 1,
                                      (size_t)sizeof(char));
        strcpy(output_buf, param->typecoarse);
        fout = mxCreateString(output_buf);
      } else if (!strcmp("typetv", fnames[ifield])) {
        output_buf = (char *)mxCalloc((size_t)strlen(param->typetv) + 1,
                                      (size_t)sizeof(char));
        strcpy(output_buf, param->typetv);
        fout = mxCreateString(output_buf);
      } else if (!strcmp("FCpart", fnames[ifield])) {
        output_buf = (char *)mxCalloc((size_t)strlen(param->FCpart) + 1,
                                      (size_t)sizeof(char));
        strcpy(output_buf, param->FCpart);
        fout = mxCreateString(output_buf);
      } else if (!strcmp("solver", fnames[ifield])) {
        output_buf = (char *)mxCalloc((size_t)strlen(param->solver) + 1,
                                      (size_t)sizeof(char));
        strcpy(output_buf, param->solver);
        fout = mxCreateString(output_buf);
      } else if (!strcmp("ordering", fnames[ifield])) {
        output_buf = (char *)mxCalloc((size_t)strlen(param->ordering) + 1,
                                      (size_t)sizeof(char));
        strcpy(output_buf, param->ordering);
        fout = mxCreateString(output_buf);
      } else {
        /* Get the length of the input string. */
        buflen = (mxGetM(tmp) * mxGetN(tmp)) + 1;

        /* Allocate memory for input and output strings. */
        input_buf = mxCalloc((size_t)buflen, (size_t)sizeof(char));
        output_buf = mxCalloc((size_t)buflen, (size_t)sizeof(char));

        /* Copy the string data from tmp into a C string
           input_buf. */
        status = mxGetString(tmp, input_buf, buflen);

        sizebuf = (size_t)buflen * sizeof(char);
        memcpy(output_buf, input_buf, sizebuf);
        fout = mxCreateString(output_buf);
      }
    } else if (!strcmp("shiftmatrix", fnames[ifield])) {
#ifdef PRINT_INFO
      mexPrintf("ZSYMilupackfactor: return shift matrix\n");
      fflush(stdout);
#endif
      mrows = mxGetM(tmp);
      ncols = mxGetN(tmp);
      nnz = mxGetNzmax(tmp);

      A_ja = (mwIndex *)mxGetIr(tmp);
      A_ia = (mwIndex *)mxGetJc(tmp);
      A_valuesR = (double *)mxGetPr(tmp);
      A_valuesI = (double *)mxGetPi(tmp);

      if (A_valuesI == NULL) {
        fout =
            mxCreateSparse((mwSize)mrows, (mwSize)ncols, (mwSize)nnz, mxREAL);
        si = NULL;
      } else {
        fout = mxCreateSparse((mwSize)mrows, (mwSize)ncols, (mwSize)nnz,
                              mxCOMPLEX);
        si = (double *)mxGetPi(fout);
      }
      irs = (mwIndex *)mxGetIr(fout);
      jcs = (mwIndex *)mxGetJc(fout);
      sr = (double *)mxGetPr(fout);

      for (i = 0; i <= ncols; i++)
        jcs[i] = A_ia[i];
      for (i = 0; i < nnz; i++)
        irs[i] = A_ja[i];
      for (i = 0; i < nnz; i++)
        sr[i] = A_valuesR[i];
      if (A_valuesI != NULL)
        for (i = 0; i < nnz; i++)
          si[i] = A_valuesI[i];
#ifdef PRINT_INFO
      mexPrintf("ZSYMilupackfactor: shift matrix returned\n");
      fflush(stdout);
#endif
    } else {
      /* real case */
      if (mxGetPi(tmp) == NULL && strcmp("damping", fnames[ifield]))
        fout = mxCreateNumericArray(ndim, dims, classIDflags[ifield], mxREAL);
      else { /* complex case */
        fout =
            mxCreateNumericArray(ndim, dims, classIDflags[ifield], mxCOMPLEX);
      }
      pdata = mxGetData(fout);

      sizebuf = mxGetElementSize(tmp);
      if (!strcmp("elbow", fnames[ifield])) {
        dbuf = param->elbow;
        memcpy(pdata, &dbuf, sizebuf);
      } else if (!strcmp("lfilS", fnames[ifield])) {
        dbuf = param->lfilS;
        memcpy(pdata, &dbuf, sizebuf);
      } else if (!strcmp("lfil", fnames[ifield])) {
        dbuf = param->lfil;
        memcpy(pdata, &dbuf, sizebuf);
      } else if (!strcmp("maxit", fnames[ifield])) {
        dbuf = param->maxit;
        memcpy(pdata, &dbuf, sizebuf);
      } else if (!strcmp("droptolS", fnames[ifield])) {
        dbuf = param->droptolS;
        memcpy(pdata, &dbuf, sizebuf);
      } else if (!strcmp("droptolc", fnames[ifield])) {
        dbuf = param->droptolc;
        memcpy(pdata, &dbuf, sizebuf);
      } else if (!strcmp("droptol", fnames[ifield])) {
        dbuf = param->droptol;
        memcpy(pdata, &dbuf, sizebuf);
      } else if (!strcmp("condest", fnames[ifield])) {
        dbuf = param->condest;
        memcpy(pdata, &dbuf, sizebuf);
      } else if (!strcmp("restol", fnames[ifield])) {
        dbuf = param->restol;
        memcpy(pdata, &dbuf, sizebuf);
      } else if (!strcmp("npresmoothing", fnames[ifield])) {
        dbuf = param->npresmoothing;
        memcpy(pdata, &dbuf, sizebuf);
      } else if (!strcmp("npostmoothing", fnames[ifield])) {
        dbuf = param->npostsmoothing;
        memcpy(pdata, &dbuf, sizebuf);
      } else if (!strcmp("ncoarse", fnames[ifield])) {
        dbuf = param->ncoarse;
        memcpy(pdata, &dbuf, sizebuf);
      } else if (!strcmp("matching", fnames[ifield])) {
        dbuf = param->matching;
        memcpy(pdata, &dbuf, sizebuf);
      } else if (!strcmp("nrestart", fnames[ifield])) {
        dbuf = param->nrestart;
        memcpy(pdata, &dbuf, sizebuf);
      } else if (!strcmp("damping", fnames[ifield])) {
        pr = mxGetPr(fout);
        pi = mxGetPi(fout);
        *pr = param->damping;
        *pi = 0.0;
      } else if (!strcmp("contraction", fnames[ifield])) {
        dbuf = param->contraction;
        memcpy(pdata, &dbuf, (size_t)sizebuf);
      } else if (!strcmp("mixedprecision", fnames[ifield])) {
        dbuf = param->mixedprecision;
        memcpy(pdata, &dbuf, (size_t)sizebuf);
      } else if (!strcmp("shift0", fnames[ifield])) {
#ifdef PRINT_INFO
        mexPrintf("ZSYMilupackfactor: return shift0\n");
        fflush(stdout);
#endif
        pr = mxGetPr(tmp);
        pi = mxGetPi(tmp);
        sr = mxGetPr(fout);
        si = mxGetPi(fout);
        *sr = *pr;
        if (pi != NULL)
          *si = *pi;
#ifdef PRINT_INFO
        mexPrintf("ZSYMilupackfactor: shift0 returned\n");
        fflush(stdout);
#endif
      } else if (!strcmp("shiftmax", fnames[ifield])) {
        pr = mxGetPr(tmp);
        pi = mxGetPi(tmp);
        sr = mxGetPr(fout);
        si = mxGetPi(fout);
        *sr = *pr;
        if (pi != NULL)
          *si = *pi;
      } else if (!strcmp("shifts", fnames[ifield])) {
        mrows = mxGetM(tmp);
        ncols = mxGetN(tmp);
        pr = mxGetPr(tmp);
        sr = mxGetPr(fout);
        pi = mxGetPi(tmp);
        si = mxGetPi(fout);
        for (i = 0; i < mrows * ncols; i++)
          sr[i] = pr[i];
        if (pi != NULL)
          for (i = 0; i < mrows * ncols; i++)
            si[i] = pi[i];
      } else {
        memcpy(pdata, mxGetData(tmp), sizebuf);
      }
    }

    /* Set each field in output structure */
    mxSetFieldByNumber(options_output, 0, ifield, fout);
  }
  mxFree(fnames);
  mxFree(classIDflags);

  /* create preconditioner for output */
  plhs[0] = mxCreateStructMatrix((mwSize)1, (mwSize)PRE->nlev, 25, pnames);
  if (plhs[0] == NULL)
    mexErrMsgTxt("Could not create structure mxArray\n");
  PRE_output = plhs[0];

  current = PRE;
  if (PRE->issingle) {
    SPRE = (CAMGlevelmat *)PRE;
    scurrent = SPRE;
  }
  n = A.nr;
  /* ibuff=(integer*)MAlloc((size_t)n*sizeof(integer),"ZSYMilupackfactor:ibuff");
   */
  istack = (integer *)MAlloc((size_t)n * sizeof(integer),
                             "ZSYMilupackfactor:istack");
  convert =
      (double *)MAlloc((size_t)n * sizeof(double), "ZSYMilupackfactor:convert");
  for (jstruct = 0; jstruct < PRE->nlev; jstruct++) {

    /*  1. save level size to field `n' */
    ifield = 0;
    fout = mxCreateDoubleMatrix(1, 1, mxREAL);
    pr = mxGetPr(fout);

    if (PRE->issingle)
      *pr = scurrent->n;
    else
      *pr = current->n;

    /* set each field in output structure */
    mxSetFieldByNumber(PRE_output, jstruct, ifield, fout);

    /*  2. field `nB' */
    ++ifield;
    fout = mxCreateDoubleMatrix((mwSize)1, (mwSize)1, mxREAL);
    pr = mxGetPr(fout);

    if (PRE->issingle)
      *pr = scurrent->nB;
    else
      *pr = current->nB;

    /* set each field in output structure */
    mxSetFieldByNumber(PRE_output, jstruct, ifield, fout);

    /*  3. field `L' */
    ++ifield;
    if (param->rcomflag == 0) {
      /* switched to full-matrix processing */
      if (jstruct == PRE->nlev - 1 &&
          ((PRE->issingle) ? scurrent->LU.ja : current->LU.ja) == NULL) {

        if (PRE->issingle)
          fout = mxCreateDoubleMatrix((mwSize)scurrent->nB,
                                      (mwSize)scurrent->nB, mxCOMPLEX);
        else
          fout = mxCreateDoubleMatrix((mwSize)current->nB, (mwSize)current->nB,
                                      mxCOMPLEX);

        /* complete data using the return values from LAPACK's
           Bunch-Kaufman pivoting */
        pr = mxGetPr(fout);
        pi = mxGetPi(fout);
        for (i = 0; i < ((PRE->issingle) ? scurrent->nB * scurrent->nB
                                         : current->nB * current->nB);
             i++) {
          pr[i] = pi[i] = 0.0;
        }

        i = 0;
        j = 0;
        k = 0;
        while (i < ((PRE->issingle) ? scurrent->nB : current->nB)) {
          if (((PRE->issingle) ? scurrent->LU.ia[i] : current->LU.ia[i]) > 0) {
            j += i;
            pr[j] = 1.0;
            pi[j++] = 0.0;
            l = ((PRE->issingle) ? scurrent->nB : current->nB) - i - 1;
            k++;
            while (l--) {
              pr[j] =
                  ((PRE->issingle) ? scurrent->LU.a[k].r : current->LU.a[k].r);
              pi[j++] = ((PRE->issingle) ? scurrent->LU.a[k++].i
                                         : current->LU.a[k++].i);
            }
            i++;
          } else {
            j += i;
            pr[j] = 1.0;
            pi[j++] = 0.0;
            pr[j] = 0.0;
            pi[j++] = 0.0;
            l = ((PRE->issingle) ? scurrent->nB : current->nB) - i - 2;
            k += 2;
            while (l--) {
              pr[j] =
                  ((PRE->issingle) ? scurrent->LU.a[k].r : current->LU.a[k].r);
              pi[j++] = ((PRE->issingle) ? scurrent->LU.a[k++].i
                                         : current->LU.a[k++].i);
            }
            j += i + 1;
            pr[j] = 1.0;
            pi[j++] = 0.0;
            l = ((PRE->issingle) ? scurrent->nB : current->nB) - i - 2;
            k++;
            while (l--) {
              pr[j] =
                  ((PRE->issingle) ? scurrent->LU.a[k].r : current->LU.a[k].r);
              pi[j++] = ((PRE->issingle) ? scurrent->LU.a[k++].i
                                         : current->LU.a[k++].i);
            }
            i += 2;
          }
        }
        /* reorder L-factor */
        i = 0;
        while (i < ((PRE->issingle) ? scurrent->nB : current->nB)) {
          k = ((PRE->issingle) ? scurrent->LU.ia[i] : current->LU.ia[i]);
          /* 1x1 pivot */
          if (k > 0) {
            /* rows and columns i+1 and k+1 were interchanged */
            if (k != i + 1) {
              if (PRE->issingle) {
                for (j = 0; j < i; j++) {
                  dbuf = pr[j * scurrent->nB + i];
                  pr[j * scurrent->nB + i] = pr[j * scurrent->nB + k - 1];
                  pr[j * scurrent->nB + k - 1] = dbuf;
                  dbuf = pi[j * scurrent->nB + i];
                  pi[j * scurrent->nB + i] = pi[j * scurrent->nB + k - 1];
                  pi[j * scurrent->nB + k - 1] = dbuf;
                }
              } else {
                for (j = 0; j < i; j++) {
                  dbuf = pr[j * current->nB + i];
                  pr[j * current->nB + i] = pr[j * current->nB + k - 1];
                  pr[j * current->nB + k - 1] = dbuf;
                  dbuf = pi[j * current->nB + i];
                  pi[j * current->nB + i] = pi[j * current->nB + k - 1];
                  pi[j * current->nB + k - 1] = dbuf;
                }
              }
            }
            i++;
          } else {
            if (PRE->issingle) {
              for (j = 0; j < i; j++) {
                dbuf = pr[j * scurrent->nB + i + 1];
                pr[j * scurrent->nB + i + 1] = pr[j * scurrent->nB - k - 1];
                pr[j * scurrent->nB - k - 1] = dbuf;
                dbuf = pi[j * scurrent->nB + i + 1];
                pi[j * scurrent->nB + i + 1] = pi[j * scurrent->nB - k - 1];
                pi[j * scurrent->nB - k - 1] = dbuf;
              }
            } else {
              for (j = 0; j < i; j++) {
                dbuf = pr[j * current->nB + i + 1];
                pr[j * current->nB + i + 1] = pr[j * current->nB - k - 1];
                pr[j * current->nB - k - 1] = dbuf;
                dbuf = pi[j * current->nB + i + 1];
                pi[j * current->nB + i + 1] = pi[j * current->nB - k - 1];
                pi[j * current->nB - k - 1] = dbuf;
              }
            }
            i += 2;
          } /* end else */
        }   /* end while */

        /* set each field in output structure */
        mxSetFieldByNumber(PRE_output, jstruct, ifield, fout);
      } else { /* sparse case */
        if (PRE->isblock) {
          if (PRE->issingle)
            nnz = scurrent->LU.nnz;
          else
            nnz = current->LU.nnz;
          if (param->flags & COARSE_REDUCE) {
            if (PRE->issingle)
              fout = mxCreateSparse((mwSize)scurrent->nB, (mwSize)scurrent->nB,
                                    (mwSize)nnz, mxCOMPLEX);
            else
              fout = mxCreateSparse((mwSize)current->nB, (mwSize)current->nB,
                                    (mwSize)nnz, mxCOMPLEX);
          } else {
            if (PRE->issingle)
              fout = mxCreateSparse((mwSize)scurrent->n, (mwSize)scurrent->nB,
                                    (mwSize)nnz - scurrent->nB, mxCOMPLEX);
            else
              fout = mxCreateSparse((mwSize)current->n, (mwSize)current->nB,
                                    (mwSize)nnz - current->nB, mxCOMPLEX);
          }
          /* mexPrintf("nnz=%d\n",nnz-current->nB); */

          sr = (double *)mxGetPr(fout);
          si = (double *)mxGetPi(fout);
          irs = (mwIndex *)mxGetIr(fout);
          jcs = (mwIndex *)mxGetJc(fout);

          if (PRE->issingle) {
            nnzU = scurrent->LU.ja[scurrent->nB];
            scurrent->LU.ja[scurrent->nB] = scurrent->LU.nnz + 1;
          } else {
            nnzU = current->LU.ja[current->nB];
            current->LU.ja[current->nB] = current->LU.nnz + 1;
          }

          m = (PRE->issingle) ? SPRE->maxblocksize : PRE->maxblocksize;
          Dbuff = (doublecomplex *)MAlloc(
              (size_t)2 * m * m * sizeof(doublecomplex), "ZSYMilupackfactor");
          Ibuff = (integer *)MAlloc((size_t)m * sizeof(integer),
                                    "ZSYMilupackfactor");

          m = 0;
          /* mexPrintf("nB=%d\n",((PRE->issingle)?scurrent->nB:current->nB)); */
          for (i = 0; i < ((PRE->issingle) ? scurrent->nB : current->nB);) {
            blocksize = ((PRE->issingle) ? scurrent->blocksize[i]
                                         : current->blocksize[i]);
            ii = ((PRE->issingle) ? scurrent->LU.ja[i] - 1
                                  : current->LU.ja[i] - 1);
            /* copy block */
            for (k = 0; k < blocksize; k++) {
              for (l = 0; l < blocksize; l++) {
                Dbuff[l + k * blocksize].r =
                    ((PRE->issingle) ? scurrent->LU.a[ii].r
                                     : current->LU.a[ii].r);
                Dbuff[l + k * blocksize].i =
                    ((PRE->issingle) ? scurrent->LU.a[ii].i
                                     : current->LU.a[ii].i);
                ii++;
              }
            }
            /* factorize diagonal block */
            kk = blocksize * blocksize;
            zsytrf("u", &blocksize, Dbuff, &blocksize, Ibuff, Dbuff + kk, &kk,
                   &ll, 1);
            if (ll < 0) {
              mexPrintf("_SYTRF: argument %d has an illegal value", -ll);
              return;
            }
            if (ll > 0) {
              mexPrintf("_SYTRF: matrix is singular");
              return;
            }
            /* invert diagonal block */
            kk = blocksize * blocksize;
            zsytri("u", &blocksize, Dbuff, &blocksize, Ibuff, Dbuff + kk, &ll,
                   1);
            if (ll < 0) {
              mexPrintf("_SYTRI: argument %d has an illegal value", -ll);
              return;
            }
            /* complete upper triangular part */
            for (k = 0; k < blocksize; k++) {
              for (l = k + 1; l < blocksize; l++) {
                Dbuff[l + k * blocksize].r = Dbuff[k + l * blocksize].r;
                Dbuff[l + k * blocksize].i = Dbuff[k + l * blocksize].i;
              }
            }
            /*
            for (k=0; k<blocksize; k++) {
                for (l=0; l<blocksize; l++) {
                  mexPrintf("%16.9le",Dbuff[l+k*blocksize]);
                }
                mexPrintf("\n");
            }
            */

            /* transfer column i+l */
            for (l = 0; l < blocksize; l++) {
              jcs[i + l] = m;
              /* mexPrintf("jcs[%d]=%d\n",i+l,m);fflush(stdout); */
              /* transfer column i+l of the diagonal block */
              for (k = 0; k < blocksize; k++) {
                /* row i+k */
                irs[m] = i + k;
                sr[m] = Dbuff[l + k * blocksize].r;
                si[m++] = Dbuff[l + k * blocksize].i;
                /* mexPrintf("%d,%d,%8.1le\n",irs[m-1],i+l+1,sr[m-1]); */
              }
              k = ((PRE->issingle) ? scurrent->LU.ja[i] - 1
                                   : current->LU.ja[i] - 1);
              ii = k + l + blocksize * blocksize;
              k += blocksize;
              /* mexPrintf("%d,%d\n",k,(PRE->issingle)?
                       scurrent->LU.ja[i+1]-1:
                       current->LU.ja[i+1]-1); */
              for (; k < ((PRE->issingle) ? scurrent->LU.ja[i + 1] - 1
                                          : current->LU.ja[i + 1] - 1);
                   k++) {
                /* row i+k */
                irs[m] = ((PRE->issingle) ? scurrent->LU.ja[k] - 1
                                          : current->LU.ja[k] - 1);
                sr[m] = ((PRE->issingle) ? scurrent->LU.a[ii].r
                                         : current->LU.a[ii].r);
                si[m++] = ((PRE->issingle) ? scurrent->LU.a[ii].i
                                           : current->LU.a[ii].i);
                /* mexPrintf("%d,%d,%d,%8.1le\n",k,irs[m-1],i+l+1,sr[m-1]); */
                ii += blocksize;
              }
            }

            jcs[i + blocksize] = m;
            /* mexPrintf("jcs[%d]=%d\n",i+blocksize,m);fflush(stdout); */
            i += blocksize;
          } /* end for i */
          free(Dbuff);
          free(Ibuff);

          if (PRE->issingle)
            scurrent->LU.ja[scurrent->nB] = nnzU;
          else
            current->LU.ja[current->nB] = nnzU;

        }      /* end if PRE->isblock */
        else { /* scalar version */
          if (PRE->issingle)
            nnz = scurrent->LU.nnz + 1 - scurrent->LU.ja[0] + 3 * scurrent->nB -
                  2;
          else
            nnz = current->LU.nnz + 1 - current->LU.ja[0] + 3 * current->nB - 2;

          if (param->flags & COARSE_REDUCE) {
            if (PRE->issingle)
              fout = mxCreateSparse((mwSize)scurrent->nB, (mwSize)scurrent->nB,
                                    nnz, mxCOMPLEX);
            else
              fout = mxCreateSparse((mwSize)current->nB, (mwSize)current->nB,
                                    nnz, mxCOMPLEX);
          } else {
            if (PRE->issingle)
              fout = mxCreateSparse((mwSize)scurrent->n, (mwSize)scurrent->nB,
                                    nnz, mxCOMPLEX);
            else
              fout = mxCreateSparse((mwSize)current->n, (mwSize)current->nB,
                                    nnz, mxCOMPLEX);
          }

          sr = (double *)mxGetPr(fout);
          si = (double *)mxGetPi(fout);
          irs = (mwIndex *)mxGetIr(fout);
          jcs = (mwIndex *)mxGetJc(fout);

          if (PRE->issingle) {
            nnzU = scurrent->LU.ja[scurrent->nB];
            scurrent->LU.ja[scurrent->nB] = scurrent->LU.nnz + 1;
          } else {
            nnzU = current->LU.ja[current->nB];
            current->LU.ja[current->nB] = current->LU.nnz + 1;
          }
          k = 0;
          if (PRE->issingle) {
            for (i = 0; i < scurrent->nB;) {
              /* extract (block) diagonal entry */
              if (scurrent->LU.ja[scurrent->nB + 1 + i] == 0) {
                /* column i */
                jcs[i] = k;

                /* diagonal entry (inverted) */
                irs[k] = i;
                det = 1.0 / (scurrent->LU.a[i].r * scurrent->LU.a[i].r +
                             scurrent->LU.a[i].i * scurrent->LU.a[i].i);
                /* 1/z = conjug(z)/|z|^2 */
                sr[k] = det * scurrent->LU.a[i].r;
                si[k++] = -det * scurrent->LU.a[i].i;

                j = scurrent->LU.ja[i] - 1;
                jj = scurrent->LU.ja[i + 1] - scurrent->LU.ja[i];
                Cqsort(scurrent->LU.a + j, scurrent->LU.ja + j, istack, &jj);

                /* strict lower triangular part */
                for (j = scurrent->LU.ja[i] - 1; j < scurrent->LU.ja[i + 1] - 1;
                     j++) {
                  irs[k] = scurrent->LU.ja[j] - 1;
                  sr[k] = scurrent->LU.a[j].r;
                  si[k++] = scurrent->LU.a[j].i;
                }
                i++;
              } else {
                /* determinant of the 2x2  diagonal block */
                detr = scurrent->LU.a[i].r * scurrent->LU.a[i + 1].r -
                       scurrent->LU.a[i].i * scurrent->LU.a[i + 1].i -
                       scurrent->LU.a[scurrent->nB + 1 + i].r *
                           scurrent->LU.a[scurrent->nB + 1 + i].r +
                       scurrent->LU.a[scurrent->nB + 1 + i].i *
                           scurrent->LU.a[scurrent->nB + 1 + i].i;
                deti = scurrent->LU.a[i].r * scurrent->LU.a[i + 1].i +
                       scurrent->LU.a[i].i * scurrent->LU.a[i + 1].r -
                       scurrent->LU.a[scurrent->nB + 1 + i].r *
                           scurrent->LU.a[scurrent->nB + 1 + i].i -
                       scurrent->LU.a[scurrent->nB + 1 + i].i *
                           scurrent->LU.a[scurrent->nB + 1 + i].r;
                /* 1/determinant = conjug(determinant)/|determinant|^2 */
                det = 1.0 / (detr * detr + deti * deti);

                /* column i */
                jcs[i] = k;

                /* diagonal entry (inverted) */
                /*      -1         1
                   [a b]    = -----------  [ c -b]
                   [b c]      determinant  [-b  a]
                */
                irs[k] = i;
                sr[k] = det * (detr * scurrent->LU.a[i + 1].r +
                               deti * scurrent->LU.a[i + 1].i);
                si[k++] = det * (detr * scurrent->LU.a[i + 1].i -
                                 deti * scurrent->LU.a[i + 1].r);

                /* sub-diagonal entry */
                irs[k] = i + 1;
                sr[k] = -det * (detr * scurrent->LU.a[scurrent->nB + i + 1].r +
                                deti * scurrent->LU.a[scurrent->nB + i + 1].i);
                si[k++] =
                    -det * (detr * scurrent->LU.a[scurrent->nB + i + 1].i -
                            deti * scurrent->LU.a[scurrent->nB + i + 1].r);

                j = scurrent->LU.ja[i] - 1;
                jj = scurrent->LU.ja[i + 1] - scurrent->LU.ja[i];
                Cqsort2(scurrent->LU.a + j, scurrent->LU.ja + j, istack, &jj);

                /* strict lower triangular part */
                l = scurrent->LU.ja[i] - 1;
                for (j = l; j < scurrent->LU.ja[i + 1] - 1; j++) {
                  irs[k] = scurrent->LU.ja[j] - 1;
                  sr[k] = scurrent->LU.a[l].r;
                  si[k++] = scurrent->LU.a[l].i;
                  l += 2;
                }

                /* column i+1 */
                jcs[i + 1] = k;

                /* super-diagonal entry */
                irs[k] = i;
                sr[k] = -det * (detr * scurrent->LU.a[scurrent->nB + i + 1].r +
                                deti * scurrent->LU.a[scurrent->nB + i + 1].i);
                si[k++] =
                    -det * (detr * scurrent->LU.a[scurrent->nB + i + 1].i -
                            deti * scurrent->LU.a[scurrent->nB + i + 1].r);

                /* diagonal entry */
                irs[k] = i + 1;
                sr[k] = det * (detr * scurrent->LU.a[i].r +
                               deti * scurrent->LU.a[i].i);
                si[k++] = det * (detr * scurrent->LU.a[i].i -
                                 deti * scurrent->LU.a[i].r);

                l = scurrent->LU.ja[i] - 1;
                /* strict lower triangular part */
                for (j = l; j < scurrent->LU.ja[i + 1] - 1; j++) {
                  irs[k] = scurrent->LU.ja[j] - 1;
                  sr[k] = scurrent->LU.a[l + 1].r;
                  si[k++] = scurrent->LU.a[l + 1].i;
                  l += 2;
                }

                i += 2;
              }
            } /* end for i */
            scurrent->LU.ja[scurrent->nB] = nnzU;
          } else { /* if-else PRE->issingle */
            for (i = 0; i < current->nB;) {
              /* extract (block) diagonal entry */
              if (current->LU.ja[current->nB + 1 + i] == 0) {
                /* column i */
                jcs[i] = k;

                /* diagonal entry (inverted) */
                irs[k] = i;
                det = 1.0 / (current->LU.a[i].r * current->LU.a[i].r +
                             current->LU.a[i].i * current->LU.a[i].i);
                /* 1/z = conjug(z)/|z|^2 */
                sr[k] = det * current->LU.a[i].r;
                si[k++] = -det * current->LU.a[i].i;

                j = current->LU.ja[i] - 1;
                jj = current->LU.ja[i + 1] - current->LU.ja[i];
                Zqsort(current->LU.a + j, current->LU.ja + j, istack, &jj);

                /* strict lower triangular part */
                for (j = current->LU.ja[i] - 1; j < current->LU.ja[i + 1] - 1;
                     j++) {
                  irs[k] = current->LU.ja[j] - 1;
                  sr[k] = current->LU.a[j].r;
                  si[k++] = current->LU.a[j].i;
                }
                i++;
              } else {
                /* determinant of the 2x2  diagonal block */
                detr = current->LU.a[i].r * current->LU.a[i + 1].r -
                       current->LU.a[i].i * current->LU.a[i + 1].i -
                       current->LU.a[current->nB + 1 + i].r *
                           current->LU.a[current->nB + 1 + i].r +
                       current->LU.a[current->nB + 1 + i].i *
                           current->LU.a[current->nB + 1 + i].i;
                deti = current->LU.a[i].r * current->LU.a[i + 1].i +
                       current->LU.a[i].i * current->LU.a[i + 1].r -
                       current->LU.a[current->nB + 1 + i].r *
                           current->LU.a[current->nB + 1 + i].i -
                       current->LU.a[current->nB + 1 + i].i *
                           current->LU.a[current->nB + 1 + i].r;
                /* 1/determinant = conjug(determinant)/|determinant|^2 */
                det = 1.0 / (detr * detr + deti * deti);

                /* column i */
                jcs[i] = k;

                /* diagonal entry (inverted) */
                /*      -1         1
                   [a b]    = -----------  [ c -b]
                   [b c]      determinant  [-b  a]
                */
                irs[k] = i;
                sr[k] = det * (detr * current->LU.a[i + 1].r +
                               deti * current->LU.a[i + 1].i);
                si[k++] = det * (detr * current->LU.a[i + 1].i -
                                 deti * current->LU.a[i + 1].r);

                /* sub-diagonal entry */
                irs[k] = i + 1;
                sr[k] = -det * (detr * current->LU.a[current->nB + i + 1].r +
                                deti * current->LU.a[current->nB + i + 1].i);
                si[k++] = -det * (detr * current->LU.a[current->nB + i + 1].i -
                                  deti * current->LU.a[current->nB + i + 1].r);

                j = current->LU.ja[i] - 1;
                jj = current->LU.ja[i + 1] - current->LU.ja[i];
                Zqsort2(current->LU.a + j, current->LU.ja + j, istack, &jj);

                /* strict lower triangular part */
                l = current->LU.ja[i] - 1;
                for (j = l; j < current->LU.ja[i + 1] - 1; j++) {
                  irs[k] = current->LU.ja[j] - 1;
                  sr[k] = current->LU.a[l].r;
                  si[k++] = current->LU.a[l].i;
                  l += 2;
                }

                /* column i+1 */
                jcs[i + 1] = k;

                /* super-diagonal entry */
                irs[k] = i;
                sr[k] = -det * (detr * current->LU.a[current->nB + i + 1].r +
                                deti * current->LU.a[current->nB + i + 1].i);
                si[k++] = -det * (detr * current->LU.a[current->nB + i + 1].i -
                                  deti * current->LU.a[current->nB + i + 1].r);

                /* diagonal entry */
                irs[k] = i + 1;
                sr[k] = det *
                        (detr * current->LU.a[i].r + deti * current->LU.a[i].i);
                si[k++] = det * (detr * current->LU.a[i].i -
                                 deti * current->LU.a[i].r);

                l = current->LU.ja[i] - 1;
                /* strict lower triangular part */
                for (j = l; j < current->LU.ja[i + 1] - 1; j++) {
                  irs[k] = current->LU.ja[j] - 1;
                  sr[k] = current->LU.a[l + 1].r;
                  si[k++] = current->LU.a[l + 1].i;
                  l += 2;
                }

                i += 2;
              }
            } /* end for i */
            current->LU.ja[current->nB] = nnzU;
          } /* end if-else PRE->issingle */
          jcs[i] = k;
        }

        /* set each field in output structure */
        mxSetFieldByNumber(PRE_output, jstruct, ifield, fout);
      }
    } else {
      fout = mxCreateDoubleMatrix((mwSize)0, (mwSize)0, mxREAL);
      /* set each field in output structure */
      mxSetFieldByNumber(PRE_output, jstruct, ifield, fout);
    }

    /* mexPrintf("4. field `D'\n");fflush(stdout); */
    /*  4. field `D' */
    ++ifield;
    if (param->rcomflag == 0) {
      if (PRE->issingle) {
        if (scurrent->isblock)
          fout = mxCreateSparse((mwSize)scurrent->nB, (mwSize)scurrent->nB,
                                (mwSize)SPRE->maxblocksize * scurrent->nB,
                                mxCOMPLEX);
        else
          fout = mxCreateSparse((mwSize)scurrent->nB, (mwSize)scurrent->nB,
                                (mwSize)2 * scurrent->nB, mxCOMPLEX);
      } else {
        if (current->isblock) {
          fout = mxCreateSparse((mwSize)current->nB, (mwSize)current->nB,
                                (mwSize)PRE->maxblocksize * current->nB,
                                mxCOMPLEX);
          /* mexPrintf("nnz=%d\n",PRE->maxblocksize*current->nB); */
        } else
          fout = mxCreateSparse((mwSize)current->nB, (mwSize)current->nB,
                                (mwSize)2 * current->nB, mxCOMPLEX);
      }

      sr = (double *)mxGetPr(fout);
      si = (double *)mxGetPi(fout);
      irs = (mwIndex *)mxGetIr(fout);
      jcs = (mwIndex *)mxGetJc(fout);

      /* switched to full-matrix processing */
      if (jstruct == PRE->nlev - 1 &&
          ((PRE->issingle) ? scurrent->LU.ja : current->LU.ja) == NULL) {
        /* complete data using the return values from LAPACK's
           Bunch-Kaufman pivoting */
        k = 0;
        j = 0;
        for (i = 0; i < ((PRE->issingle) ? scurrent->nB : current->nB);) {
          /* extract (block) diagonal entry */
          if (((PRE->issingle) ? scurrent->LU.ia[i] : current->LU.ia[i]) > 0) {
            /* column i */
            jcs[i] = k;

            /* diagonal entry */
            irs[k] = i;
            sr[k] =
                ((PRE->issingle) ? scurrent->LU.a[j].r : current->LU.a[j].r);
            si[k++] =
                ((PRE->issingle) ? scurrent->LU.a[j].i : current->LU.a[j].i);

            j += ((PRE->issingle) ? scurrent->nB : current->nB) - i;
            i++;
          } else {

            /* column i */
            jcs[i] = k;

            /* diagonal entry */
            irs[k] = i;
            sr[k] =
                ((PRE->issingle) ? scurrent->LU.a[j].r : current->LU.a[j].r);
            si[k++] =
                ((PRE->issingle) ? scurrent->LU.a[j].i : current->LU.a[j].i);

            /* sub-diagonal entry */
            irs[k] = i + 1;
            sr[k] = ((PRE->issingle) ? scurrent->LU.a[j + 1].r
                                     : current->LU.a[j + 1].r);
            si[k++] = ((PRE->issingle) ? scurrent->LU.a[j + 1].i
                                       : current->LU.a[j + 1].i);

            /* column i+1 */
            jcs[i + 1] = k;

            /* super-diagonal entry */
            irs[k] = i;
            sr[k] = ((PRE->issingle) ? scurrent->LU.a[j + 1].r
                                     : current->LU.a[j + 1].r);
            si[k++] = ((PRE->issingle) ? scurrent->LU.a[j + 1].i
                                       : current->LU.a[j + 1].i);

            /* diagonal entry */
            irs[k] = i + 1;
            if (PRE->issingle) {
              sr[k] = scurrent->LU.a[j + scurrent->nB - i].r;
              si[k++] = scurrent->LU.a[j + scurrent->nB - i].i;
              j += 2 * (scurrent->nB - i) - 1;
            } else {
              sr[k] = current->LU.a[j + current->nB - i].r;
              si[k++] = current->LU.a[j + current->nB - i].i;
              j += 2 * (current->nB - i) - 1;
            }
            i += 2;
          }
        }
        jcs[i] = k;
      } else { /* sparse case */
        if (PRE->isblock) {
          m = ((PRE->issingle) ? SPRE->maxblocksize : PRE->maxblocksize);
          Dbuff = (doublecomplex *)MAlloc(
              (size_t)2 * m * m * sizeof(doublecomplex), "ZSYMilupackfactor");
          Ibuff = (integer *)MAlloc((size_t)m * sizeof(integer),
                                    "ZSYMilupackfactor");

          m = 0;
          /* mexPrintf("nB=%d\n",current->nB); */
          for (i = 0; i < ((PRE->issingle) ? scurrent->nB : current->nB);) {
            blocksize = ((PRE->issingle) ? scurrent->blocksize[i]
                                         : current->blocksize[i]);
            ii = ((PRE->issingle) ? scurrent->LU.ja[i] - 1
                                  : current->LU.ja[i] - 1);
            /* copy block */
            for (k = 0; k < blocksize; k++) {
              for (l = 0; l < blocksize; l++) {
                Dbuff[l + k * blocksize].r =
                    ((PRE->issingle) ? scurrent->LU.a[ii].r
                                     : current->LU.a[ii].r);
                Dbuff[l + k * blocksize].i =
                    ((PRE->issingle) ? scurrent->LU.a[ii].i
                                     : current->LU.a[ii].i);
                ii++;
              }
            }
            /* factorize diagonal block */
            kk = blocksize * blocksize;
            zsytrf("u", &blocksize, Dbuff, &blocksize, Ibuff, Dbuff + kk, &kk,
                   &ll, 1);
            if (ll < 0) {
              mexPrintf("_SYTRF: argument %d has an illegal value", -ll);
              return;
            }
            if (ll > 0) {
              mexPrintf("_SYTRF: matrix is singular");
              return;
            }
            /* invert diagonal block */
            kk = blocksize * blocksize;
            zsytri("u", &blocksize, Dbuff, &blocksize, Ibuff, Dbuff + kk, &ll,
                   1);
            if (ll < 0) {
              mexPrintf("_SYTRI: argument %d has an illegal value", -ll);
              return;
            }
            for (k = 0; k < blocksize; k++) {
              for (l = k + 1; l < blocksize; l++) {
                Dbuff[l + k * blocksize].r = Dbuff[k + l * blocksize].r;
                Dbuff[l + k * blocksize].i = Dbuff[k + l * blocksize].i;
              }
            }
            for (k = 0; k < blocksize; k++) {
              /* column i+k */
              jcs[i + k] = m;
              /* mexPrintf("jcs[%d]=%d\n",i+k,m); */
              for (l = 0; l < blocksize; l++) {
                /* row i+l */
                irs[m] = i + l;
                sr[m] = Dbuff[l + k * blocksize].r;
                si[m++] = Dbuff[l + k * blocksize].i;
                /* mexPrintf("%d,%d,%8.1le\n",i+k,irs[m-1],sr[m-1]); */
              }
            }
            jcs[i + k] = m;
            /* mexPrintf("jcs[%d]=%d\n",i+k,m); */
            i += blocksize;
          } /* end for i */
          free(Dbuff);
          free(Ibuff);
        } else { /* non-block version */
          k = 0;
          if (PRE->issingle) {
            for (i = 0; i < scurrent->nB;) {
              /* extract (block) diagonal entry */
              if (scurrent->LU.ja[scurrent->nB + 1 + i] == 0) {
                /* column i */
                jcs[i] = k;

                /* diagonal entry */
                irs[k] = i;
                det = 1.0 / (scurrent->LU.a[i].r * scurrent->LU.a[i].r +
                             scurrent->LU.a[i].i * scurrent->LU.a[i].i);
                sr[k] = det * scurrent->LU.a[i].r;
                si[k++] = -det * scurrent->LU.a[i].i;

                i++;
              } else {
                detr = scurrent->LU.a[i].r * scurrent->LU.a[i + 1].r -
                       scurrent->LU.a[i].i * scurrent->LU.a[i + 1].i -
                       scurrent->LU.a[scurrent->nB + 1 + i].r *
                           scurrent->LU.a[scurrent->nB + 1 + i].r +
                       scurrent->LU.a[scurrent->nB + 1 + i].i *
                           scurrent->LU.a[scurrent->nB + 1 + i].i;
                deti = scurrent->LU.a[i].r * scurrent->LU.a[i + 1].i +
                       scurrent->LU.a[i].i * scurrent->LU.a[i + 1].r -
                       scurrent->LU.a[scurrent->nB + 1 + i].r *
                           scurrent->LU.a[scurrent->nB + 1 + i].i -
                       scurrent->LU.a[scurrent->nB + 1 + i].i *
                           scurrent->LU.a[scurrent->nB + 1 + i].r;
                det = 1.0 / (detr * detr + deti * deti);

                /* column i */
                jcs[i] = k;

                /* diagonal entry */
                irs[k] = i;
                sr[k] = det * (detr * scurrent->LU.a[i + 1].r +
                               deti * scurrent->LU.a[i + 1].i);
                si[k++] = det * (detr * scurrent->LU.a[i + 1].i -
                                 deti * scurrent->LU.a[i + 1].r);

                /* sub-diagonal entry */
                irs[k] = i + 1;
                sr[k] = -det * (detr * scurrent->LU.a[scurrent->nB + i + 1].r +
                                deti * scurrent->LU.a[scurrent->nB + i + 1].i);
                si[k++] =
                    -det * (detr * scurrent->LU.a[scurrent->nB + i + 1].i -
                            deti * scurrent->LU.a[scurrent->nB + i + 1].r);

                /* column i+1 */
                jcs[i + 1] = k;

                /* super-diagonal entry */
                irs[k] = i;
                sr[k] = -det * (detr * scurrent->LU.a[scurrent->nB + i + 1].r +
                                deti * scurrent->LU.a[scurrent->nB + i + 1].i);
                si[k++] =
                    -det * (detr * scurrent->LU.a[scurrent->nB + i + 1].i -
                            deti * scurrent->LU.a[scurrent->nB + i + 1].r);

                /* diagonal entry */
                irs[k] = i + 1;
                sr[k] = det * (detr * scurrent->LU.a[i].r +
                               deti * scurrent->LU.a[i].i);
                si[k++] = det * (detr * scurrent->LU.a[i].i -
                                 deti * scurrent->LU.a[i].r);

                i += 2;
              }
            }
          } else { /* if-else PRE->issingle */
            for (i = 0; i < current->nB;) {
              /* extract (block) diagonal entry */
              if (current->LU.ja[current->nB + 1 + i] == 0) {
                /* column i */
                jcs[i] = k;

                /* diagonal entry */
                irs[k] = i;
                det = 1.0 / (current->LU.a[i].r * current->LU.a[i].r +
                             current->LU.a[i].i * current->LU.a[i].i);
                sr[k] = det * current->LU.a[i].r;
                si[k++] = -det * current->LU.a[i].i;

                i++;
              } else {
                detr = current->LU.a[i].r * current->LU.a[i + 1].r -
                       current->LU.a[i].i * current->LU.a[i + 1].i -
                       current->LU.a[current->nB + 1 + i].r *
                           current->LU.a[current->nB + 1 + i].r +
                       current->LU.a[current->nB + 1 + i].i *
                           current->LU.a[current->nB + 1 + i].i;
                deti = current->LU.a[i].r * current->LU.a[i + 1].i +
                       current->LU.a[i].i * current->LU.a[i + 1].r -
                       current->LU.a[current->nB + 1 + i].r *
                           current->LU.a[current->nB + 1 + i].i -
                       current->LU.a[current->nB + 1 + i].i *
                           current->LU.a[current->nB + 1 + i].r;
                det = 1.0 / (detr * detr + deti * deti);

                /* column i */
                jcs[i] = k;

                /* diagonal entry */
                irs[k] = i;
                sr[k] = det * (detr * current->LU.a[i + 1].r +
                               deti * current->LU.a[i + 1].i);
                si[k++] = det * (detr * current->LU.a[i + 1].i -
                                 deti * current->LU.a[i + 1].r);

                /* sub-diagonal entry */
                irs[k] = i + 1;
                sr[k] = -det * (detr * current->LU.a[current->nB + i + 1].r +
                                deti * current->LU.a[current->nB + i + 1].i);
                si[k++] = -det * (detr * current->LU.a[current->nB + i + 1].i -
                                  deti * current->LU.a[current->nB + i + 1].r);

                /* column i+1 */
                jcs[i + 1] = k;

                /* super-diagonal entry */
                irs[k] = i;
                sr[k] = -det * (detr * current->LU.a[current->nB + i + 1].r +
                                deti * current->LU.a[current->nB + i + 1].i);
                si[k++] = -det * (detr * current->LU.a[current->nB + i + 1].i -
                                  deti * current->LU.a[current->nB + i + 1].r);

                /* diagonal entry */
                irs[k] = i + 1;
                sr[k] = det *
                        (detr * current->LU.a[i].r + deti * current->LU.a[i].i);
                si[k++] = det * (detr * current->LU.a[i].i -
                                 deti * current->LU.a[i].r);

                i += 2;
              } /* end else */
            }   /* end for i */
          }     /* end if-else PRE->issingle */
          jcs[i] = k;
        } /* end if-else PRE->isblock */
      }   /* end if-else */
      /* set each field in output structure */
      mxSetFieldByNumber(PRE_output, jstruct, ifield, fout);
    } else {
      fout = mxCreateDoubleMatrix((mwSize)0, (mwSize)0, mxREAL);
      /* set each field in output structure */
      mxSetFieldByNumber(PRE_output, jstruct, ifield, fout);
    }

    /*  5. field `U', U=L^T */
    ++ifield;
    fout = mxCreateDoubleMatrix((mwSize)0, (mwSize)0, mxREAL);
    /* set each field in output structure */
    mxSetFieldByNumber(PRE_output, jstruct, ifield, fout);

    /*  6. field `E' */
    ++ifield;
    if (param->rcomflag == 0) {
      if (jstruct < PRE->nlev - 1) {

        if (param->flags & COARSE_REDUCE) {
          if (PRE->issingle) {
            nnz = scurrent->F.ia[scurrent->nB] - 1;
            fout = mxCreateSparse((mwSize)n - scurrent->nB,
                                  (mwSize)scurrent->nB, nnz, mxCOMPLEX);
          } else {
            nnz = current->F.ia[current->nB] - 1;
            fout = mxCreateSparse((mwSize)n - current->nB, (mwSize)current->nB,
                                  nnz, mxCOMPLEX);
          }

          sr = (double *)mxGetPr(fout);
          si = (double *)mxGetPi(fout);
          irs = (mwIndex *)mxGetIr(fout);
          jcs = (mwIndex *)mxGetJc(fout);

          k = 0;
          if (PRE->issingle) {
            for (i = 0; i < scurrent->nB; i++) {
              jcs[i] = k;

              j = scurrent->F.ia[i] - 1;
              jj = scurrent->F.ia[i + 1] - scurrent->F.ia[i];
              Cqsort(scurrent->F.a + j, scurrent->F.ja + j, istack, &jj);

              for (j = scurrent->F.ia[i] - 1; j < scurrent->F.ia[i + 1] - 1;
                   j++) {
                irs[k] = scurrent->F.ja[j] - 1;
                sr[k] = scurrent->F.a[j].r;
                si[k++] = scurrent->F.a[j].i;
              }
            }
          } else {
            for (i = 0; i < current->nB; i++) {
              jcs[i] = k;

              j = current->F.ia[i] - 1;
              jj = current->F.ia[i + 1] - current->F.ia[i];
              Zqsort(current->F.a + j, current->F.ja + j, istack, &jj);

              for (j = current->F.ia[i] - 1; j < current->F.ia[i + 1] - 1;
                   j++) {
                irs[k] = current->F.ja[j] - 1;
                sr[k] = current->F.a[j].r;
                si[k++] = current->F.a[j].i;
              }
            }
          }
          jcs[i] = k;
        } else {
          nnz = 0;
          if (PRE->issingle)
            fout = mxCreateSparse((mwSize)n - scurrent->nB,
                                  (mwSize)scurrent->nB, nnz, mxCOMPLEX);
          else
            fout = mxCreateSparse((mwSize)n - current->nB, (mwSize)current->nB,
                                  nnz, mxCOMPLEX);
        }
        /* set each field in output structure */
        mxSetFieldByNumber(PRE_output, jstruct, ifield, fout);
      }
    } else {
      fout = mxCreateDoubleMatrix((mwSize)0, (mwSize)0, mxREAL);
      /* set each field in output structure */
      mxSetFieldByNumber(PRE_output, jstruct, ifield, fout);
    }

    /*  7. field `F' */
    ++ifield;
    fout = mxCreateDoubleMatrix((mwSize)0, (mwSize)0, mxREAL);
    /* set each field in output structure */
    mxSetFieldByNumber(PRE_output, jstruct, ifield, fout);

    /*  8. field `rowscal' */
    ++ifield;
    fout = mxCreateDoubleMatrix((mwSize)1, (mwSize)n, mxCOMPLEX);
    sr = mxGetPr(fout);
    si = mxGetPi(fout);

    if (param->rcomflag == 0) {
      if (PRE->issingle) {
        for (i = 0; i < n; i++) {
          sr[i] = scurrent->rowscal[i].r;
          si[i] = scurrent->rowscal[i].i;
        }
      } else {
        for (i = 0; i < n; i++) {
          sr[i] = current->rowscal[i].r;
          si[i] = current->rowscal[i].i;
        }
      }
    }
    /* set each field in output structure */
    mxSetFieldByNumber(PRE_output, jstruct, ifield, fout);

    /*  9. field `colscal' */
    ++ifield;
    fout = mxCreateDoubleMatrix((mwSize)1, (mwSize)n, mxCOMPLEX);
    sr = mxGetPr(fout);
    si = mxGetPi(fout);

    if (param->rcomflag == 0) {
      if (PRE->issingle) {
        for (i = 0; i < n; i++) {
          sr[i] = scurrent->colscal[i].r;
          si[i] = scurrent->colscal[i].i;
        }
      } else {
        for (i = 0; i < n; i++) {
          sr[i] = current->colscal[i].r;
          si[i] = current->colscal[i].i;
        }
      }
    }

    /* set each field in output structure */
    mxSetFieldByNumber(PRE_output, jstruct, ifield, fout);

    /* 10. field `p' */
    ++ifield;
    if (param->rcomflag == 0) {
      fout = mxCreateDoubleMatrix((mwSize)1, (mwSize)n, mxREAL);
      pdata = mxGetData(fout);

      if (PRE->issingle) {
        for (i = 0; i < n; i++)
          convert[i] = scurrent->p[i];
      } else {
        for (i = 0; i < n; i++)
          convert[i] = current->p[i];
      }

      /* switched to full-matrix processing, adapt permutation */
      if (jstruct == PRE->nlev - 1 &&
          ((PRE->issingle) ? scurrent->LU.ja : current->LU.ja) == NULL &&
          ((PRE->issingle) ? (void *)scurrent->LU.a : (void *)current->LU.a) !=
              NULL) {

        /* complete data using the return values from LAPACK's
           Bunch-Kaufman pivoting */
        i = 0;
        while (i < n) {
          /* 2x2 pivot */
          j = ((PRE->issingle) ? scurrent->LU.ia[i] : current->LU.ia[i]);
          if (j < 0) {
            k = convert[i + 1];
            convert[i + 1] = convert[-j - 1];
            convert[-j - 1] = k;
            i += 2;
          } else { /* 1x1 pivot */
            if (j != i + 1) {
              k = convert[i];
              convert[i] = convert[j - 1];
              convert[j - 1] = k;
            }
            i++;
          }
        }
      }

      memcpy(pdata, convert, (size_t)n * sizeof(double));

      /* set each field in output structure */
      mxSetFieldByNumber(PRE_output, jstruct, ifield, fout);
    } else {
      fout = mxCreateDoubleMatrix((mwSize)0, (mwSize)0, mxREAL);
      /* set each field in output structure */
      mxSetFieldByNumber(PRE_output, jstruct, ifield, fout);
    }

    /* 11. field `invq' */
    ++ifield;
    if (param->rcomflag == 0) {
      fout = mxCreateDoubleMatrix((mwSize)1, (mwSize)n, mxREAL);
      pr = mxGetPr(fout);

      for (i = 0; i < n; i++) {
        k = convert[i];
        pr[k - 1] = i + 1;
      }
      /* memcpy(pdata, convert, (size_t)n*sizeof(double)); */

      /* set each field in output structure */
      mxSetFieldByNumber(PRE_output, jstruct, ifield, fout);
    } else {
      fout = mxCreateDoubleMatrix(0, 0, mxREAL);
      /* set each field in output structure */
      mxSetFieldByNumber(PRE_output, jstruct, ifield, fout);
    }

    /* 12. field `param' */
    ++ifield;
    fout = mxCreateNumericArray((mwSize)1, mydims, mxUINT64_CLASS, mxREAL);
    pdata = mxGetData(fout);

    memcpy(pdata, &param, (size_t)sizeof(size_t));

    /* set each field in output structure */
    mxSetFieldByNumber(PRE_output, jstruct, ifield, fout);

    /* 13. field `ptr' */
    ++ifield;
    fout = mxCreateNumericArray((mwSize)1, mydims, mxUINT64_CLASS, mxREAL);
    pdata = mxGetData(fout);

    memcpy(pdata, &PRE, (size_t)sizeof(size_t));

    /* set each field in output structure */
    mxSetFieldByNumber(PRE_output, jstruct, ifield, fout);

    /* 14. save non-real property to field `isreal' */
    ++ifield;
    fout = mxCreateDoubleMatrix((mwSize)1, (mwSize)1, mxREAL);
    pr = mxGetPr(fout);

    *pr = 0;

    /* set each field in output structure */
    mxSetFieldByNumber(PRE_output, jstruct, ifield, fout);

    /* 15. save non-positive definite property to field `isdefinite' */
    ++ifield;
    fout = mxCreateDoubleMatrix((mwSize)1, (mwSize)1, mxREAL);
    pr = mxGetPr(fout);

    *pr = 0;

    /* set each field in output structure */
    mxSetFieldByNumber(PRE_output, jstruct, ifield, fout);

    /* 16. save symmetry property to field `issymmetric' */
    ++ifield;
    fout = mxCreateDoubleMatrix((mwSize)1, (mwSize)1, mxREAL);
    pr = mxGetPr(fout);

    *pr = 1;

    /* set each field in output structure */
    mxSetFieldByNumber(PRE_output, jstruct, ifield, fout);

    /* 17. save non-Hermitian property to field `ishermitian' */
    ++ifield;
    fout = mxCreateDoubleMatrix((mwSize)1, (mwSize)1, mxREAL);
    pr = mxGetPr(fout);

    *pr = 0;

    /* set each field in output structure */
    mxSetFieldByNumber(PRE_output, jstruct, ifield, fout);

    /* 18. save single precision property to field `issingle' */
    ++ifield;
    fout = mxCreateDoubleMatrix((mwSize)1, (mwSize)1, mxREAL);
    pr = mxGetPr(fout);

    if (PRE->issingle)
      *pr = scurrent->issingle;
    else
      *pr = current->issingle;

    /* set each field in output structure */
    mxSetFieldByNumber(PRE_output, (mwIndex)jstruct, ifield, fout);

    /* 19. save coarse grid system `A_H' */
    ++ifield;
    if (jstruct >= PRE->nlev - 1) {
      fout = mxCreateSparse((mwSize)0, (mwSize)0, (mwSize)0, mxCOMPLEX);
    } else if (param->ipar[16] & DISCARD_MATRIX) {
      if (PRE->issingle)
        fout = mxCreateSparse((mwSize)n - scurrent->nB,
                              (mwSize)n - scurrent->nB, (mwSize)0, mxCOMPLEX);
      else
        fout = mxCreateSparse((mwSize)n - current->nB, (mwSize)n - current->nB,
                              (mwSize)0, mxCOMPLEX);
    } else {
      /* switched to full-matrix processing */
      if (jstruct == PRE->nlev - 2 &&
          ((PRE->issingle) ? scurrent->next->LU.ja : current->next->LU.ja) ==
              NULL)
        fout = mxCreateSparse((mwSize)0, (mwSize)0, (mwSize)0, mxCOMPLEX);
      else {
        if (PRE->issingle)
          nnz = scurrent->next->A.ia[scurrent->next->A.nr] - 1;
        else
          nnz = current->next->A.ia[current->next->A.nr] - 1;

        /*
        mexPrintf("level %d,
        %d,%d,%d\n",jstruct+1,current->next->A.nr,current->next->A.nc,nnz);fflush(stdout);
        fout=mxCreateSparse((mwSize)0,(mwSize)0,(mwSize)0, mxCOMPLEX);
        */

        if (PRE->issingle)
          fout = mxCreateSparse((mwSize)scurrent->next->A.nr,
                                (mwSize)scurrent->next->A.nc, nnz, mxCOMPLEX);
        else
          fout = mxCreateSparse((mwSize)current->next->A.nr,
                                (mwSize)current->next->A.nc, nnz, mxCOMPLEX);

        sr = (double *)mxGetPr(fout);
        si = (double *)mxGetPi(fout);
        irs = (mwIndex *)mxGetIr(fout);
        jcs = (mwIndex *)mxGetJc(fout);

        k = 0;
        if (PRE->issingle) {
          for (i = 0; i < scurrent->next->A.nr; i++) {
            jcs[i] = k;

            j = scurrent->next->A.ia[i] - 1;
            jj = scurrent->next->A.ia[i + 1] - scurrent->next->A.ia[i];
            Cqsort(scurrent->next->A.a + j, scurrent->next->A.ja + j, istack,
                   &jj);

            for (j = scurrent->next->A.ia[i] - 1;
                 j < scurrent->next->A.ia[i + 1] - 1; j++) {
              irs[k] = scurrent->next->A.ja[j] - 1;
              sr[k] = scurrent->next->A.a[j].r;
              si[k++] = scurrent->next->A.a[j].i;
            }
          }
        } else {
          for (i = 0; i < current->next->A.nr; i++) {
            jcs[i] = k;

            j = current->next->A.ia[i] - 1;
            jj = current->next->A.ia[i + 1] - current->next->A.ia[i];
            Zqsort(current->next->A.a + j, current->next->A.ja + j, istack,
                   &jj);

            for (j = current->next->A.ia[i] - 1;
                 j < current->next->A.ia[i + 1] - 1; j++) {
              irs[k] = current->next->A.ja[j] - 1;
              sr[k] = current->next->A.a[j].r;
              si[k++] = current->next->A.a[j].i;
            }
          }
        }
        jcs[i] = k;
      }
    }
    /* set each field in output structure */
    mxSetFieldByNumber(PRE_output, jstruct, ifield, fout);

    /* 20. save error in L to field `errorL' */
    ++ifield;
    fout = mxCreateDoubleMatrix((mwSize)1, (mwSize)1, mxREAL);
    pr = mxGetPr(fout);
    if (PRE->issingle)
      *pr = scurrent->errorL;
    else
      *pr = current->errorL;
    /* set each field in output structure */
    mxSetFieldByNumber(PRE_output, (mwIndex)jstruct, ifield, fout);

    /* 21. save error in U to field `errorU' */
    ++ifield;
    fout = mxCreateDoubleMatrix((mwSize)1, (mwSize)1, mxREAL);
    pr = mxGetPr(fout);
    if (PRE->issingle)
      *pr = scurrent->errorU;
    else
      *pr = current->errorU;
    /* set each field in output structure */
    mxSetFieldByNumber(PRE_output, (mwIndex)jstruct, ifield, fout);

    /* 22. save error in S to field `errorS' */
    ++ifield;
    fout = mxCreateDoubleMatrix((mwSize)1, (mwSize)1, mxREAL);
    pr = mxGetPr(fout);
    if (PRE->issingle)
      *pr = scurrent->errorS;
    else
      *pr = current->errorS;
    /* set each field in output structure */
    mxSetFieldByNumber(PRE_output, (mwIndex)jstruct, ifield, fout);

    /* mexPrintf("23. field `isblock'\n");fflush(stdout); */
    /* 23. save trivial block property to field `isblock' */
    ++ifield;
    fout = mxCreateDoubleMatrix((mwSize)1, (mwSize)1, mxREAL);
    pr = mxGetPr(fout);

    if (PRE->issingle)
      *pr = scurrent->isblock;
    else
      *pr = current->isblock;

    /* set each field in output structure */
    mxSetFieldByNumber(PRE_output, (mwIndex)jstruct, ifield, fout);

    /* mexPrintf("24. field `indB'\n");fflush(stdout); */
    /* 24. field `indB' */
    ++ifield;
    /* 1D cell array */
    if (PRE->issingle)
      i = scurrent->cnt;
    else
      i = current->cnt;
    fout = mxCreateCellMatrix((mwSize)1, (mwSize)i);

    /* extract sequence of indices */
    /* mexPrintf("24. field `indB': extract indices\n");fflush(stdout);  */
    for (j = 0; j < i; j++) {
      if (PRE->issingle)
        k = scurrent->cntB[j];
      else
        k = current->cntB[j];
      /* mexPrintf("24. field `indB': local elimination %d, size %d\n",
       * j,k);fflush(stdout); */

      /* create a local vector of length k */
      tmp = mxCreateDoubleMatrix((mwSize)k, (mwSize)1, mxREAL);
      /* copy index data */
      pr = mxGetPr(tmp);
      if (PRE->issingle)
        for (l = 0; l < k; l++)
          *pr++ = scurrent->indB[j][l] + 1;
      else {
        for (l = 0; l < k; l++) {
          *pr++ = current->indB[j][l] + 1;
          /* mexPrintf("%4d", current->indB[j][l]+1); */
        }
        /* mexPrintf("\n"); fflush(stdout); */
      }

      /* mexPrintf("24. field `indB': transfer vector into cell array component
       * %d\n", j);fflush(stdout);  */
      /* transfer vector into the current cell array component */
      mxSetCell(fout, (mwIndex)j, mxDuplicateArray(tmp));

    } /* end for j */

    /* set each field in output structure */
    mxSetFieldByNumber(PRE_output, (mwIndex)jstruct, ifield, fout);

    /* mexPrintf("25. field `LB'\n");fflush(stdout); */
    /* 25. field `LB' */
    ++ifield;
    /* 1D cell array */
    if (PRE->issingle)
      i = scurrent->cnt;
    else
      i = current->cnt;
    fout = mxCreateCellMatrix((mwSize)1, (mwSize)i);

    /* create sequence of local elimination matrices */
    for (j = 0; j < i; j++) {
      if (PRE->issingle)
        k = scurrent->cntB[j];
      else
        k = current->cntB[j];
      /* create a local vector of length k */
      tmp = mxCreateDoubleMatrix((mwSize)k, (mwSize)k, mxCOMPLEX);

      /* init with zeros */
      pr = mxGetPr(tmp);
      pi = mxGetPi(tmp);
      for (l = 0; l < k * k; l++)
        *pr++ = *pi++ = 0;
      /* diagonal part is set to one */
      pr = mxGetPr(tmp);
      for (l = 0; l < k; l++) {
        *pr = 1.0;
        pr += k + 1;
      } /* end for l */

      /* strict lower triangular part */
      pr = mxGetPr(tmp);
      pi = mxGetPi(tmp);
      pr++;
      pi++;
      if (PRE->issingle) {
        for (l = 1; l < k; l++) {
          *pr++ = -scurrent->LB[j][l].r;
          *pi++ = -scurrent->LB[j][l].i;
        }
        /* is a second elimination step required */
        if (scurrent->indB2[j] != NULL) {
          pr = mxGetPr(tmp);
          pi = mxGetPi(tmp);
          pr += k + 2;
          pi += k + 2;
          for (l = 2; l < k; l++) {
            *pr++ = -scurrent->LB2[j][l].r;
            *pi++ = -scurrent->LB2[j][l].i;
          }
        } /* end if */
      } else {
        for (l = 1; l < k; l++) {
          *pr++ = -current->LB[j][l].r;
          *pi++ = -current->LB[j][l].i;
        }
        /* is a second elimination step required */
        if (current->indB2[j] != NULL) {
          pr = mxGetPr(tmp);
          pi = mxGetPi(tmp);
          pr += k + 2;
          pi += k + 2;
          for (l = 2; l < k; l++) {
            *pr++ = -current->LB2[j][l].r;
            *pi++ = -current->LB2[j][l].i;
          }
        } /* end if */
      }   /* end if-else */

      /* transfer vector into the current cell array component*/
      mxSetCell(fout, (mwIndex)j, mxDuplicateArray(tmp));

    } /* end for j */

    /* set each field in output structure */
    mxSetFieldByNumber(PRE_output, (mwIndex)jstruct, ifield, fout);

    if (PRE->issingle) {
      n -= scurrent->nB;
      scurrent = scurrent->next;
    } else {
      n -= current->nB;
      current = current->next;
    }
  }
  /* free(ibuff); */

  free(A.ia);
  free(A.ja);
  free(A.a);

  free(istack);
  free(convert);

#ifdef PRINT_INFO
  mexPrintf("ZSYMilupackfactor: memory released\n");
  fflush(stdout);
#endif

  return;
}
