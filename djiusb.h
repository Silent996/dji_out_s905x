#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include <libusb-1.0/libusb.h>

#define USB_VID 0x2ca3
#define USB_PID 0x1f
#define USB_CONFIG 1
#define USB_INTERFACE 3
#define USB_ENDPOINT_VIDEO_IN 0x84
#define USB_ENDPOINT_CONTROL_OUT 0x03

#define MAGIC_LENGTH 4
#define MAGIC_TIMEOUT_MS 500

int djifpv_usb_transfer(libusb_device_handle *dev_handle, unsigned char *buf, int *bytes_read);
libusb_device_handle *djifpv_get_device();
int djifpv_usb_init();
void djifpv_usb_close(libusb_device_handle *dev_handle);
void djifpv_usb_exit();