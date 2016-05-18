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

#include "fft_test.h"
#include "balib.h"
#include "fft.h"
#include "math.h"

#define EPS 1e-0
#define FFT_N 128

float sin_table[SAMPLELENOFPIBY2 + 1];

const float fft_test_s[FFT_N] =
{ 1.945257, -0.227066, 0.616916, 2.266481, 0.074667, 2.051560, 2.371281,
  2.117470, 0.704993, 1.669004, -0.028489, 0.972075, -0.141536, 0.378296,
  -0.784549, 0.364186, -0.538184, -0.816568, -2.023747, 0.846082, 2.326251,
  0.235405, 2.394775, 0.873011, 0.264157, 1.082638, 0.417458, 1.254738,
  -0.639619, -0.043335, -0.300014, -0.866319, -1.071762, -2.076924,
  -3.334843, -1.612980, -2.878971, -0.693419, -1.893818, -0.428569,
  0.773206, -0.935164, -0.858325, 1.150215, 0.583408, 0.449498, 0.611524,
  -0.298123, 1.388940, -0.276207, 1.110110, -0.669834, 1.116828, -0.143762,
  -3.572007, -0.594809, 0.993458, -0.357371, -0.394097, 1.044647, 2.415057,
  2.033231, 1.699096, 2.322260, 2.451675, 1.770910, 2.253485, 1.640414,
  2.059768, 1.614658, -0.839757, -0.374194, -1.450164, -0.464259,
  -3.103447, -1.118044, -0.018235, -0.621412, 0.455614, -0.805383,
  -1.125577, -2.210291, 2.452805, 0.255463, 1.111828, 0.584171, -0.418575,
  -0.508649, -2.934765, -1.679554, -1.375214, -1.875051, -0.704546,
  -1.122916, -2.141774, -2.772431, -0.651023, -1.257524, -0.502313,
  -0.088310, -0.119492, 1.150555, 1.064419, 1.223798, 0.368064, 1.627645,
  2.552468, 1.170772, 1.833190, 1.848821, 4.281642, -0.045188, 0.206275,
  -0.240903, -1.501739, 0.134754, 0.279188, -0.327145, 0.137775, 0.273150,
  0.285578, -0.062824, 0.236614, 0.140675, 1.775302, 0.974122, 1.153774,
  -0.140730 };
const float fft_output_matlab[FFT_N] =
{ 21.185713, 19.805192, 47.408321, 37.469556, 11.605984, 19.703723, 59.756529,
  39.542534, 4.317404, 18.162965, 7.284442, 6.181045, 4.386819, 9.618674,
  8.282810, 18.302766, 24.071668, 4.147750, 6.866261, 9.404756, 13.075975,
  5.861184, 7.267146, 5.902168, 6.256138, 8.402579, 6.702438, 14.010836,
  9.508359, 7.418988, 6.900744, 19.011968, 10.133160, 9.014157, 12.830557,
  3.272032, 3.081280, 10.041971, 17.474738, 17.726261, 2.137024, 12.547784,
  6.438847, 4.203690, 14.078877, 7.658746, 4.676831, 8.159222, 5.739348,
  18.361522, 8.508133, 16.189061, 5.030030, 5.384587, 7.230502, 17.184128,
  19.632995, 3.505345, 13.042237, 10.525951, 13.664763, 4.193167, 8.657453,
  10.891414, 1.654820, 10.891414, 8.657453, 4.193167, 13.664763, 10.525951,
  13.042237, 3.505345, 19.632995, 17.184128, 7.230502, 5.384587, 5.030030,
  16.189061, 8.508133, 18.361522, 5.739348, 8.159222, 4.676831, 7.658746,
  14.078877, 4.203690, 6.438847, 12.547784, 2.137024, 17.726261, 17.474738,
  10.041971, 3.081280, 3.272032, 12.830557, 9.014157, 10.133160, 19.011968,
  6.900744, 7.418988, 9.508359, 14.010836, 6.702438, 8.402579, 6.256138,
  5.902168, 7.267146, 5.861184, 13.075975, 9.404756, 6.866261, 4.147750,
  24.071668, 18.302766, 8.282810, 9.618674, 4.386819, 6.181045, 7.284442,
  18.162965, 4.317404, 39.542534, 59.756529, 19.703723, 11.605984,
  37.469556, 47.408321, 19.805192 };

float temparray[1032 / 4];

int fft_test_sw(int testcase)
{
	int i, nRet;
	struct compx *s = (struct compx *)temparray;

	switch (testcase) {
	case 0:
		nRet = init_fft(FFT_N);
		if (nRet != 0) return nRet;
		nRet = create_sin_tab_sw(sin_table);
		if (nRet != 0) return nRet;
		for (i = 0; i < FFT_N; i++) {
			s[i].real = (float)fft_test_s[i];
			s[i].imag = 0;
		}
		nRet = FFT_SW(s);
		if (nRet != 0) return nRet;

		for (i = 0; i < FFT_N; i++)
			s[i].real =
				sqrt(
					s[i].real * s[i].real + s[i].imag *
					s[i].imag);

		for (i = 0; i < FFT_N; i++) {
			if (fabs(s[i].real - fft_output_matlab[i]) > EPS)
				return -2;
		}
		break;
	case 1:
		nRet = init_fft(0);
		if (nRet == ERR_INVALID_PARAMTER)
			return SUCCESS;
		else
			return ERR_UNDEFINED;
		break;
	case 2:
		nRet = create_sin_tab_sw(NULL);
		if (nRet == ERR_INVALID_PARAMTER)
			return SUCCESS;
		else
			return ERR_UNDEFINED;
		break;
	case 3:
		nRet = FFT_SW(NULL);
		if (nRet == ERR_INVALID_PARAMTER)
			return SUCCESS;
		else
			return ERR_UNDEFINED;
		break;
	case 4:
		nRet = FFT_HW(NULL);
		if (nRet == ERR_INVALID_PARAMTER)
			return SUCCESS;
		else
			return ERR_UNDEFINED;
		break;
	default:
		break;
	}
	return 0;
}

int fft_test_hw()
{
	int i, nRet;
	struct compx s[FFT_N];

	nRet = init_fft(FFT_N);
	if (nRet != 0) return nRet;
	for (i = 0; i < FFT_N; i++) {
		s[i].real = fft_test_s[i];
		s[i].imag = 0;
	}
	nRet = FFT_HW(s);
	if (nRet != 0) return nRet;

	for (i = 0; i < FFT_N; i++)
		s[i].real = sqrt(s[i].real * s[i].real + s[i].imag * s[i].imag);

	for (i = 0; i < FFT_N; i++) {
		if (fabs(s[i].real) - fft_output_matlab[i] >
		    EPS) return ERR_UNDEFINED;
	}

	return 0;
}
