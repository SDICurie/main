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
 */

#include "infra/log.h"
#include "infra/part.h"
#include "infra/partition.h"
#include "util/workqueue.h"
#include "util/assert.h"
#include "drivers/soc_flash.h"
#include "usb.h"
#include "usb_driver_interface.h"
#include "msc_function.h"
#include "scsi_command_handler.h"
#include "msc_file_system.h"
#include "drivers/msc/emfat.h"
#include "msc.h"

#include "drivers/soc_flash.h"
#include "drivers/soc_rom.h"
#include "util/cunit_test.h"

#include "machine.h"
#include "storage.h"
#include "project_mapping.h"

/* Flash storage */
#include "drivers/spi_flash.h"
#include "soc_config.h"
#include "drivers/serial_bus_access.h"

#include "machine/soc/intel/quark_se/quark_se_mapping.h"

//#define MSC_DEBUG

#ifdef MSC_DEBUG
#define pr_debug_msc(...) pr_debug(LOG_MODULE_USB, __VA_ARGS__)
#define pr_fname() pr_debug(LOG_MODULE_USB, "%s\n", __func__)
#else
#define pr_debug_msc(...) \
	do {\
} while (0)
#define pr_fname() \
	do {\
} while (0)
#endif

#define CONFIG_USB_MSC_VID CONFIG_USB_VENDOR_ID
#define CONFIG_USB_MSC_PID CONFIG_USB_PRODUCT_ID
#define CONFIG_MSC_EMFAT_DEVICE_NAME CONFIG_USB_MSC_DEVICE_NAME

#define PARTIOTION_COUNT (msc_partition_config_count)
#define MAX_ENTRY_COUNT (PARTIOTION_COUNT+2) // one for root directory and one for NULL entry

#define GET_MAX_LUN_REQUEST 0xFE

#define MSC_MAX_PACKET 64
#define MSC_EP_COUNT 2

#define MSC_CONF_SIZE (USB_CONFIG_DESCRIPTOR_SIZE \
+ (USB_INTERFACE_DESCRIPTOR_SIZE) \
+ USB_ENDPOINT_DESCRIPTOR_SIZE * MSC_EP_COUNT)

#define EP0_BUFFER_SIZE 64

typedef struct
{
	uint8_t *dest;
	uint32_t length;
	uint32_t offset;
	uint32_t start_address;
	uint8_t storage_id;
} partition_info_struct_t;

struct msc_usb_descriptor
{
	usb_config_descriptor_t config_descriptor;
	usb_interface_descriptor_t interface_descriptor;
	usb_endpoint_descriptor_t endpoint_descriptor[MSC_EP_COUNT];
} UPACKED;

static usb_device_descriptor_t msc_device_desc =
{
	.bLength = USB_DEVICE_DESCRIPTOR_SIZE,
	.bDescriptorType = 0x01,
	.bcdUSB[0] = UD_USB_2_0 & 0xff,
	.bcdUSB[1] = (UD_USB_2_0 >> 8) & 0xff,
	.bDeviceClass = 0x00,
	.bDeviceSubClass = 0x00,
	.bDeviceProtocol = 0x00,
	.bMaxPacketSize = 64,
	.idVendor[0] = ((CONFIG_USB_MSC_VID) & 0xff),
	.idVendor[1] = (((CONFIG_USB_MSC_VID) >> 8) & 0xff),
	.idProduct[0] = ((CONFIG_USB_MSC_PID) & 0xff),
	.idProduct[1] = (((CONFIG_USB_MSC_PID) >> 8) & 0xff),
	.bcdDevice[0] = 0x87,
	.bcdDevice[1] = 0x80,
	/* STRING_MANUFACTURER */
	.iManufacturer = 1,
	/* STRING_PRODUCT */
	.iProduct = 2,
	/* STRING_SERIAL */
	.iSerialNumber = 3,
	.bNumConfigurations = 1
};

static usb_endpoint_descriptor_t endpoint_descriptor[MSC_EP_COUNT] = {
	{
		.bLength = USB_ENDPOINT_DESCRIPTOR_SIZE,
		.bDescriptorType = 0x05,
		.bEndpointAddress = MSD_IN_EP_ADDR,
		.bmAttributes = UE_BULK,
		.wMaxPacketSize[0] = MSC_MAX_PACKET,
		.wMaxPacketSize[1] = 0,
		.bInterval = 0x00,
	},
	{
		.bLength = USB_ENDPOINT_DESCRIPTOR_SIZE,
		.bDescriptorType = 0x05,
		.bEndpointAddress = MSD_OUT_EP_ADDR,
		.bmAttributes = UE_BULK,
		.wMaxPacketSize[0] = MSC_MAX_PACKET,
		.wMaxPacketSize[1] = 0,
		.bInterval = 0x00,
	},
};

