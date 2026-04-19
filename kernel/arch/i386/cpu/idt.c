#include "../include/kernel_cpu.h"

#define IDT_ENTRIES 256

typedef struct {
    uint16_t offset_low;
    uint16_t selector;
    uint8_t ist;
    uint8_t type_attr;
    uint16_t offset_middle;
    uint32_t offset_high;
    uint32_t reserved;
} __attribute__((packed)) idt_entry_t;

typedef struct {
    uint16_t limit;
    uint32_t base;
} __attribute__((packed)) idt_ptr_t;

static idt_entry_t idt[IDT_ENTRIES];
static idt_ptr_t idt_ptr;

extern void brights_i386_isr0(void);
extern void brights_i386_isr1(void);
extern void brights_i386_isr2(void);
extern void brights_i386_isr3(void);
extern void brights_i386_isr4(void);
extern void brights_i386_isr5(void);
extern void brights_i386_isr6(void);
extern void brights_i386_isr7(void);
extern void brights_i386_isr8(void);
extern void brights_i386_isr9(void);
extern void brights_i386_isr10(void);
extern void brights_i386_isr11(void);
extern void brights_i386_isr12(void);
extern void brights_i386_isr13(void);
extern void brights_i386_isr14(void);
extern void brights_i386_isr15(void);
extern void brights_i386_isr16(void);
extern void brights_i386_isr17(void);
extern void brights_i386_isr18(void);
extern void brights_i386_isr19(void);
extern void brights_i386_isr20(void);
extern void brights_i386_isr21(void);
extern void brights_i386_isr22(void);
extern void brights_i386_isr23(void);
extern void brights_i386_isr24(void);
extern void brights_i386_isr25(void);
extern void brights_i386_isr26(void);
extern void brights_i386_isr27(void);
extern void brights_i386_isr28(void);
extern void brights_i386_isr29(void);
extern void brights_i386_isr30(void);
extern void brights_i386_isr31(void);

extern void brights_i386_irq0(void);
extern void brights_i386_irq1(void);
extern void brights_i386_irq2(void);
extern void brights_i386_irq3(void);
extern void brights_i386_irq4(void);
extern void brights_i386_irq5(void);
extern void brights_i386_irq6(void);
extern void brights_i386_irq7(void);
extern void brights_i386_irq8(void);
extern void brights_i386_irq9(void);
extern void brights_i386_irq10(void);
extern void brights_i386_irq11(void);
extern void brights_i386_irq12(void);
extern void brights_i386_irq13(void);
extern void brights_i386_irq14(void);
extern void brights_i386_irq15(void);

static void idt_set_gate(int num, void *handler, uint16_t selector, uint8_t ist, uint8_t type)
{
    uint32_t addr = (uint32_t)handler;
    idt[num].offset_low = addr & 0xFFFF;
    idt[num].offset_middle = (addr >> 16) & 0xFFFF;
    idt[num].offset_high = (addr >> 32) & 0xFFFFFFFF;
    idt[num].selector = selector;
    idt[num].ist = ist;
    idt[num].type_attr = type;
    idt[num].reserved = 0;
}

