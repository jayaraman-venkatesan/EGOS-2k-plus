/*
 * (C) 2022, Cornell University
 * All rights reserved.
 */

/* Author: Yunhao Zhang
 * Description: a simple disk device driver
 *
 * updated by CS6640 23fall staff
 */

#include "egos.h"
#include "disk.h"
#include "bus_gpio.c"
#include <string.h>
#include "sd/sd.h"

enum {
      FLASH_ROM,
      SD_CARD
};
static int type;

int disk_read(int block_no, int nblocks, void* dst) {
    if (type == FLASH_ROM) {
        char* src = (char*)0x20800000 + block_no * BLOCK_SIZE;
        memcpy(dst, src, nblocks * BLOCK_SIZE);
    } else if (type == SD_CARD) {
        sdread(block_no, nblocks, dst);
    } else {
        ASSERT(0, "disk type is unknown");
    }
    return 0;
}

int disk_write(int block_no, int nblocks, void* src) {
    if (type == FLASH_ROM) {
        FATAL("Attempt to write the on-board ROM");
    } else if (type == SD_CARD) {
        sdwrite(block_no, nblocks, src);
    } else {
        ASSERT(0, "write to disk type unknown");
    }
    return 0;
}

void disk_init() {
    earth->disk_read = disk_read;
    earth->disk_write = disk_write;

#ifdef SDON
    sdinit();
    type = SD_CARD;
#else
    type = FLASH_ROM;
#endif
    return;
}
