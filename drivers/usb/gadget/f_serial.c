/* Ingenic JZ Fastboot Command Explain Function Driver
 *
 *  Copyright (C) 2013 Ingenic Semiconductor Co., LTD.
 *  Sun Jiwei <jwsun@ingenic.cn>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */

#include <config.h>
#include <malloc.h>
#include <common.h>
#include <errno.h>

#include <version.h>
#include <asm/unaligned.h>
#include <linux/err.h>
#include <linux/usb/ch9.h>
#include <linux/usb/gadget.h>
#include <linux/usb/composite.h>
#include <usb/lin_gadget_compat.h>


#include <linux/usb/cdc.h>
#define RET_LENGTH	64


struct f_gser {
//	struct gserial			port;
	struct usb_function              func;

	struct usb_ep			*epin;
	struct usb_ep			*epout;
	struct usb_ep			*epnotify;

	struct usb_request		*datain_req;
	struct usb_request		*dataout_req;

	char				*datain_buf;
	char				*dataout_buf;

	u8				data_id;
	u8				port_num;
	char				*name;
};


static inline struct f_gser *func_to_gser(struct usb_function *f)
{
	return container_of(f, struct f_gser, func);
}

/*-------------------------------------------------------------------------*/

#define GS_NOTIFY_INTERVAL_MS		32
#define GS_NOTIFY_MAXPACKET		10	/* notification + 2 bytes */
//#define USB_MS_TO_HS_INTERVAL(x)	(ilog2((x * 1000 / 125)) + 1)

static struct usb_interface_assoc_descriptor
acm_iad_descriptor = {
	.bLength =		sizeof acm_iad_descriptor,
	.bDescriptorType =	USB_DT_INTERFACE_ASSOCIATION,

	/* .bFirstInterface =	DYNAMIC, */
	.bInterfaceCount = 	2,	// control + data
	.bFunctionClass =	USB_CLASS_COMM,
	.bFunctionSubClass =	USB_CDC_SUBCLASS_ACM,
	.bFunctionProtocol =	USB_CDC_ACM_PROTO_AT_V25TER,
	/* .iFunction =		DYNAMIC */
};


static struct usb_interface_descriptor acm_control_interface_desc = {
	.bLength =		USB_DT_INTERFACE_SIZE,
	.bDescriptorType =	USB_DT_INTERFACE,
	/* .bInterfaceNumber = DYNAMIC */
	.bNumEndpoints =	1,
	.bInterfaceClass =	USB_CLASS_COMM,
	.bInterfaceSubClass =	USB_CDC_SUBCLASS_ACM,
	.bInterfaceProtocol =	USB_CDC_ACM_PROTO_AT_V25TER,
	/* .iInterface = DYNAMIC */
};

static struct usb_interface_descriptor acm_data_interface_desc = {
	.bLength =		USB_DT_INTERFACE_SIZE,
	.bDescriptorType =	USB_DT_INTERFACE,
	/* .bInterfaceNumber = DYNAMIC */
	.bNumEndpoints =	2,
	.bInterfaceClass =	USB_CLASS_CDC_DATA,
	.bInterfaceSubClass =	0,
	.bInterfaceProtocol =	0,
	/* .iInterface = DYNAMIC */
};

static struct usb_cdc_header_desc acm_header_desc = {
	.bLength =		sizeof(acm_header_desc),
	.bDescriptorType =	USB_DT_CS_INTERFACE,
	.bDescriptorSubType =	USB_CDC_HEADER_TYPE,
	.bcdCDC =		cpu_to_le16(0x0110),
};

static struct usb_cdc_call_mgmt_descriptor
acm_call_mgmt_descriptor = {
	.bLength =		sizeof(acm_call_mgmt_descriptor),
	.bDescriptorType =	USB_DT_CS_INTERFACE,
	.bDescriptorSubType =	USB_CDC_CALL_MANAGEMENT_TYPE,
	.bmCapabilities =	0,
	/* .bDataInterface = DYNAMIC */
};

static struct usb_cdc_acm_descriptor acm_descriptor = {
	.bLength =		sizeof(acm_descriptor),
	.bDescriptorType =	USB_DT_CS_INTERFACE,
	.bDescriptorSubType =	USB_CDC_ACM_TYPE,
	.bmCapabilities =	USB_CDC_CAP_LINE,
};

static struct usb_cdc_union_desc acm_union_desc = {
	.bLength =		sizeof(acm_union_desc),
	.bDescriptorType =	USB_DT_CS_INTERFACE,
	.bDescriptorSubType =	USB_CDC_UNION_TYPE,
	/* .bMasterInterface0 =	DYNAMIC */
	/* .bSlaveInterface0 =	DYNAMIC */
};