void brights_i386_idt_init(void)
{
    idt_ptr.limit = sizeof(idt) - 1;
    idt_ptr.base = (uint32_t)idt;
    
    for (int i = 0; i < 32; i++) {
        idt_set_gate(i, (void*)((uint32_t*)&brights_i386_isr0)[i], 0x08, 0, 0x8E);
    }
    
    idt_set_gate(0, (void*)brights_i386_isr0, 0x08, 0, 0x8E);
    idt_set_gate(1, (void*)brights_i386_isr1, 0x08, 0, 0x8E);
    idt_set_gate(2, (void*)brights_i386_isr2, 0x08, 0, 0x8E);
    idt_set_gate(3, (void*)brights_i386_isr3, 0x08, 0, 0x8E);
    idt_set_gate(4, (void*)brights_i386_isr4, 0x08, 0, 0x8E);
    idt_set_gate(5, (void*)brights_i386_isr5, 0x08, 0, 0x8E);
    idt_set_gate(6, (void*)brights_i386_isr6, 0x08, 0, 0x8E);
    idt_set_gate(7, (void*)brights_i386_isr7, 0x08, 0, 0x8E);
    idt_set_gate(8, (void*)brights_i386_isr8, 0x08, 0, 0x8E);
    idt_set_gate(9, (void*)brights_i386_isr9, 0x08, 0, 0x8E);
    idt_set_gate(10, (void*)brights_i386_isr10, 0x08, 0, 0x8E);
    idt_set_gate(11, (void*)brights_i386_isr11, 0x08, 0, 0x8E);
    idt_set_gate(12, (void*)brights_i386_isr12, 0x08, 0, 0x8E);
    idt_set_gate(13, (void*)brights_i386_isr13, 0x08, 0, 0x8E);
    idt_set_gate(14, (void*)brights_i386_isr14, 0x08, 0, 0x8E);
    idt_set_gate(15, (void*)brights_i386_isr15, 0x08, 0, 0x8E);
    idt_set_gate(16, (void*)brights_i386_isr16, 0x08, 0, 0x8E);
    idt_set_gate(17, (void*)brights_i386_isr17, 0x08, 0, 0x8E);
    idt_set_gate(18, (void*)brights_i386_isr18, 0x08, 0, 0x8E);
    idt_set_gate(19, (void*)brights_i386_isr19, 0x08, 0, 0x8E);
    idt_set_gate(20, (void*)brights_i386_isr20, 0x08, 0, 0x8E);
    idt_set_gate(21, (void*)brights_i386_isr21, 0x08, 0, 0x8E);
    idt_set_gate(22, (void*)brights_i386_isr22, 0x08, 0, 0x8E);
    idt_set_gate(23, (void*)brights_i386_isr23, 0x08, 0, 0x8E);
    idt_set_gate(24, (void*)brights_i386_isr24, 0x08, 0, 0x8E);
    idt_set_gate(25, (void*)brights_i386_isr25, 0x08, 0, 0x8E);
    idt_set_gate(26, (void*)brights_i386_isr26, 0x08, 0, 0x8E);
    idt_set_gate(27, (void*)brights_i386_isr27, 0x08, 0, 0x8E);
    idt_set_gate(28, (void*)brights_i386_isr28, 0x08, 0, 0x8E);
    idt_set_gate(29, (void*)brights_i386_isr29, 0x08, 0, 0x8E);
    idt_set_gate(30, (void*)brights_i386_isr30, 0x08, 0, 0x8E);
    idt_set_gate(31, (void*)brights_i386_isr31, 0x08, 0, 0x8E);
    
    idt_set_gate(32, (void*)brights_i386_irq0, 0x08, 0, 0x8E);
    idt_set_gate(33, (void*)brights_i386_irq1, 0x08, 0, 0x8E);
    idt_set_gate(34, (void*)brights_i386_irq2, 0x08, 0, 0x8E);
    idt_set_gate(35, (void*)brights_i386_irq3, 0x08, 0, 0x8E);
    idt_set_gate(36, (void*)brights_i386_irq4, 0x08, 0, 0x8E);
    idt_set_gate(37, (void*)brights_i386_irq5, 0x08, 0, 0x8E);
    idt_set_gate(38, (void*)brights_i386_irq6, 0x08, 0, 0x8E);
    idt_set_gate(39, (void*)brights_i386_irq7, 0x08, 0, 0x8E);
    idt_set_gate(40, (void*)brights_i386_irq8, 0x08, 0, 0x8E);
    idt_set_gate(41, (void*)brights_i386_irq9, 0x08, 0, 0x8E);
    idt_set_gate(42, (void*)brights_i386_irq10, 0x08, 0, 0x8E);
    idt_set_gate(43, (void*)brights_i386_irq11, 0x08, 0, 0x8E);
    idt_set_gate(44, (void*)brights_i386_irq12, 0x08, 0, 0x8E);
    idt_set_gate(45, (void*)brights_i386_irq13, 0x08, 0, 0x8E);
    idt_set_gate(46, (void*)brights_i386_irq14, 0x08, 0, 0x8E);
    idt_set_gate(47, (void*)brights_i386_irq15, 0x08, 0, 0x8E);
    
    brights_i386_lidt(&idt_ptr, sizeof(idt_ptr));
}