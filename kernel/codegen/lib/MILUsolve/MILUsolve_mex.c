/*
 * MILUsolve_mex.c
 *
 * Auxiliary code for mexFunction of MILUsolve
 *
 * C source code generated by m2c.
 * %#m2c options:94891620e08dacb2a65c4e47079e6ef8
 *
 */

#include "mex.h"
#if !defined(MATLAB_MEX_FILE) && defined(printf)
#undef printf
#endif
/* Include the C header file generated by codegen in lib mode */
#include "MILUsolve.h"
#include "m2c.c"

/* Include declaration of some helper functions. */
#include "lib2mex_helper.c"


static void marshallin_const_struct1_T(struct1_T *pStruct, const mxArray *mx, const char *mname) {
    mxArray             *sub_mx;

    if (!mxIsStruct(mx))
        M2C_error("marshallin_const_struct1_T:WrongType",
            "Input argument %s has incorrect data type; struct is expected.", mname);
    if (!mxGetField(mx, 0, "col_ptr"))
        M2C_error("marshallin_const_struct1_T:WrongType",
            "Input argument %s is missing the field col_ptr.", mname);
    if (!mxGetField(mx, 0, "row_ind"))
        M2C_error("marshallin_const_struct1_T:WrongType",
            "Input argument %s is missing the field row_ind.", mname);
    if (!mxGetField(mx, 0, "val"))
        M2C_error("marshallin_const_struct1_T:WrongType",
            "Input argument %s is missing the field val.", mname);
    if (!mxGetField(mx, 0, "nrows"))
        M2C_error("marshallin_const_struct1_T:WrongType",
            "Input argument %s is missing the field nrows.", mname);
    if (!mxGetField(mx, 0, "ncols"))
        M2C_error("marshallin_const_struct1_T:WrongType",
            "Input argument %s is missing the field ncols.", mname);
    if (mxGetNumberOfFields(mx) > 5)
        M2C_warn("marshallin_const_struct1_T:ExtraFields",
            "Extra fields in %s and are ignored.", mname);

    sub_mx = mxGetField(mx, 0, "col_ptr");
    if (mxGetNumberOfElements(sub_mx) && mxGetClassID(sub_mx) != mxINT32_CLASS)
        mexErrMsgIdAndTxt("marshallin_const_struct1_T:WrongInputType",
            "Input argument col_ptr has incorrect data type; int32 is expected.");
    if (mxGetNumberOfElements(sub_mx) && mxGetDimensions(sub_mx)[1] != 1) 
        mexErrMsgIdAndTxt("marshallin_const_struct1_T:WrongSizeOfInputArg",
            "Dimension 2 of col_ptr should be equal to 1.");
    pStruct->col_ptr = mxMalloc(sizeof(emxArray_int32_T));
    init_emxArray((emxArray__common*)(pStruct->col_ptr), 1);
    alias_mxArray_to_emxArray(sub_mx, (emxArray__common *)(pStruct->col_ptr), "col_ptr", 1);

    sub_mx = mxGetField(mx, 0, "row_ind");
    if (mxGetNumberOfElements(sub_mx) && mxGetClassID(sub_mx) != mxINT32_CLASS)
        mexErrMsgIdAndTxt("marshallin_const_struct1_T:WrongInputType",
            "Input argument row_ind has incorrect data type; int32 is expected.");
    if (mxGetNumberOfElements(sub_mx) && mxGetDimensions(sub_mx)[1] != 1) 
        mexErrMsgIdAndTxt("marshallin_const_struct1_T:WrongSizeOfInputArg",
            "Dimension 2 of row_ind should be equal to 1.");
    pStruct->row_ind = mxMalloc(sizeof(emxArray_int32_T));
    init_emxArray((emxArray__common*)(pStruct->row_ind), 1);
    alias_mxArray_to_emxArray(sub_mx, (emxArray__common *)(pStruct->row_ind), "row_ind", 1);

    sub_mx = mxGetField(mx, 0, "val");
    if (mxGetNumberOfElements(sub_mx) && mxGetClassID(sub_mx) != mxDOUBLE_CLASS)
        mexErrMsgIdAndTxt("marshallin_const_struct1_T:WrongInputType",
            "Input argument val has incorrect data type; double is expected.");
    if (mxGetNumberOfElements(sub_mx) && mxGetDimensions(sub_mx)[1] != 1) 
        mexErrMsgIdAndTxt("marshallin_const_struct1_T:WrongSizeOfInputArg",
            "Dimension 2 of val should be equal to 1.");
    pStruct->val = mxMalloc(sizeof(emxArray_real_T));
    init_emxArray((emxArray__common*)(pStruct->val), 1);
    alias_mxArray_to_emxArray(sub_mx, (emxArray__common *)(pStruct->val), "val", 1);

    sub_mx = mxGetField(mx, 0, "nrows");
    if (mxGetNumberOfElements(sub_mx) && mxGetClassID(sub_mx) != mxINT32_CLASS)
        mexErrMsgIdAndTxt("marshallin_const_struct1_T:WrongInputType",
            "Input argument nrows has incorrect data type; int32 is expected.");
    if (mxGetNumberOfElements(sub_mx) != 1)
        mexErrMsgIdAndTxt("marshallin_const_struct1_T:WrongSizeOfInputArg",
            "Argument nrows should be a scalar.");
    pStruct->nrows = *(int32_T*)mxGetData(sub_mx);

    sub_mx = mxGetField(mx, 0, "ncols");
    if (mxGetNumberOfElements(sub_mx) && mxGetClassID(sub_mx) != mxINT32_CLASS)
        mexErrMsgIdAndTxt("marshallin_const_struct1_T:WrongInputType",
            "Input argument ncols has incorrect data type; int32 is expected.");
    if (mxGetNumberOfElements(sub_mx) != 1)
        mexErrMsgIdAndTxt("marshallin_const_struct1_T:WrongSizeOfInputArg",
            "Argument ncols should be a scalar.");
    pStruct->ncols = *(int32_T*)mxGetData(sub_mx);
}
static void destroy_struct1_T(struct1_T *pStruct) {

    free_emxArray((emxArray__common*)(pStruct->col_ptr));
    mxFree(pStruct->col_ptr);

    free_emxArray((emxArray__common*)(pStruct->row_ind));
    mxFree(pStruct->row_ind);

    free_emxArray((emxArray__common*)(pStruct->val));
    mxFree(pStruct->val);


}


