#include <mm/virt_mm.h>
#include <mm/phys_mm.h>

#include <interrupts/interrupt_handler.h>
#include <panic/panic.h>
#include <interrupts/idt.h>
#include <debug/debug.h>
#include <types/memory.h>
#include <process/process.h>
#include <process/procfault.h>
#include <debug/debug.h>
#include <interrupts/interrupt_handler.h>
#include <interrupts/interrupts.h>
#include <mm/virtual.h>

#define _reload_cr3() \
      __asm__ __volatile__ ("push %eax;mov %cr3,%eax;mov %eax,%cr3;pop %eax");

#define _flush_tlb_single(addr) \
   __asm__ __volatile__("invlpg %0": :"m" (*(char *) addr))

uint8_t paging_setup = 0;
uint8_t paging_enabled = 0;

page_directory_t* current_pagedir = 0;
page_directory_t* kernel_pagedir = 0;

uint32_t* page_directory = (uint32_t*) PAGE_DIR_VIRTUAL_ADDR;
uint32_t* page_tables = (uint32_t*) PAGE_TABLE_VIRTUAL_ADDR;

POINTER kernelFirstFreeVirtualAddress();
char getMapping(MEM_LOC va, MEM_LOC* pa);
char getPageEntry(MEM_LOC va, MEM_LOC* pa);

unsigned int PAGE_SIZE = 4096;

void switchPageDirectory(page_directory_t* nd);
void startPaging();

void page_fault(idt_call_registers_t regs) {
	int present = regs.err_code & 0x1 ? 1 : 0; // Page not present
	int rw = regs.err_code & 0x2 ? 1 : 0; // Write operation?
	int us = regs.err_code & 0x4 ? 1 : 0; // Processor was in user-mode?
	int reserved = regs.err_code & 0x8 ? 1 : 0; // Overwritten CPU-reserved bits of page entry?
	int id = regs.err_code & 0x10 ? 1 : 0; // Caused by an instruction fetch?

	//Read the address at which the fault occured from CR2 (Where the page fault handler puts it)
	uint32_t faulting_address;
	__asm__ volatile("mov %%cr2, %0" : "=r" (faulting_address));

	int mapping = getMapping(faulting_address, 0);

	char buffer[1024];
	memset(buffer, 0, 1024);

	printFormattedStringToBuffer(buffer, "Page fault at location 0x%x (present: %i write: %i us: %i reserved: %i instr: %i mapping: %i)", faulting_address, present, rw, us, reserved, id, mapping);

	handleFatalProcessFault(FAULT_ID_PAGEFAULT, buffer);

	PANIC("Ahhh (Virtual memory manager set to crash on pagefault)\n");
}

//Map the virtual address VA to the physical address PA with the appropriate flags.
//VA = Virtual Address, PA = Physical Address, Flags = The flags to be set with the page.
void ia32_map(MEM_LOC va, MEM_LOC pa, uint32_t flags) {

	MEM_LOC virtual_page = (MEM_LOC)(((MEM_LOC) va) / 0x1000);
	PAGE_INDEX pt_idx = PAGE_DIR_IDX(virtual_page); //Page table index

	// Find the appropriate page table for the physical address.
	// If the page table does not exist then create a new one for it.
	if (page_directory[pt_idx] == 0) {

		//Null page table (Needs to be created) so allocate a frame and initialize (Null) it.
		page_directory[pt_idx] = (allocateFrame()) | PAGE_PRESENT | PAGE_USER
				| PAGE_WRITE;

		//Reload the CR3 register to update virtual mappings
		_reload_cr3();

		unsigned int i = 0;

		for (i = 0; i < 1024; i++) {
			page_tables[(pt_idx * 1024) + i] = 0;
		}

	}

	// Now that the page table definately exists, we can update the PTE.
	page_tables[(MEM_LOC) virtual_page] = (((MEM_LOC) pa)) | (flags & 0xFFF);

	_flush_tlb_single(va);
}

//Unmap the virtual address VA from its physical address
void ia32_unmap(POINTER va) {
	POINTER virtual_page = (POINTER)(((MEM_LOC) va) / 0x1000);

	page_tables[(MEM_LOC) virtual_page] = 0;

	//Signal to the CPU that a page mapping has been invalidated.
	_flush_tlb_single((MEM_LOC)va);
}

//Get whether the virtual address is mapped or not (Returns 1 or 0, true or false)
//If Pa is non null sets pa to the physical address of the mapping
char getMapping(MEM_LOC va, MEM_LOC *pa) {
	uint32_t virtual_page = va / 0x1000;
	uint32_t pt_idx = PAGE_DIR_IDX(virtual_page);

	// Find the appropriate page table for 'va'.
	if (page_directory[pt_idx] == 0)
		return 0;

	if (page_tables[virtual_page] != 0) {
		if (pa)
			*pa = page_tables[virtual_page] & PAGE_MASK;
		return 1;
	}

	return 0;
}

