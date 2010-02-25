#include "idt.h"

struct idt_entry idt[256];
struct idt_ptr idt_ptr;

extern void idt_flush(uint32 ptr);

//This loads the IDT
void Initialize_IDT() {
	idt_ptr.limit = sizeof(struct idt_entry) * 256 -1;
   	idt_ptr.base  = (uint32)&idt;

	idt_flush((uint32) &idt_ptr);
}