static void marshallin_const_struct2_T(struct2_T *pStruct, const mxArray *mx, const char *mname) {
    mxArray             *sub_mx;

    if (!mxIsStruct(mx))
        M2C_error("marshallin_const_struct2_T:WrongType",
            "Input argument %s has incorrect data type; struct is expected.", mname);
    if (!mxGetField(mx, 0, "row_ptr"))
        M2C_error("marshallin_const_struct2_T:WrongType",
            "Input argument %s is missing the field row_ptr.", mname);
    if (!mxGetField(mx, 0, "col_ind"))
        M2C_error("marshallin_const_struct2_T:WrongType",
            "Input argument %s is missing the field col_ind.", mname);
    if (!mxGetField(mx, 0, "val"))
        M2C_error("marshallin_const_struct2_T:WrongType",
            "Input argument %s is missing the field val.", mname);
    if (!mxGetField(mx, 0, "nrows"))
        M2C_error("marshallin_const_struct2_T:WrongType",
            "Input argument %s is missing the field nrows.", mname);
    if (!mxGetField(mx, 0, "ncols"))
        M2C_error("marshallin_const_struct2_T:WrongType",
            "Input argument %s is missing the field ncols.", mname);
    if (mxGetNumberOfFields(mx) > 5)
        M2C_warn("marshallin_const_struct2_T:ExtraFields",
            "Extra fields in %s and are ignored.", mname);

    sub_mx = mxGetField(mx, 0, "row_ptr");
    if (mxGetNumberOfElements(sub_mx) && mxGetClassID(sub_mx) != mxINT32_CLASS)
        mexErrMsgIdAndTxt("marshallin_const_struct2_T:WrongInputType",
            "Input argument row_ptr has incorrect data type; int32 is expected.");
    if (mxGetNumberOfElements(sub_mx) && mxGetDimensions(sub_mx)[1] != 1) 
        mexErrMsgIdAndTxt("marshallin_const_struct2_T:WrongSizeOfInputArg",
            "Dimension 2 of row_ptr should be equal to 1.");
    pStruct->row_ptr = mxMalloc(sizeof(emxArray_int32_T));
    init_emxArray((emxArray__common*)(pStruct->row_ptr), 1);
    alias_mxArray_to_emxArray(sub_mx, (emxArray__common *)(pStruct->row_ptr), "row_ptr", 1);

    sub_mx = mxGetField(mx, 0, "col_ind");
    if (mxGetNumberOfElements(sub_mx) && mxGetClassID(sub_mx) != mxINT32_CLASS)
        mexErrMsgIdAndTxt("marshallin_const_struct2_T:WrongInputType",
            "Input argument col_ind has incorrect data type; int32 is expected.");
    if (mxGetNumberOfElements(sub_mx) && mxGetDimensions(sub_mx)[1] != 1) 
        mexErrMsgIdAndTxt("marshallin_const_struct2_T:WrongSizeOfInputArg",
            "Dimension 2 of col_ind should be equal to 1.");
    pStruct->col_ind = mxMalloc(sizeof(emxArray_int32_T));
    init_emxArray((emxArray__common*)(pStruct->col_ind), 1);
    alias_mxArray_to_emxArray(sub_mx, (emxArray__common *)(pStruct->col_ind), "col_ind", 1);

    sub_mx = mxGetField(mx, 0, "val");
    if (mxGetNumberOfElements(sub_mx) && mxGetClassID(sub_mx) != mxDOUBLE_CLASS)
        mexErrMsgIdAndTxt("marshallin_const_struct2_T:WrongInputType",
            "Input argument val has incorrect data type; double is expected.");
    if (mxGetNumberOfElements(sub_mx) && mxGetDimensions(sub_mx)[1] != 1) 
        mexErrMsgIdAndTxt("marshallin_const_struct2_T:WrongSizeOfInputArg",
            "Dimension 2 of val should be equal to 1.");
    pStruct->val = mxMalloc(sizeof(emxArray_real_T));
    init_emxArray((emxArray__common*)(pStruct->val), 1);
    alias_mxArray_to_emxArray(sub_mx, (emxArray__common *)(pStruct->val), "val", 1);

    sub_mx = mxGetField(mx, 0, "nrows");
    if (mxGetNumberOfElements(sub_mx) && mxGetClassID(sub_mx) != mxINT32_CLASS)
        mexErrMsgIdAndTxt("marshallin_const_struct2_T:WrongInputType",
            "Input argument nrows has incorrect data type; int32 is expected.");
    if (mxGetNumberOfElements(sub_mx) != 1)
        mexErrMsgIdAndTxt("marshallin_const_struct2_T:WrongSizeOfInputArg",
            "Argument nrows should be a scalar.");
    pStruct->nrows = *(int32_T*)mxGetData(sub_mx);

    sub_mx = mxGetField(mx, 0, "ncols");
    if (mxGetNumberOfElements(sub_mx) && mxGetClassID(sub_mx) != mxINT32_CLASS)
        mexErrMsgIdAndTxt("marshallin_const_struct2_T:WrongInputType",
            "Input argument ncols has incorrect data type; int32 is expected.");
    if (mxGetNumberOfElements(sub_mx) != 1)
        mexErrMsgIdAndTxt("marshallin_const_struct2_T:WrongSizeOfInputArg",
            "Argument ncols should be a scalar.");
    pStruct->ncols = *(int32_T*)mxGetData(sub_mx);
}
static void destroy_struct2_T(struct2_T *pStruct) {

    free_emxArray((emxArray__common*)(pStruct->row_ptr));
    mxFree(pStruct->row_ptr);

    free_emxArray((emxArray__common*)(pStruct->col_ind));
    mxFree(pStruct->col_ind);

    free_emxArray((emxArray__common*)(pStruct->val));
    mxFree(pStruct->val);


}


