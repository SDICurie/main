/****************************************************************************************
 *
 * BSD LICENSE
 *
 * Copyright(c) 2015 Intel Corporation.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 * * Redistributions in binary form must reproduce the above copyright
 * notice, this list of conditions and the following disclaimer in
 * the documentation and/or other materials provided with the
 * distribution.
 * * Neither the name of Intel Corporation nor the names of its
 * contributors may be used to endorse or promote products derived
 * from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 ***************************************************************************************/
/*
 * Author(s) : Binuraj Ravindran (binuraj.ravindran@intel.com)
 * Group : NBG/NTG
 */

/* *INDENT-OFF* */

/*
* This file contains macros that will enable the usage of ARC DSP and IDE
* instructions so that they can be called as C-subroutines. This file
* specifically addresses the usage with GCC compiler.
*/

#define intrinsic_2OP(NAME, MOP, SOP)                                           \
    ".extInstruction " NAME "," #MOP "," #SOP ",SUFFIX_NONE, SYNTAX_2OP\n\t"
#define intrinsic_3OP(NAME, MOP, SOP)                                           \
    ".extInstruction " NAME "," #MOP "," #SOP ",SUFFIX_NONE, SYNTAX_3OP\n\t"


/*******************************************************************************
 *
 * Single Precision Floating Point Extensions
 *
 *******************************************************************************/

#define __ide_dsp_fp_div(src1, src2) ({float __dst;                             \
    __asm__ __volatile__ ( intrinsic_3OP("dsp_fp_div", 0x07, 0x2a)              \
             "dsp_fp_div %0, %1, %2\n\t"                                        \
             : "=r" (__dst)                                                     \
             : "r" (src1), "r" (src2));                                         \
             __dst;})

#define __ide_dsp_fp_flt2i(src) ({int __dst;                                    \
    __asm__ __volatile__ ( intrinsic_2OP("dsp_fp_flt2i", 0x07, 0x2b)            \
             "dsp_fp_flt2i %0, %1\n\t"                                          \
             : "=r" (__dst)                                                     \
             : "r" (src));                                                      \
             __dst;})

#define __ide_dsp_fp_i2flt(src) ({float __dst;                                  \
      __asm__ __volatile__ ( intrinsic_2OP("dsp_fp_i2flt", 0x07, 0x2c)          \
             "dsp_fp_i2flt %0, %1\n\t"                                          \
             : "=r" (__dst)                                                     \
             : "r" (src));                                                      \
             __dst;})

#define __ide_dsp_fp_sqrt(src) ({float __dst;                                   \
    __asm__ __volatile__ ( intrinsic_2OP("dsp_fp_sqrt", 0x07, 0x2d)             \
             "dsp_fp_sqrt %0, %1\n\t"                                           \
             : "=r" (__dst)                                                     \
             : "r" (src));                                                      \
             __dst;})

#define __ide_dsp_fp_cmp(src1, src2) ({int __dst;                               \
    __asm__ __volatile__ ( intrinsic_3OP("dsp_fp_cmp", 0x07, 0x2b)              \
             "dsp_fp_cmp %0, %1, %2\n\t"                                        \
             : "=r" (__dst)                                                     \
             : "r" (src1), "r" (src2));                                         \
             __dst;})

#define __ide_dsp_fp_cmp_eq(src1, src2) ({int __dst;                            \
    __asm__ __volatile__ ( intrinsic_3OP("dsp_fp_cmp", 0x07, 0x2b)              \
             "dsp_fp_cmp %0, %1, %2\n\t"                                        \
             "cmp    %0,1\n\t"                                                  \
             "mov    %0,0\n\t"                                                  \
             "mov.eq %0,1\n\t"                                                  \
             : "=r" (__dst)                                                     \
             : "r" (src1), "r" (src2));                                         \
             __dst;})

#define __ide_dsp_fp_cmp_lt(src1, src2) ({int __dst;                            \
    __asm__ __volatile__ ( intrinsic_3OP("dsp_fp_cmp", 0x07, 0x2b)              \
             "dsp_fp_cmp %0, %1, %2\n\t"                                        \
             "cmp    %0,2\n\t"                                                  \
             "mov    %0,0\n\t"                                                  \
             "mov.eq %0,1\n\t"                                                  \
             : "=r" (__dst)                                                     \
             : "r" (src1), "r" (src2));                                         \
             __dst;})

