LEAF(test_superio)
move s1,ra
PCICONF_WRITEW(VTSB_DEV,0,0x4,0x7);
PCICONF_WRITEB(VTSB_DEV,0,0x85,3);
li t6,0
1:
move a0,t6
bal hexserial
nop
li a0,':';
bal tgt_putchar
nop
SUPERIO_RD_R(t6,t7)
move a0,t7
bal hexserial
nop
PRINTSTR("\r\n")
addu t6,1
sgt t0,t6,0x100
beqz t0,1b
nop
PCICONF_WRITEB(VTSB_DEV,0,0x85,1);
jr s1
nop
END(test_superio)


