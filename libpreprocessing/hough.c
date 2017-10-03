/*
 * hough.c
 *
 *  Created on: 16 may. 2017
 *      Author: zaca
 */


#include "preprocessing/ana.h"

#include "preprocessing/def.h"
#include "preprocessing/mydef.h"
#include "preprocessing/vmem.h"
#include "preprocessing/hough.h"
/* from libeve */
#include "../libeve/eve/fixed_point.h"

/* from std c */
#include <limits.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/**
 * Calculate eight bit image histogram
 *
 * @param val		input image
 * @return hist 	histogram of input image
 */

//ImageValInt histogram (ImageValChar val){
//	valarray<unsigned int> hist(GRAYLEVEL_8);
//
//	int size_val = val.size();
//
//	for(int i = 0; i < size_val; i++){
//		hist[ val[i] ]++;
//	}
//
//	return hist;
//}

int preprocessing_hough_hist(uint32_t sdSrc, uint16_t rows, uint16_t cols,
        unsigned int valor, uint32_t sdDst)
{
    int status = PREPROCESSING_SUCCESSFUL;
    unsigned int size = (unsigned int)(rows) * cols;
    unsigned int p = 0;

    const int32_t* src = preprocessing_vmem_getDataAddress(sdSrc);
    int32_t* dst = preprocessing_vmem_getDataAddress(sdDst);
    int32_t scalar;
    int32_t indice;//usado
    scalar=256;//es un 1 en 24.8 punto fijo

    // Check whether given rows and columns are in a valid range.
    if ((!preprocessing_vmem_isProcessingSizeValid(sdSrc, rows, cols))
            || (!preprocessing_vmem_isProcessingSizeValid(sdDst, 1, valor)))
    {
        return PREPROCESSING_INVALID_SIZE;
    }

    // Process.
    indice=0;
    for (unsigned int r = 0; r < rows; r++)
    {
        for (unsigned int c = 0; c < cols; c++)
        {
            p = r * cols + c;

            // Check for valid pointer position.
            PREPROCESSING_DEF_CHECK_POINTER(src, p, size)
            PREPROCESSING_DEF_CHECK_POINTER(dst, indice, valor)

			indice=src[p]>>8; //24.8 to int32
            dst[indice] = eve_fp_add32(dst[indice], scalar);//Hist count



            if (dst[p] == EVE_FP32_NAN)
            {
                status = PREPROCESSING_INVALID_NUMBER;
            }
        }
    }

    return status;
}
int preprocessing_arith_abs(uint32_t sdSrc, uint16_t rows, uint16_t cols, uint32_t sdDst)
{
    int status = PREPROCESSING_SUCCESSFUL;
    unsigned int size = (unsigned int)(rows) * cols;
    unsigned int p = 0;

    const int32_t* src = preprocessing_vmem_getDataAddress(sdSrc);
    int32_t* dst = preprocessing_vmem_getDataAddress(sdDst);

    // Check whether given rows and columns are in a valid range.
    if ((!preprocessing_vmem_isProcessingSizeValid(sdSrc, rows, cols))
            || (!preprocessing_vmem_isProcessingSizeValid(sdDst, rows, cols)))
    {
        return PREPROCESSING_INVALID_SIZE;
    }

    // Process.
    for (unsigned int r = 0; r < rows; r++)
    {
        for (unsigned int c = 0; c < cols; c++)
        {

            p = r * cols + c;

            // Check for valid pointer position.
            PREPROCESSING_DEF_CHECK_POINTER(src, p, size)
            PREPROCESSING_DEF_CHECK_POINTER(dst, p, size)
			if (src[p]<0)
				dst[p]=eve_fp_multiply32(src[p],-256,FP32_FWL);		//If negative multiply by -1(-256) in 24.8 format
			else
				dst[p]=src[p];

            if (dst[p] == EVE_FP32_NAN)
            {
                status = PREPROCESSING_INVALID_NUMBER;
            }
        }
    }

    return status;
}

/*****************************************************************************/
/*****************************************************************************/

int preprocessing_arith_meanImage2(uint32_t sdSrc, uint16_t rows, uint16_t cols,
        uint32_t sdDst)
{
    int status = PREPROCESSING_SUCCESSFUL;
    int32_t* dst = preprocessing_vmem_getDataAddress(sdDst);

    if (dst == 0)
    {
        return PREPROCESSING_INVALID_ADDRESS;
    }

    status = preprocessing_arith_sumImage2(sdSrc, rows, cols, sdDst);

    if (status != PREPROCESSING_SUCCESSFUL)
    {
        return status;
    }

//    dst[0] = eve_fp_divide32(dst[0],
//            eve_fp_int2s32((int)(rows) * cols, FP32_FWL), FP32_FWL);

    return PREPROCESSING_SUCCESSFUL;
}

