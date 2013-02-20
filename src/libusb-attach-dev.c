/*
 * Name        : libusb-attach-dev.c
 * Author      : Maciej Muszkowski
 * Version     : 0.0.0.6
 * Copyright   : GPL
 * Description : Patch for libusb allowing reattaching the kernel driver
 */

#include "libusb-attach-dev.h"

int usb_attach_kernel_driver_np(struct usb_dev_handle* dev, int interFace)
{
	struct usb_ioctl command;

	command.ifno = interFace;
	command.ioctl_code = IOCTL_USB_CONNECT;
	command.data = NULL;

	ioctl(dev->fd, IOCTL_USB_IOCTL, &command); /* returns 1 if succeded */

	return 0; /* libusb returns 0 if succeded */
}