static void marshallin_const_emxArray_struct0_T(emxArray_struct0_T *pEmx, const mxArray *mx, const char *mname, const int dim) {
    mxArray             *sub_mx;
    int                  i;
    int                  nElems = mxGetNumberOfElements(mx);;

    if (!mxIsStruct(mx))
        M2C_error("marshallin_const_emxArray_struct0_T:WrongType",
            "Input argument %s has incorrect data type; struct is expected.", mname);
    if (!mxGetField(mx, 0, "p"))
        M2C_error("marshallin_const_emxArray_struct0_T:WrongType",
            "Input argument %s is missing the field p.", mname);
    if (!mxGetField(mx, 0, "q"))
        M2C_error("marshallin_const_emxArray_struct0_T:WrongType",
            "Input argument %s is missing the field q.", mname);
    if (!mxGetField(mx, 0, "rowscal"))
        M2C_error("marshallin_const_emxArray_struct0_T:WrongType",
            "Input argument %s is missing the field rowscal.", mname);
    if (!mxGetField(mx, 0, "colscal"))
        M2C_error("marshallin_const_emxArray_struct0_T:WrongType",
            "Input argument %s is missing the field colscal.", mname);
    if (!mxGetField(mx, 0, "L"))
        M2C_error("marshallin_const_emxArray_struct0_T:WrongType",
            "Input argument %s is missing the field L.", mname);
    if (!mxGetField(mx, 0, "U"))
        M2C_error("marshallin_const_emxArray_struct0_T:WrongType",
            "Input argument %s is missing the field U.", mname);
    if (!mxGetField(mx, 0, "d"))
        M2C_error("marshallin_const_emxArray_struct0_T:WrongType",
            "Input argument %s is missing the field d.", mname);
    if (!mxGetField(mx, 0, "negE"))
        M2C_error("marshallin_const_emxArray_struct0_T:WrongType",
            "Input argument %s is missing the field negE.", mname);
    if (!mxGetField(mx, 0, "negF"))
        M2C_error("marshallin_const_emxArray_struct0_T:WrongType",
            "Input argument %s is missing the field negF.", mname);
    if (mxGetNumberOfFields(mx) > 9)
        M2C_warn("marshallin_const_emxArray_struct0_T:ExtraFields",
            "Extra fields in %s and are ignored.", mname);

    alloc_emxArray_from_mxArray(mx, (emxArray__common*)pEmx, mname, dim, sizeof(struct0_T));
    for (i=0; i<nElems; ++i) {
        sub_mx = mxGetField(mx, i, "p");
        if (mxGetNumberOfElements(sub_mx) && mxGetClassID(sub_mx) != mxINT32_CLASS)
            mexErrMsgIdAndTxt("marshallin_const_emxArray_struct0_T:WrongInputType",
                "Input argument p has incorrect data type; int32 is expected.");
        if (mxGetNumberOfElements(sub_mx) && mxGetDimensions(sub_mx)[1] != 1) 
            mexErrMsgIdAndTxt("marshallin_const_emxArray_struct0_T:WrongSizeOfInputArg",
                "Dimension 2 of p should be equal to 1.");
        pEmx->data[i].p = mxMalloc(sizeof(emxArray_int32_T));
        init_emxArray((emxArray__common*)(pEmx->data[i].p), 1);
        alias_mxArray_to_emxArray(sub_mx, (emxArray__common *)(pEmx->data[i].p), "p", 1);

        sub_mx = mxGetField(mx, i, "q");
        if (mxGetNumberOfElements(sub_mx) && mxGetClassID(sub_mx) != mxINT32_CLASS)
            mexErrMsgIdAndTxt("marshallin_const_emxArray_struct0_T:WrongInputType",
                "Input argument q has incorrect data type; int32 is expected.");
        if (mxGetNumberOfElements(sub_mx) && mxGetDimensions(sub_mx)[1] != 1) 
            mexErrMsgIdAndTxt("marshallin_const_emxArray_struct0_T:WrongSizeOfInputArg",
                "Dimension 2 of q should be equal to 1.");
        pEmx->data[i].q = mxMalloc(sizeof(emxArray_int32_T));
        init_emxArray((emxArray__common*)(pEmx->data[i].q), 1);
        alias_mxArray_to_emxArray(sub_mx, (emxArray__common *)(pEmx->data[i].q), "q", 1);

        sub_mx = mxGetField(mx, i, "rowscal");
        if (mxGetNumberOfElements(sub_mx) && mxGetClassID(sub_mx) != mxDOUBLE_CLASS)
            mexErrMsgIdAndTxt("marshallin_const_emxArray_struct0_T:WrongInputType",
                "Input argument rowscal has incorrect data type; double is expected.");
        if (mxGetNumberOfElements(sub_mx) && mxGetDimensions(sub_mx)[1] != 1) 
            mexErrMsgIdAndTxt("marshallin_const_emxArray_struct0_T:WrongSizeOfInputArg",
                "Dimension 2 of rowscal should be equal to 1.");
        pEmx->data[i].rowscal = mxMalloc(sizeof(emxArray_real_T));
        init_emxArray((emxArray__common*)(pEmx->data[i].rowscal), 1);
        alias_mxArray_to_emxArray(sub_mx, (emxArray__common *)(pEmx->data[i].rowscal), "rowscal", 1);

        sub_mx = mxGetField(mx, i, "colscal");
        if (mxGetNumberOfElements(sub_mx) && mxGetClassID(sub_mx) != mxDOUBLE_CLASS)
            mexErrMsgIdAndTxt("marshallin_const_emxArray_struct0_T:WrongInputType",
                "Input argument colscal has incorrect data type; double is expected.");
        if (mxGetNumberOfElements(sub_mx) && mxGetDimensions(sub_mx)[1] != 1) 
            mexErrMsgIdAndTxt("marshallin_const_emxArray_struct0_T:WrongSizeOfInputArg",
                "Dimension 2 of colscal should be equal to 1.");
        pEmx->data[i].colscal = mxMalloc(sizeof(emxArray_real_T));
        init_emxArray((emxArray__common*)(pEmx->data[i].colscal), 1);
        alias_mxArray_to_emxArray(sub_mx, (emxArray__common *)(pEmx->data[i].colscal), "colscal", 1);

        sub_mx = mxGetField(mx, i, "L");
        if (mxGetNumberOfElements(sub_mx) && mxGetClassID(sub_mx) != mxSTRUCT_CLASS)
            mexErrMsgIdAndTxt("marshallin_const_emxArray_struct0_T:WrongInputType",
                "Input argument L has incorrect data type; struct is expected.");
        if (mxGetNumberOfElements(sub_mx) != 1)
            mexErrMsgIdAndTxt("marshallin_const_emxArray_struct0_T:WrongSizeOfInputArg",
                "Argument L should be a scalar.");
        marshallin_const_struct1_T(&pEmx->data[i].L, sub_mx, "L");

        sub_mx = mxGetField(mx, i, "U");
        if (mxGetNumberOfElements(sub_mx) && mxGetClassID(sub_mx) != mxSTRUCT_CLASS)
            mexErrMsgIdAndTxt("marshallin_const_emxArray_struct0_T:WrongInputType",
                "Input argument U has incorrect data type; struct is expected.");
        if (mxGetNumberOfElements(sub_mx) != 1)
            mexErrMsgIdAndTxt("marshallin_const_emxArray_struct0_T:WrongSizeOfInputArg",
                "Argument U should be a scalar.");
        marshallin_const_struct1_T(&pEmx->data[i].U, sub_mx, "U");

        sub_mx = mxGetField(mx, i, "d");
        if (mxGetNumberOfElements(sub_mx) && mxGetClassID(sub_mx) != mxDOUBLE_CLASS)
            mexErrMsgIdAndTxt("marshallin_const_emxArray_struct0_T:WrongInputType",
                "Input argument d has incorrect data type; double is expected.");
        if (mxGetNumberOfElements(sub_mx) && mxGetDimensions(sub_mx)[1] != 1) 
            mexErrMsgIdAndTxt("marshallin_const_emxArray_struct0_T:WrongSizeOfInputArg",
                "Dimension 2 of d should be equal to 1.");
        pEmx->data[i].d = mxMalloc(sizeof(emxArray_real_T));
        init_emxArray((emxArray__common*)(pEmx->data[i].d), 1);
        alias_mxArray_to_emxArray(sub_mx, (emxArray__common *)(pEmx->data[i].d), "d", 1);

        sub_mx = mxGetField(mx, i, "negE");
        if (mxGetNumberOfElements(sub_mx) && mxGetClassID(sub_mx) != mxSTRUCT_CLASS)
            mexErrMsgIdAndTxt("marshallin_const_emxArray_struct0_T:WrongInputType",
                "Input argument negE has incorrect data type; struct is expected.");
        if (mxGetNumberOfElements(sub_mx) != 1)
            mexErrMsgIdAndTxt("marshallin_const_emxArray_struct0_T:WrongSizeOfInputArg",
                "Argument negE should be a scalar.");
        marshallin_const_struct2_T(&pEmx->data[i].negE, sub_mx, "negE");

        sub_mx = mxGetField(mx, i, "negF");
        if (mxGetNumberOfElements(sub_mx) && mxGetClassID(sub_mx) != mxSTRUCT_CLASS)
            mexErrMsgIdAndTxt("marshallin_const_emxArray_struct0_T:WrongInputType",
                "Input argument negF has incorrect data type; struct is expected.");
        if (mxGetNumberOfElements(sub_mx) != 1)
            mexErrMsgIdAndTxt("marshallin_const_emxArray_struct0_T:WrongSizeOfInputArg",
                "Argument negF should be a scalar.");
        marshallin_const_struct2_T(&pEmx->data[i].negF, sub_mx, "negF");
    }
}
static void destroy_emxArray_struct0_T(emxArray_struct0_T *pEmx) {
    int                  nElems = numElements(pEmx->numDimensions, pEmx->size);
    int                  i;

    for (i=0; i<nElems; ++i) {
        free_emxArray((emxArray__common*)(pEmx->data[i].p));
        mxFree(pEmx->data[i].p);

        free_emxArray((emxArray__common*)(pEmx->data[i].q));
        mxFree(pEmx->data[i].q);

        free_emxArray((emxArray__common*)(pEmx->data[i].rowscal));
        mxFree(pEmx->data[i].rowscal);

        free_emxArray((emxArray__common*)(pEmx->data[i].colscal));
        mxFree(pEmx->data[i].colscal);

        destroy_struct1_T(&pEmx->data[i].L);
        destroy_struct1_T(&pEmx->data[i].U);
        free_emxArray((emxArray__common*)(pEmx->data[i].d));
        mxFree(pEmx->data[i].d);

        destroy_struct2_T(&pEmx->data[i].negE);
        destroy_struct2_T(&pEmx->data[i].negF);
    }
    free_emxArray((emxArray__common *)pEmx);
}


