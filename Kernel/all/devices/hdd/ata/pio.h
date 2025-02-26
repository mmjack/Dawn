#ifndef _PIO_TRANSFER_DEF_H_
#define _PIO_TRANSFER_DEF_H_
#include <types/stdint.h>
#include <stdio.h>
#include <devices/hdd/disk_device.h>
typedef disk_device pio_device;

uint8_t pio_soft_reset(pio_device dev);

void pio_init();

#endif //_PIO_TRANSFER_DEF_H_
