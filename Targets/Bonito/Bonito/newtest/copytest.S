#define count  t6
#define allerror t7
#define TESTCOUNT 10
	li count,TESTCOUNT
	la	t2, _edata
	la	t0, start
11:
	li a0,'.'
	bal tgt_putchar
	nop
	li	    t1, 0xbfc00000
    or      t0, 0xa0000000
    or      t2, 0xa0000000

	/* copy text section */
1:	lw	t3, 0(t1)
	nop
	sw	t3, 0(t0)
	addu	t0, 4
	addu	t1, 4
	bne	t2, t0, 1b
	nop
	la v0,start
	la v1,_edata
	subu t2,v0
	addu t2,v1
	addu count,-1
	bnez count,11b
	nop

PRINTSTR("\r\n");
	
	li count,TESTCOUNT
	la	t0, start
	la	t2, _edata
11:
	li a0,'+'
	bal tgt_putchar
	nop
	li	t1, 0xbfc00000
    or      t0, 0xa0000000
    or      t2, 0xa0000000

	/* copy text section */
	
1:	lw	t4, 0(t1)
	nop
	lw	t5, 0(t0)
	addu	t0, 4
	addu	t1, 4
	beq	t4, t5, 2f
	nop
	move	a0, t0
	subu	a0, 4
	bal	hexserial
	nop
	TTYDBG	(" ")
	move	a0, t4
	bal	hexserial
	nop
	TTYDBG	(" ")
	move	a0, t5
	bal	hexserial
	nop
	TTYDBG	(" ")
	xor a0,t4,t5
	or allerror,a0
	bal	hexserial
	nop	
	TTYDBG	(" ")
	move a0,allerror
	bal hexserial
	nop
	TTYDBG	("\r\n")
2:	bne	t2, t0, 1b
	nop

	la v0,start
	la v1,_edata
	subu t2,v0
	addu t2,v1
	addu count,-1
	bnez count,11b
	nop

