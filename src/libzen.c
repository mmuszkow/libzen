/*
 * Name        : libzen.c
 * Author      : Maciej Muszkowski
 * Version     : 0.0.0.6
 * Copyright   : GPL
 * Description : Library that allows us to communicate with Zen Stone
 */

#include "libzen.h"

usb_dev_handle* init_zen(int vid, int pid) {
    struct usb_bus *busses;
    struct usb_bus *bus;

    if(vid == 0)
        vid = ZEN_VENDOR;

    if(pid == 0)
        pid = ZEN_PRODUCT;
    
    usb_init();
    usb_find_busses();
    usb_find_devices();
    busses = usb_get_busses();
    
    for (bus = busses; bus; bus = bus->next) {
           struct usb_device *dev;
        for (dev = bus->devices; dev; dev = dev->next) {
            if(dev->descriptor.idVendor == vid && dev->descriptor.idProduct == pid) {       
                usb_dev_handle* hdev = usb_open(dev);
                if(hdev) { 
#if defined(LIBUSB_HAS_DETACH_KERNEL_DRIVER_NP) && (LIBUSB_HAS_DETACH_KERNEL_DRIVER_NP == 1) && defined(NP_DRIVER_DEATTACH)
                    if (usb_detach_kernel_driver_np(hdev, dev->config->interface->altsetting->bInterfaceNumber) < 0) 
                        zen_log("usb_detach_kernel_driver_np: %s\nContinuing anyway...\n", usb_strerror());
#endif
                    if(usb_set_configuration(hdev,dev->config->bConfigurationValue) < 0) {
                        zen_log("usb_set_configuration: %s\n", usb_strerror());
                        deinit_zen(hdev);
                    } else if(usb_claim_interface(hdev, dev->config->interface->altsetting->bInterfaceNumber) < 0) {
                        zen_log("usb_claim_interface: %s\n", usb_strerror());
                        deinit_zen(hdev);
                    } else
                        return hdev;
                }
                else
                    zen_log("usb_open: %s\n", usb_strerror());    
            }
        }
    }
    return NULL;
}

int send_packet(usb_dev_handle* hdev, struct sCBW* cbw, void* data, size_t dataSize) {
    struct sCSW csw;

    if(hdev==NULL) 
        return ZEN_ERROR;

    if(usb_bulk_write(hdev, ZEN_ENDP_OUT, (char*)cbw, sizeof(struct sCBW), ZEN_TIMEOUT) < 0) {
        zen_log("send_packet, usb_bulk_write: %s\n", usb_strerror());
        return ZEN_ERROR;
    }

    if(data && usb_bulk_write(hdev, ZEN_ENDP_OUT, data, dataSize, ZEN_TIMEOUT) < 0) {
        zen_log("send_packet, usb_bulk_write2: %s\n", usb_strerror());
        /**
         * Sometimes strange things happen on linux here,
         * bult_read returns -110 (timeout) but everything is ok,
         * usb_strerror return "No error",
         * if so comment the line below
         **/
        return ZEN_ERROR;
    }

    if(usb_bulk_read(hdev, ZEN_ENDP_IN, (char*)&csw, sizeof(struct sCSW), ZEN_TIMEOUT) < 0) {
        zen_log("send_packet, usb_bulk_read: %s\n", usb_strerror());
        return ZEN_ERROR;
    }

    if(csw.signature != CSW_SIG || csw.tag != cbw->tag || csw.status != CSW_OK || csw.dataResidue > cbw->transferLength) {
        zen_log("send_packet, CSW check failed -> sig=0x%X, tag_eq=%d, status=0x%X, dataResidue=%d\n", \
            csw.signature, csw.tag == cbw->tag, csw.status, csw.dataResidue);
        return ZEN_ERROR;
    }

    return ZEN_SUCC;
}

