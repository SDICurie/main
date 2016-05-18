/*
 * Copyright (c) 2015, Intel Corporation. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.
 *
 * 3. Neither the name of the copyright holder nor the names of its contributors
 * may be used to endorse or promote products derived from this software without
 * specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */
#include "statistics_test.h"
#include "statistics.h"
#include <math.h>

#define EPS 1e-3
int CheckResult(float fOutput, float fTrueValue)
{
	float fRelativeErr = fabs(fOutput - fTrueValue);

	if (fabs(fTrueValue) > EPS) fRelativeErr /= fTrueValue;
	if (fRelativeErr < EPS) return 0;
	else return 1;
}
int Sum_test(float *pIn, int nLen, int nRightRet, float fRightSum)
{
	float fSum = 0;
	int nRet = Sum(pIn, nLen, &fSum);

	if (nRet != nRightRet ||
	    (nRet == 0 && CheckResult(fSum, fRightSum) != 0)) return -1;
	return 0;
}

int Mean_test(float *pIn, int nLen, int nRightRet, float fRightMean)
{
	float fMean = 0;
	int nRet = Mean(pIn, nLen, &fMean);

	if (nRet != nRightRet ||
	    (nRet == 0 && CheckResult(fMean, fRightMean) != 0)) return -1;
	return 0;
}

int Var_test(float *pIn, int nLen, int flag, int nRightRet, float fRightStdVar)
{
	float fStdVar = 0;
	int nRet = Var(pIn, nLen, flag, &fStdVar);

	if (nRet != nRightRet ||
	    (nRet == 0 && CheckResult(fStdVar, fRightStdVar) != 0)) return -1;
	return 0;
}

int Kurtosis_test(float *pIn, int nLen, int nRightRet, float fRightKurtosis)
{
	float fKurtosis = 0;
	int nRet = Kurtosis(pIn, nLen, &fKurtosis);

	if (nRet != nRightRet ||
	    (nRet == 0 &&
	     CheckResult(fKurtosis, fRightKurtosis) != 0)) return -1;
	return 0;
}

int Skewness_test(float *pIn, int nLen, int nRightRet, float fRightSkewness)
{
	float fSkewnesss = 0;
	int nRet = Skewness(pIn, nLen, &fSkewnesss);

	if (nRet != nRightRet ||
	    (nRet == 0 &&
	     CheckResult(fSkewnesss, fRightSkewness) != 0)) return -1;
	return 0;
}

int K_Center_Moment_test(float *pIn, int nLen, int K, int nRightRet,
			 float fRightCenterMoment)
{
	float fCenterMoment = 0;
	int nRet = K_Center_Moment(pIn, nLen, K, &fCenterMoment);

	if (nRet != nRightRet ||
	    (nRet == 0 &&
	     CheckResult(fCenterMoment, fRightCenterMoment) != 0)) return -1;
	return 0;
}

int K_Moment_test(float *pIn, int nLen, int K, int nRightRet,
		  float fRightMoment)
{
	float fMoment = 0;
	int nRet = K_Moment(pIn, nLen, K, &fMoment);

	if (nRet != nRightRet ||
	    (nRet == 0 && CheckResult(fMoment, fRightMoment) != 0)) return -1;
	return 0;
}

int NegEntropy_test(float *pIn, int nLen, int nRightRet, float fRightNegEntropy)
{
	float fNegEntropy = 0;
	int nRet = NegEntropy(pIn, nLen, &fNegEntropy);

	if (nRet != nRightRet ||
	    (nRet == 0 &&
	     CheckResult(fNegEntropy, fRightNegEntropy) != 0)) return -1;
	return 0;
}

int Max_Array_test(float *pIn, int nLen, int nRightRet, float fRightMax)
{
	float fMax = 0;
	int nRet = Max_Array(pIn, nLen, &fMax);

	if (nRet != nRightRet ||
	    (nRet == 0 && CheckResult(fMax, fRightMax) != 0)) return -1;
	return 0;
}

int Min_Array_test(float *pIn, int nLen, int nRightRet, float fRightMin)
{
	float fMin = 0;
	int nRet = Min_Array(pIn, nLen, &fMin);

	if (nRet != nRightRet ||
	    (nRet == 0 && CheckResult(fMin, fRightMin) != 0)) return -1;
	return 0;
}
