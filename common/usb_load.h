#ifndef __USB_LOAD_H__
#define __USB_LOAD_H__

int usb_load_run_stage1_firmware(unsigned char *data, unsigned int offset, int len);
int usb_load_run_stage2_firmware(unsigned char *data, unsigned int offset, int len);
int usb_load_run_send_data(unsigned char *data, unsigned int offset, int len);

#endif
