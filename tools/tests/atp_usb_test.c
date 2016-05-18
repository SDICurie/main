/*
 * Copyright (c) 2016, Intel Corporation. All rights reserved.
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

 *****************************************************************************
 * Compile with:
 * gcc atp_usb_test.c `pkg-config --libs --cflags libusb-1.0` -o atp_usb_test
 */

#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <libusb-1.0/libusb.h>

struct usb_bus *busses;

char buf[256];
/* This buffer shall contain exactly the same string as "out_buf" of
 * bsp/src/drivers/usb/function_drivers/usb_test_function_driver.c */
char received[] = "The USB test function driver is alive!";

int main(int argc, char **argv) {
	int i;
	for (i=0;i<256;i++) buf[i] = i;

	libusb_init(NULL);
	libusb_device_handle* devh = libusb_open_device_with_vid_pid(NULL, 0x8087, 0x0a99);
	assert(devh != NULL);
	libusb_claim_interface(devh, 0);
	int count = 5000;

	while(count--) {
		int actual = 0;
		int ret;
		ret = libusb_bulk_transfer(devh, 0x81, buf, 128, &actual, 1000);
		assert(ret == 0);
		assert(actual == sizeof(received));
		assert(strncmp(received, buf, sizeof(received)) == 0);
		actual = 0;
		ret = libusb_bulk_transfer(devh, 0x1, buf, 64, &actual, 1000);
		assert(ret == 0);
		assert(actual == 64);
	}

	printf("TEST PASSED!\n");
	libusb_close(devh);
	libusb_exit(NULL);
	return 0;
}
