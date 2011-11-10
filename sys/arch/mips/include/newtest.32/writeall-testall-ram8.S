#undef MAXSIZE
#undef STEPSIZE
#undef maxaddr
#undef curaddr
#undef writedata
#undef writedata
#undef paddr
#undef errorall
#undef I_STORE
#undef I_LOAD
#undef I_WIDTH
#undef tmpaddr
#undef tmp1addr

#define MAXSIZE 64*1024*1024
#define STEPSIZE 0x100000 
#define maxaddr t0
#define curaddr t1
#define writedata t2
#define readdata t3
#define paddr t5 
#define errorall t7
#define I_STORE sw
#define I_LOAD  lw
#define I_WIDTH 4
#define tmpaddr t4 
#define tmp1addr t5

#undef MYDBG
#define	MYDBG(x) \
	.rdata;98: .asciz x; .text; la a0, 98b; addu a0,s0; la v0, stringserial; addu v0,s0;jalr v0; nop	
#define NEWADDR 0x80000000


	MYDBG("begin copy\r\n")
	la t0,121f
	addu t0,s0
	la t1,122f
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

	la tmpaddr,121f
	addu tmpaddr,s0
	la tmp1addr,122f
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
121: 


#define BAL(x) \
	la v0,x; \
	addu v0,s0; \
	jalr v0; \
	nop;

#undef WMEM
#define WMEM(STARTDATA,DECDATA) \
	li	maxaddr, 0x80100000+MAXSIZE; \
	li	curaddr, 0x80100000; \
	li	writedata, STARTDATA; \
	li  paddr, 0x80100000+STEPSIZE; \
1: \
	I_STORE	writedata, 0(curaddr); \
	subu	writedata,  DECDATA; \
	addu	curaddr, I_WIDTH; \
	bne curaddr,paddr,2f; \
	nop; \
	li	a0, 'w'; \
	BAL(tgt_putchar); \
	nop; \
	move a0,curaddr; \
	BAL(hexserial);\
	nop; \
	MYDBG("\r");\
	addu paddr,STEPSIZE; \
2: \
	bne	curaddr, maxaddr, 1b; \
	nop; \
	MYDBG("\r\n");

#undef TMEM
#define TMEM(STARTDATA,DECDATA) \
	li	maxaddr, 0x80100000+MAXSIZE; \
	li	curaddr, 0x80100000; \
	li  paddr, 0x80100000+STEPSIZE; \
	li writedata, STARTDATA; \
1: \
	I_LOAD	readdata, 0(curaddr); \
	beq readdata, writedata,11f; \
	nop; \
	bal 111f; \
	nop; \
11: \
	subu	writedata, DECDATA; \
	addu	curaddr, I_WIDTH; \
	bne curaddr,paddr,2f; \
	nop; \
	li	a0, 'r'; \
	BAL(tgt_putchar); \
	nop; \
	move a0,curaddr; \
	BAL(hexserial);\
	nop; \
	MYDBG("\r");\
	addu paddr,STEPSIZE; \
2: \
	beq	curaddr, maxaddr, 3f; \
	nop; \
	b	1b; \
	nop;  \
3: \
	MYDBG("\r\n");


	nop;
	MYDBG("Testing memory now\r\n")
	move errorall,zero
    WMEM(0xffffffff,1);
	TMEM(0xffffffff,1);
	WMEM(0,1);
	TMEM(0,1);
	MYDBG("\r\nTesting ok\r\n");
1:	
	b	1b
	nop

111:
	move t6,ra
	MYDBG("\r\nMemory test failed at ");
	move	a0,	curaddr
	BAL(hexserial);
	nop
	MYDBG("\r\nWrite=");
	move	a0, writedata
	li a1,I_WIDTH*2
	BAL(Hexserial);
	nop
	MYDBG("\r\nRead =");
	move	a0, readdata
	li a1,I_WIDTH*2
	BAL(Hexserial);
	nop

	MYDBG("\r\nxor =");
	xor a0,writedata,readdata
	or errorall,a0
	li a1,I_WIDTH*2
	BAL(Hexserial);
	nop
	MYDBG("\r\nallerror =");
	move a0,errorall
	li a1,I_WIDTH*2
	BAL(Hexserial);
	nop
	jr t6
	nop

.align 3
122:

#undef MAXSIZE
#undef STEPSIZE
#undef maxaddr
#undef curaddr
#undef writedata
#undef writedata
#undef paddr
#undef errorall
#undef I_STORE
#undef I_LOAD
#undef I_WIDTH
#undef tmpaddr
#undef tmp1addr
