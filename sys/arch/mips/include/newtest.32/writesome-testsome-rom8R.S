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
#undef errorall
#undef TESTONCE
#undef I_STORE
#undef I_LOAD
#undef I_WIDTH

#define I_STORE sd
#define I_LOAD  ld
#define I_WIDTH 8
#define MAXSIZE 256*1024*1024
#define STARTDATA 0xffffffff
#define DECDATA 1
#define ADDRMASK 0xfffff
#define STEPS (0x100000/I_WIDTH)
#define tmp t0
#define curaddr t1
#define writedata t2
#define readdata t3
#define count t4
#define errorall t5


#define TESTONCE \
	subu writedata,STEPS*DECDATA; \
    addu curaddr,STEPS*I_WIDTH;	\
	li count,STEPS;\
1: \
	addu	writedata,DECDATA; \
	subu	curaddr, I_WIDTH; \
	I_STORE	writedata, 0(curaddr); \
	subu count,1; \
	bnez count,1b; \
	nop; \
	\
	li count,STEPS;\
1: \
	I_LOAD	readdata, 0(curaddr); \
	beq readdata, writedata,11f; \
	nop; \
	bal 111f; \
	nop; \
11: \
	subu	writedata, DECDATA; \
	addu	curaddr, I_WIDTH; \
	subu count,1;\
	bnez count, 1b;\
	nop;
	
   
	nop;
	TTYDBG("Testing memory now\r\n")
	
	move errorall,zero
	li writedata,STARTDATA
	li curaddr,0xa0000000
21:
    TESTONCE;
	addu writedata,STEPS*DECDATA; 
    subu curaddr,STEPS*I_WIDTH;	
    not writedata
	TESTONCE;
    
    li  tmp,ADDRMASK
	and tmp,curaddr
	bnez tmp,22f
	nop;

	move a0,curaddr
	bal hexserial
	nop
	PRINTSTR("\r");
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
	li a1,I_WIDTH*2
	bal	Hexserial
	nop
	TTYDBG("\r\nRead =");
	move	a0, readdata
	li a1,I_WIDTH*2
	bal	Hexserial
	nop
	TTYDBG("\r\nxor =");
	xor a0,writedata,readdata
	or errorall,a0
	li a1,I_WIDTH*2
	bal Hexserial
	nop
	TTYDBG("\r\nallerror =");
	move a0,errorall
	li a1,I_WIDTH*2
	bal Hexserial
	nop
	jr t6
	nop

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
#undef errorall
#undef TESTONCE
#undef I_STORE
#undef I_LOAD
#undef I_WIDTH
