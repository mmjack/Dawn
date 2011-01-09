#include <mm/general.h>
#include <stack/kstack.h>

void initializeMemory(struct multiboot* mboot_ptr, MEM_LOC kernel_start, MEM_LOC kernel_end, MEM_LOC initial_esp)
{
   //Initializes the global descriptor table and the TSS
   initialize_gdt();
   initializeTss();
   printf("GDT [OK]\n");
 
   //Initializes the memory manager and maps free pages
   kernel_end = kernel_end - kernel_start;
   initializePhysicalMemoryManager(kernel_end);
   initializeVirtualMemoryManager(kernel_end);
   map_free_pages(mboot_ptr);
   printf("Memory Managers (PMM, VMM) [OK]\n");

   //Moves the stack to the correct location in memory
   moveStack(USER_STACK_START, USER_STACK_SIZE, initial_esp);
}