int read_packet(usb_dev_handle* hdev, struct sCBW* cbw, void* ret, size_t retSize) {
     struct sCSW csw;

    if(hdev==NULL) 
        return ZEN_ERROR;

    if(usb_bulk_write(hdev, ZEN_ENDP_OUT, (char*)cbw, sizeof(struct sCBW), ZEN_TIMEOUT) < 0) {
        zen_log("read_packet, usb_bulk_write: %s\n", usb_strerror());
        return ZEN_ERROR;
    }

    if(usb_bulk_read(hdev, ZEN_ENDP_IN, ret, retSize, ZEN_TIMEOUT) < 0) {
         zen_log("read_packet, usb_bulk_read: %s\n", usb_strerror());
        /**
         * Sometimes strange things happen on linux here,
         * bulk_read returns -110 (timeout) but everything is ok,
         * usb_strerror return "No error",
         * if so comment the line below
         **/
        return ZEN_ERROR;
    }

    if(usb_bulk_read(hdev, ZEN_ENDP_IN, (char*)&csw, sizeof(struct sCSW), ZEN_TIMEOUT) < 0) {
        zen_log("read_packet, usb_bulk_read2: %s\n", usb_strerror());
        return ZEN_ERROR;    
    }

    if(csw.signature != CSW_SIG || csw.tag != cbw->tag || csw.status != CSW_OK || csw.dataResidue > cbw->transferLength) {
        zen_log("read_packet, CSW check failed -> sig=0x%X, tag_eq=%d, status=0x%X, dataResidue=%d\n", \
            csw.signature, csw.tag == cbw->tag, csw.status, csw.dataResidue);
        return ZEN_ERROR;
    }
    
    return ZEN_SUCC;
}


int device_ready(usb_dev_handle* hdev) {
    struct sCBW cbw = { 
        CBW_SIG,     /* CBW Signature */
        rand(),      /* Tag */
        0x00,        /* Transfer length */
        CBW_DIR_OUT, /* Direction */
        0x00,        /* Reserved */
        0x06,        /* Length of command */
        {CMD_SCSI_TEST_UNIT_READY, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00} /* Command */
    };

    if(hdev==NULL) 
        return ZEN_ERROR;

    return send_packet(hdev,&cbw,NULL,0);
}

int read_firmware_ver(usb_dev_handle* hdev, struct sFirmwVer* verPtr) {
    struct sDevInfo devInfo;
    struct sCBW     cbw = { 
        CBW_SIG,    /* CBW Signature */
        rand(),     /* Tag */
        0x60,       /* Transfer length */
        CBW_DIR_IN, /* Direction */
        0x00,       /* Reserved */
        0x06,       /* Length of command */
        {CMD_SCSI_INQUIRY, 0x00, 0x00, 0x00, 0x60, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00} /* Command */
    };

    if(hdev==NULL) 
        return ZEN_ERROR;
 
    if(read_packet(hdev,&cbw,&devInfo,sizeof(struct sDevInfo)) == ZEN_SUCC) {
        verPtr->major = devInfo.productRevisionLevel[0];
        verPtr->minor[0] = devInfo.productRevisionLevel[0];
        verPtr->minor[1] = devInfo.productRevisionLevel[1];
        verPtr->micro = devInfo.productRevisionLevel[2];

        return ZEN_SUCC;
    }

    verPtr->major = '?';
    verPtr->minor[0] = '?';
    verPtr->minor[1] = '?';
    verPtr->micro = '?';

    return ZEN_ERROR;
}

int read_batt_level(usb_dev_handle* hdev) {
    struct sBattResp resp;
    struct sCBW cbw = {    
        CBW_SIG,     /* CBW Signature */
        rand(),      /* Tag */
        sizeof(struct sBattResp), /* Transfer length */
        CBW_DIR_IN,  /* Direction */
        0x00,        /* Reserved */
        0x10,        /* Length of command */
        {    CMD_SCSI_SIGMATEL_READ, /* Command */
            CMD_ZEN_BATT_LEVEL, 
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
        } 
    };

     if(hdev==NULL) 
        return ZEN_ERROR;
    
    if(read_packet(hdev,&cbw,&resp,sizeof(struct sBattResp)) == ZEN_SUCC) {    
        switch(resp.full) {
            case ZEN_BATT_NOT_FULL: zen_log("Battery level (CHARGING): %d%%\n", resp.level); break;
            case ZEN_BATT_FULL: zen_log("Battery level (FULL): %d%%\n", resp.level); break;
            default: 
                zen_log("Battery level (UNKNOWN): 0x%.2x %d%%\n", resp.full, resp.level);
                return ZEN_ERROR;
         }
         return resp.level;
    }

    return ZEN_ERROR;
}

int read_capacity(usb_dev_handle *hdev) {
    struct sCapResp capacity;
    struct sCBW     cbw = { 
        CBW_SIG,             /* CBW Signature  */
         rand(),             /* Tag */
        sizeof(struct sCapResp),    /* Transfer length */
        CBW_DIR_IN,          /* Direction */
        0x00,                /* Reserved */
        0x0a,                /* Length of command */
        {CMD_SCSI_CAPACITY, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00} /* Command */
        };

    if(hdev==NULL) 
        return ZEN_ERROR;
    
    if(read_packet(hdev,&cbw,(char*)&capacity,sizeof(struct sCapResp)) == ZEN_SUCC) {
        if(capacity.sectors == 0xFFFFFFFF) {
            zen_log("Reading capacity error, maybe this is not a SCSI device?\n");
            return ZEN_ERROR;
        }
        
        return (DWSWAP(capacity.sectors) * DWSWAP(capacity.sectorSize)) >> 20;
    }

    return ZEN_ERROR;    
}

