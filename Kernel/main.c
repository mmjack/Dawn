// SimpleOS Entry Point
// Main function is the entry point of the kernel.

#include "multiboot.h"
#include "screen.h"
#include "gdt.h"
#include "idt.h"
#include "interrupt_handler.h"
#include "cmos_time.h"
#include "panic.h"
#include "cmos.h"
#include "headers/printf.h"
#include "phys_mm.h"
#include "virt_mm.h"

extern uint32 end; //The end of the kernel

void Print_Memory_Information(struct multiboot *mboot_ptr) {

    prints("Memory Limits:\n");
	
    prints("Lower Memory: ");
    printuh(mboot_ptr->mem_lower);
    prints("\n");
	
    prints("Upper Memory: ");
    printuh(mboot_ptr->mem_upper);
    prints("\n");
}

void Print_Loaded_Module_Information(struct multiboot *mboot_ptr) {

    prints("Modules Information:\n");
    prints("Modules Count: ");
    printi(mboot_ptr->mods_count); 
    prints("\n");
}

void Init_GDT() {
   prints("Initializing GDT");

   initialize_gdt();

   text_mode_set_x(70);
   text_mode_set_fg_color(GREEN);
   text_mode_write("[ OK ]");
   text_mode_set_fg_color(WHITE);

   prints("\n");
}

void Init_IDT() {
   prints("Initializing IDT");
   
   Initialize_IDT();
   text_mode_set_x(70);
   text_mode_set_fg_color(GREEN);
   text_mode_write("[ OK ]");
   text_mode_set_fg_color(WHITE);

   prints("\n");
}

void Init_Timer() {
   prints("Initializing the system timer");

   init_timer(50);

   text_mode_set_x(70);
   text_mode_set_fg_color(GREEN);
   text_mode_write("[ OK ]");
   text_mode_set_fg_color(WHITE);

   prints("\n");
}

void Print_Time_Info() {
	cmos_time_struct LoadedTime;
	fill_cmos_tstruct(&LoadedTime);

	char TBuf[128];
	itoa(LoadedTime.seconds, TBuf, 10);
	text_mode_write("Seconds: ");
	text_mode_write(TBuf);
	text_mode_write("\n");

	itoa(LoadedTime.minutes, TBuf, 10);
	text_mode_write("Minutes: ");
	text_mode_write(TBuf);
	text_mode_write("\n");

	itoa(LoadedTime.hours, TBuf, 10);
	text_mode_write("Hours: ");
	text_mode_write(TBuf);
	text_mode_write("\n");

	itoa(LoadedTime.dayofweek, TBuf, 10);
	text_mode_write("Day of the week: ");
	text_mode_write(TBuf);
	text_mode_write("\n");

	itoa(LoadedTime.day, TBuf, 10);
	text_mode_write("Day of the month: ");
	text_mode_write(TBuf);
	text_mode_write("\n");

	itoa(LoadedTime.month, TBuf, 10);
	text_mode_write("Month: ");
	text_mode_write(TBuf);
	text_mode_write("\n");

	itoa(LoadedTime.year, TBuf, 10);
	text_mode_write("Year: ");
	text_mode_write(TBuf);
	text_mode_write("\n");

	itoa(LoadedTime.century, TBuf, 10);
	text_mode_write("Century: ");
	text_mode_write(TBuf);
	text_mode_write("\n");
}

void Init_VM(struct multiboot * mboot_ptr) { //Initialize virtual memory

   prints("Initializing virtual memory");
   
   init_phys_mm(end);
   init_virt_mm();
   map_free_pages(mboot_ptr);
   text_mode_set_x(70);
   text_mode_set_fg_color(GREEN);
   text_mode_write("[ OK ]");
   text_mode_set_fg_color(WHITE);

   prints("\n");
}

//Main entry point of the Kernel. It is passed the multiboot header by GRUB when the bootloader begins the Kernel execution. (Multiboot header defined in multiboot.h)
int main(struct multiboot *mboot_ptr)
{
    //Clear all the screen as there may be (Probably will be) stuff written on the screen by the bootloader.
    text_mode_clearscreen();

    //Begin the boot procedure.
    prints("SimpleOS Booting...\n\n");

    Init_GDT();
    Init_IDT();
    Init_Timer();
    Init_VM(mboot_ptr);
    prints("\n");

    //Print out the upper and lower limits of memory
    Print_Memory_Information(mboot_ptr);
    prints("\n");

    Print_Loaded_Module_Information(mboot_ptr);
    prints("\n");

    enable_interrupts();

    printf("Performing some virtual memory texts. tests\n");
    uint32 fraddr = 0;
    uint32 virtaddr = 3765694;
    fraddr = alloc_frame();
    printf("Maping physical location 0x%x to virtual location 0x%x\n", fraddr, virtaddr);
    map(virtaddr, fraddr, PAGE_PRESENT | PAGE_WRITE);
    printf("Done\n");

    uint32 * ptr = virtaddr; //At the start of our now mapped frame
    uint32 i;
    for (i = 0; i < 1024; i++) {
	*ptr = 32;
	ptr++;
    }

    printf("Assigned the hole frame to the number 32 1024 times\nProceeding to print the first 3 ints of the frame.\n");
    ptr = virtaddr;
    
    for (i = 0; i < 3; i++) {
	printf("Number %i Value %i\n", i, *ptr);    	
	ptr++;
    }

    printf("Unmapping the page and trying to access pointer (This should throw a page fault and crash the kernel)\n");
    unmap(virtaddr);
    *ptr = 3;

    for (;;)

    return 0xDEADBABA;
}