/*****************************************************************************/

int preprocessing_arith_sumImage2(uint32_t sdSrc, uint16_t rows, uint16_t cols,
        uint32_t sdDst)
{
    int status = PREPROCESSING_SUCCESSFUL;
    unsigned int size = (unsigned int)(rows) * cols;
    unsigned int p = 0;
    double sum,s;

    const int32_t* src = preprocessing_vmem_getDataAddress(sdSrc);
    int32_t* dst = preprocessing_vmem_getDataAddress(sdDst);
    sum=0.0;
    // Check whether given rows and columns are in a valid range.
    if (!preprocessing_vmem_isProcessingSizeValid(sdSrc, rows, cols))
    {
        return PREPROCESSING_INVALID_SIZE;
    }

    // Refuse null pointer.
    if (dst == 0)
    {
        return PREPROCESSING_INVALID_ADDRESS;
    }

    // Process.
    for (unsigned int r = 0; r < rows; r++)
    {
        for (unsigned int c = 0; c < cols; c++)
        {
            p = r * cols + c;

            // Check for valid pointer position.
            PREPROCESSING_DEF_CHECK_POINTER(src, p, size)

          //  dst[0] = eve_fp_add32(dst[0], src[p]);
            s=eve_fp_signed32ToDouble(src[p], FP32_FWL);
            sum=sum+s;
           // printf("numero  = %d\n",  dst[0]);
           // printf("destino y[%d] = %f\n", p, eve_fp_signed32ToDouble(dst[0], FP32_FWL));

        }
    }
    sum=sum/(rows*cols);
    printf("media= %f\n", sum);

    dst[0]=eve_fp_double2s32(sum, FP32_FWL);
    if (dst[0] == EVE_FP32_NAN)

    {
    	printf("la media no es un numero  = %d\n",  dst[0]);
        return PREPROCESSING_INVALID_NUMBER;
    }

    return status;
}


/*****************************************************************************/
int preprocessing_Threshold(uint32_t sdSrc, uint16_t rows, uint16_t cols,
        int32_t thresh,int32_t *newThresh)
{
    int status = PREPROCESSING_SUCCESSFUL;
    unsigned int size = (unsigned int)(rows) * cols;
    unsigned int p = 0;
    int th;
    double m1=0.0;
    double m2=0.0;
    double s=0.0;
    unsigned int count1;
    unsigned int count2;

    const int32_t* src = preprocessing_vmem_getDataAddress(sdSrc);
    //int32_t* dst = preprocessing_vmem_getDataAddress(sdDst);

    // Check whether given rows and columns are in a valid range.
    if ((!preprocessing_vmem_isProcessingSizeValid(sdSrc, rows, cols)))
    {
        return PREPROCESSING_INVALID_SIZE;
    }

    // Process.
    count1=0;
    count2=0;
   // printf("TRESHOLD ========  = %f  \n",  eve_fp_signed32ToDouble(thresh, FP32_FWL));
    for (unsigned int r = 0; r < rows; r++)
    {
        for (unsigned int c = 0; c < cols; c++)
        {
            p = r * cols + c;

            // Check for valid pointer position.
            PREPROCESSING_DEF_CHECK_POINTER(src, p, size)

			th=eve_fp_compare32(src + p, &thresh);
            s=eve_fp_signed32ToDouble(src[p], FP32_FWL);
            if (th==1)
            {

            	m1=m1+s;
            	count1++;

            }else if( th==0) {
            	m1=m1+s;
            	count1++;
            	}
            else if(th==-1)
            {
            	m2=m2+s;
            	count2++;
            }
        }
    }
   // printf("M1 M2 y media  ========  = %f  %f %f %d %d \n",  m1,m2,s,count1,count2);
    m1=m1/count1;
    m2=m2/count2;
    s=(m1+m2)/2;
   //printf("M1 M2 y media  ========  = %f  %f %f %d %d \n",  m1,m2,s,count1,count2);
    newThresh[0]=eve_fp_double2s32(s, FP32_FWL);
    return status;
}
int preprocessing_ana_over_eq_Thresh(uint32_t sdSrc, uint16_t rows, uint16_t cols,
        int32_t thresh, uint32_t sdDst)
{
    int status = PREPROCESSING_SUCCESSFUL;
    unsigned int size = (unsigned int)(rows) * cols;
    unsigned int p = 0;

    const int32_t* src = preprocessing_vmem_getDataAddress(sdSrc);
    int32_t* dst = preprocessing_vmem_getDataAddress(sdDst);

    // Check whether given rows and columns are in a valid range.
    if ((!preprocessing_vmem_isProcessingSizeValid(sdSrc, rows, cols))
            || (!preprocessing_vmem_isProcessingSizeValid(sdDst, rows, cols)))
    {
        return PREPROCESSING_INVALID_SIZE;
    }

    // Process.
    for (unsigned int r = 0; r < rows; r++)
    {
        for (unsigned int c = 0; c < cols; c++)
        {
            p = r * cols + c;

            // Check for valid pointer position.
            PREPROCESSING_DEF_CHECK_POINTER(src, p, size)
            PREPROCESSING_DEF_CHECK_POINTER(dst, p, size)
//ojo mirar este if
            if (((eve_fp_compare32(src + p, &thresh) == 1 )|| (eve_fp_compare32(src + p, &thresh) == 0)))//&&eve_fp_compare32(src + p, &thresh2) == -1)
            {
                dst[p] = FP32_BINARY_TRUE;
            }
            else
            {
                dst[p] = 0;
            }
        }
    }

    return status;
}

