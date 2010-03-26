#include <mm/phys_mm.h>
#include <mm/virt_mm.h>

#include "multiboot.h"

#include "panic.h"

uint32 used_mem_end = 0;

uint32 phys_mm_slock = PHYS_MM_STACK_ADDR;
uint32 phys_mm_smax = PHYS_MM_STACK_ADDR;

extern uint32 paging_enabled;

void init_phys_mm(uint32 start) 
{

	used_mem_end = (start + 0x1000) & PAGE_MASK; //This ensures that the used_mem_end address is on a page-aligned boundry (Which it has to be if I wish to identity map from 0 to used_mem_end)

}

uint32 alloc_frame() 
{
	if (paging_enabled == 0) 
	{
		used_mem_end += 4096; //Add 4096 bytes (4kb) to used_mem_end address
		return used_mem_end - 4096; //Return the old address
		//The reason why this works is that all memory up to used_mem_end is identity mapped when paging is enabled
		//This meens that when paging is enabled the address will be mapped directly to the physical address (0x1000 will still access 0x1000 in memory for example) 
	} 
	else 
	{
		//Paging is enabled	
		if (phys_mm_slock == PHYS_MM_STACK_ADDR)
		{
			PANIC("Error:out of memory."); //This will crash the OS when we run out of frames (Bad idea much?) TODO: Revise this to not crash the OS, at least, not immediately
		}

		// Pop off the stack.
		phys_mm_slock -= sizeof (uint32);
		uint32 * stack = (uint32 *)phys_mm_slock;
		  
		return *stack;
	}
}

void free_frame(uint32 frame) 
{
	if (paging_enabled == 0) 
	{
		return; //If paging isn't enabled we are not going to be able to free a frame of virtual memory and the stacks location is virtual (Cannot be accessed without paging)	
	}	

	if (frame < used_mem_end) 
	{
		return; //Anything under used_mem_end is identity mapped (Physical Address == Virtual Address) never remap it.
	}

	if (phys_mm_smax <= phys_mm_slock) //Run out of stack space *Shock Horror* Allocate this frame to the end of the stack (Giving another 4kb (4096 bytes) of stack space)
	{

	    map (phys_mm_smax, frame, PAGE_PRESENT | PAGE_WRITE);
	    phys_mm_smax += 4096;
	
	}
	else
	{
		//We still have stack space left to pop to,
		uint32 * stack = phys_mm_slock;
		*stack = frame;
		phys_mm_slock += sizeof(uint32);	
	}
}

//Does the initialization of the free pages using the memory map provided by the mboot header.
void map_free_pages(struct multiboot * mboot_ptr) 
{

  uint32 i = mboot_ptr->mmap_addr;

  while (i < mboot_ptr->mmap_addr + mboot_ptr->mmap_length)
  {
    mmap_entry_t *me = (mmap_entry_t*) i;
    
    // Does this entry specify usable RAM?
    if (me->type == 1)
    {

      uint32 j;

      // For every page in this entry, add to the free page stack.
      for (j = me->base_addr_low; j < me->base_addr_low+me->length_low; j += 0x1000)
      {
        free_frame(j);
      }

    }

    // The multiboot specification is strange in this respect - the size member does not include "size" itself in its calculations,
    // so we must add sizeof a 32bit int
    i += me->size + sizeof (uint32);
  }

}
