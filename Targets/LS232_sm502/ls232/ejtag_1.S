
.set mips32
	lui 	t5,0xa500
95:	mfc0    t6,$16,6
	and     t6,t6,0x10
	beq	t6,zero,95b
	nop

	mfc0	t7,$11,6

	sw	t7,0(t5)
	add	t5,t5,4
	
	mtc0	zero,$16,6
	nop
	bal	95b		
	nop
.set mips3
	b	ejtag_done
	nop	