static struct msc_usb_descriptor msc_config_desc = {
	.config_descriptor = {
		.bLength = USB_CONFIG_DESCRIPTOR_SIZE,
		.bDescriptorType = 0x2,
		.wTotalLength[0] = MSC_CONF_SIZE & 0xff,
		.wTotalLength[1] = (MSC_CONF_SIZE >> 8) & 0xff,
		.bNumInterface = 1,
		.bConfigurationValue = 1,
		.iConfiguration = 0x04,
		.bmAttributes = UC_BUS_POWERED | UC_SELF_POWERED,
		.bMaxPower = 50, /* max current in 2mA units */
	},
	.interface_descriptor = {
		.bLength = USB_INTERFACE_DESCRIPTOR_SIZE,
		.bDescriptorType = 0x4,
		.bInterfaceNumber = 0,
		.bAlternateSetting = 0,
		.bNumEndpoints = 2,
		.bInterfaceClass = 0x08,
		.bInterfaceSubClass = 0x06,
		.bInterfaceProtocol = 0x50,
		.iInterface = 0x05,
	},
	.endpoint_descriptor = {
		{
			.bLength = USB_ENDPOINT_DESCRIPTOR_SIZE,
			.bDescriptorType = 0x05,
			.bEndpointAddress = MSD_IN_EP_ADDR,
			.bmAttributes = UE_BULK,
			.wMaxPacketSize[0] = MSC_MAX_PACKET,
			.wMaxPacketSize[1] = 0,
			.bInterval = 0x00,
		},
		{
			.bLength = USB_ENDPOINT_DESCRIPTOR_SIZE,
			.bDescriptorType = 0x05,
			.bEndpointAddress = MSD_OUT_EP_ADDR,
			.bmAttributes = UE_BULK,
			.wMaxPacketSize[0] = MSC_MAX_PACKET,
			.wMaxPacketSize[1] = 0,
			.bInterval = 0x00,
		}
	},
};

static usb_string_descriptor_t msc_strings_desc[] = {
	{
		/*String descriptor language, only one, so min size 4 bytes */
		/* 0x0409 English(US) language code used */
		.bLength = 4,
		.bDescriptorType = UDESC_STRING,
		.bString = { {0x09, 0x04}, }
	},
	{
		.bLength = sizeof("Intel MSC") * 2,
		.bDescriptorType = UDESC_STRING,
		.bString = { {'I', 0}, {'n', 0}, {'t', 0}, {'e', 0}, {'l', 0}, {' ', 0} , {'M', 0}, {'S', 0}, {'C', 0}, }
	},
	{
		.bLength = sizeof("CURIECRB MSC") * 2,
		.bDescriptorType = UDESC_STRING,
		.bString = { {'C', 0}, {'U', 0}, {'R', 0}, {'I', 0}, {'E', 0}, {'C', 0}, {'R', 0}, {'B', 0} , {' ', 0} , {'M', 0}, {'S', 0}, {'C', 0}, }
	},
	{
		.bLength = sizeof("123456789COCA") * 2,
		.bDescriptorType = UDESC_STRING,
		.bString = { {'1', 0}, {'2', 0}, {'3', 0}, {'4', 0}, {'5', 0}, {'6', 0}, {'7', 0}, {'8', 0}, {'9', 0}, {'C', 0}, {'0', 0}, {'C', 0}, {'A', 0}, }
	},
	{
		.bLength = sizeof("MSC Config") * 2,
		.bDescriptorType = UDESC_STRING,
		.bString = { {'M', 0}, {'S', 0}, {'C', 0}, {' ', 0}, {'C', 0}, {'o', 0}, {'n', 0}, {'f', 0}, {'i', 0}, {'g', 0 }, }
	},
	{
		.bLength = sizeof("MSC Interface") * 2,
		.bDescriptorType = UDESC_STRING,
		.bString = {{'M', 0}, {'S', 0}, {'C', 0}, {' ', 0}, {'I', 0}, {'n', 0}, {'t', 0}, {'e', 0}, {'r', 0}, {'f', 0}, {'a', 0}, {'c', 0}, {'e', 0}, }
	}
};

uint8_t ep0_buffer[EP0_BUFFER_SIZE];
endpoint_handle_t ep_handler_params;
static partition_info_struct_t partition_info;
//static emfat_entry_t *entries = NULL;
static emfat_entry_t entries[20];
emfat_t emfat; //FAT32 file system emulation


