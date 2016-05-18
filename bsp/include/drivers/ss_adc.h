/*******************************************************************************
 *
 * Synopsys DesignWare Sensor and Control IP Subsystem IO Software Driver and
 * documentation (hereinafter, "Software") is an Unsupported proprietary work
 * of Synopsys, Inc. unless otherwise expressly agreed to in writing between
 * Synopsys and you.
 *
 * The Software IS NOT an item of Licensed Software or Licensed Product under
 * any End User Software License Agreement or Agreement for Licensed Product
 * with Synopsys or any supplement thereto. You are permitted to use and
 * redistribute this Software in source and binary forms, with or without
 * modification, provided that redistributions of source code must retain this
 * notice. You may not view, use, disclose, copy or distribute this file or
 * any information contained herein except pursuant to this license grant from
 * Synopsys. If you do not agree with this notice, including the disclaimer
 * below, then you are not authorized to use the Software.
 *
 * THIS SOFTWARE IS BEING DISTRIBUTED BY SYNOPSYS SOLELY ON AN "AS IS" BASIS
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE HEREBY DISCLAIMED. IN NO EVENT SHALL SYNOPSYS BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH
 * DAMAGE.
 *
 ******************************************************************************/

/*******************************************************************************
 *
 * Modifications Copyright (c) 2015, Intel Corporation. All rights reserved.
 *
 ******************************************************************************/

#ifndef SS_ADC_H_
#define SS_ADC_H_

#include "drivers/data_type.h"
#include "infra/device.h"
#include "drivers/clk_system.h"

/**
 * @defgroup adc_arc_driver ADC Driver
 * ARC Analog/Digital Converter driver API.
 *
 * <table>
 * <tr><th><b>Include file</b><td><tt> \#include "drivers/ss_adc.h"</tt>
 * <tr><th><b>Source path</b> <td><tt>bsp/src/drivers/adc</tt>
 * <tr><th><b>Config flag</b> <td><tt>SS_ADC</tt>
 * </table>
 *
 * @note
 *    To get ADC value at a higher level, see \ref adc_service.
 *
 * @ingroup arc_drivers
 * @{
 */

/**
 * SS ADC power management driver.
 */
extern struct driver ss_adc_driver;

typedef enum {
	SINGLED_ENDED = 0,
	DIFFERENTIAL = 1
} INPUT_MODE;

typedef enum {
	PARALLEL = 0,
	SERIAL = 1
} OUTPUT_MODE;

typedef enum {
	SINGLESHOT = 0,
	REPETITIVE = 1
}SEQ_MODE;


typedef enum {
	RISING_EDGE = 0,
	FALLING_EDGE = 1
} CAPTURE_MODE;

typedef enum {
	WIDTH_6_BIT = 0x0,
	WIDTH_8_BIT = 0x1,
	WIDTH_10_BIT = 0x2,
	WIDTH_12_BIT = 0x3
} SAMPLE_WIDTH;

/**
 *  Callback function signature
 */
typedef void (*adc_callback)(uint32_t, void *);

/* Structure representing AD converter configuration */
typedef struct {
	INPUT_MODE in_mode;          /**< ADC input mode: single ended or differential */
	OUTPUT_MODE out_mode;        /**< ADC output mode: parallel or serial */
	uint8_t serial_dly;          /**< Number of adc_clk the first bit of serial output is delayed for after start of conversion */
	CAPTURE_MODE capture_mode;   /**< ADC controller capture mode: by rising or falling edge of adc_clk */
	SAMPLE_WIDTH sample_width;   /**< ADC sample width */
	SEQ_MODE seq_mode;           /**< ADC sequence mode - single run/repetitive */
	uint32_t clock_ratio;        /**< TODO */
	adc_callback cb_rx;          /**< Callback function for notification of data received and available to application, NULL if callback not required by application code */
	adc_callback cb_err;         /**< Callback function on transaction error, NULL if callback not required by application code */
	void *priv;                  /**< This will be passed back by the callback routine - can be used to keep information needed in the callback */
} ss_adc_cfg_data_t;

/* Simple macro to convert resolution to sample width to be used in the
 * configuration structure. It converts from 6,8,10,12 to 0x0,0x01,0x2,0x3
 */
#define ss_adc_res_to_sample_width(_res_) \
	((SAMPLE_WIDTH)(((_res_ - 6) / 2) & 0x3))

#define ADC_VREF 3300 /* mV = 3.3V */
/* Result in mV is given by converting raw data:
 * Result = (data * ADC_VREF) / (2^resolution)
 */
#define ss_adc_data_to_mv(_data_, _resolution_)	\
	((_data_ * ADC_VREF) / (1 << _resolution_))


#define ADC_BUFS_NUM               (2)
#define ADC_RESOLUTION            (12)

/**
 * Internal context used by the device driver
 */
struct adc_info_t {
	uint32_t reg_base;       /* Base address of device register set */
	uint32_t creg_slv;       /* Address of creg slave control register */
	uint32_t creg_mst;       /* Address of creg master control register */
	uint8_t state;
	uint8_t seq_mode;
	uint8_t index;
	uint32_t seq_size;
	uint32_t rx_len;
	uint32_t *rx_buf[ADC_BUFS_NUM];
	/* Interrupt numbers and handlers */
	uint8_t rx_vector; /* ISR vectors */
	uint8_t err_vector;
	uint16_t fifo_tld;
	uint16_t fifo_depth;
	/* SSS Interrupt Routing Mask Registers */
	uint32_t adc_irq_mask;
	uint32_t adc_err_mask;
	ss_adc_cfg_data_t cfg;
	T_MUTEX adc_in_use; /* whether the controller is in use */
	struct clk_gate_info_s *clk_gate_info;
};

/** Read ADC channel
 *
 *  @param  channel_id      ADC channel
 *  @param  result_value    Buffer where to return value
 *  @return : Status of the ADC read
 *
 */
DRIVER_API_RC ss_adc_read(uint8_t channel_id, uint16_t *result_value);

/** @} */

#endif  /* SS_ADC_H_ */
