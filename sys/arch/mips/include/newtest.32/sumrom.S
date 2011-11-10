#undef STARTDATA
#undef DECDATA
#undef MAXSIZE
#define MAXSIZE 512*1024
	
	nop
	TTYDBG("Testing memory now\r\n")
	
	li	t7, 10
10:
	li	t0, 0xbfc00000+MAXSIZE
	li	t1, 0xbfc00000
	li	t2, 0
1:
	lw	t3, 0(t1)
	addu	t2,t3
	addu	t1, 4
	bne	t1, t0,1b
	nop
	TTYDBG("\r\nsum=");
	move	a0,	t2
	bal	hexserial
	nop
1:
	b 1b
	nop
