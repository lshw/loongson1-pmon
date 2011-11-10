#define STARTDATA 0x55aa55aa
#define DECDATA 1
//#define MAXSIZE 64*1024*1024
#define MAXSIZE 8
	
	nop
	TTYDBG("Trying to find out DFCDL value...\r\n")

     li t1,0x55aa55aa
     li t2,0xa8000000

     li t0, 0
	 GT_REGWR_R(SRAM_DATA0, t0);

	 sw t1, 0(t2)
	 lw t3, 0(t2)
	 beq t1,t3,found_dfcdl
	 nop

	 li t0, 1
	 GT_REGWR_R(SRAM_DATA0, t0);

	 sw t1, 0(t2)
	 lw t3, 0(t2)
	 beq t1,t3,found_dfcdl
	 nop

	 li t0, 2
	 GT_REGWR_R(SRAM_DATA0, t0);

	 sw t1, 0(t2)
	 lw t3, 0(t2)
	 beq t1,t3,found_dfcdl
	 nop

	 li t7, 61
	 move  t8, t0
1:
	 addiu t8,t8,1
	 addiu t0,t8,-2
	 sll   t0,t0,6
	 or    t0,t0,t8
	 GT_REGWR_R(SRAM_DATA0, t0);
	 move a0,t0
	 bal  hexserial
	 nop
	 li   a0,'\r'
	 bal  tgt_putchar
	 nop
	 li   a0,'\n'
	 bal  tgt_putchar
	 nop
	 sw t1, 0(t2)
	 lw t3, 0(t2)
	 beq t1,t3,found_dfcdl
	 nop
	 addiu t7,t7,-1
	 bnez  t7,1b
	 nop
	 b _not_found_dfcdl;
	 nop

found_dfcdl:
	PRINTSTR("\r\ngood dfcdl found!!!");
	move	a0,	t0
	bal	hexserial
	nop
	TTYDBG("\r\nWrite=");
	move	a0, t1
	bal	hexserial
	nop
	TTYDBG("\r\nRead =");
	move	a0, t3
	bal	hexserial
	nop

_not_found_dfcdl:
   PRINTSTR("\r\nSorry! no value is usable!\n");

