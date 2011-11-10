#undef MAXSIZE
#undef STEPSIZE
#undef maxaddr
#undef curaddr
#undef writedata
#undef writedata
#undef paddr

#define MAXSIZE 256*1024*1024
#define STEPSIZE MAXSIZE/32
#define maxaddr t0
#define curaddr t1
#define writedata t2
#define readdata t3
#define paddr t5 

#define WMEM(STARTDATA,DECDATA) \
	li	maxaddr, 0xa0000000+MAXSIZE; \
	li	curaddr, 0xa0000000; \
	li	writedata, STARTDATA; \
	li  paddr, 0xa0000000+STEPSIZE; \
1: \
	sw	writedata, 0(curaddr); \
	subu	writedata,  DECDATA; \
	addu	curaddr, 4; \
	bne curaddr,paddr,2f; \
	nop; \
	li	a0, '*'; \
	bal	tgt_putchar; \
	nop; \
	addu paddr,STEPSIZE; \
2: \
	bne	curaddr, maxaddr, 1b; \
	nop; \
	TTYDBG("\r\n");

#define TMEM(STARTDATA,DECDATA) \
	li	maxaddr, 0xa0000000+MAXSIZE; \
	li	curaddr, 0xa0000000; \
	li  paddr, 0xa0000000+STEPSIZE; \
	li writedata, STARTDATA; \
1: \
	lw	readdata, 0(curaddr); \
	beq readdata, writedata,11f; \
	nop; \
	bal 111f; \
	nop; \
11: \
	subu	writedata, DECDATA; \
	addu	curaddr, 4; \
	bne curaddr,paddr,2f; \
	nop; \
	li	a0, '.'; \
	bal	tgt_putchar; \
	nop; \
	addu paddr,STEPSIZE; \
2: \
	beq	curaddr, maxaddr, 3f; \
	nop; \
	b	1b; \
	nop;  \
3: \
	TTYDBG("\r\n");


	nop;
	TTYDBG("Testing memory now\r\n")
    WMEM(0xffffffff,1);
	TMEM(0xffffffff,1);
	WMEM(0,1);
	TMEM(0,1);
	TTYDBG("\r\nTesting ok\r\n");
1:	
	b	1b
	nop

111:
	move t6,ra
	TTYDBG("\r\nMemory test failed at ");
	move	a0,	curaddr
	bal	hexserial
	nop
	TTYDBG("\r\nWrite=");
	move	a0, writedata
	bal	hexserial
	nop
	TTYDBG("\r\nRead =");
	move	a0, readdata
	bal	hexserial
	nop
	jr t6
	nop