#define __ide_dsp_fp_cmp_gt(src1, src2) ({int __dst;                            \
    __asm__ __volatile__ ( intrinsic_3OP("dsp_fp_cmp", 0x07, 0x2b)              \
             "dsp_fp_cmp %0, %1, %2\n\t"                                        \
             "cmp    %0,4\n\t"                                                  \
             "mov    %0,0\n\t"                                                  \
             "mov.eq %0,1\n\t"                                                  \
             : "=r" (__dst)                                                     \
             : "r" (src1), "r" (src2));                                         \
             __dst;})


/*******************************************************************************
 *
 * Fixed Point Trignometic Functions
 *
 *******************************************************************************/

#define __ide_exprst(src1, src2) ({int __dst;                                   \
    __asm__ __volatile__ ( intrinsic_3OP("scgexpjrst", 0x07, 0x1c)              \
             " scgexpjrst %0, %1, %2\n\t"                                       \
             : "=r" (__dst)                                                     \
             : "r" (src1), "r" (src2));                                         \
             __dst;})

#define __ide_sin(src1, src2) ({int __dst;                                      \
    __asm__ __volatile__ ( intrinsic_3OP("scgsin", 0x07, 0x12)                  \
             " scgsin %0, %1, %2\n\t"                                           \
             : "=r" (__dst)                                                     \
             : "r" (src1), "r" (src2));                                         \
             __dst;})

#define __ide_cos(src1, src2) ({int __dst;                                      \
    __asm__ __volatile__ ( intrinsic_3OP("scgcos", 0x07, 0x13)                  \
             " scgcos %0, %1, %2\n\t"                                           \
             : "=r" (__dst)                                                     \
             : "r" (src1), "r" (src2));                                         \
             __dst;})

#define __ide_expj(src1, src2) ({int __dst;                                     \
    __asm__ __volatile__ ( intrinsic_3OP("scgexpj", 0x07, 0x10)                 \
             " scgexpj %0, %1, %2\n\t"                                          \
             : "=r" (__dst)                                                     \
             : "r" (src1), "r" (src2));                                         \
             __dst;})

#define __ide_expnj(src1, src2) ({int __dst;                                    \
    __asm__ __volatile__ ( intrinsic_3OP("scgexpnj", 0x07, 0x11)                \
             " scgexpnj %0, %1, %2\n\t"                                         \
             : "=r" (__dst)                                                     \
             : "r" (src1), "r" (src2));                                         \
             __dst;})

/*******************************************************************************
 *
 * Log/Alog Functions
 *
 *******************************************************************************/



#define __ide_alg2(src) ({int   __dst;                                          \
    __asm__ __volatile__ ( intrinsic_2OP("alg2", 0x07, 0x20)                    \
             "alg2 %0, %1\n\t"                                                  \
             : "=r" (__dst)                                                     \
             : "r" (src));                                                      \
             __dst;})


#define __ide_lg2(src) ({int   __dst;                                           \
    __asm__ __volatile__ ( intrinsic_2OP("lg2", 0x07, 0x21)                     \
             "lg2 %0, %1\n\t"                                                   \
             : "=r" (__dst)                                                     \
             : "r" (src));                                                      \
             __dst;})


/*******************************************************************************
 *
 * Complex arithmetic
 *
 *******************************************************************************/

#define __ide_mulcc(src1, src2) ({int  __dst;                                   \
    __asm__ __volatile__ ( intrinsic_3OP("mulcc", 0x07, 0x0)                    \
             " mulcc  %0, %1, %2\n\t"                                           \
             : "=r" (__dst)                                                     \
             : "r" (src1), "r" (src2));                                         \
             __dst;})


#define __ide_mulrcc(src1, src2) ({int  __dst;                                  \
    __asm__ __volatile__ ( intrinsic_3OP("mulrcc", 0x07, 0x1)                   \
             " mulrcc  %0, %1, %2\n\t"                                          \
             : "=r" (__dst)                                                     \
             : "r" (src1), "r" (src2));                                         \
             __dst;})

#define __ide_mulcj(src1, src2) ({int  __dst;                                   \
    __asm__ __volatile__ ( intrinsic_3OP("mulcj", 0x07, 0x2)                    \
             " mulcj %0, %1, %2\n\t"                                            \
             : "=r" (__dst)                                                     \
             : "r" (src1), "r" (src2));                                         \
             __dst;})