static void __MILUsolve_api(mxArray **plhs, const mxArray ** prhs) {
    emxArray_struct0_T   M;
    emxArray_real_T      b;
    emxArray_real_T      b_y1;
    emxArray_real_T      y2;

    /* Marshall in inputs and preallocate outputs */
    if (mxGetNumberOfElements(prhs[0]) && mxGetClassID(prhs[0]) != mxSTRUCT_CLASS)
        mexErrMsgIdAndTxt("MILUsolve:WrongInputType",
            "Input argument M has incorrect data type; struct is expected.");
    if (mxGetNumberOfElements(prhs[0]) && mxGetDimensions(prhs[0])[1] != 1) 
        mexErrMsgIdAndTxt("MILUsolve:WrongSizeOfInputArg",
            "Dimension 2 of M should be equal to 1.");
    marshallin_const_emxArray_struct0_T(&M, prhs[0], "M", 1);

    if (mxGetNumberOfElements(prhs[1]) && mxGetClassID(prhs[1]) != mxDOUBLE_CLASS)
        mexErrMsgIdAndTxt("MILUsolve:WrongInputType",
            "Input argument b has incorrect data type; double is expected.");
    if (mxGetNumberOfElements(prhs[1]) && mxGetDimensions(prhs[1])[1] != 1) 
        mexErrMsgIdAndTxt("MILUsolve:WrongSizeOfInputArg",
            "Dimension 2 of b should be equal to 1.");
    copy_mxArray_to_emxArray(prhs[1], (emxArray__common *)(&b), "b", 1);

    if (mxGetNumberOfElements(prhs[2]) && mxGetClassID(prhs[2]) != mxDOUBLE_CLASS)
        mexErrMsgIdAndTxt("MILUsolve:WrongInputType",
            "Input argument y1 has incorrect data type; double is expected.");
    if (mxGetNumberOfElements(prhs[2]) && mxGetDimensions(prhs[2])[1] != 1) 
        mexErrMsgIdAndTxt("MILUsolve:WrongSizeOfInputArg",
            "Dimension 2 of y1 should be equal to 1.");
    copy_mxArray_to_emxArray(prhs[2], (emxArray__common *)(&b_y1), "y1", 1);

    if (mxGetNumberOfElements(prhs[3]) && mxGetClassID(prhs[3]) != mxDOUBLE_CLASS)
        mexErrMsgIdAndTxt("MILUsolve:WrongInputType",
            "Input argument y2 has incorrect data type; double is expected.");
    if (mxGetNumberOfElements(prhs[3]) && mxGetDimensions(prhs[3])[1] != 1) 
        mexErrMsgIdAndTxt("MILUsolve:WrongSizeOfInputArg",
            "Dimension 2 of y2 should be equal to 1.");
    copy_mxArray_to_emxArray(prhs[3], (emxArray__common *)(&y2), "y2", 1);

    /* Invoke the target function */
    MILUsolve(&M, &b, &b_y1, &y2);

    /* Deallocate input and marshall out function outputs */
    destroy_emxArray_struct0_T(&M);
    plhs[0] = move_emxArray_to_mxArray((emxArray__common*)(&b), mxDOUBLE_CLASS);
    mxFree(b.size);
    plhs[1] = move_emxArray_to_mxArray((emxArray__common*)(&b_y1), mxDOUBLE_CLASS);
    mxFree(b_y1.size);
    plhs[2] = move_emxArray_to_mxArray((emxArray__common*)(&y2), mxDOUBLE_CLASS);
    mxFree(y2.size);

}