/* high speed support: */
static struct usb_endpoint_descriptor acm_hs_notify_desc = {
	.bLength =		USB_DT_ENDPOINT_SIZE,
	.bDescriptorType =	USB_DT_ENDPOINT,
	.bEndpointAddress =	USB_DIR_IN,
	.bmAttributes =		USB_ENDPOINT_XFER_INT,
	.wMaxPacketSize =	cpu_to_le16(GS_NOTIFY_MAXPACKET),
//	.bInterval =		USB_MS_TO_HS_INTERVAL(GS_NOTIFY_INTERVAL_MS),
	.bInterval =		9,
};

static struct usb_endpoint_descriptor acm_hs_in_desc = {
	.bLength =		USB_DT_ENDPOINT_SIZE,
	.bDescriptorType =	USB_DT_ENDPOINT,
	.bmAttributes =		USB_ENDPOINT_XFER_BULK,
	.wMaxPacketSize =	cpu_to_le16(512),
};

static struct usb_endpoint_descriptor acm_hs_out_desc = {
	.bLength =		USB_DT_ENDPOINT_SIZE,
	.bDescriptorType =	USB_DT_ENDPOINT,
	.bmAttributes =		USB_ENDPOINT_XFER_BULK,
	.wMaxPacketSize =	cpu_to_le16(512),
};


#define ACM_CTRL_IDX	0
#define ACM_DATA_IDX	1
#define ACM_IAD_IDX	2

/* static strings, in UTF-8 */
static struct usb_string acm_string_defs[] = {
	[ACM_CTRL_IDX].s = "CDC Abstract Control Model (ACM)",
	[ACM_DATA_IDX].s = "CDC ACM Data",
	[ACM_IAD_IDX ].s = "CDC Serial",
	{  } /* end of list */
};

static struct usb_gadget_strings acm_string_table = {
	.language =		0x0409,	/* en-us */
	.strings =		acm_string_defs,
};

static struct usb_gadget_strings *acm_strings[] = {
	&acm_string_table,
	NULL,
};



/* high speed support: */
static struct usb_endpoint_descriptor acm_fs_in_desc = {
	.bLength =		USB_DT_ENDPOINT_SIZE,
	.bDescriptorType =	USB_DT_ENDPOINT,
	.bEndpointAddress =	USB_DIR_IN,
	.bmAttributes =		USB_ENDPOINT_XFER_BULK,
};

static struct usb_endpoint_descriptor acm_fs_out_desc = {
	.bLength =		USB_DT_ENDPOINT_SIZE,
	.bDescriptorType =	USB_DT_ENDPOINT,
	.bEndpointAddress =	USB_DIR_OUT,
	.bmAttributes =		USB_ENDPOINT_XFER_BULK,
};


static struct usb_endpoint_descriptor acm_fs_notify_desc = {
	.bLength =		USB_DT_ENDPOINT_SIZE,
	.bDescriptorType =	USB_DT_ENDPOINT,
	.bEndpointAddress =	USB_DIR_IN,
	.bmAttributes =		USB_ENDPOINT_XFER_INT,
	.wMaxPacketSize =	cpu_to_le16(GS_NOTIFY_MAXPACKET),
	.bInterval =		GS_NOTIFY_INTERVAL_MS,
};


static struct usb_descriptor_header *gser_hs_function[] = {
	(struct usb_descriptor_header *) &acm_iad_descriptor,
	(struct usb_descriptor_header *) &acm_control_interface_desc,
	(struct usb_descriptor_header *) &acm_header_desc,
	(struct usb_descriptor_header *) &acm_call_mgmt_descriptor,
	(struct usb_descriptor_header *) &acm_descriptor,
	(struct usb_descriptor_header *) &acm_union_desc,
	(struct usb_descriptor_header *) &acm_hs_notify_desc,
	(struct usb_descriptor_header *) &acm_data_interface_desc,
	(struct usb_descriptor_header *) &acm_hs_in_desc,
	(struct usb_descriptor_header *) &acm_hs_out_desc,
	NULL,
};


/* string descriptors: */

static struct usb_string gser_string_defs[] = {
	[0].s = "Generic Serial",
	{  } /* end of list */
};

static struct usb_gadget_strings gser_string_table = {
	.language =		0x0409,	/* en-us */
	.strings =		gser_string_defs,
};

static struct usb_gadget_strings *gser_strings[] = {
	&gser_string_table,
	NULL,
};


/*-------------------------------------------------------------------------*/

static int first_enable_endpoint = 0;