void usb_ep_complete(int ep_address, void *priv, int status, int actual);
int class_handle_req(usb_device_request_t *setup_packet, uint32_t *len, uint8_t **data);
void usb_event_cb(struct usb_event *evt);

// emfat read callback
static void file_read_proc(uint8_t *dest, int size, uint32_t offset, size_t userdata);

static struct usb_interface_init_data msc_init_data = {
	.ep_complete = usb_ep_complete,
	.class_handler = class_handle_req,
	.usb_evt_cb = usb_event_cb,
	.ep0_buffer = &(ep0_buffer[0]),
	.ep0_buffer_size = EP0_BUFFER_SIZE,
	.dev_desc = &msc_device_desc,
	.conf_desc = (usb_config_descriptor_t *)&msc_config_desc,
	.conf_desc_size = sizeof(msc_config_desc),
	.strings_desc = msc_strings_desc,
	.num_strings = sizeof(msc_strings_desc) /
	sizeof(usb_string_descriptor_t),
	.eps = (usb_endpoint_descriptor_t *)(endpoint_descriptor),
	.num_eps = MSC_EP_COUNT,
};


void ep_complete_work_callback(void *arg)
{
	endpoint_handle_t *params = (endpoint_handle_t *)arg;

#ifdef MSC_DEBUG
	pr_debug_msc("function param:");
	pr_debug_msc("ep_address: 0x%x\n", params->ep_address);
	pr_debug_msc("status: 0x%x\n", params->status);
	pr_debug_msc("priv: 0x%x\n", params->priv);
	pr_debug_msc("actual length: %i\n", params->actual);

	if (params->priv != NULL) {
		pr_debug_msc("priv is not null\n");
	}

	if (params->status != 0) {
		pr_debug_msc("status is not null: 0x%x\n");
	}
#endif

	if (params->ep_address == MSD_IN_EP_ADDR) {
		msc_handle_data_to_host(params, MSD_IN_EP_ADDR);
	}
	else if (params->ep_address == MSD_OUT_EP_ADDR) {
			msc_handle_data_from_host(params, MSD_OUT_EP_ADDR);
		}
		else {
			pr_debug_msc("Uknown EP\n");
		}


}

void usb_ep_complete(int ep_address, void *priv, int status, int actual)
{
	ep_handler_params.ep_address = ep_address;
	ep_handler_params.priv = priv;
	ep_handler_params.status = status;
	ep_handler_params.actual = actual;

	workqueue_queue_work(ep_complete_work_callback, &ep_handler_params);

	pr_debug_msc("usb_ep_complete Work queued!\n");

}

int class_handle_req(usb_device_request_t *setup_packet, uint32_t *len, uint8_t **data)
{
#ifdef MSC_DEBUG
	pr_debug_msc("setup_packet:\n");

	pr_debug_msc("bmRequestType: 0x%x\n", setup_packet->bmRequestType);
	pr_debug_msc("bRequest: 0x%x\n", setup_packet->bRequest);

	pr_debug_msc("wValue: 0x%x\n", UGETW(setup_packet->wValue));
	pr_debug_msc("wIndex: 0x%x\n", UGETW(setup_packet->wIndex));
	pr_debug_msc("wLength: 0x%x\n", UGETW(setup_packet->wLength));

	pr_debug_msc("\ndata length: 0x%x\n", *len);
#endif

	// Response to Get Max Lun Request
	if (setup_packet->bRequest == GET_MAX_LUN_REQUEST && setup_packet->bmRequestType == UT_READ_CLASS_INTERFACE) {
		pr_debug_msc("Get Max LUN request\n");
		unsigned char *buffer = *data;
		buffer[0]= msc_get_max_lun();
		*len=1;
	}

	// Handle Clear Feature request in MSC function code
	if(setup_packet->bRequest == UR_CLEAR_FEATURE && setup_packet->bmRequestType == UT_WRITE_ENDPOINT) {
		uint8_t ep_number = UGETW(setup_packet->wIndex);
		pr_debug_msc("\nMSC_BOT_CplClrFeature with Endpoint 0x%x\n", ep_number);
		msc_handle_clear_feature(ep_number);
	}

	// Other requests are not handled by MSC driver

	return 0;
}

void usb_event_cb(struct usb_event *evt)
{
	if (evt->event == USB_EVENT_SET_CONFIG) {
		pr_debug_msc("msc_init:\n");
		msc_init();
	}
}