//Get Page entry
char getPageEntry(MEM_LOC va, MEM_LOC* pa) {
	uint32_t virtual_page = va / 0x1000;
	uint32_t pt_idx = PAGE_DIR_IDX(virtual_page);

	// Find the appropriate page table for 'va'.
	if (page_directory[pt_idx] == 0) {
		return 0;
	}

	if (page_tables[virtual_page] != 0) {
		if (pa) {
			*pa = page_tables[virtual_page];
		}
		return 1;
	}

	return 0;
}

inline void switchPageDirectory(page_directory_t* nd) {
	current_pagedir = nd;
	//Move the page directory into cr3
	__asm__ volatile ("mov %0, %%cr3" : : "r" (nd));
}

inline void startPaging() {
	uint32_t cr0;
	//Get the current value of cr0
	__asm__ volatile ("mov %%cr0, %0" : "=r" (cr0));
	cr0 |= 0x80000000; //Bitwise or
	//Set the value of cr0 to the new desired value
	__asm__ volatile ("mov %0, %%cr0" : : "r" (cr0));
}

void markPagingEnabled() {
	paging_setup = 1;
	paging_enabled = 1;
}

uint32_t getTable(uint32_t address) {
	return address / PAGE_SIZE / 1024;
}

/**
 * Used in initialisation only!
 * Identity maps the first page of memory
 * TODO: Work out what this actually does
 */
//TODO: Fix up this code - Possible problems = frame out of first 4mb causing a boot failure
void identityMapPages(page_directory_t* pagedir) {

	//Map a page at the end of used memory
	MEM_LOC frame = allocateFrame();
	pagedir[0] = frame | PAGE_PRESENT | PAGE_USER;

	//Create a pointer to the new page table
	LPOINTER pt = (POINTER) frame; //Pointer to the page directory

	//Iterate through, setting each page to the correct location in memories
	//Loop 1024 times so 1024 * 4096 bytes of data are mapped (4MB)
	for (unsigned int i = 0; i < 1024; i++) {
		pt[i] = (i * PAGE_SIZE) | PAGE_PRESENT | PAGE_USER;
	}

	//Set the KERNEL_START address to the pagedir address
	pagedir[getTable(KERNEL_START)] = pagedir[0];
}

/**
 * @brief Initialize the virtual memory manager (Keeps track of virtual memory mappings and handles the mapping of logical addresses to physical ones.
 * @callgraph
 */

void initializeVirtualMemoryManager(MEM_LOC mem_end) {
	unsigned int i = 0; //Used as a iterator throughout the function

	registerInterruptHandler(14, (isr_t) &page_fault); //Register the page fault handler.
	//Called when bad little processes try to access memory they can't (Doesn't exist or not available to them)

	page_directory_t * pagedir = (page_directory_t *) allocateFrame(); //Paging isn't enabled so this should just give us 4kb at the end of used memory.

	//Null it all!!! (Clear the page directory)
	memset(pagedir, 0, PAGE_SIZE);

	//Identity map the first 4MBs of memory to KERNEL_START and 0x0
	identityMapPages(pagedir);

	// Assign the second-last table and zero it.
	MEM_LOC frame = allocateFrame(); //Allocate a 4KB frame
	pagedir[1022] = (frame & PAGE_MASK) | PAGE_PRESENT | PAGE_USER; //Set the 1022nd page table to the new frame address

	LPOINTER pt = (POINTER) frame; //Pointer to the new frame
	memset(pt, 0, PAGE_SIZE); //Null the frame

	pt[1023] = ((MEM_LOC) pagedir & PAGE_MASK) | PAGE_PRESENT | PAGE_USER; //The last entry of table 1022 is the page directory. So when paging is active PAGE_DIR_VIRTUAL_ADDR = the page directory

	pagedir[1023] = ((MEM_LOC) pagedir & PAGE_MASK) | PAGE_PRESENT | PAGE_USER; //Loop back to the page directory. Causing page_tables to link back to the phyical page tables

	switchPageDirectory(pagedir); //Set the current page dirm

	startPaging(); //Paging should already be active (Higher-Half Kernel) - But just to be sure

	kernel_pagedir = pagedir; //Set the kernel page directory to the directory that was just created

	//Map where the physical memory manager keeps its stack. (Just the page table), this is because on the first expansion of the stack the map would need 2 frames while only having one (The first free page of memory) available. Bad things would follow
	uint32_t pt_idx = PAGE_DIR_IDX((PHYS_MM_STACK_ADDR / 0x1000));

	frame = allocateFrame(); //Allocate a frame for the page table
	page_directory[pt_idx] = (frame & PAGE_MASK) | PAGE_PRESENT | PAGE_USER;

	//Nulll it
	memset((POINTER) frame, 0, PAGE_SIZE);

	for (i = getTable(KERNEL_START); i < 1022; i++) {
		if (page_directory[i] == 0) {
			MEM_LOC address = allocateFrame();
			page_directory[i] = (address & PAGE_MASK) | PAGE_PRESENT | PAGE_USER;
			_reload_cr3();
		}
	}

	//Mark paging enabled (Other areas of the kernel will now consider paging to be active)
	markPagingEnabled();
}

