LEAF(test_isaio)
move s1,ra
PCICONF_WRITEW(VTSB_DEV,0,0x4,0x7);
li t6,0
1:
move a0,t6
bal hexserial
nop
li a0,':';
bal tgt_putchar
nop
add v0,t6,PCI_IO_SPACE_BASE
lbu v1,(v0)
move a0,v1
bal hexserial
nop
PRINTSTR("\r\n")
addu t6,1
sgt t0,t6,0x10000
beqz t0,1b
nop
jr s1
nop
END(test_isaio)
