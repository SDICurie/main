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
#include "fsort_test.h"
#include "fsort.h"
#include <math.h>
#include "balib.h"

#define EPS 1e-3
int fsort_test_single(float *pfInput, int nlen, int nRightRet,
		      float *pfRightOut)
{
	int i;
	int nRet = fsort(pfInput, nlen);

	if (nRet != nRightRet) return 1;
	if (nRet != 0) return 0;
	for (i = 0; i < nlen; i++) {
		if (fabs(pfInput[i] - pfRightOut[i]) > EPS) return 1;
	}
	return 0;
}

float afIn[] =
{ -0.911369, -1.678493, -0.783715, 1.493947, 0.521545, -1.087397, 1.332717,
  -0.845684, -0.036572, -0.701775, -0.012914, -0.685540, -0.742344,
  -0.742570, 2.034339, -2.005902, 1.023107, -0.463266, 0.551152, 1.034414,
  -1.466799, 0.740570, -0.254725, -1.030599, -0.031905, -0.092056,
  0.307973, 0.403648, -0.305417, 0.748379, 0.307784, 1.274122, 1.423835,
  -0.490563, -0.276598, 1.070125, 2.027250, 1.297419, -0.000924, 0.211657,
  1.609454, -1.122569, -0.245941, -0.791850, 1.397067, -1.250427,
  -0.521203, 0.060456, 0.330186, -0.396981, -1.016279, -0.331201,
  -0.325771, -1.185146, -1.338219, 0.203908, 0.694576, 0.533736, -0.000137,
  -0.626460, -1.658805, -1.009229, -0.621249, -0.297532, 0.243113,
  0.220733, 0.818476, 0.340453, -1.401057, 0.776808, -1.083954, -0.482764,
  -0.749304, 0.618081, -1.778909, 0.575176, -0.764732, -0.889437,
  -1.367501, -0.869616, -1.272249, -0.948765, 1.131688, -0.235521,
  -1.107441, 1.135382, 0.560841, 0.335338, -0.591889, 0.626455, -0.692940,
  -0.831883, -0.652079, -0.500097, 0.482538, 0.860262, -2.041654,
  -0.503305, -1.485972, 1.163908 };
const float afTrueOut[] =
{ -2.041654, -2.005902, -1.778909, -1.678493, -1.658805, -1.485972, -1.466799,
  -1.401057, -1.367501, -1.338219, -1.272249, -1.250427, -1.185146,
  -1.122569, -1.107441, -1.087397, -1.083954, -1.030599, -1.016279,
  -1.009229, -0.948765, -0.911369, -0.889437, -0.869616, -0.845684,
  -0.831883, -0.791850, -0.783715, -0.764732, -0.749304, -0.742570,
  -0.742344, -0.701775, -0.692940, -0.685540, -0.652079, -0.626460,
  -0.621249, -0.591889, -0.521203, -0.503305, -0.500097, -0.490563,
  -0.482764, -0.463266, -0.396981, -0.331201, -0.325771, -0.305417,
  -0.297532, -0.276598, -0.254725, -0.245941, -0.235521, -0.092056,
  -0.036572, -0.031905, -0.012914, -0.000924, -0.000137, 0.060456,
  0.203908, 0.211657, 0.220733, 0.243113, 0.307784, 0.307973, 0.330186,
  0.335338, 0.340453, 0.403648, 0.482538, 0.521545, 0.533736, 0.551152,
  0.560841, 0.575176, 0.618081, 0.626455, 0.694576, 0.740570, 0.748379,
  0.776808, 0.818476, 0.860262, 1.023107, 1.034414, 1.070125, 1.131688,
  1.135382, 1.163908, 1.274122, 1.297419, 1.332717, 1.397067, 1.423835,
  1.493947, 1.609454, 2.027250, 2.034339 };
int fsort_test(int testcase)
{
	int nRet = 0;
	int nLen = sizeof(afIn) / sizeof(float);
	int nRightRet = 0;

	switch (testcase) {
	case 0:
		nRightRet = 0;
		nRet = fsort_test_single(afIn, nLen, nRightRet,
					 (float *)afTrueOut);
		if (nRet != 0) return nRet;
		break;
	case 1:
		nRightRet = ERR_INVALID_PARAMTER;
		nRet = fsort_test_single(NULL, nLen, nRightRet,
					 (float *)afTrueOut);
		if (nRet != 0) return nRet;
		break;
	case 2:
		nRightRet = ERR_INVALID_PARAMTER;
		nRet = fsort_test_single(afIn, 0, nRightRet, (float *)afTrueOut);
		if (nRet != 0) return nRet;
		break;
	}
	return 0;
}