MEM_LOC copyPage(MEM_LOC pt, process_t* process) {

	MEM_LOC new_page_addr = allocateFrameForProcess(process);

	MEM_LOC temp_read_addr = kernelFirstFreeVirtualAddress();
	map((MEM_LOC) temp_read_addr, (MEM_LOC) pt, MEMORY_RESTRICTED_ACCESS);

	MEM_LOC temp_write_addr = kernelFirstFreeVirtualAddress();
	map((MEM_LOC) temp_write_addr, (MEM_LOC) new_page_addr,
			MEMORY_RESTRICTED_ACCESS);

	memcpy(temp_write_addr, temp_read_addr, PAGE_SIZE);

	unmap(temp_read_addr);
	unmap(temp_write_addr);

	return new_page_addr;
}

MEM_LOC copyPageTable(MEM_LOC pt, uint8_t copy, process_t* process) {

	MEM_LOC new_page_table = allocateFrameForProcess(process);

	LPOINTER temp_read_addr = kernelFirstFreeVirtualAddress();
	map(temp_read_addr, pt, MEMORY_RESTRICTED_ACCESS);

	LPOINTER temp_write_addr = kernelFirstFreeVirtualAddress();
	map(temp_write_addr, new_page_table, MEMORY_RESTRICTED_ACCESS);
	memset(temp_write_addr, 0, PAGE_SIZE);

	unsigned int i = 0;
	for (i = 0; i < 1024; i++) {
		if ((temp_read_addr[i]) != 0) {
			if (copy == 1) {
				MEM_LOC New_Frame = copyPage(temp_read_addr[i] & ~(0xFFF),
						process);
				temp_write_addr[i] = New_Frame | PAGE_PRESENT | PAGE_USER
						| PAGE_WRITE;
			} else {
				temp_write_addr[i] = temp_read_addr[i];
			}
		} else {
			temp_write_addr[i] = 0;
		}
	}

	unmap(temp_read_addr);
	unmap(temp_write_addr);

	return new_page_table;
}

page_directory_t* copyPageDir(page_directory_t* pagedir, process_t* process) {
	disableInterrupts(); //Disable interrupts

	page_directory_t* return_location =
			(page_directory_t*) allocateFrameForProcess(process);

	LPOINTER being_copied = kernelFirstFreeVirtualAddress();
	map(being_copied, pagedir, MEMORY_RESTRICTED_ACCESS);

	LPOINTER copying_to = kernelFirstFreeVirtualAddress();
	map(copying_to, return_location, MEMORY_RESTRICTED_ACCESS);
	memset(copying_to, 0, PAGE_SIZE);

	//First 4 megabytings are ID Mapped. Kernel pages are identical across all page directories. The rest gets copied
	copying_to[0] = being_copied[0];

	for (unsigned int i = 1; i < getTable(KERNEL_START); i++) {
		if ((being_copied[i]) != 0) {
			MEM_LOC Location = copyPageTable(being_copied[i] & ~(0xFFF), 1,
					process);
			copying_to[i] = Location | PAGE_PRESENT | PAGE_USER | PAGE_WRITE;
		} else {
			copying_to[i] = 0;
		}
	}

	for (unsigned int i = getTable(KERNEL_START); i < 1022; i++) {
		copying_to[i] = being_copied[i];
	}

	// Assign the second-last table and zero it.
	MEM_LOC frame = allocateFrameForProcess(process);
	copying_to[1022] = frame | PAGE_PRESENT | PAGE_USER;

	LPOINTER pt = kernelFirstFreeVirtualAddress();
	map(pt, frame, MEMORY_RESTRICTED_ACCESS);
	memset(pt, 0, PAGE_SIZE);

	LPOINTER opt = kernelFirstFreeVirtualAddress();
	map(opt, being_copied[1022], MEMORY_RESTRICTED_ACCESS);

	memcpy(pt, opt, PAGE_SIZE);

	pt[1023] = ((MEM_LOC) return_location & PAGE_MASK) | PAGE_PRESENT
			| PAGE_USER; //The last entry of table 1022 is the page directory

	unmap(pt);
	unmap(opt);

	copying_to[1023] = ((MEM_LOC) return_location & PAGE_MASK) | PAGE_PRESENT | PAGE_USER; //Loop back address

	unmap(being_copied);
	unmap(copying_to);

	return return_location;
}

//Search through the kernel reserved memory find a unmapped page and map it so it can be used by the kernel for some temp process
POINTER kernelFirstFreeVirtualAddress() {

	//Start after the IDMapped region
	for (unsigned int i = KERNEL_RESERVED_START; i < KERNEL_MEMORY_END; i += PAGE_SIZE) {
		if (getMapping(i, 0) == 0) {
			return i;
		}
	}

	return 0;
}

//Definition of MAP - architecture specific calls made here
void map(MEM_LOC va, MEM_LOC pa, unsigned char flags) {
	if (flags & MEMORY_RESTRICTED_ACCESS) {
		ia32_map(va, pa, PAGE_PRESENT | PAGE_USER);
	} else {
		ia32_map(va, pa, PAGE_PRESENT | PAGE_USER | PAGE_WRITE);
	}

}

//Definition of UNMAP - architecture specific calls made here
void unmap(MEM_LOC virtual_location) {
	ia32_unmap(virtual_location);
}