#define __ide_mulrcj(src1, src2) ({int  __dst;                                  \
    __asm__ __volatile__ ( intrinsic_3OP("mulrcj", 0x07, 0x3)                   \
             " mulrcj %0, %1, %2\n\t"                                           \
             : "=r" (__dst)                                                     \
             : "r" (src1), "r" (src2));                                         \
             __dst;})

#define __ide_maccc(src1, src2) ({int  __dst;                                   \
    __asm__ __volatile__ ( intrinsic_3OP("maccc", 0x07, 0x24)                   \
             " maccc  %0, %1, %2\n\t"                                           \
             : "=r" (__dst)                                                     \
             : "r" (src1), "r" (src2));                                         \
             __dst;})

#define __ide_macrcc(src1, src2) ({int  __dst;                                  \
    __asm__ __volatile__ ( intrinsic_3OP("macrcc", 0x07, 0x09)                  \
             " macrcc  %0, %1, %2\n\t"                                          \
             : "=r" (__dst)                                                     \
             : "r" (src1), "r" (src2));                                         \
             __dst;})

#define __ide_maccj(src1, src2) ({int  __dst;                                   \
    __asm__ __volatile__ ( intrinsic_3OP("maccj", 0x07, 0x0A)                   \
             " maccj  %0, %1, %2\n\t"                                           \
             : "=r" (__dst)                                                     \
             : "r" (src1), "r" (src2));                                         \
             __dst;})

#define __ide_macrcj(src1, src2) ({int  __dst;                                  \
    __asm__ __volatile__ ( intrinsic_3OP("macrcj", 0x07, 0x0B)                  \
             " macrcj  %0, %1, %2\n\t"                                          \
             : "=r" (__dst)                                                     \
             : "r" (src1), "r" (src2));                                         \
             __dst;})


/*******************************************************************************
 *
 * Dual MAC instructions
 *
 *******************************************************************************/

#define __ide_mul2w(src1, src2) ({int  __dst;                                   \
    __asm__ __volatile__ ( intrinsic_3OP("mul2w", 0x07, 0x21)                   \
             " mul2w   %0, %1, %2\n\t"                                          \
             : "=r" (__dst)                                                     \
             : "r" (src1), "r" (src2));                                         \
             __dst;})

#define __ide_mulr2w(src1, src2) ({int  __dst;                                  \
    __asm__ __volatile__ ( intrinsic_3OP("mulr2w", 0x07, 0x22)                  \
             " mulr2w   %0, %1, %2\n\t"                                         \
             : "=r" (__dst)                                                     \
             : "r" (src1), "r" (src2));                                         \
             __dst;})

#define __ide_mac2w(src1, src2) ({int  __dst;                                   \
    __asm__ __volatile__ ( intrinsic_3OP("mac2w", 0x07, 0x0C)                   \
             " mac2w   %0, %1, %2\n\t"                                          \
             : "=r" (__dst)                                                     \
             : "r" (src1), "r" (src2));                                         \
             __dst;})

#define __ide_macr2w(src1, src2) ({int  __dst;                                  \
    __asm__ __volatile__ ( intrinsic_3OP("macr2w", 0x07, 0x0D)                  \
             " macr2w   %0, %1, %2\n\t"                                         \
             : "=r" (__dst)                                                     \
             : "r" (src1), "r" (src2));                                         \
             __dst;})

#define __ide_macdwo(src1, src2) ({int  __dst;                                  \
    __asm__ __volatile__ ( intrinsic_3OP("macdwo", 0x07, 0x0E)                  \
             " macdwo   %0, %1, %2\n\t"                                         \
             : "=r" (__dst)                                                     \
             : "r" (src1), "r" (src2));                                         \
             __dst;})

#define __ide_macrdwo(src1, src2) ({int  __dst;                                 \
    __asm__ __volatile__ ( intrinsic_3OP("macrdwo", 0x07, 0x0F)                 \
             " macrdwo   %0, %1, %2\n\t"                                        \
             : "=r" (__dst)                                                     \
             : "r" (src1), "r" (src2));                                         \
             __dst;})