static int gser_set_alt(struct usb_function *f, unsigned intf, unsigned alt)
{
	struct f_gser		*gser = func_to_gser(f);
	struct usb_composite_dev *cdev = f->config->cdev;
	int status;
	if(first_enable_endpoint == 0){

		status = usb_ep_enable(gser->epin, &acm_hs_in_desc);
		if (status < 0) {
			printf("usb enable inep in failed\n");
			goto failed;
		}

		status = usb_ep_enable(gser->epout, &acm_hs_out_desc);
		if (status < 0) {
			printf("usb enable outep in failed\n");
			goto failed;
		}


		status = usb_ep_enable(gser->epnotify, &acm_hs_notify_desc);
		if (status < 0) {
			printf("usb enable outep in failed\n");
			goto failed;
		}

		first_enable_endpoint = 1;
	}
failed:

	gser_process_handle(gser);
	return 0;
}

static void gser_disable(struct usb_function *f)
{
}


static int gser_setup(struct usb_function *f, const struct usb_ctrlrequest *ctrl)
{
	struct f_gser		*gser = func_to_gser(f);
	struct usb_composite_dev *cdev = f->config->cdev;
	struct usb_request	*req = cdev->req;
	int			value = -EOPNOTSUPP;
	int			ret = 0;
	u16			w_index = le16_to_cpu(ctrl->wIndex);
	u16			w_value = le16_to_cpu(ctrl->wValue);
	u16			w_length = le16_to_cpu(ctrl->wLength);



	switch ((ctrl->bRequestType << 8) | ctrl->bRequest) {

	case ((USB_DIR_OUT | USB_TYPE_CLASS | USB_RECIP_INTERFACE) << 8)
			| USB_CDC_REQ_SET_CONTROL_LINE_STATE:
		value = 0;
		req->zero = 0;
		req->length = value;

		debug("%s-----%d, value = %d\n", __func__, __LINE__, value);
		ret = usb_ep_queue(cdev->gadget->ep0, req, 0);
		if (ret) {
			printf("%s: Error in usb_ep_queue,%d\n", __func__, ret);
			return ret;
		}
		break;

	case ((USB_DIR_OUT | USB_TYPE_CLASS | USB_RECIP_INTERFACE) << 8)
			| USB_CDC_REQ_SET_LINE_CODING:
		value = w_length;
		req->length = value;
		debug("%s-----%d, value = 0x%x\n", __func__, __LINE__, value);
		ret = usb_ep_queue(cdev->gadget->ep0, req, 0);
		if (ret) {
			printf("%s: Error in usb_ep_queue,%d\n", __func__, ret);
			return ret;
		}
		break;

	default:
		debug("%s-----%d, default\n", __func__, __LINE__);
		break;
	}

	return value;
}



/*-------------------------------------------------------------------------*/

/* serial function driver setup/binding */

extern int usb_assign_descriptors(struct usb_function *f,
		struct usb_descriptor_header **fs,
		struct usb_descriptor_header **hs,
		struct usb_descriptor_header **ss);



static int gser_bind(struct usb_configuration *c, struct usb_function *f)
{
	struct usb_composite_dev *cdev = c->cdev;
	struct f_gser		*gser = func_to_gser(f);
	int			status;

	/* REVISIT might want instance-specific strings to help
	 * distinguish instances ...
	 */
	gser->data_id = status;
	gser->name = "ingenic";
	acm_control_interface_desc.iInterface = acm_string_defs[ACM_CTRL_IDX].id;
	acm_data_interface_desc.iInterface = acm_string_defs[ACM_DATA_IDX].id;
	acm_iad_descriptor.iFunction = acm_string_defs[ACM_IAD_IDX].id;


	status = usb_interface_id(c, f);
	if (status < 0)
		goto fail;
	acm_iad_descriptor.bFirstInterface = status;

	acm_control_interface_desc.bInterfaceNumber = status;
	acm_union_desc .bMasterInterface0 = status;
	status = usb_interface_id(c, f);
	if (status < 0)
		goto fail;
	acm_data_interface_desc.bInterfaceNumber = status;
	acm_union_desc.bSlaveInterface0 = status;
	acm_call_mgmt_descriptor.bDataInterface = status;

	gser->epin = usb_ep_autoconfig(cdev->gadget, &acm_fs_in_desc);
	if (!gser->epin)
		goto fail;

	gser->epin->driver_data = cdev;

	gser->epout = usb_ep_autoconfig(cdev->gadget, &acm_fs_out_desc);
	if (!gser->epout)
		goto fail;
	gser->epout->driver_data = cdev;

	gser->epnotify = usb_ep_autoconfig(cdev->gadget, &acm_fs_notify_desc);
	if (!gser->epnotify)
		goto fail;

	gser->epnotify->driver_data = cdev;

	acm_hs_in_desc.bEndpointAddress = acm_fs_in_desc.bEndpointAddress;
	acm_hs_out_desc.bEndpointAddress = acm_fs_out_desc.bEndpointAddress;
	acm_hs_notify_desc.bEndpointAddress = acm_fs_notify_desc.bEndpointAddress;


	status = usb_assign_descriptors(f, NULL, gser_hs_function,
			NULL);

	debug ("%s---%d, inep_addr = 0x%x, inep_name = %c%c%c%c\n",
			__func__, __LINE__,
			acm_hs_in_desc.bEndpointAddress,
			gser->epin->name[0], gser->epin->name[1],gser->epin->name[2],gser->epin->name[3]);

	debug ("%s---%d, outep_addr = 0x%x, outep_name = %c%c%c%c\n",
			__func__, __LINE__,
			acm_hs_out_desc.bEndpointAddress,
			gser->epout->name[0], gser->epout->name[1],gser->epout->name[2],gser->epout->name[3]);


	debug ("%s---%d, notifyep_addr = 0x%x, notifyep_name = %c%c%c%c\n",
			__func__, __LINE__,
			acm_hs_notify_desc.bEndpointAddress,
			gser->epnotify->name[0], gser->epnotify->name[1],gser->epnotify->name[2],gser->epnotify->name[3]);

	if (status)
		goto fail;
	return 0;

fail:
	printk("%s---%d, failed\n", __func__, __LINE__);
	return status;
}