int preprocessing_hough(int32_t sdSrc,uint32_t sdDst, uint16_t rows,
		uint16_t cols, unsigned int radio , int disp_max, float step)
{
	int status = PREPROCESSING_SUCCESSFUL;
//	unsigned int size = (unsigned int)(rows) * cols;
//	unsigned int p = 0;
//	unsigned int p1=0;//vector index in output image(hough space)
//	float det,xc,yc;
//	float det1,xc1,yc1;
//	float b;
//	int bb, aa;
//	int32_t val=FP32_BINARY_TRUE;


	unsigned int Xmin=XC-disp_max; 	// Xmin and Xmax define a square boundaries of coordinates around of CCD center (XC,YC)
	unsigned int Xmax=XC+disp_max;

	const int32_t* src = preprocessing_vmem_getDataAddress(sdSrc);
	int32_t* dst = preprocessing_vmem_getDataAddress(sdDst);

	// Check whether given rows and columns are in a valid range.
	if ((!preprocessing_vmem_isProcessingSizeValid(sdSrc, rows, cols))
			|| (!preprocessing_vmem_isProcessingSizeValid(sdDst, rows, cols)))
	{
		return PREPROCESSING_INVALID_SIZE;
	}
	float r2;
	// Process do hough

		r2=(float)radio*radio;
		preprocessing_do_hough(src,dst,rows, cols, Xmin,Xmax,r2,step);
	//}

	return status;
}

int preprocessing_do_hough(const int32_t *src, int32_t *dst, uint16_t rows,
		uint16_t cols, unsigned int Xmin,unsigned int Xmax, float r2, float step)
{

	int status = PREPROCESSING_SUCCESSFUL;
	unsigned int size = (unsigned int)(rows) * cols;
	unsigned int p = 0;
	unsigned int p1=0;//vector index in output image(hough space)
	float det,xc,yc;
	float det1,xc1,yc1;
	float b;
	int bb, aa;
	int32_t val=FP32_BINARY_TRUE;
	for (unsigned int r = 0; r < rows; r++)
	{
		for (unsigned int c = 0; c < cols; c++)
		{
			p = r * cols + c;


			// Check for valid pointer position.
			PREPROCESSING_DEF_CHECK_POINTER(src, p, size)

			if (eve_fp_compare32(src + p, &val) == 0){//check for data equal to one
				//proceso
				for(float a=Xmin; a<Xmax;a+=step){
					det=r2-(c-a)*(c-a);//det to loop over xc and compute yc
					det1=r2-(r-a)*(r-a);//de1t to loop over yc and compute xc


					//yc estimation from xc loop
					if (det>0){
						b=((float)r-sqrt(det));//yc
						if (b>Xmin && b<Xmax){//check for yc €[Xmin,Xmax]
							aa=(int)round((a-Xmin)/step);//xc
							bb=(int)round((b-Xmin)/step);//yc
							if (bb>0 && aa>0){
								p1=bb*cols + aa;
								PREPROCESSING_DEF_CHECK_POINTER(dst, p1, size)
								dst[p1] = eve_fp_add32(dst[p1], FP32_BINARY_TRUE);
								if (dst[p1] == EVE_FP32_NAN)
								{
									status = PREPROCESSING_INVALID_NUMBER;
									printf("hhhhhhh\n");
								}
								//acu_ini[bb*dimensionAcumulador + aa] = acu_ini[bb*dimensionAcumulador + aa] + 1;
							}
						}
					}
					//xc1 estimation from yc1 loop
					if (det1>0){
						b=((float)c-sqrt(det1));//xc1
						if (b>Xmin && b<Xmax){ //check for xc€[Xmin,Xmax]
							aa=(int)round((a-Xmin)/step);//yc
							bb=(int)round((b-Xmin)/step);//xc
							if (bb>0 && aa>0){
								p1=aa*cols + bb;
								PREPROCESSING_DEF_CHECK_POINTER(dst, p1, size)
								dst[p1] = eve_fp_add32(dst[p1], FP32_BINARY_TRUE);
								if (dst[p1] == EVE_FP32_NAN)
								{
									status = PREPROCESSING_INVALID_NUMBER;
									printf("yyyyyyyyy\n");
								}
								//acu_ini[aa*dimensionAcumulador + bb] = acu_ini[aa*dimensionAcumulador + bb] + 1;
							}
						}
					}
				}
			}
		}
	}
	return status;
}

