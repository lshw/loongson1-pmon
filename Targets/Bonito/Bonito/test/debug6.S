#define GT_REGRD_IDX(offs)			\
	add t0,offs,GT_BASE_ADDR;\
	lw	v0, (t0);
bal dump_MV_regs
nop
1:
b 1b
nop
LEAF(dump_MV_regs)
move s1,ra
li t6,0
1:
move a0,t6
bal hexserial
nop
li a0,':'
bal tgt_putchar
nop
GT_REGRD_IDX(t6)
move a0,v0
bal hexserial
nop
PRINTSTR("\r\n")
add t6,4
sgt t0,t6,0x10000
beqz t0,1b
nop
jr s1
nop
END(dump_MV_regs)

