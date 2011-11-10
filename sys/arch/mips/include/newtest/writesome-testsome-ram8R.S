#undef STEPS
#undef STARTADDR
#undef MAXSIZE
#undef STARTDATA
#undef DECDATA
#undef ADDRMASK
#undef tmp
#undef curaddr
#udef  tmpaddr
#undef writedata
#undef readdata
#undef count
#undef errorall
#undef tmp1addr
#undef TESTONCE
#undef I_STORE
#undef I_LOAD
#undef I_WIDTH

#define I_STORE sd
#define I_LOAD  ld
#define I_WIDTH 8
#define STEPS (0x100000/I_WIDTH)
#define STARTADDR 0x80100000
#define MAXSIZE (256*1024*1024-STEPS*I_WIDTH)
#define STARTDATA 0xffffffff
#define DECDATA 1
#define ADDRMASK (1024*1024-1)
#define tmp t0
#define curaddr t1
#define tmpaddr t7
#define writedata t2
#define readdata t3
#define count t4
#define errorall t5
#define tmp1addr t4


#define TESTONCE \
    addu curaddr,STEPS*I_WIDTH;	\
	subu writedata,STEPS*DECDATA; \
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
	

#define	MYDBG(x) \
	.rdata;98: .asciz x; .text; la a0, 98b; la v0, stringserial; addu v0,s0;jalr v0; nop	
#define NEWADDR 0x80000000


	la t0,testmem1_start
	addu t0,s0
	la t1,testmem1_end
	addu t1,s0
	
	li t2,NEWADDR
	or t2,0xa0000000
1:
	I_LOAD  v0,(t0)
	I_STORE v0,(t2)
	addu t0,I_WIDTH
	addu t2,I_WIDTH
	ble t0,t1,1b
	nop

	move errorall,zero
	la tmpaddr,testmem1_start
	addu tmpaddr,s0
	la tmp1addr,testmem1_end
	addu tmp1addr,s0
	
	li curaddr,NEWADDR
	or curaddr,0xa0000000
2:
	I_LOAD writedata,(tmpaddr)
	I_LOAD readdata,(curaddr)
	beq readdata,writedata,3f
	nop
	bal 111f
	nop
3:
	addu tmpaddr,I_WIDTH
	addu curaddr,I_WIDTH
	ble tmpaddr,tmp1addr,2b
	nop
	bnez errorall,1f
	nop
	li t0,NEWADDR
   	jr t0	
	nop		
1:
	MYDBG("copy failed,stop\r\n")
1:
	b 1b
	nop

.align 3
testmem1_start: 
	MYDBG("Testing memory now\r\n")
	
	move errorall,zero
	li writedata,STARTDATA
	li curaddr,STARTADDR
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
	la  v0,hexserial
	add v0,s0
	jalr v0; 
	nop; 
	MYDBG("\r");
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
	add v0,s0
	jalr v0
	nop
	MYDBG("\r\nWrite=");
	move	a0, writedata
	li a1,I_WIDTH*2
	la v0,Hexserial
	add v0,s0
	jalr v0
	nop
	MYDBG("\r\nRead =");
	move	a0, readdata
	li a1,I_WIDTH*2
	la v0,Hexserial
	add v0,s0
	jalr v0
	nop
	MYDBG("\r\nxor =");
	xor a0,writedata,readdata
	or errorall,a0
	li a1,I_WIDTH*2
	la v0,Hexserial
	add v0,s0
	jalr v0
	nop
	MYDBG("\r\nallerror =");
	move a0,errorall
	li a1,I_WIDTH*2
	la v0,Hexserial
	add v0,s0
	jalr v0
	nop
	jr t6
	nop
.align 3
testmem1_end:





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



#undef STEPS
#undef STARTADDR
#undef MAXSIZE
#undef STARTDATA
#undef DECDATA
#undef ADDRMASK
#undef tmp
#undef curaddr
#udef  tmpaddr
#undef writedata
#undef readdata
#undef count
#undef errorall
#undef tmp1addr
#undef TESTONCE
#undef I_STORE
#undef I_LOAD
#undef I_WIDTH
