/* 
 * Name        : libzen.h
 * Author      : Maciej Muszkowski
 * Version     : 0.0.0.6
 * Copyright   : GPL
 * Description : Library that allows us to communicate with Zen Stone
 */

#ifndef LIBZEN_H
#define LIBZEN_H

#include <stdio.h>
#include <usb.h>
#include <memory.h>
#include <string.h>

#ifdef WIN32
# define snprintf sprintf_s
# if (_MSC_VER > 1300)
#  pragma comment(lib, "libusb.lib")
#  pragma warning(disable: 4333) /* disable right shift by too large amount, data loss */
# endif
#endif

#ifdef NP_DRIVER_DEATTACH
# include "libusb-attach-dev.h"
#endif

#define zen_log printf

typedef unsigned char 		u8;
typedef unsigned short 		u16;
typedef unsigned int  		u32;
typedef unsigned long long int	u64;

/* USB device ids */
#define ZEN_VENDOR	0x041E
#define ZEN_PRODUCT	0x4154
/* Zen Stone uses Sigmatel chip */
#define ZEN_CHIP_ID	0x3500 /* SMTP3550 */
#define ZEN_PROTO_VER	0x0200
/* Timeout in ms for all operations from libusb */
#define ZEN_TIMEOUT 3000
/* Values returned */
#define ZEN_SUCC 	0
#define ZEN_ERROR 	-1
/* Maximal battery level */
#define ZEN_MAX_BATT 100
/* Endpoints */
#define ZEN_ENDP_IN	0x81
#define	ZEN_ENDP_OUT	0x02

#define WSWAP(x)	( ((x) << 8) | ((x) >> 8) )
#define DWSWAP(x)	( ((x) << 24) |	(((x) << 8) & 0x00ff0000) | (((x) >> 8) & 0x0000ff00) | ((x) >> 24) )
#define QSWAP(x) \
	(((x & 0x00000000000000FF) << 56) | \
	((x & 0x000000000000FF00) << 40) | \
	((x & 0x0000000000FF0000) << 24) | \
	((x & 0x00000000FF000000) << 8)  | \
	((x & 0x000000FF00000000) >> 8)  | \
	((x & 0x0000FF0000000000) >> 24) | \
	((x & 0x00FF000000000000) >> 40) | \
	((x & 0xFF00000000000000) >> 56) )


#pragma pack(push, 1)

/* If you want to read about CBW, CSW etc go to www.usb.org/developers/devclass_docs/usbmassbulk_10.pdf */

/** Request struct - Command Block Wrapper. */
struct sCBW
{
	/** CBW Signature (0x55 0x53 0x42 0x43). */
	u32	signature; 	
	/** Tag. */
	u32	tag;
	/** Transfer length. */
	u32	transferLength;
	/** Direction. */
	u8	direction;	
	/** Reserved (0x00). */
	u8	reserved;	
	/** Length of command. */
	u8	lengthOfCommand;
	/** Command. */
	u8	command[16];	
};

#define CBW_SIG		0x43425355

#define CBW_DIR_OUT	0X00
#define CBW_DIR_IN	0x80

#define CMD_SCSI_TEST_UNIT_READY	0x00
#define CMD_SCSI_INQUIRY		0x12
#define CMD_SCSI_CAPACITY		0x25
#define CMD_SCSI_SIGMATEL_READ		0xC0 /* Zen Stone is using Sigmatel STMP3550 */
#define CMD_SCSI_SIGMATEL_WRITE		0xC1

/* from Sigmatel SDK, not all tested */
#define CMD_SIGMATEL_GET_PROTOCOL_VERSION		0x00 /* tested */
#define CMD_SIGMATEL_GET_STATUS				0x01
#define CMD_SIGMATEL_GET_LOGICAL_MEDIA_INFO		0x02 /* tested */
#define CMD_SIGMATEL_GET_LOGICAL_MEDIA_NUM		0x03 /* tested */
#define CMD_SIGMATEL_GET_ALLOCATION_TABLE		0x05 /* tested */
#define CMD_SIGMATEL_ALLOCATE_MEDIA			0x06
#define CMD_SIGMATEL_ERASE_LOGICAL_MEDIA		0x07
#define CMD_SIGMATEL_GET_LOGICAL_DRIVE_INFO		0x12 /* tested */
#define CMD_SIGMATEL_READ_LOGICAL_DRIVE_SECTOR		0x13 /* tested */
#define CMD_SIGMATEL_SET_LOGICAL_DRIVE_INFO		0x20
#define CMD_SIGMATEL_WRITE_LOGICAL_DRIVE_SECTOR		0x23
#define CMD_SIGMATEL_ERASE_LOGICAL_DRIVE		0x2f
#define CMD_SIGMATEL_GET_CHIP_ID			0x30 /* tested */
#define CMD_SIGMATEL_CHIP_RESET				0x31