#define __ide_macdwe(src1, src2) ({int  __dst;                                  \
    __asm__ __volatile__ ( intrinsic_3OP("macdwe", 0x07, 0x14)                  \
             " macdwe   %0, %1, %2\n\t"                                         \
             : "=r" (__dst)                                                     \
             : "r" (src1), "r" (src2));                                         \
             __dst;})

#define __ide_macrdwe(src1, src2) ({int  __dst;                                 \
    __asm__ __volatile__ ( intrinsic_3OP("macrdwe", 0x07, 0x15)                 \
             " macrdwe   %0, %1, %2\n\t"                                        \
             : "=r" (__dst)                                                     \
             : "r" (src1), "r" (src2));                                         \
             __dst;})


#define __ide_macrdwe(src1, src2) ({int  __dst;                                 \
    __asm__ __volatile__ ( intrinsic_3OP("macrdwe", 0x07, 0x15)                 \
             " macrdwe   %0, %1, %2\n\t"                                        \
             : "=r" (__dst)                                                     \
             : "r" (src1), "r" (src2));                                         \
             __dst;})

#define __ide_macdwz(src1, src2) ({int  __dst;                                  \
    __asm__ __volatile__ ( intrinsic_3OP("macdwz", 0x07, 0x28)                  \
             " macdwz   %0, %1, %2\n\t"                                         \
             : "=r" (__dst)                                                     \
             : "r" (src1), "r" (src2));                                         \
             __dst;})

#define __ide_macrdwz(src1, src2) ({int  __dst;                                 \
    __asm__ __volatile__ ( intrinsic_3OP("macrdwz", 0x07, 0x29)                 \
             " macrdwz   %0, %1, %2\n\t"                                        \
             : "=r" (__dst)                                                     \
             : "r" (src1), "r" (src2));                                         \
             __dst;})

/*******************************************************************************
 *
 * Multiply and Accumulate instructions - 16x16 and 32x16
 *
 *******************************************************************************/

#define __ide_mpy16x16_ll(src1, src2) ({int  __dst;                             \
    __asm__ __volatile__ ( intrinsic_3OP("mpy16x16_ll", 0x07, 0x3E)             \
             " mpy16x16_ll %0, %1, %2\n\t"                                      \
             : "=r" (__dst)                                                     \
             : "r" (src1), "r" (src2));                                         \
             __dst;})

#define __ide_mac16x16_ll(src1, src2) ({int  __dst;                             \
    __asm__ __volatile__ ( intrinsic_3OP("mac16x16_ll", 0x07, 0x37)             \
             " mac16x16_ll %0, %1, %2\n\t"                                      \
             : "=r" (__dst)                                                     \
             : "r" (src1), "r" (src2));                                         \
             __dst;})

#define __ide_msub16x16_ll(src1, src2) ({int  __dst;                            \
    __asm__ __volatile__ ( intrinsic_3OP("msub16x16_ll", 0x07, 0x3B)            \
             " msub16x16_ll %0, %1, %2\n\t"                                     \
             : "=r" (__dst)                                                     \
             : "r" (src1), "r" (src2));                                         \
             __dst;})

#define __ide_mpy16x16_hl(src1, src2) ({int  __dst;                             \
    __asm__ __volatile__ ( intrinsic_3OP("mpy16x16_hl", 0x07, 0x31)             \
             " mpy16x16_hl %0, %1, %2\n\t"                                      \
             : "=r" (__dst)                                                     \
             : "r" (src1), "r" (src2));                                         \
             __dst;})

#define __ide_mac16x16_hl(src1, src2) ({int  __dst;                             \
    __asm__ __volatile__ ( intrinsic_3OP("mac16x16_hl", 0x07, 0x32)             \
             " mac16x16_hl %0, %1, %2\n\t"                                      \
             : "=r" (__dst)                                                     \
             : "r" (src1), "r" (src2));                                         \
             __dst;})


#define __ide_msub16x16_hl(src1, src2) ({int  __dst;                            \
    __asm__ __volatile__ ( intrinsic_3OP("msub16x16_hl", 0x07, 0x35)            \
             " msub16x16_hl %0, %1, %2\n\t"                                     \
             : "=r" (__dst)                                                     \
             : "r" (src1), "r" (src2));                                         \
             __dst;})

