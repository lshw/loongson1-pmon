#undef MAXSIZE
#undef STARTDATA
#undef DECDATA
#undef ADDRMASK
#undef STEPS
#undef tmp
#undef curaddr
#undef writedata
#undef readdata
#undef count
#undef TESTONCE

#define MAXSIZE 256*1024*1024
#define STARTDATA 0xffffffff
#define DECDATA 0
#define ADDRMASK ((MAXSIZE-1)>>5)
#define STEPS 0x100000/4
#define tmp t0
#define curaddr t1
#define writedata t2
#define readdata t3
#define count t4


#define TESTONCE \
	li count,STEPS;\
1: \
	sw	writedata, 0(curaddr); \
	subu	writedata,DECDATA; \
	addu	curaddr, 4; \
	subu count,1; \
	bnez count,1b; \
	nop; \
	\
	li a0,'+';\
	bal tgt_putchar;\
	nop;\
	addu writedata,STEPS*DECDATA; \
    subu curaddr,STEPS*4;	\
	li count,STEPS;\
1: \
	lw	readdata, 0(curaddr); \
	beq readdata, writedata,11f; \
	nop; \
	bal 111f; \
	nop; \
11: \
	subu	writedata, DECDATA; \
	addu	curaddr, 4; \
	subu count,1;\
	bnez count, 1b;\
	nop;\
	li a0,'-';\
	bal tgt_putchar;\
	nop;
	
   
	nop;
	TTYDBG("Testing memory now\r\n")

	li writedata,STARTDATA
	li curaddr,0xa0000000
21:
    TESTONCE;
	addu writedata,STEPS*DECDATA; 
    subu curaddr,STEPS*4;	
    not writedata
	TESTONCE;
    
    li  tmp,ADDRMASK
	and tmp,curaddr
	bnez tmp,22f
	nop;
	li	a0, '.'; 
	bal	tgt_putchar; 
	nop; 
22:
    li tmp,MAXSIZE+0xa0000000
	subu tmp,curaddr
	bgtz tmp,21b
	nop
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

