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

#include "wavelet_test.h"
#include "wavelet.h"
#include "balib.h"
#include "math.h"
#define SIGLEN 128
#define WPLEVEL 3
#define LEN_WP_FILTER 6

float LPF_DB6[LEN_WP_FILTER +
	      1] =
{ LEN_WP_FILTER, 0.03523, -0.08544, -0.135, 0.4599, 0.8069, 0.3327 };
float HPF_DB6[LEN_WP_FILTER +
	      1] =
{ LEN_WP_FILTER, -0.3327, 0.8069, -0.4599, -0.135, 0.08544, 0.03523 };

#define EPS 1e-4

const float wavelet_test_s[SIGLEN] =
{ 0.679728, 0.161087, 0.770236, 0.180127, 0.751303, 0.615664, 0.924194,
  0.883482,
  1.095062, 1.104700, 0.569861, 0.955797, 0.475595, 0.328391, 1.061271,
  0.835802, 0.833476, 1.275198, 0.996372, 1.019270, 1.275177, 1.234354,
  1.017682, 0.634917, 0.701872, 1.357284, 0.507144, 0.974917, 0.658320,
  1.473269, 1.210287, 0.999869, 0.971088, 0.559017, 1.179564, 0.537019,
  0.561838, 1.006665, 0.575200, 1.288921, 1.279487, 1.174434, 0.590826,
  1.088470, 0.934330, 1.374578, 1.035497, 1.170806, 0.807351, 0.768171,
  1.142510, 0.381319, 0.410956, 0.430440, 0.626636, 1.045157, 0.994706,
  0.228916, 0.544400, 0.648366, 0.514345, 0.730225, 0.676982, 0.316518,
  0.431651, -0.009047, 0.935055, 0.093803, 0.008671, 0.250920, 0.052976,
  0.321243, 0.148152, 0.737853, 0.684634, -0.204374, 0.460073, -0.028730,
  0.105639, 0.212091, 0.589184, 0.047269, 0.596547, -0.100149, 0.285364,
  0.237475, 0.098166, 0.246111, 0.204588, -0.292640, -0.350456, 0.514065,
  -0.319272, -0.461987, 0.063607, 0.382469, 0.169175, -0.308964, -0.128676,
  -0.033862, 0.491245, -0.328611, 0.377053, 0.173993, -0.085668, -0.261071,
  -0.012708, 0.053158, -0.295123, 0.187904, -0.160318, 0.014144, 0.229433,
  -0.083973, -0.026756, 0.319241, -0.012504, 0.567325, 0.746965, 0.516471,
  0.152535, 0.415624, -0.037373, 0.784818, 0.782109, 0.744395, 0.211719,
  0.569822 };
const float wavelet_output_matlab[SIGLEN] =
{ 0.000030, -0.006051, 0.017861, -0.111355, 1.748194, 2.136449, 2.862089,
  2.815960, 2.335731, 3.141204, 1.985110, 1.785461, 0.832582, 0.747691,
  0.684896, -0.180015, -0.000281, 0.055680, 0.130213, 0.655980, -0.005632,
  -0.431476, 0.213820, 0.303658, 0.179342, 0.182574, 0.014520, 0.349105,
  0.085953, 0.008997, 0.065977, -0.068796, -0.000281, 0.023201, -0.124707,
  -0.085513, 0.098397, -0.133944, 0.058189, 0.169277, -0.338376, 0.407074,
  -0.091302, 0.372667, 0.015619, -0.074579, 0.140178, -0.111433, 0.002651,
  -0.205292, 0.026554, 0.185207, -0.117531, 0.405785, -0.058787, 0.409930,
  0.040631, 0.221018, -0.392676, -0.241680, 0.059321, 0.251817, 0.358307,
  -0.094164, -0.000281, -0.014777, 0.105070, -0.491732, -0.140293,
  -0.038774, 0.410093, -0.060396, 0.383810, 0.128028, -0.050193, -0.442333,
  0.056633, -0.570367, -0.002252, -0.067624, 0.002651, 0.153375, -0.272978,
  0.185525, -0.051828, 0.549976, -0.260049, 0.116246, -0.317400, 0.300789,
  -0.308999, -0.523214, -0.084819, 0.330011, -0.340862, 0.438789, 0.002651,
  0.005400, -0.032602, 0.154663, -0.218802, 0.476090, 0.247398, 0.160890,
  -0.021582, -0.186448, -0.235355, 0.353849, 0.130280, 0.006052, -0.775495,
  -0.538453, -0.025032, -0.181519, 0.121488, 0.491165, 0.214376, -0.394883,
  -0.362464, -0.083936, -0.616823, -0.242514, 0.042286, -0.402108,
  -0.272707, 0.324277, -0.031308, -0.338270 };

