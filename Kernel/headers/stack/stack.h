#ifndef _STACK_DEF_H_
#define _STACK_DEF_H_
#include <common.h>
#include <types/size_t.h>

/* A set of utility functions concerning the stack */

typedef uint32* stack_t;

stack_t move_stack(stack_t new_start, size_t size, size_t initial_esp);

#endif //_STACK_DEF_H_