#define CMD_ZEN_BATT_LEVEL				0x84
#define CMD_ZEN_VOL_LIMIT_READ				0x81
#define CMD_ZEN_VOL_LIMIT_WRITE				0x82

/* For CMD_SIGMATEL_READ_LOGICAL_DRIVE_SECTOR */
#define CMD2_SIGMATEL_SECTOR_SIZE	0x00 /* not sure */
#define CMD2_SIGMATEL_BANK_SIZE		0x04

/* Request confirmation struct - Command Status Wrapper. */
struct sCSW
{
	/** CSW Signature (0x55 0x53 0x42 0x53). */
	u32	signature; 	
	/** Tag. */
	u32	tag;
	/** CBW Transfer Length - really used. */
	u32	dataResidue;
	/** Status. */
	u8	status;
};

#define CSW_SIG 		0x53425355

#define	CSW_OK			0x00
#define CSW_CMD_FAILED	0x01
#define CSW_PHASE_ERR	0x02

/** Firmare version container, all values are chars not ints! */
struct sFirmwVer
{
	u8	major;
	u8	minor[2];
	u8	micro;
};

#define ZEN_BATT_NOT_FULL 0x01
#define ZEN_BATT_FULL 	  0x02

/** Response from usb in case of battery check. */
struct sBattResp
{
	u8	reserved;
	u8	level;
	u8	full;
};

/** For SCSI INQUIRY. */
struct sDevInfo 
{ 
	u8	deviceType[2];
	u8	versions; 
	u8	responseDataFormat; 
	u8	additionalLength; 
	u8	reserved[2]; 
	u8	flags;
	u8	vendorId[8]; 
	u8	productId[16]; 
	u8	productRevisionLevel[4]; 
	u8	vendorSpecific[20]; 
	u8	reserved2[40]; 
};

/** Response from usb in case of capacity check. */
struct sCapResp
{
	u32	sectors;
	u32	sectorSize;
};

/** Response from usb in case of volume limit check. */
struct sVolLimitRead
{
	u8	reserved;
	u8	limit;
};

/** Struct send to usb in case of volume limit change. */
struct sVolLimitWrite
{
	u8	pass[16];
	u8	reserved;
	u8	limit;	   
};

/** Information about memory bank. */
struct sAllocTableRow
{
	u8	bankNo;
	u8	type;
	u8	tag;
	u64	size;
};

#define SIGMATEL_BANK_TYPE_DATA		0x00
#define	SIGMATEL_BANK_TYPE_SYSTEM	0x01
#define	SIGMATEL_BANK_TYPE_UNKNOWN	0x02

#define SIGMATEL_BANK_TAG_STMPSYS		0x00
#define SIGMATEL_BANK_TAG_USBMSC		0x01
#define SIGMATEL_BANK_TAG_RESOURCE_BIN		0x02
#define SIGMATEL_BANK_TAG_DATA			0x0A
#define SIGMATEL_BANK_TAG_RESOURCE_BIN_RAM	0x10
#define SIGMATEL_BANK_TAG_BOOTMANAGER		0x50

/** Allocation table. */
struct sAllocTable
{
	u16	rowsCount;
	struct sAllocTableRow row[10]; /* max 10 -> SDK for SMTP36xx says so.. */
};

/** Internal, for reading. */
struct sBankSize
{
	u32	sectorsCount;
	u32	sectorSize;
};

#pragma pack(pop)

/**
 * @brief 
 * Finds device on bus, creates interface and sets configuration
 * 
 * @param vid Vendor id, set to 0 to use default Creatice VID
 * @param pid Product id, set to 0 to use default Zen Stone PID
 * @return pointer to usb_dev_handle if succeded, NULL if failed
**/
usb_dev_handle* init_zen(int vid, int pid);

/**
 * @brief 
 * Checks the firmware version
 * 
 * @param hdev pointer to ZenStone created with initZen()
 * @param verPtr pointer to struct where version will be saved
 * @return ZEN_SUCC if succesfully read version
**/
int read_firmware_ver(usb_dev_handle* hdev, struct sFirmwVer* verPtr);

/**
 * @brief 
 * Checks the battery level
 * 
 * @param hdev pointer to ZenStone created with initZen() 
 * @return battery level - max 100 or ZEN_ERROR if failed
**/
int read_batt_level(usb_dev_handle* hdev);

/**
 * @brief 
 * Gets Zen Stone flash disk capacity in megabytes
 * 
 * @param hdev pointer to ZenStone created with initZen()
 * @return zen stone capacity in megabytes or ZEN_ERROR if failed
**/
int read_capacity(usb_dev_handle *hdev);

