#ifndef IDT_H
#define IDT_H

#include <stdint.h>

/* ============================================================
 *  idt.h — Interrupt Descriptor Table declarations
 * ============================================================ */

/* One 8-byte IDT gate descriptor.
   Layout (IA-32 Manual Vol. 3, Figure 6-2):
     bits  0-15  offset_low
     bits 16-31  selector
     bits 32-39  zero (always 0)
     bits 40-47  type_attr (P | DPL<<5 | 0 | D | 1 | 1)
     bits 48-63  offset_high                              */
typedef struct __attribute__((packed)) {
    uint16_t offset_low;   /* Handler address bits 0-15   */
    uint16_t selector;     /* Code segment selector        */
    uint8_t  zero;         /* Reserved, always 0           */
    uint8_t  type_attr;    /* Gate type + DPL + present    */
    uint16_t offset_high;  /* Handler address bits 16-31   */
} idt_entry_t;

/* 6-byte pointer fed to LIDT */
typedef struct __attribute__((packed)) {
    uint16_t limit;        /* sizeof(idt) - 1              */
    uint32_t base;         /* Address of idt[]             */
} idt_ptr_t;

/* CPU-pushed register frame passed to every ISR/IRQ handler.
   Stack at call site (bottom → top of push sequence):
     SS, USERESP, EFLAGS, CS, EIP  ← pushed by CPU
     ERR_CODE, INT_NO               ← pushed by stub
     pushad (EAX..EDI)              ← pushed by stub
     DS                             ← pushed by stub       */
typedef struct __attribute__((packed)) {
    uint32_t ds;
    /* pushad order: EDI ESI EBP (orig)ESP EBX EDX ECX EAX */
    uint32_t edi, esi, ebp, esp, ebx, edx, ecx, eax;
    uint32_t int_no, err_code;
    /* CPU auto-pushed on interrupt entry */
    uint32_t eip, cs, eflags, useresp, ss;
} registers_t;

/* Populate one gate in the IDT.
   @num      vector number (0-255)
   @handler  address of the assembly stub
   @sel      code-segment selector (0x08 for kernel CS)
   @flags    type/attribute byte (0x8E = present, ring0, 32-bit int gate) */
void idt_set_gate(uint8_t num, uint32_t handler, uint16_t sel, uint8_t flags);

/* Initialise all 256 gates and call idt_load() */
void idt_init(void);

/* Defined in isr.asm — loads IDTR */
extern void idt_load(idt_ptr_t *ptr);

#endif /* IDT_H */
