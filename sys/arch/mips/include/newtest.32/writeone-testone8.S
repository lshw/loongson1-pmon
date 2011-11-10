#undef STARTDATA
#undef DECDATA
#undef MAXSIZE
#undef errorall
#define STARTDATA 0xffffffff
#define DECDATA 0
#define MAXSIZE 64*1024*1024
#define I_STORE sw
#define I_LOAD  lw
#define I_WIDTH 4
#define errorall t5
	
	nop
	TTYDBG("Testing memory now\r\n")
	move errorall,zero
	
	li	t7, 10
10:
	li	t0, 0xa0000000+MAXSIZE
	li	t1, 0xa0000000
	li	t2, STARTDATA
1:
	I_STORE	t2, 0(t1)
	I_LOAD	t3, 0(t1)
	beq t3, t2,11f
	nop
	bal 111f
	nop
11:
	not	t2
	I_STORE	t2, 0(t1)
	I_LOAD	t3, 0(t1)
	beq	t3, t2, 11f
	nop
	bal 111f
	nop
11:
	not	t2
	subu	t2, DECDATA
	addu	t1, I_WIDTH
	beq	t1, t0, 3f
	nop
	and	t4, t1, 0x000fffff
	bnez	t4, skipdot
	nop
	move a0,t1; 
	bal	hexserial;
	nop; 
	PRINTSTR("\r");
skipdot:
	b	1b
	nop
3:
	TTYDBG("Testing ok...\r\n");
	sub	t7,1
	beqz	t7, 1f
	nop
	b	10b
	nop
1:	
	b	1b
	nop

111:
	move t6,ra
	TTYDBG("\r\nMemory test failed at ");
	move	a0,	t1
	bal	hexserial
	nop
	TTYDBG("\r\nWrite=");
	move	a0, t2
	li a1,I_WIDTH*2
	bal	Hexserial
	nop
	TTYDBG("\r\nRead =");
	move	a0, t3
	li a1,I_WIDTH*2
	bal	Hexserial
	nop

	TTYDBG("\r\nxor =");
	xor a0,t2,t3
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


#undef STARTDATA
#undef DECDATA
#undef MAXSIZE
#undef errorall
#define STARTDATA 0xffffffff
#define DECDATA 0
#define MAXSIZE 64*1024*1024
#define I_STORE sw
#define I_LOAD  lw
#define I_WIDTH 4
#define errorall t5
