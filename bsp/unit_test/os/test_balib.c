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
#include "balib.h"
#include "test_balib.h"
#include "balib_test/balib_test_data.h"
#include "balib_test/statistics_test.h"
#include "balib_test/fsort_test.h"
#include <math.h>
#include "balib_test/wavelet_test.h"
#include "balib_test/fft_test.h"
#include "util/cunit_test.h"
#include "infra/log.h"

int test_balib(void)
{
	int i = 0, j = 0;
	int nRet, nOk, nRightRet;
	pFun1 pTmpFun1;
	pFun2 pTmpFun2;
	float *pfRightRes = NULL;

	cu_print(
		"##############################################################\n");
	cu_print(
		"# Purpose of the balib test:                                   #\n");
	cu_print(
		"# Check that all implemented functions in balib package       #\n");
	cu_print(
		"# return the right results for a set of input values          #\n");
	cu_print(
		"##############################################################\n");

	int nTestSampleNum = sizeof(DataLen) / sizeof(int);
	cu_print("nTestSampleNum == %d\n", nTestSampleNum);
	for (j = 0; j < FUN1_NUM; j++) {
		pTmpFun1 = apFun1[j];
		pfRightRes = apfRightRes1[j];
		nOk = 0;
		for (i = 0; i < nTestSampleNum; i++) {
			nRightRet = 0;
			if (fabs(pfRightRes[i] - ERR_INVALIDPARAMTER_VALUE) <
			    1) nRightRet = ERR_INVALID_PARAMTER;
			else if (fabs(pfRightRes[i] -
				      ERR_DEVIDED_BY_ZERO_VALUE) <
				 1) nRightRet =
					ERR_DEVIDED_BY_ZERO;

			nRet = pTmpFun1(pTestData[i], DataLen[i], nRightRet,
					pfRightRes[i]);
			CU_ASSERT("balib statiscs", nRet == SUCCESS);
		}
	}
	cu_print(
		"The first half of statistics function test of balib test is done\n");
	for (j = 0; j < FUN2_NUM; j++) {
		pTmpFun2 = apFun2[j];
		nOk = 0;
		pfRightRes = apfRightRes2[j];
		for (i = 0; i < nTestSampleNum; i++) {
			nRightRet = 0;
			if (fabs(pfRightRes[i] - ERR_INVALIDPARAMTER_VALUE) <
			    1) nRightRet = ERR_INVALID_PARAMTER;
			else if (fabs(pfRightRes[i] -
				      ERR_DEVIDED_BY_ZERO_VALUE) <
				 1) nRightRet =
					ERR_DEVIDED_BY_ZERO;
			nRet =
				pTmpFun2(pTestData[i], DataLen[i], anFlag[j],
					 nRightRet,
					 pfRightRes[i]);
			CU_ASSERT("balib statiscs", nRet == SUCCESS);
		}
	}
	cu_print(
		"The second half of statistics function test of balib test is done\n");

	int nSortTestSampleNum = 3;
	for (i = 0; i < nSortTestSampleNum; i++) {
		nRet = fsort_test(i);
		CU_ASSERT("balib fsort", nRet == SUCCESS);
	}
	cu_print("fsort of balib test is done\n");

	int fft_testcase = 0;
	for (fft_testcase = 0; fft_testcase < 5; fft_testcase++) {
		nRet = fft_test_sw(fft_testcase);
		CU_ASSERT("balib fft", nRet == SUCCESS);
	}
	cu_print("fft test of balib is done\n");

	int wavelet_testcase = 0;
	for (wavelet_testcase = 0; wavelet_testcase < 10; wavelet_testcase++) {
		nRet = wavelet_test(wavelet_testcase);
		CU_ASSERT("balib wavelet", nRet == SUCCESS);
	}
	cu_print("wavelet of balib test is done\n");
	return 0;
}