void hexdump(u8* buff, int len) {
    int i;
    for(i=0; i< len; i++)
        zen_log("0x%.2X ", buff[i]);
    zen_log("\n");
}

int read_sector(usb_dev_handle* hdev, FILE* fd, u8 bank, u32 sectorSize, u32 from, u32 to) {
    u8*        bin; /* read buffer */
    struct     sCBW cbw = {    
        CBW_SIG,    /* CBW Signature */
        rand(),     /* Tag */
        0,          /* Transfer length - unknown yet */
        CBW_DIR_IN, /* Direction */
        0x00,       /* Reserved */
        0x10,       /* Length of command */
        {    
            CMD_SCSI_SIGMATEL_READ, /* Command */
            CMD_SIGMATEL_READ_LOGICAL_DRIVE_SECTOR, 
            0x04, /* bank */
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* start sector */
            0x00, 0x00, 0x00, 0x08, /* sector count */
            0x00 
        } 

    };
    u32    i, bufferSize;

     if(hdev==NULL) 
        return ZEN_ERROR;

    bufferSize = sectorSize * 8;
    bin = (u8*)malloc(bufferSize);

    if(bin==NULL)
        return ZEN_ERROR;

    cbw.transferLength = bufferSize;
    for(i=from; i<to; i+= 8) {
        cbw.command[2] = bank;

        cbw.command[10] = i & 0xFF;
        cbw.command[9] = (i & 0xFF00) >> 8;
        cbw.command[8] = (i & 0xFF0000) >> 16;
        cbw.command[7] = (i & 0xFF000000) >> 24;

        /* in fact start sector is 64-bit but fuck it */

        if(read_packet(hdev,&cbw,bin,bufferSize) != ZEN_SUCC)
            return ZEN_ERROR;
        fwrite(bin, 1, bufferSize, fd);
    }

    free(bin);

    return ZEN_SUCC;
}

int read_bank_size(usb_dev_handle* hdev, u8 bank, struct sBankSize* result) {
    u32     sectorsCount[2]={0,0}; /* it's 64-bit but we won't for sure read more than 4GB */
    struct sCBW    cbw = {    
        CBW_SIG,    /* CBW Signature */
        rand(),     /* Tag */
        0x08,       /* Transfer length */
        CBW_DIR_IN, /* Direction */
        0x00,       /* Reserved */
        0x10,       /* Length of command */
        {    
            CMD_SCSI_SIGMATEL_READ, /* Command */
            CMD_SIGMATEL_GET_LOGICAL_DRIVE_INFO, 
            bank, 
            CMD2_SIGMATEL_BANK_SIZE, 
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
        } 
    };
    u32        sectorSize;
    struct sCBW    cbw2 = {    
        CBW_SIG,    /* CBW Signature */
        rand(),     /* Tag */
        0x04,       /* Transfer length */
        CBW_DIR_IN, /* Direction */
        0x00,       /* Reserved */
        0x10,       /* Length of command */
        {    
            CMD_SCSI_SIGMATEL_READ, /* Command */
            CMD_SIGMATEL_GET_LOGICAL_DRIVE_INFO, 
            bank, 
            CMD2_SIGMATEL_SECTOR_SIZE, 
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
        } 
    };

    if(read_packet(hdev,&cbw,(char*)&sectorsCount,8) != ZEN_SUCC)
        return ZEN_ERROR;

    /* something is not ok here, in SDK sectorsCount is 64-bit but here only 24-bits are valid */
    result->sectorsCount    = DWSWAP(sectorsCount[1] & 0xFFFFFF00);

    if(read_packet(hdev,&cbw2,(char*)&sectorSize,4) != ZEN_SUCC)
        return ZEN_ERROR;

    result->sectorSize = DWSWAP(sectorSize);

    return result->sectorsCount * result->sectorSize;
}

