#include "virt_mm.h"
#include "phys_mm.h"

#include "headers/int_types.h"
#include "interrupt_handler.h"
#include "panic.h"
#include "idt.h"

uint8 paging_enabled = 0;
page_directory_t * current_pagedir = 0;

uint32 * page_directory = (uint32 *) PAGE_DIR_VIRTUAL_ADDR;
uint32 * page_tables = (uint32 *) PAGE_TABLE_VIRTUAL_ADDR;

void page_fault (idt_call_registers_t *regs)
{
  PANIC("Page Fault!");
  for (;;) ;
}


inline void switch_page_directory (page_directory_t * nd)
{
  current_pagedir = nd;
  asm volatile ("mov %0, %%cr3" : : "r" (nd));
}

inline void start_paging() {
  uint32 cr0;

  asm volatile ("mov %%cr0, %0" : "=r" (cr0)); //Get the current value of cr0

  cr0 |= 0x80000000; //Bitwise or

  asm volatile ("mov %0, %%cr0" : : "r" (cr0)); //Set the value of cr0 to the new desired value
}

void mark_paging_enabled() {
	paging_enabled = 1;
}

void init_virt_mm() {
	register_interrupt_handler (14, &page_fault); //Register the page fault handler.
	
	uint32 i = 0;

	page_directory_t * pagedir = (page_directory_t *) alloc_frame(); //Paging isn't enabled so this should just give us 4kb at the end of used memory.
	
	//Null it all!!! (Initialize the table)
	memset(pagedir, 0, 4096);

	//TODO: Remake this strip of code
	//Needs replacing - IDMap the first 4MB of memory
	pagedir[0] = alloc_frame() | PAGE_PRESENT | PAGE_WRITE; //Allocate a 4kb "Frame" of memory for this page.
	 
	uint32 * pt = (uint32*) (pagedir[0] & PAGE_MASK); //Pointer to the page directory
 	
	for (i = 0; i < 1024; i++) //Loop 1024 times so 1024 * 4096 bytes of data are mapped (4MB)
		pt[i] = i * 4096 | PAGE_PRESENT | PAGE_WRITE;	

	// Assign the second-last table and zero it. (TODO: Find out if this is necessary)
	pagedir[1022] = alloc_frame() | PAGE_PRESENT | PAGE_WRITE;
	pt = (uint32*) (pagedir[1022] & PAGE_MASK);
	memset (pt, 0, 0x1000);
	pt[1023] = (uint32)pagedir | PAGE_PRESENT | PAGE_WRITE; //The last entry of table 1022 is the page directory

	pagedir[1023] = (uint32)pagedir | PAGE_PRESENT | PAGE_WRITE; //Loop back to the page directory

	switch_page_directory(pagedir); //Set the current page dire
	start_paging();

	uint32 pt_idx = PAGE_DIR_IDX((PHYS_MM_STACK_ADDR  >> 12));
	page_directory[pt_idx] = alloc_frame() | PAGE_PRESENT | PAGE_WRITE;
	memset (page_tables[pt_idx*1024], 0, 0x1000);

	mark_paging_enabled();
}


