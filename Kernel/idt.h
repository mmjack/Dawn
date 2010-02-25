#ifndef _INTERRUPT_DESCRIPTOR_TABLE_DEF_H_
#define _INTERRUPT_DESCRIPTOR_TABLE_DEF_H_
#include "int_types.h"

/* A entry onto the interrupt descriptor table */

struct idt_entry
{
    uint16 base_lo;
    uint16 sel;        /* Our kernel segment goes here! */
    uint8 always0;     /* This will ALWAYS be set to 0! */
    uint8 flags;       /* Set using the above table! */
    uint16 base_hi;
} __attribute__((packed));


/* A pointer passed to the processor to show the  size and position of the IDT */

struct idt_ptr
{
    uint16 limit;
    uint32 base;

} __attribute__((packed));

void Initialize_IDT(); //Start the IDT!

#endif //_INTERRUPT_DESCRIPTOR_TABLE_DEF_H_