int read_bank(usb_dev_handle* hdev, u8 bank, FILE* f) {
    int                 res;
    struct sBankSize    bankSize;

    if(read_bank_size(hdev, bank, &bankSize) == ZEN_ERROR)
        return ZEN_ERROR;

    if(bankSize.sectorsCount == 0) {
        zen_log("Memory bank %u contains no data\n", bank);
        return ZEN_SUCC;
    }

    if((bankSize.sectorsCount * bankSize.sectorSize) > (50<<20)) { /* > 50 MB */
        zen_log("WARNING: You have chosen memory bank bigger than 50MB\n");
        zen_log("You can stop the reading process by Ctrl+C\n");
    }

    res = read_sector(hdev, f, bank, bankSize.sectorSize, 0, bankSize.sectorsCount);

    return res;
}

int read_chip_id(usb_dev_handle *hdev) {
    u16            id;
    struct sCBW    cbw = {    
        CBW_SIG,    /* CBW Signature */
        rand(),     /* Tag */
        0x02,       /* Transfer length */
        CBW_DIR_IN, /* Direction */
        0x00,       /* Reserved */
        0x10,       /* Length of command */
        {    
            CMD_SCSI_SIGMATEL_READ, /* Command */
            CMD_SIGMATEL_GET_CHIP_ID, 
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
        }
    };

    if(read_packet(hdev,&cbw,(char*)&id,2) != ZEN_SUCC)
        return ZEN_ERROR;

    return ((id<<8)|(id>>8));
}

int read_protocol_ver(usb_dev_handle *hdev) {
    u16    ver;
    struct sCBW cbw = {    
        CBW_SIG,    /* CBW Signature */ 
        rand(),     /* Tag */
        0x02,       /* Transfer length */
        CBW_DIR_IN, /* Direction */
        0x00,       /* Reserved */
        0x10,       /* Length of command */
        {    
            CMD_SCSI_SIGMATEL_READ, /* Command */
            CMD_SIGMATEL_GET_PROTOCOL_VERSION, 
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
        }
    };


    if(read_packet(hdev,&cbw,(char*)&ver,2) != ZEN_SUCC)
        return ZEN_ERROR;

    return ((ver<<8)|(ver>>8));
}

int read_debug_info(usb_dev_handle *hdev, FILE* fd) {
    return 0;
}

int read_firmware(usb_dev_handle *hdev) {
    int    i;
    struct sCBW    cbw = {    
        CBW_SIG,    /* CBW Signature */ 
        rand(),     /* Tag */
        sizeof(struct sAllocTable), /* Transfer length */
        CBW_DIR_IN, /* Direction */
        0x00,       /* Reserved */
        0x10,       /* Length of command */
        {    
            CMD_SCSI_SIGMATEL_READ, /* Command */
            CMD_SIGMATEL_GET_ALLOCATION_TABLE, 
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
        } 
    };
    struct sAllocTable    table;

    if(hdev==NULL) 
        return ZEN_ERROR;

    if(read_packet(hdev,&cbw,(char*)&table,sizeof(struct sAllocTable)) != ZEN_SUCC)
        return ZEN_ERROR;

    table.rowsCount = WSWAP(table.rowsCount);

    if(table.rowsCount > 10) {
        zen_log("More than 10 partitions (%u), contact developer\n", table.rowsCount);
        return ZEN_ERROR;
    }

    for(i=0; i<table.rowsCount; i++) {
        char* type;

        switch(table.row[i].type) {
            case SIGMATEL_BANK_TYPE_DATA: type = "DATA"; break;
            case SIGMATEL_BANK_TYPE_SYSTEM: type = "SYSTEM"; break;
            default: type = "UNKNOWN";
        }
        table.row[i].size = QSWAP(table.row[i].size);

        zen_log("Bank %u [%s] %lluB\n", table.row[i].bankNo, type, table.row[i].size);
    }

    for(i=0; i<table.rowsCount; i++) {
        switch(table.row[i].tag) {
            case SIGMATEL_BANK_TAG_STMPSYS: {
                FILE* f = fopen("stmpsys.sb", "wb");

                zen_log("Reading system\n");
                
                if(f == NULL) {
                    zen_log("File creating error, check privileges");
                    return ZEN_ERROR;
                }

                if(read_bank(hdev, table.row[i].bankNo, f) != ZEN_SUCC) {
                    fclose(f);
                    return ZEN_ERROR;
                }

                fclose(f);

                break;
            }
            case SIGMATEL_BANK_TAG_USBMSC: {
                FILE* f = fopen("usbmsc.sb", "wb");

                zen_log("Reading USB Mass Storage driver\n");
                
                if(f == NULL) {
                    zen_log("File creating error, check privileges");
                    return ZEN_ERROR;
                }

                if(read_bank(hdev, table.row[i].bankNo, f) != ZEN_SUCC) {
                    fclose(f);
                    return ZEN_ERROR;
                }

                fclose(f);
                break;
            }
            case SIGMATEL_BANK_TAG_RESOURCE_BIN: {
                FILE* f = fopen("resource.bin", "wb");

                zen_log("Reading resources\n");
                
                if(f == NULL) {
                    zen_log("File creating error, check privileges");
                    return ZEN_ERROR;
                }

                if(read_bank(hdev, table.row[i].bankNo, f) != ZEN_SUCC) {
                    fclose(f);
                    return ZEN_ERROR;
                }

                fclose(f);

                break;
            }
            case SIGMATEL_BANK_TAG_DATA: {
                zen_log("Data bank, skipping\n");
                break;
            }
            case SIGMATEL_BANK_TAG_RESOURCE_BIN_RAM: {
                zen_log("Resource RAM bank, skipping\n");
                break;
            }
            case SIGMATEL_BANK_TAG_BOOTMANAGER: {
                FILE* f = fopen("bootmanager.sb", "wb");

                zen_log("Reading bootmanager\n");
                
                if(f == NULL) {
                    zen_log("File creating error, check privileges");
                    return ZEN_ERROR;
                }

                if(read_bank(hdev, table.row[i].bankNo, f) != ZEN_SUCC) {
                    fclose(f);
                    return ZEN_ERROR;
                }

                fclose(f);
                break;
            }
            default: {
                FILE*   f;
                char    filename[13];

                zen_log("Unkown tag 0x%X, saving anyway as bank%u.bin\n", table.row[i].tag, table.row[i].bankNo);

                snprintf(filename, 13, "bank%u.bin", table.row[i].bankNo);
                f = fopen(filename, "wb");
                
                if(f == NULL) {
                    zen_log("File creating error, check privileges");
                    return ZEN_ERROR;
                }

                if(read_bank(hdev, table.row[i].bankNo, f) != ZEN_SUCC) {
                    fclose(f);
                    return ZEN_ERROR;
                }

                fclose(f);
            }
        }
    }

    return ZEN_SUCC;
}

