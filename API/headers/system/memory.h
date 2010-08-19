#ifndef _MEMORY_DEF_H_
#define _MEMORY_DEF_H_
#include <types/memory.h>
#include <syscall/syscall.h>

MEM_LOC getNumberOfFreeFrames();
unsigned long getPageSize();

#endif //_MEMORY_DEF_H_
