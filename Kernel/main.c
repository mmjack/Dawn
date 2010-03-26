// SimpleOS Entry Point
// Main function is the entry point of the kernel.

#include "multiboot.h"
#include "drivers/screen.h"
#include "gdt.h"
#include "cmos_time.h"
#include "panic.h"
#include "cmos.h"
#include <printf.h>
#include <mm/phys_mm.h>
#include <mm/virt_mm.h>
#include <heap/heap.h>
#include "threads.h"
#include "fs/initrd.h"
#include "reboot.h"

#include <fs/vfs.h>
#include <fs/dir.h>

#include <stdlib.h>

//Main entry point of the Kernel. It is passed the multiboot header by GRUB when the bootloader begins the Kernel execution. (Multiboot header defined in multiboot.h)
int main(struct multiboot *mboot_ptr)
{
    init_kernel(mboot_ptr, 0);

    enable_interrupts();

    post_init();
    for (;;) {  }

    return 0xDEADBABA;
}