int preprocessing_zero(uint32_t sdSrc, uint16_t rows, uint16_t cols, uint32_t sdDst)
{
    int status = PREPROCESSING_SUCCESSFUL;
    unsigned int size = (unsigned int)(rows) * cols;
    unsigned int p = 0;

    const int32_t* src = preprocessing_vmem_getDataAddress(sdSrc);
    int32_t* dst = preprocessing_vmem_getDataAddress(sdDst);

    // Check whether given rows and columns are in a valid range.
    if ((!preprocessing_vmem_isProcessingSizeValid(sdSrc, rows, cols))
            || (!preprocessing_vmem_isProcessingSizeValid(sdDst, rows, cols)))
    {
        return PREPROCESSING_INVALID_SIZE;
    }

    // Process.
    for (unsigned int r = 0; r < rows; r++)
    {
        for (unsigned int c = 0; c < cols; c++)
        {
            p = r * cols + c;

            // Check for valid pointer position.
            PREPROCESSING_DEF_CHECK_POINTER(src, p, size)
            PREPROCESSING_DEF_CHECK_POINTER(dst, p, size)

				dst[p]=0;

            if (dst[p] == EVE_FP32_NAN)
            {
                status = PREPROCESSING_INVALID_NUMBER;
            }
        }
    }

    return status;
}

/*****************************************************************************/

int preprocessing_maximumValue(uint32_t sdSrc, uint16_t rows, uint16_t cols,
        uint32_t sdDst, int16_t index)
{
    int32_t max = EVE_FP32_NAN;
    unsigned int size = (unsigned int)(rows) * cols;
    unsigned int sizeLMAX = (unsigned int)(LMAX_ROWS) * LMAX_COLS;
    unsigned int p = 0;
    int32_t maxX;
    int32_t maxY;

    const int32_t* src = preprocessing_vmem_getDataAddress(sdSrc);
    int32_t* dst = preprocessing_vmem_getDataAddress(sdDst);

    // Check whether given rows and columns are in a valid range.
    if ((!preprocessing_vmem_isProcessingSizeValid(sdSrc, rows, cols))
            || (!preprocessing_vmem_isProcessingSizeValid(sdDst, LMAX_ROWS, LMAX_COLS)))
    {
        return PREPROCESSING_INVALID_SIZE;
    }

    // Process.
    for (unsigned int r = 0; r < rows; r++)
    {
        for (unsigned int c = 0; c < cols; c++)
        {
            p = r * cols + c;

            // Check for valid pointer position.
            PREPROCESSING_DEF_CHECK_POINTER(src, p, size)

            if (eve_fp_compare32(src + p, &max) == 1)
            {
                max = src[p];
                maxX = (int32_t)c;
                maxY = (int32_t)r;

            }
        }
    }

    if(eve_fp_compare32(dst + index, &max) == -1){
    	dst[index] = max;
    	dst[index+1] = maxX;
    	dst[index+2] = maxY;

    }

    printf("%d   %d   %d\n", max, maxX, maxY);	//OJO Pasarlo a punto fijo
    return 0;
}