int read_vol_limit(usb_dev_handle *hdev) {
    struct sCBW cbw = {     
        CBW_SIG,    /* CBW Signature */ 
         rand(),    /* Tag */
        sizeof(struct sVolLimitRead), /* Transfer length */
        CBW_DIR_IN, /* Direction */
        0x00,       /* Reserved */
        0x10,       /* Length of command */
        {    CMD_SCSI_SIGMATEL_READ, /* Command */
            CMD_ZEN_VOL_LIMIT_READ, 
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
        } 
    };
    struct sVolLimitRead vol;

    if(hdev==NULL) 
        return ZEN_ERROR;

    if(read_packet(hdev,&cbw,&vol,sizeof(struct sVolLimitRead)) == ZEN_SUCC)
        return vol.limit;

    return ZEN_ERROR;
}

int write_vol_limit_pass(usb_dev_handle *hdev, u8 limit, const char* pass) {
    struct sVolLimitWrite vol;
    struct sCBW cbw = {     
        CBW_SIG,     /* CBW Signature */ 
          rand(),    /* Tag */
        sizeof(struct sVolLimitWrite), /* Transfer length */
        CBW_DIR_OUT, /* Direction */
        0x00,        /* Reserved */
        0x10,        /* Length of command */
        {    
            CMD_SCSI_SIGMATEL_WRITE, /* Command */
            CMD_ZEN_VOL_LIMIT_WRITE,
            0x00, 0x12, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
        } 
    };

    if(hdev==NULL) 
        return ZEN_ERROR;

    if(pass && strlen(pass)>16) {
        zen_log("Password too long\n");
        return ZEN_ERROR;
    }

    memset(vol.pass,0,16);
    if(pass)
        strcpy((char*)vol.pass,pass);

    vol.reserved = 0x00;
    vol.limit = limit;


    return (send_packet(hdev,&cbw,&vol,sizeof(struct sVolLimitWrite)));
}

void deinit_zen(usb_dev_handle* hdev) {
    if(hdev) {
        usb_release_interface(hdev, 0);

#if defined(LIBUSB_HAS_DETACH_KERNEL_DRIVER_NP) && (LIBUSB_HAS_DETACH_KERNEL_DRIVER_NP == 1) && defined(NP_DRIVER_DEATTACH)
        usb_attach_kernel_driver_np(hdev, 0);
#endif
/** To prevent -110 (timeout) error */
#ifdef unix
        usb_reset(hdev);
#endif
        usb_close(hdev);
     }
}