extern float temparray[];

int wavelet_test(int testcase)
{
	int nRet, i;
	float *si = temparray;
	float *wpt = temparray + (SIGLEN + 1);

	switch (testcase) {
	case 0:
		for (i = 0; i < SIGLEN; i++)
			si[i + 1] = (float)wavelet_test_s[i];

		nRet = SetWPFilter(LPF_DB6, HPF_DB6);
		if (nRet != SUCCESS) return nRet;

		nRet = WPAnalysis(si, SIGLEN, WPLEVEL, wpt);
		if (nRet != SUCCESS) return nRet;

		for (i = 0; i < SIGLEN; i++) {
			if (fabs(wpt[i] - wavelet_output_matlab[i]) > EPS)
				return ERR_UNDEFINED;
		}
		break;
	case 1:
		nRet = SetWPFilter(NULL, HPF_DB6);
		if (nRet == ERR_INVALID_PARAMTER)
			return SUCCESS;
		else
			return ERR_UNDEFINED;
		break;
	case 2:
		nRet = SetWPFilter(LPF_DB6, NULL);
		if (nRet == ERR_INVALID_PARAMTER)
			return SUCCESS;
		else
			return ERR_UNDEFINED;
		break;
	case 3:
		LPF_DB6[0] = 0;
		nRet = SetWPFilter(LPF_DB6, HPF_DB6);
		LPF_DB6[0] = LEN_WP_FILTER;
		if (nRet == ERR_INVALID_PARAMTER)
			return SUCCESS;
		else
			return ERR_UNDEFINED;
		break;
	case 4:
		HPF_DB6[0] = 0;
		nRet = SetWPFilter(LPF_DB6, HPF_DB6);
		HPF_DB6[0] = LEN_WP_FILTER;
		if (nRet == ERR_INVALID_PARAMTER)
			return SUCCESS;
		else
			return ERR_UNDEFINED;
		break;
	case 5:
		nRet = SetWPFilter(LPF_DB6, HPF_DB6);
		if (nRet == SUCCESS)
			return SUCCESS;
		else
			return ERR_UNDEFINED;

		break;
	case 6:
		nRet = WPAnalysis(NULL, SIGLEN, WPLEVEL, wpt);
		if (nRet == ERR_INVALID_PARAMTER)
			return SUCCESS;
		else
			return ERR_UNDEFINED;
		break;
	case 7:
		nRet = WPAnalysis(si, SIGLEN, WPLEVEL, NULL);
		if (nRet == ERR_INVALID_PARAMTER)
			return SUCCESS;
		else
			return ERR_UNDEFINED;
		break;
	case 8:
		nRet = WPAnalysis(si, 0, WPLEVEL, wpt);
		if (nRet == ERR_INVALID_PARAMTER)
			return SUCCESS;
		else
			return ERR_UNDEFINED;
		break;
	case 9:
		nRet = WPAnalysis(si, SIGLEN, 0, wpt);
		if (nRet == ERR_INVALID_PARAMTER)
			return SUCCESS;
		else
			return ERR_UNDEFINED;
		break;
	default:
		break;
	}
	return SUCCESS;
}
