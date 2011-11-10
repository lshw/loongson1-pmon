#include "test/macro.S"
LEAF(test_dev)
move s1,ra
li t7,1
81:
PRINTSTR("dev:")
move a0,t7
bal hexserial
nop
PRINTSTR("\r\n")
PRINTSTR("pcispace 0:")
PCICONF_READW(t7,0,0,a0);
bal hexserial
nop
PRINTSTR("\r\n")
li t6,0x10
1:
PRINTSTR("pcispace ")
move a0,t6
bal hexserial
nop
li a0,':'
bal tgt_putchar
nop
PCICONF_WRITEW(t7,0,t6,-1);
PCICONF_READW(t7,0,t6,a0);
PCICONF_WRITEW(t7,0,t6,0);
bal hexserial
nop
PRINTSTR("\r\n")
addu t6,4
sgt t0,t6,0x24
beqz t0,1b
nop


li t6,0x10
61:
PCICONF_WRITEW(t7,0,t6,-1);
PCICONF_READW(t7,0,t6,a0);
and t0,a0,1
beqz t0,62f
nop
li t0,-1
beq t0,a0,62f
nop
#define NEWALLOC_IO_BASE 0x101000
#define	PCI_MAPREG_MEM_ADDR_MASK		0xfffffff0
and a0,a0,~PCI_MAPREG_MEM_ADDR_MASK
li a1,NEWALLOC_IO_BASE
or a0,a1
PCICONF_WRITEW(t7,0,t6,a0);
move a0,t6
bal hexserial
nop
PRINTSTR(" adress set to ")
PCICONF_READW(t7,0,t6,a0);
bal hexserial
nop
PRINTSTR("\r\ncmdstatus regs=")

PCICONF_WRITEW(t7,0,4,7);
PCICONF_READW(t7,0,4,a0);
bal hexserial
nop
PRINTSTR("\r\n")


li t4,0
li t5,PCI_IO_SPACE_BASE+NEWALLOC_IO_BASE
1:
move a0,t5
bal hexserial
nop
li a0,':';
bal tgt_putchar
nop
lbu a0,(t5)
bal hexserial
nop
PRINTSTR("\r\n")
addu t4,1
addu t5,1
sgtu t0,t4,0x100
beqz t0,1b
nop
PCICONF_WRITEW(t7,0,4,0x0);
62:
PCICONF_WRITEW(t7,0,t6,0x0);

addu t6,4
sgt t0,t6,0x24
beqz t0,61b
nop
addu t7,1
sgt t0,t7,30
beqz t0,81b
nop
jr s1
nop
END(test_dev)