/**
 * @brief
 * Reads the volume limit in %
 * @param hdev pointer to ZenStone created with initZen() 
 * @return volume limit in % or ZEN_ERROR if failed
**/
int read_vol_limit(usb_dev_handle *hdev);

/**
 * @brief
 * Sets the volume limit with pasword
 * @param hdev pointer to ZenStone created with initZen() 
 * @param limit to be set, 0-100 (in %)
 * @param pass pass to change the limit, NULL if no pass
 * @return ZEN_SUCC if set
**/  
int write_vol_limit_pass(usb_dev_handle *hdev, u8 limit, const char* pass);

/**
 * @brief 
 * Releases interface and closes device handle 
 * 
 * @param hdev pointer to ZenStone created with initZen()
**/
void deinit_zen(usb_dev_handle* hdev);

/**
 * @brief
 * Sends a cbr, then makes a bulk read into ret parametr
 * @param hdev pointer to ZenStone created with initZen()
 * @param cbw pointer to sCBW struct with request params
 * @param ret pointer to struct where data will be saved
 * @param retSize size of struct where data will be saved
 * @return ZEN_SUCC if successfully read data into ret struct
**/
int read_packet(usb_dev_handle* hdev, struct sCBW* cbw, void* ret, size_t retSize);

/**
 * @brief
 * Sends a cbr, then makes a bulk write with struct from data parametr
 * @param hdev pointer to ZenStone created with initZen()
 * @param cbw pointer to sCBW struct with request params
 * @param ret pointer to struct from where data would be taken and send
 * @param dataSize size of struct with data
 * @return ZEN_SUCC if successfully send data to usb
**/
int send_packet(usb_dev_handle* hdev, struct sCBW* cbw, void* data, size_t dataSize);

/**
 * @brief
 * Reads sector size, first and last sector LBA
 * @param hdev pointer to ZenStone created with initZen()
 * @param bank bank id
 * @param result result
 * @return bank size in bytes if succeded, ZEN_ERROR if failed
**/
int read_bank_size(usb_dev_handle* hdev, u8 bank, struct sBankSize* result);

/**
 * @brief
 * Reads whole memory bank content to bank[no].bin
 * @param hdev pointer to ZenStone created with initZen()
 * @param bank bank id
 * @return ZEN_SUCC if successfully read data
**/
int read_bank(usb_dev_handle* hdev, u8 bank, FILE* f);

/**
 * @brief
 * Reads chosen fragment of memory bank
 * @param hdev pointer to ZenStone created with initZen()
 * @param fd file to which content will be saved
 * @param bank bank id
 * @param from first sector number (in fact it is 64-bit but we use only 32)
 * @param to last sector number (content won't be read)
 * @return ZEN_SUCC if successfully read data
**/
int read_sector(usb_dev_handle *hdev, FILE* fd, u8 bank, u32 sectorSize, u32 from, u32 to);

/**
 * @brief
 * Checks if you can read/write from/to device
 * @param hdev pointer to ZenStone created with initZen()
 * @return ZEN_SUCC if dev is accesible, ZEN_ERROR otherwise
**/
int device_ready(usb_dev_handle* hdev);

/**
 * @brief
 * Returns your mp3 player chip id (Zen Stone uses 2.0)
 * @param hdev pointer to ZenStone created with initZen()
 * @return 16bit version
**/
int read_chip_id(usb_dev_handle *hdev);

/**
 * @brief
 * Returns the protocol version (Zen Stone uses MTP3550 which id is 0x3500)
 * @param hdev pointer to ZenStone created with initZen()
 * @return 16bit id
**/
int read_protocol_ver(usb_dev_handle *hdev);

/**
 * @brief
 * Reads the whole firmware to files, probably will work only on Zen Stone
 *
 * Names mapping for Zen Stone:
 * bank4.bin -> bootmanager.sb
 * bank5.bin -> hostlink.sb
 * bank6.bin -> stmpsys.sb
 * bank7,8.bin (same content) -> resource.bin
 *
 * Differnce between .sb and .bin file is the begging 
 * (always the same, Product and Component version in ASCII):
 * 50 72 6F 64 75 63 74 20 56 65 72 73 69 6F 6E 3A 20 30 30 31 2E 30 30 36 2E 30 30 31 0D 0A 43 6F 6D 70 6F 6E 65 6E 74 20 56 65 72 73 69 6F 6E 3A 20 30 30 31 2E 30 30 36 2E 30 30 31 0D 0A 1A 53 54 4D
 * and that .bin files have size divided by 2048 (sector size), 
 * unnecessary bytes are filled with 0xFF
 *
 * @param hdev pointer to ZenStone created with initZen()
 * @return ZEN_SUCC if succeded
*/
int read_firmware(usb_dev_handle *hdev);

/** Internal, debug. */
void hexdump(u8* buff, int len);

#endif

