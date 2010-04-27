#include "mouse_generic_ps2.h"
#include <common.h>
#include <interrupts/irq.h>
#include <interrupts/idt.h>
#include <interrupts/interrupt_handler.h>
#include <input/devices.h>

uint8 mouse_cycle=0;     //unsigned char
int8 mouse_byte[3];    //signed char
mouse_input_t istruct;

void mouse_isr_callback(idt_call_registers_t regs) 
{
  switch(mouse_cycle)
  {
    case 0:
      mouse_byte[0]=inb(PORT_1);
      mouse_cycle++;
      break;
    case 1:
      mouse_byte[1]=inb(PORT_1);
      mouse_cycle++;
      break;
    case 2:
      mouse_byte[2]=inb(PORT_1);
      istruct.i_byte = mouse_byte[0];
      istruct.mouse_x = mouse_byte[1];
      istruct.mouse_y = mouse_byte[2];
      send_input_message(DEVICE_MOUSE, 0, &istruct);
      mouse_cycle=0;
      break;
  }
}

//Wait tell BIT_1 of PORT_1 is not set
void wait_mouse_out()
{
uint8 readbyte;

	while (1)
	{
		readbyte = inb(PORT_1);
		if (readbyte & BIT_1)
		{
			//BIT_1 still set
		}
		else
		{
			break;
		}
	}


}


//Wait untill bit 0 of PORT_2 Is set.
void wait_mouse_in()
{
uint8 readbyte;

	while (1)
	{
		readbyte = inb(PORT_2);
		if (readbyte & BIT_0)
		{
			//BIT_0 still set
			break;
		}
		else
		{
			//0 Not set
		}
	}


}

void send_mouse_command(uint8 cmd)
{
	wait_mouse_out();
	outb(PORT_2, CMD);

	wait_mouse_out();
	outb(PORT_1, cmd);
}

uint8 read_mouse_data()
{
	wait_mouse_in();
	uint8 ret = inb(PORT_2);

	return ret;
}

void init_mouse()
{
	register_interrupt_handler(GET_IRQ(12), mouse_isr_callback);

	uint8 _status;

	wait_mouse_out();
	outb(0x64, 0xA8); //Enable the auxillery mouse device

	//Enable interrupts
	wait_mouse_out();
	outb(0x64, 0x20);

	wait_mouse_in();
	_status = (inb(0x60) | 2);
	wait_mouse_out();
	outb(0x64, 0x60);
	wait_mouse_out();
	outb(0x60, _status);
	
	send_mouse_command(0xF6);
	read_mouse_data(); //ACK
	send_mouse_command(0xF4);
	read_mouse_data(); //ACK
}