#define __ide_mpy16x16_hh(src1, src2) ({int  __dst;                             \
    __asm__ __volatile__ ( intrinsic_3OP("mpy16x16_hh", 0x07, 0x30)             \
             " mpy16x16_hl %0, %1, %2\n\t"                                      \
             : "=r" (__dst)                                                     \
             : "r" (src1), "r" (src2));                                         \
             __dst;})

#define __ide_mac16x16_hh(src1, src2) ({int  __dst;                             \
    __asm__ __volatile__ ( intrinsic_3OP("mac16x16_hh", 0x07, 0x36)             \
             " mac16x16_hh %0, %1, %2\n\t"                                      \
             : "=r" (__dst)                                                     \
             : "r" (src1), "r" (src2));                                         \
             __dst;})

#define __ide_msub16x16_hh(src1, src2) ({int  __dst;                            \
    __asm__ __volatile__ ( intrinsic_3OP("msub16x16_hh", 0x07, 0x38)            \
             " msub16x16_hh %0, %1, %2\n\t"                                     \
             : "=r" (__dst)                                                     \
             : "r" (src1), "r" (src2));                                         \
             __dst;})

#define __ide_mpy32x16_fl(src1, src2) ({int  __dst;                             \
    __asm__ __volatile__ ( intrinsic_3OP("mpy32x16_fl", 0x07, 0x33)             \
             " mpy32x16_fl %0, %1, %2\n\t"                                      \
             : "=r" (__dst)                                                     \
             : "r" (src1), "r" (src2));                                         \
             __dst;})

#define __ide_mac32x16_fl(src1, src2) ({int  __dst;                             \
    __asm__ __volatile__ ( intrinsic_3OP("mac32x16_fl", 0x07, 0x39)             \
             " mac32x16_fl %0, %1, %2\n\t"                                      \
             : "=r" (__dst)                                                     \
             : "r" (src1), "r" (src2));                                         \
             __dst;})

#define __ide_msub32x16_fl(src1, src2) ({int  __dst;                            \
    __asm__ __volatile__ ( intrinsic_3OP("msub32x16_fl", 0x07, 0x3A)            \
             " msub32x16_fl %0, %1, %2\n\t"                                     \
             : "=r" (__dst)                                                     \
             : "r" (src1), "r" (src2));                                         \
             __dst;})

#define __ide_mpy32x16_fh(src1, src2) ({int  __dst;                             \
    __asm__ __volatile__ ( intrinsic_3OP("mpy32x16_fh", 0x07, 0x34)             \
             " mpy32x16_fh %0, %1, %2\n\t"                                      \
             : "=r" (__dst)                                                     \
             : "r" (src1), "r" (src2));                                         \
             __dst;})

#define __ide_mac32x16_fh(src1, src2) ({int  __dst;                             \
    __asm__ __volatile__ ( intrinsic_3OP("mac32x16_fh", 0x07, 0x3C)             \
             " mac32x16_fh %0, %1, %2\n\t"                                      \
             : "=r" (__dst)                                                     \
             : "r" (src1), "r" (src2));                                         \
             __dst;})

#define __ide_msub32x16_fh(src1, src2) ({int  __dst;                            \
    __asm__ __volatile__ ( intrinsic_3OP("msub32x16_fh", 0x07, 0x3D)            \
             " msub32x16_fh %0, %1, %2\n\t"                                     \
             : "=r" (__dst)                                                     \
             : "r" (src1), "r" (src2));                                         \
             __dst;})

/*******************************************************************************
 *
 * Helper Macros
 *
 *******************************************************************************/

#define fdiv_hw __ide_dsp_fp_div
#define f2i_hw __ide_dsp_fp_flt2i
#define i2f_hw __ide_dsp_fp_i2flt
#define fsqrt_hw __ide_dsp_fp_sqrt
#define fcmp_hw __ide_dsp_fp_cmp       //return: 4 if src1>src2, 2 if src1<src2, 1 if src1==src2
#define fcmp_eq_hw __ide_dsp_fp_cmp_eq //return: 1 equal, otherwise 0
#define fcmp_lt_hw __ide_dsp_fp_cmp_lt //return: 1 less than, otherwise 0
#define fcmp_gt_hw __ide_dsp_fp_cmp_gt //return: 1 greater than, otherwise 0

/* *INDENT-ON* */
