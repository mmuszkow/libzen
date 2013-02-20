/*
 * Name        : libusb-attach-dev.h
 * Author      : Maciej Muszkowski
 * Version     : 0.0.0.6
 * Copyright   : GPL
 * Description : Patch for libusb allowing reattaching the kernel driver
 */

#if defined(unix) && !defined(LIBUSB_HAS_ATTACH_KERNEL_DRIVER_NP)
#define LIBUSB_HAS_ATTACH_KERNEL_DRIVER_NP

#include <errno.h>
#include <sys/ioctl.h>
#include <usb.h>

struct usb_dev_handle
{
	int fd;

	struct usb_bus *bus;
	struct usb_device *device;

	int config;
	int interface;
	int altsetting;

	/* Added by RMT so implementations can store other per-open-device data */
	void *impl_info;
};

struct usb_ioctl
{
	int ifno;	/* interface 0..N ; negative numbers reserved */
	int ioctl_code;	/* MUST encode size + direction of data so the macros in <asm/ioctl.h> give correct values */
	void *data;	/* param buffer (in, or out) */
};

#define IOCTL_USB_IOCTL         _IOWR('U', 18, struct usb_ioctl)
#define IOCTL_USB_CONNECT	_IO('U', 23)

/**
 * @brief
 * Reattaches the USB device (was in older versions of libusb)
 * @param dev pointer to ZenStone created with initZen() 
 * @return always 0
**/
int usb_attach_kernel_driver_np(usb_dev_handle* dev, int interFace); /* capital F because interface is a keyword */

#endif

