LEAF(put_halfhex)
	lw	v0, GT_BASE_ADDR+(GPP_VALUE);
	or v0,(7<<4); 
	sll v1,a0,4
	not v1
	and v0,v1
	sw	v0, GT_BASE_ADDR+(GPP_VALUE); 
	li v0, 0x40000;	
99:			
	bnez	v0, 99b;
	addiu	v0, -1
	jr ra
	nop
END(put_halfhex)

LEAF(halfhex)
	move	a2, ra
	move	a1, a0
	li	a3, 16
1:
	rol	a0, a1, 2
	move	a1, a0
	and	a0, 0x3
	bal	put_halfhex
	nop
	li a0,0x7
	bal	tgt_halfhex
	nop

	bnez	a3, 1b
	addu	a3, -1

	j	a2
	nop
END(halfhex)

