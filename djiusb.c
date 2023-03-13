
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include <libusb-1.0/libusb.h>

#include <djiusb.h>

#define USB_BUFFER_SIZE_BYTES 1024 * 4

unsigned char MAGIC[] = {0x52, 0x4d, 0x56, 0x54};

int djifpv_usb_transfer(libusb_device_handle *dev_handle, unsigned char *buf, int *bytes_read)
{
	if (dev_handle == NULL)
		return -1;

	int r = libusb_bulk_transfer(dev_handle, USB_ENDPOINT_VIDEO_IN, buf, USB_BUFFER_SIZE_BYTES, bytes_read, 0);
	if (*bytes_read == 0)
	{
		r = libusb_bulk_transfer(dev_handle, USB_ENDPOINT_CONTROL_OUT, MAGIC, MAGIC_LENGTH, NULL, MAGIC_TIMEOUT_MS); // retry to activate connection. useful when you change lipo :)
		if (r < 0 && r != LIBUSB_ERROR_TIMEOUT)
		{
			fprintf(stderr, "unable to send magic: %s\n", libusb_strerror(r));
			return -1;
		}
	}

	return r;
}

libusb_device_handle *djifpv_get_device()
{
	int r = 0;
	libusb_device_handle *dev = libusb_open_device_with_vid_pid(NULL, USB_VID, USB_PID);
	if (dev == NULL)
	{
		// fprintf(stderr, "unable to open device, or device not found\n");
		// if (geteuid() != 0)
		// {
		// 	fprintf(stderr, "try running as root (with sudo)\n");
		// }
		return NULL;
	}

	// Detach the kernel (all non-fatal)
	r = libusb_reset_device(dev);
	// Detach kernel driver (RNDIS)
	r = libusb_detach_kernel_driver(dev, 0);
	// Detach kernel driver (Storage)
	r = libusb_detach_kernel_driver(dev, 2);
	// Detach kernel driver (Storage)
	r = libusb_detach_kernel_driver(dev, 4);

	// Set active configuration
	r = libusb_set_configuration(dev, USB_CONFIG);
	if (r != 0)
	{
		fprintf(stderr, "unable to set configuration: (%d) %s\n", r, libusb_strerror(r));
		libusb_close(dev);
		// libusb_exit(NULL)
		return NULL;
	}

	// Claim interface
	r = libusb_claim_interface(dev, USB_INTERFACE);
	if (r != 0)
	{
		fprintf(stderr, "unable to claim interface: %s\n", libusb_strerror(r));
		libusb_close(dev);
		// libusb_exit(NULL);
		return NULL;
	}

	for (int i = 0; i < 3; i++)
	{
		fprintf(stdout, "send magic data...\n");
		r = libusb_bulk_transfer(dev, USB_ENDPOINT_CONTROL_OUT, MAGIC, MAGIC_LENGTH, NULL, MAGIC_TIMEOUT_MS);
		if (r != 0 && r != LIBUSB_ERROR_TIMEOUT)
		{
			libusb_close(dev);
			// libusb_exit(NULL);
			// fprintf(stderr, "unable to send magic: %s,retry:%d\n", libusb_strerror(r), i);
		}
		else
		{
			return dev;
		}
	}
	return NULL;
}

int djifpv_usb_init()
{
	int r = libusb_init(NULL);

	if (r < 0)
	{
		fprintf(stderr, "unable to init libusb: %s\n", libusb_strerror(r));
		return r;
	}
	fprintf(stdout, "libusb init ok...\n");

	return r;
}

void djifpv_usb_close(libusb_device_handle *dev_handle)
{
	if (dev_handle != NULL)
	{

		libusb_close(dev_handle);
	}
}

void djifpv_usb_exit()
{
	libusb_exit(NULL);
}