static int32_t soc_flash_read_byte(uint32_t address, uint32_t data_size, uint8_t *data)
{
	unsigned int data_read_len;
	uint32_t last_data;

	/* Read integral count of 4 bytes block as flash driver read data by 4 bytes */
	if (data_size / 4) {
		if (soc_flash_read(address,
							data_size / 4,
							&data_read_len,
							(uint32_t *)data) != DRV_RC_OK) {
			pr_error(LOG_MODULE_MAIN, "SOC Read Failed");
			return -1;
		}
	}

	/* If data_size is not 4 bytes aligned, read last remaining 4 bytes and copy only required bytes in data */
	if (data_size % 4) {
		if (soc_flash_read(address + data_size - data_size % 4,
							1,
							&data_read_len,
							&last_data) != DRV_RC_OK) {
			pr_error(LOG_MODULE_MAIN, "SOC Read Failed");
			return -1;
		}
		memcpy(&data[data_size - data_size % 4], (uint8_t *)&last_data, data_size % 4);
	}
	return 0;
}

static void read_partition(void *priv)
{
	unsigned int retLen = 0;
	int retVal = -1;

	assert(priv != NULL);

	partition_info_struct_t *param = (partition_info_struct_t*) priv;

	switch(param->storage_id) {
	case INTERNAL_FLASH_0:
	{
		retVal = soc_flash_read_byte(param->start_address + param->offset,
									param->length,
									param->dest);
		break;
	}

	case SPI_FLASH_0:
	{
		struct td_device *spi = (struct td_device *)&pf_sba_device_flash_spi0;
		retVal = spi_flash_read_byte(spi,
									param->start_address + param->offset,
									param->length,
									&retLen,
									param->dest);
		break;
	}

	default:
	{
		pr_warning(LOG_MODULE_USB, "MSC partition storage_id is invalid");
		break;
	}
	}

	pr_debug_msc("SOC flash retVal is 0x%x", retVal);
	pr_debug_msc("SOC flash retLen is 0x%x", retLen);

}

void file_read_proc(uint8_t *dest, int size, uint32_t offset, size_t userdata)
{
	pr_fname();
	int len = 0;

	uint8_t partition_id = userdata;
	pr_debug_msc("partition_id aka userdata is %d\n", partition_id);

	/* Retrieve other information of the partition */
	struct partition *flash_partition = msc_storage_config[partition_id].flash_partition;

	if (offset > flash_partition->size) {
		return;
	}
	if (offset + size > flash_partition->size) {
		len = flash_partition->size - offset;
	}
	else {
		len = size;
	}

	assert(dest != NULL);

	partition_info.dest= dest;
	partition_info.length = len;
	partition_info.offset = offset;

	if (!flash_partition) {
		pr_error(LOG_MODULE_USB, "flash_partition is NULL");
		return;
	}

	partition_info.start_address = flash_partition->start;
	partition_info.storage_id=flash_partition->storage->id;
	read_partition(&partition_info);
}

void add_root_dir()
{
	if (entries == NULL) {
		return;
	}

	entries[0].name = "";
	entries[0].dir = true;
	entries[0].level = 0;
	entries[0].offset = 0;
	entries[0].curr_size = 0;
	entries[0].max_size = 0;
	entries[0].user_data = 0;
	entries[0].readcb = NULL;
	entries[0].writecb = NULL;
}

void add_storage_files()
{
	struct partition *flash_partition = NULL;
	static emfat_entry_t* entry = NULL;
	int last_entry = 1;

	add_root_dir(NULL);

	for (uint8_t i = 0; i < msc_partition_config_count && last_entry < MAX_ENTRY_COUNT; i++) {

		flash_partition = partition_get(msc_storage_config[i].partition_id);

		if (flash_partition == NULL) {
			pr_debug_msc("partition is NULL\n");
			continue;
		}

		msc_storage_config[i].flash_partition = flash_partition;

		entry = &(entries[last_entry]);

		entry->name = msc_storage_config[i].filename;
		pr_debug_msc("partition name in entry is %s\n", entry->name);
		entry->dir = false;
		entry->level = 1;
		entry->offset = 0;
		entry->curr_size = flash_partition->size;
		entry->max_size = entry->curr_size;
		entry->user_data = i;
		entry->readcb = file_read_proc;
		entry->writecb = NULL;

		(last_entry)++;
	}

	entries[last_entry].name = NULL;
	last_entry++;

}

//static emfat_entry_t mass_entries[MAX_ENTRY_COUNT];

void msc_class_init(void *priv)
{
	pr_fname();
	(void)priv;

	add_storage_files();
	emfat_init(&emfat, CONFIG_MSC_EMFAT_DEVICE_NAME, entries);

	usb_interface_init((struct usb_interface_init_data *)&msc_init_data);

	pr_debug_msc("msc_class_init done\n");
}
