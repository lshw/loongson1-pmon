#undef STEPS
#undef STARTADDR
#undef MAXSIZE
#undef STARTDATA
#undef DECDATA
#undef ADDRMASK
#undef tmp
#undef curaddr
#undef writedata
#undef readdata
#undef count
#undef errorall
#undef TESTONCE

#define STEPS 0x100000
#define STARTADDR (0x80000000+STEPS*4)
#define MAXSIZE (256*1024*1024-STEPS*4)
#define STARTDATA 0xffffffff
#define DECDATA 1
#define ADDRMASK (8*1024*1024-1)
#define tmp t0
#define curaddr t1
#define writedata t2
#define readdata t3
#define count t4
#define errorall t5


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
	nop;
	

#define	MYDBG(x) PRINTSTR(x)
#define NEWADDR 0x80000000

	move a0,s0
	bal hexserial
	nop
	PRINTSTR("\r\n");
	la v0,_start
	li v1,0xbfc00000
	subu a0,v1,v0
	bal hexserial
	nop
	PRINTSTR("\r\n");

	la t0,testmem0_start
	addu t0,s0
	la t1,testmem0_end
	addu t1,s0
	li t2,NEWADDR
	or t2,0xa0000000
1:
	lw v0,(t0)
	nop
	nop
	sw v0,(t2)
	addu t0,4
	addu t2,4
	bne t0,t1,1b
	nop
#	bal godson2_cache_flush
#	nop
	li t0,NEWADDR
   	jr t0	
	nop		

testmem0_start: 
	MYDBG("Testing memory now\r\n")
	
	move errorall,zero
	li writedata,STARTDATA
	li curaddr,STARTADDR
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
	la  v0,tgt_putchar
	addu v0,s0
	jalr v0; 
	nop; 
22:
    li tmp,MAXSIZE+STARTADDR
	subu tmp,curaddr
	bgtz tmp,21b
	nop
	MYDBG("\r\nTesting ok\r\n");
1:	
	b	1b
	nop

111:
	move t6,ra
	MYDBG("\r\nMemory test failed at ");
	move	a0,	curaddr
	la v0,hexserial
	addu v0,s0
	jalr v0
	nop
	MYDBG("\r\nWrite=");
	move	a0, writedata
	la v0,hexserial
	addu v0,s0
	jalr v0
	nop
	MYDBG("\r\nRead =");
	move	a0, readdata
	la v0,hexserial
	addu v0,s0
	jalr v0
	nop
	MYDBG("\r\nxor =");
	xor a0,writedata,readdata
	or errorall,a0
	la v0,hexserial
	addu v0,s0
	jalr v0
	nop
	MYDBG("\r\nallerror =");
	move a0,errorall
	la v0,hexserial
	addu v0,s0
	jalr v0
	nop
	jr t6
	nop
testmem0_end:





#undef STARTADDR 
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


