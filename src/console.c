/*
 * Name        : console.c
 * Author      : Maciej Muszkowski
 * Version     : 0.0.0.6
 * Copyright   : GPL
 * Description : libzen example console program
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "libzen.h"

void drawGauge(int val, int max, int width)
{
	int i, unit, complete, _max;

	if(val==max) 
	{
		putchar('[');
		for(i=0;i<width;i++) putchar('=');
		puts("] (100/100)");
		return;
	}
	unit = max / width;
	complete = val / unit;
	_max = max / unit;
	putchar('[');
	for(i=0;i<(complete-1);i++)
		putchar('=');
	putchar('>');
	for(i=complete;i<_max;i++)
		putchar('-');
	printf("] (%d/%d)\n",val,max);
}

#define	MODE_BASIC_INFO		0
#define MODE_ZEN_INFO		1
#define MODE_READ_FIRMWARE	2


int main(int argc, char* argv[]) 
{
	usb_dev_handle*	hdev;
	int		mode, vid, pid, argpos;

	if(argc <= 1)  /* do not use getopt */
	{
		printf("Usage: %s <mode> <options>\n", argv[0]);
		puts("Mode (required):");
		puts("-i\t=> shows basic information about your mp3, saves it to .txt file");
		puts("-z\t=> shows information specific to Zen Stone");
		puts("-r\t=> reads firmware");
		puts("Option:");
		puts("-vid 0x1234 => threats device with vendor id 0x1234 as Zen");
		puts("-pid 0x1234 => threats device with product id 0x1234 as Zen");
		return ZEN_ERROR;
	}

	mode = MODE_BASIC_INFO;
	
	/* reading options */
	argpos = 1;	
	if(strcmp(argv[argpos], "-i") == 0)
		mode = MODE_BASIC_INFO;
	else if(strcmp(argv[argpos], "-z") == 0)
		mode = MODE_ZEN_INFO;
	else if(strcmp(argv[argpos], "-r") == 0)
		mode = MODE_READ_FIRMWARE;
	else
	{
		printf("Unknown mode: %s\n", argv[argpos]);
		return ZEN_ERROR;
	}
	argpos++;

	/* reading options */
	vid = ZEN_VENDOR;
	pid = ZEN_PRODUCT;
	while(argpos < argc)
	{
		if(strcmp(argv[argpos], "-vid") == 0)
			sscanf(argv[++argpos], "%x", &vid);
		else if(strcmp(argv[argpos], "-pid") == 0)
			sscanf(argv[++argpos], "%x", &pid);
		else
		{
			printf("Unknown option: %s\n", argv[argpos]);
			return ZEN_ERROR;
		}
		argpos++;
	}
	
	if(vid != ZEN_VENDOR || pid != ZEN_PRODUCT)
		printf("Using non default ids -> VID=0x%.4X, PID=0x%.4X\n", vid, pid);

	hdev = init_zen(vid, pid);

	if(!hdev)
	{
		puts("Zen Stone not found or error occured.");
		puts("Windows:");
		puts("Make sure you have libusb installed and filter set for proper device");
		puts("Unix:");
		puts(	"If you get error \"Device or resource busy\" " \
				"probably some other driver is using your Zen, " \
				"try to umount your device or unload its driver (usb_storage) with rmmod driver_name.");
		return ZEN_ERROR;
	}

	if(device_ready(hdev) != ZEN_SUCC)
	{
		puts("Device detected, but is not ready, try running the program again.");
		deinit_zen(hdev);
		return ZEN_ERROR;
	}

	switch(mode)
	{
		case MODE_BASIC_INFO:
		{
			int			capacity, chipId, protoVer;
			FILE*		f;

			f = fopen("device-info.txt", "w");

			if(f == NULL)
			{
				puts("Creating device-info.txt failed, check privileges");
				goto deinit;
			}

			fprintf(f, "VID=0x%.4X\n", vid);
			fprintf(f, "PID=0x%.4X\n", pid);

			if((chipId = read_chip_id(hdev)) == ZEN_ERROR)
			{
				fclose(f);
				goto deinit;
			}
			printf("Chip id: 0x%.4X\n", chipId);
			fprintf(f, "CID=0x%.4X\n", chipId);

			if((protoVer = read_protocol_ver(hdev)) == ZEN_ERROR)
			{
				fclose(f);
				goto deinit;
			}
			printf("Protocol version: 0x%.4X\n", protoVer);
			fprintf(f, "PVER=0x%.4X\n", protoVer);

			if((capacity = read_capacity(hdev)) == ZEN_ERROR)
			{
				fclose(f);
				goto deinit;
			}
			printf("Capacity: %dMB\n", capacity);
			fprintf(f, "MB=%d\n", capacity);
			fclose(f); /* all needed data collected */
			break;
		}
		case MODE_ZEN_INFO:
		{
			struct sFirmwVer	ver;
			int					level, limit;

			if(read_firmware_ver(hdev, &ver) != ZEN_SUCC) 
				goto deinit;
			printf("Firmware version: %c.%c%c.%c\n",ver.major,ver.minor[0],ver.minor[1],ver.micro);

			if((level = read_batt_level(hdev)) == ZEN_ERROR)
				goto deinit;
			drawGauge(level,ZEN_MAX_BATT,30);

			if((limit = read_vol_limit(hdev)) == ZEN_ERROR)
				goto deinit;
			printf("Volume level limit: %d%%\n", limit);

			break;
		}
		case MODE_READ_FIRMWARE:
		{
			int	chipId, protoVer;
			if((chipId = read_chip_id(hdev)) == ZEN_ERROR)
				goto deinit;

			if(chipId != ZEN_CHIP_ID)
			{
				printf("WARNING: Chip id is different than expected: 0x%.4X\n", chipId);
				puts("Continuing anyway...");
			}

			if((protoVer = read_protocol_ver(hdev)) == ZEN_ERROR)
				goto deinit;

			if(protoVer != ZEN_PROTO_VER)
			{
				printf("WARNING: Protocol version is different than expected: 0x%.4X\n", protoVer);
				puts("Continuing anyway...");
			}
					
			if(read_firmware(hdev) == ZEN_SUCC)
				printf("Reading firmware succeded\n");

			break;
		}
	}

deinit:
	deinit_zen(hdev);

	if(mode == MODE_READ_FIRMWARE && (vid != ZEN_VENDOR || pid != ZEN_PRODUCT))
	{
		puts("\nREAD PLEASE:");
		puts("Could you be so kind and send extracted firmware + device_info.txt to my E-MAIL?");
		puts("This may help somebody in the future when looking for firmware to his/her mp3.");
		puts("Thank you. E-mail: 4m2@wp.pl");
	}

	return ZEN_SUCC;
}

