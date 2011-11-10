	nop
	TTYDBG("Trying to find out valid cl value...\r\n")

   /* t0: table address
      t1, sdram mode value
	  t2, memory address
	*/
     li t2,0xa8000000
cl_testagain:
	 PRINTSTR("testing addr=\r\n");
	 move a0,t2
	 bal hexserial
	 nop

	 la t0, cltable
	 #relocation
	 addu t0,t0,s0
clloop:
	 lw  t1,0(t0)
	 beqz t1,add_addr;
	 nop
	 #set dunit control stBurstDel
	GT_REGRD (0x1404)
	and v0,v0,~0x0f000000;
	lw  v1,24(t0)
	or  v0,v0,v1
	GT_REGWR_R (0x1404,v0)
	PRINTSTR("dunit control low=\r\n");
	GT_REGRD (0x1404)
	move a0,v0
	bal hexserial
	nop


	 b 1f;
	 nop
add_addr:
	addu t2,t2,0x110320
	li   t8, 0xaa000000
	blt  t2,t8,cl_testagain
	nop
	b _not_found_cl;
	nop

1:
   
#if 1
	 #setting sdram mode to t1
	 /* precharge to all dram banks */
	 li a0,0xa0000000
	 sw a0,0(a0)
	 li a0,0xa4000000
	 sw a0,0(a0)
	 li a0,0xa8000000
	 sw a0,0(a0)
	 li a0,0xac000000
	 sw a0,0(a0)

	/* precharge all banks */ 
	GT_REGWR(SDRAM_OPERATION, 0x1);
1:
	GT_REGRD (SDRAM_OPERATION)
	beqz v0,1f;	
	nop
	b 1b
	nop
1:

	/* load Xtended DRAM mode */ 
	GT_REGWR(SDRAM_OPERATION, 0x4);
1:
	GT_REGRD (SDRAM_OPERATION)
	beqz v0,1f;	
	nop
	b 1b
	nop
1:

	/* set to NOP */ 
	GT_REGWR(SDRAM_OPERATION, 0x5);
1:
	GT_REGRD (SDRAM_OPERATION)
	beqz v0,1f;	
	nop
	b 1b
	nop
1:

	 /* or with reset dll bit */
	 or a0,t1,0x100
	 /* write sdram mode register */
	 GT_REGWR_R(SDRAM_MODE, a0);

	 /* need 200 cycles, for 66Mhz bus, it is 200 x 15ns = 3us, 1 loop should be enough 
	  * considering the flash speed */
	 DELAY(10)

	/* load SDRAM mode register */ 
	GT_REGWR(SDRAM_OPERATION, 0x3);
1:
	GT_REGRD (SDRAM_OPERATION)
	beqz v0,1f;	
	nop
	b 1b
	nop
1:

	 /* need 200 cycles, for 66Mhz bus, it is 200 x 15ns = 3us, 1 loop should be enough 
	  * considering the flash speed */
	 DELAY(10)

	/* precharge all banks */ 
	GT_REGWR(SDRAM_OPERATION, 0x1);
1:
	GT_REGRD (SDRAM_OPERATION)
	beqz v0,1f;	
	nop
	b 1b
	nop
1:

	/* force refresh */ 
	GT_REGWR(SDRAM_OPERATION, 0x2);
1:
	GT_REGRD (SDRAM_OPERATION)
	beqz v0,1f;	
	nop
	b 1b
	nop
1:

	/* force refresh */ 
	GT_REGWR(SDRAM_OPERATION, 0x2);
1:
	GT_REGRD (SDRAM_OPERATION)
	beqz v0,1f;	
	nop
	b 1b
	nop
1:

	 /* write sdram mode register */
	 GT_REGWR_R(SDRAM_MODE, t1);

	 /* need 200 cycles, for 66Mhz bus, it is 200 x 15ns = 3us, 1 loop should be enough 
	  * considering the flash speed */
	 DELAY(10)

	/* load SDRAM mode register */ 
	GT_REGWR(SDRAM_OPERATION, 0x3);
1:
	GT_REGRD (SDRAM_OPERATION)
	beqz v0,1f;	
	nop
	b 1b
	nop
1:
#endif

	 /* write sdram mode register */
	 GT_REGWR_R(SDRAM_MODE, t1);

	 DELAY(0x100)

    li t4,0x1000 
	move t5,t2
  2:
	 sw t1, 0(t5)
	 lw t3, 0(t5)
	 beq t1,t3,found_cl
	 nop
	 addiu t4,t4,-1
	 addiu t5,t5,0x120

	 #set address control
	GT_REGRD (0x1410)
	and v0,v0,~0xf
	lw  v1,48(t0)
	or  v0,v0,v1
	GT_REGWR_R (0x1410,v0)

	 bnez t4,2b
	 nop
	 move a0,t1
	 bal hexserial
	 nop
	 PRINTSTR("FAIL\r\n");
	 addiu t0,t0,4
	 b clloop
	 nop

cltable:
.word 0x22,0x32,0x42,0x52,0x62,0
.word 0x3,0x3,0x4,0x4,0x5,0
.word 0x3,0x4,0x5,0x6,0x7,0

found_cl:
	PRINTSTR("\r\ngood cl found!!!");
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
	b _end_cl;

_not_found_cl:
   PRINTSTR("\r\nSorry! no value is usable!\n");

_end_cl:

#if 1
	/* return to normal */ 
	GT_REGWR(SDRAM_OPERATION, 0x0);
1:
	GT_REGRD (SDRAM_OPERATION)
	beqz v0,1f;	
	nop
	b 1b
	nop
1:

#endif