int gser_bind_config(struct usb_configuration *c)
{
	int status;

	struct f_gser *gser= calloc(sizeof(*gser), 1);

	if (!gser)
		return -ENOMEM;

	gser->func.name = "usb_serial";
	gser->func.strings = gser_strings;
	gser->func.bind = gser_bind;
	gser->func.set_alt = gser_set_alt;
	gser->func.setup = gser_setup;
	gser->func.disable = gser_disable;

	status = usb_add_function(c, &gser->func);
	if (status){
		printf("%s--%d, error, status = %d\n", __func__, __LINE__, status);
		free(gser);
	}

	return status;
}


static void gser_epin_complete(struct usb_ep *ep, struct usb_request *req)
{
	struct f_gser *f_gser = req->context;
	int ret = 0;
	char *buf = req->buf;

	debug("IN, req->actual = %d,  req->status = %d, req->buf = 0x%x, 0x%x\n", req->actual, req->status, buf[0], buf[1]);
	req->status = 0;
	req->actual = 0;
	ret = usb_ep_queue(ep, req, 0);
	if (ret) {
		printf("%s: Error in usb_ep_queue,%d\n", __func__, ret);
		return ret;
	}


}

static void gser_epout_complete(struct usb_ep *ep, struct usb_request *req)
{
	int ret = 0;
	char *buf = req->buf;
	int i = 0;

	debug("OUT, req->actual = %d,  req->status = %d, req->buf = 0x%x = %c\n", req->actual, req->status, buf[0], buf[0]);
	req->status = 0;
	req->actual = 0;

	ret = usb_ep_queue(ep, req, 0);
	if (ret) {
		printf("%s: Error in usb_ep_queue,%d \n", __func__, ret);
		return ret;
	}


}


int gser_process_handle(struct f_gser *gser)
{
	int ret = 0;
	gser->datain_req = usb_ep_alloc_request(gser->epin, 0);
	if (!gser->datain_req){
		printf("%s--%d, error\n", __func__, __LINE__);
		return -ENOMEM;
	}


	gser->dataout_req = usb_ep_alloc_request(gser->epout, 0);
	if (!gser->dataout_req){
		printf("%s--%d, error\n", __func__, __LINE__);
		return -ENOMEM;
	}

	gser->datain_buf = malloc(RET_LENGTH * sizeof(char));
	if (!gser->datain_buf){
		printf("%s--%d, error\n", __func__, __LINE__);
		return -ENOMEM;
	}


	gser->dataout_buf = malloc(RET_LENGTH * sizeof(char));
	if (!gser->dataout_buf){
		printf("%s--%d, error\n", __func__, __LINE__);
		return -ENOMEM;
	}


	gser->datain_req->buf = gser->datain_buf;
	gser->datain_req->length = 64;
	gser->datain_req->status = 0;
	gser->datain_req->actual = 0;
	gser->datain_req->complete = gser_epin_complete;

	ret = usb_ep_queue(gser->epin, gser->datain_req, 0);
	if (ret) {
		printf("%s: Error in usb_ep_queue,%d\n", __func__, ret);
		return ret;
	}


	gser->dataout_req->buf = gser->dataout_buf;
	gser->dataout_req->length = 64;
	gser->dataout_req->status = 0;
	gser->dataout_req->actual = 0;
	gser->dataout_req->complete = gser_epout_complete;

	ret = usb_ep_queue(gser->epout, gser->dataout_req, 0);
	if (ret) {
		printf("%s: Error in usb_ep_queue,%d\n", __func__, ret);
		return ret;
	}

	return 0;
}







