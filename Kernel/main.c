// SimpleOS Entry Point
// Main function is the entry point of the kernel.

#include "multiboot.h"
#include "screen.h"

void Print_Memory_Information(struct multiboot *mboot_ptr) {
    char TEXT_BUFFER[256];

    text_mode_write("Memory Limits:\n");

    //Write out the lower and upper memory limits in the multiboot header
    itoa(mboot_ptr->mem_lower, TEXT_BUFFER, 10);
	
    text_mode_write("Lower Memory: ");
    text_mode_write(TEXT_BUFFER);
    text_mode_write("\n");

    itoa(mboot_ptr->mem_upper, TEXT_BUFFER, 10);
	
    text_mode_write("Upper Memory: ");
    text_mode_write(TEXT_BUFFER);
    text_mode_write("\n");
}

void Print_Loaded_Module_Information(struct multiboot *mboot_ptr) {
    char TEXT_BUFFER[256];

    text_mode_write("Modules Information:\n");
    itoa(mboot_ptr->mods_count, TEXT_BUFFER, 10);
    text_mode_write("Modules Count: ");
    text_mode_write(TEXT_BUFFER);    
    text_mode_write("\n");
}

void Init_GDT() {
   text_mode_write("Initializing GDT\n");
   initialize_gdt();
   text_mode_write("Done Initializing GDT\n");
}

//Main entry point of the Kernel. It is passed the multiboot header by GRUB when the bootloader begins the Kernel execution. (Multiboot header defined in multiboot.h)
int main(struct multiboot *mboot_ptr)
{
    //Clear all the screen as there may be (Probably will be) stuff written on the screen by the bootloader.
    text_mode_clearscreen();

    //Begin the boot procedure.
    text_mode_write("SimpleOS Booting...\n\n");
    Init_GDT();
    text_mode_write("\n");

    //Print out the upper and lower limits of memory
    Print_Memory_Information(mboot_ptr);
    text_mode_write("\n");

    Print_Loaded_Module_Information(mboot_ptr);
    text_mode_write("\n");

    return 0xDEADBABA;
}
