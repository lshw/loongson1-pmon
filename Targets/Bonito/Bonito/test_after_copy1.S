
/* zhb */
zhb:
	TTYDBG("Testing...\r\n")
	la	a0, start
	li	a1, 0xbfc00000
	la	a2, _edata
	or	a2,7
	xor	a2,7
        or      a0, 0xa0000000
        or      a2, 0xa0000000

	move	t0, a0
	move	t1, a1
	move	t2, a2
	/* copy text section */
	
1:	ld	t4, 0(t1)
	nop
	ld	t5, 0(t0)
	addu	t0, 8
	addu	t1, 8
	beq	t4, t5, 2f
	nop
	move	a0, t0
	subu	a0, 8
	bal hexserial
	nop
	TTYDBG	(" ")
	move	a0, t4
	li a1,16
	bal Hexserial
	nop
	TTYDBG	(" ")
	move	a0, t5
	li a1,16
	bal Hexserial
	nop
	TTYDBG	(" ")
	xor a0,t4,t5
	li a1,16
	bal Hexserial
	nop
	TTYDBG	("\r\n")
2:	bne	t2, t0, 1b
	nop
	TTYDBG	("test ok!\r\n")
//3:	beqz	zero, 3b
	nop
