#include "pio.h"
#include <common.h>
#include "ide.h"

//Returns 1 if fail
//Returns 0 if success
uint8 pio_28bit_wait_busy_drq(pio_device device, uint32 timeout)
{
	uint8 timeout_timer = timeout;

	for (;;)
	{
		if (timeout_timer < 1)
		{
			return 1; //Fail
		}

		uint8 rb = inb(device.device_ctrl);
		if ((rb & 0x80) == 0)
		{
			//BSY Not set
			if ((rb & 0x8) == 0x8)
			{
				//DRQ Set
				break;
			}
		}
	}

	return 0;
}

//PIO 28bit read
//Returns 1 if success
//Returns 0 if fail
uint8 pio_28bit_read(pio_device device, uint16* destination, uint32 lba, uint8 numberOfSectors)
{

	outb(device.device_base + REG_DEVSEL, 0xE0 | (device.disk << 4) | ((lba >> 24) & 0x0F));  //Select the device (Slave or master) and send the last 4 bits of the LBA
	outb(device.device_base + 1, 0); //Null the register
	outb(device.device_base + 2, numberOfSectors); //Set the numberOfSectors
	outb(device.device_base + 3, (uint8) lba); //First 8 bits of the lba
	outb(device.device_base + 4, (uint8) (lba >> 8)); //Bits 8-16 of the LBA
	outb(device.device_base + 5, (uint8) (lba >> 16)); //Bits 16-24 of the LBA
	outb(device.device_base + 7, 0x20); //Send the read sectors command

	unsigned int toRead = numberOfSectors;
	if (numberOfSectors == 0)
	{
		toRead = 256;
	}

	for (;;)
	{
		if (toRead < 1)
		{
			break;
		}

		inb(device.device_base); //Wait 400ns
		inb(device.device_base);
		inb(device.device_base);
		inb(device.device_base);

		if (pio_28bit_wait_busy_drq(device, RESET_TIMEOUT) == 1)
		{
			printf("ERROR: DISK READ ERROR WAIT TIMEOUT\n");
			return 0;
		}

		unsigned int i = 0;
		for (i = 0; i < 256; i++) {		
			*destination = inw(device.device_base);
			destination++;
		}

		toRead--;
	}

	return 1;
}