static void __MILUsolve_2args_api(mxArray **plhs, const mxArray ** prhs) {
    emxArray_struct0_T   M;
    emxArray_real_T      b;

    /* Marshall in inputs and preallocate outputs */
    if (mxGetNumberOfElements(prhs[0]) && mxGetClassID(prhs[0]) != mxSTRUCT_CLASS)
        mexErrMsgIdAndTxt("MILUsolve_2args:WrongInputType",
            "Input argument M has incorrect data type; struct is expected.");
    if (mxGetNumberOfElements(prhs[0]) && mxGetDimensions(prhs[0])[1] != 1) 
        mexErrMsgIdAndTxt("MILUsolve_2args:WrongSizeOfInputArg",
            "Dimension 2 of M should be equal to 1.");
    marshallin_const_emxArray_struct0_T(&M, prhs[0], "M", 1);

    if (mxGetNumberOfElements(prhs[1]) && mxGetClassID(prhs[1]) != mxDOUBLE_CLASS)
        mexErrMsgIdAndTxt("MILUsolve_2args:WrongInputType",
            "Input argument b has incorrect data type; double is expected.");
    if (mxGetNumberOfElements(prhs[1]) && mxGetDimensions(prhs[1])[1] != 1) 
        mexErrMsgIdAndTxt("MILUsolve_2args:WrongSizeOfInputArg",
            "Dimension 2 of b should be equal to 1.");
    copy_mxArray_to_emxArray(prhs[1], (emxArray__common *)(&b), "b", 1);

    /* Invoke the target function */
    MILUsolve_2args(&M, &b);

    /* Deallocate input and marshall out function outputs */
    destroy_emxArray_struct0_T(&M);
    plhs[0] = move_emxArray_to_mxArray((emxArray__common*)(&b), mxDOUBLE_CLASS);
    mxFree(b.size);

}


void mexFunction(int nlhs, mxArray *plhs[], int nrhs, const mxArray *prhs[]) {
    /* Temporary copy for mex outputs. */
    mxArray *outputs[3] = {NULL, NULL, NULL};
    int i;
    int nOutputs = (nlhs < 1 ? 1 : nlhs);

    if (nrhs == 4) {
        if (nlhs > 3)
            mexErrMsgIdAndTxt("MILUsolve:TooManyOutputArguments",
                "Too many output arguments for entry-point MILUsolve.\n");
        /* Call the API function. */
        __MILUsolve_api(outputs, prhs);
    }
    else if (nrhs == 2) {
        if (nlhs > 1)
            mexErrMsgIdAndTxt("MILUsolve_2args:TooManyOutputArguments",
                "Too many output arguments for entry-point MILUsolve_2args.\n");
        /* Call the API function. */
        __MILUsolve_2args_api(outputs, prhs);
    }
    else
        mexErrMsgIdAndTxt("MILUsolve:WrongNumberOfInputs",
            "Incorrect number of input variables for entry-point MILUsolve.");

    /* Copy over outputs to the caller. */
    for (i = 0; i < nOutputs; ++i) {
        plhs[i] = outputs[i];
    }
}
