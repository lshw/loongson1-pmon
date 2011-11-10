#define WMEM(STARTDATA,DECDATA) \
	li	t0, 0xa0000000+1*1024*1024; \
	li	t1, 0xa0000000; \
	li	t2, STARTDATA; \
1: \
	sw	t2, 0(t1); \
	subu	t2,  DECDATA; \
	addu	t1, 4; \
	bne	t1, t0, 1b; \
	nop; 

#define TMEM(STARTDATA,DECDATA) \
	li	t0, 0xa0000000+1*1024*1024; \
	li	t1, 0xa0000000; \
	li t2, STARTDATA; \
1: \
	lw	t3, 0(t1); \
	beq t3, t2,11f; \
	nop; \
	bal reporterr; \
	nop; \
11: \
	subu	t2, DECDATA; \
	addu	t1, 4; \
	beq	t1, t0, 3f; \
	nop; \
	and	t4, t1, 0x0000ffff; \
	bnez	t4, 12f; \
	nop; \
	li	a0, '.'; \
	bal	tgt_putchar; \
	nop; \
12: \
	b	1b; \
	nop;  \
3: \


	nop;
	TTYDBG("Testing memory now\r\n")
    WMEM(0xffffffff,0);
	TMEM(0xffffffff,0);
	WMEM(0,0);
	TMEM(0,0);
	TTYDBG("\r\nTesting ok\r\n");
1:	
	b	1b
	nop

LEAF(reporterr)
	move t6,ra
	TTYDBG("\r\nMemory test failed at ");
	move	a0,	t1
	bal	hexserial
	nop
	TTYDBG("\r\nWrite=");
	move	a0, t2
	bal	hexserial
	nop
	TTYDBG("\r\nRead =");
	move	a0, t3
	bal	hexserial
	nop
	jr t6
	nop
END(reporterr)

