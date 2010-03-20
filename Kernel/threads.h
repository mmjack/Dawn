#ifndef _THREAD_DEF_H_
#define _THREAD_DEF_H_
#include <types/int_types.h>
#include <stdlib.h>
#include <printf.h>
#include "common.h"

#define RR_QUANT 100

typedef struct {
	uint32 t_id;
	uint32 esp, ebp, ebx, esi, edi;
} thread_t;

struct thread_entry;

struct thread_entry {
	uint32 quant;
	thread_t * thread;
	struct thread_entry * next;
};

typedef struct thread_entry thread_entry_t;

void initialize_thread_scheduler(thread_t * start);
void thread_schedule_tick();
void add_thread(thread_t * thread);

thread_t * initialize_threading();
thread_t * create_thread (int (*fn)(void*), void *arg, uint32 *stack);
void switch_thread (thread_t * next);
uint32 current_thread_id();

#endif
