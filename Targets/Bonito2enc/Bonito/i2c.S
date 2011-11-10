#define i2cread newi2cread
	li	msize,0
	PRINTSTR("DIMM read\r\n")

	/* only one memory slot, slave address is 1010000b */
	li	sdCfg,0x04000000 /*bit 26突发式读写时的块内顺序*/
	li  	a1, 0x0
    li      a0,0xa1
    bal     i2cread
	nop
	beq	v0,0xff,1f
	nop
	beq v0,0x80,1f
	nop
	move a0,v0
	bal hexserial
	nop
    PRINTSTR ("\r\nNo DIMM in slot 0 \r\n");
	b 2f
	nop
1:
	or  	sdCfg, 0x1<<29
	nop
	li a0,0xa1
	bal ii2c_cfg
	nop
2:
	li  	a1, 0x0
    li      a0,0xa3
    bal     i2cread
	nop
	li 	a1,0x0
	beq	v0,0xff,1f
	nop
	beq v0,0x80,1f
	nop
	move a0,v0
	bal hexserial
	nop
    PRINTSTR ("\r\nNo DIMM in slot 1 \r\n");
	b 2f
	nop
1:
	li a0,0xa3
	bal ii2c_cfg
	nop

	b 2f
	nop

2:
	b 211f
	nop
/*ic2 cfg
* a0=0xa1 for slot 0,a0=0xa1 for slot 1
* t5 used for save i2c addr a0,t6 save ra.
*/
LEAF(ii2c_cfg)
	move t6,ra
	move t5,a0

#ifdef I2C_DEBUG
	li t1,0
1:
	move a1,t1
	move a0,t5
	bal	i2cread
	nop

	#print
	move a0, v0
	bal  hexserial
	nop
	PRINTSTR("\r\n")
	addiu t1,t1,1
	li   v0, 0x20
	bleu  t1, v0, 1b
	nop
#endif

	# set some parameters for DDR333
	# rank number and DDR type field will be filled later
	# to check: fix TCAS?

       
	PRINTSTR("read memory type\r\n") 

	/* read DIMM number of rows */
	move	a0,t5	/* #zgj-11-17 */
	li	a1,3
	bal	i2cread
	nop	
	move	a0, v0
	subu	v0, 12
	bgtu	v0, 2,.nodimm1
	nop
	move	t1, v0
	PRINTSTR("read number of rows\r\n")

2:	/* read DIMM number of cols */

    move      a0,t5   /* #zgj-11-17 */
	li	a1,4
	bal	i2cread
	nop
	
	subu	v0, 8
	bgtu	v0, 4,.nodimm1
	nop
	
	bne	t1, 0, 10f
	nop
	bne	v0, 2, 20f
	nop
	li	v0, 0
	b	.ddrtype1
	nop

20:	bne	v0, 1, 21f
	nop
	li	v0, 1
	b	.ddrtype1
	nop
21:	bne	v0, 0, 22f
	nop
	
	li	v0, 2
	b	.ddrtype1
	nop
22:	bne	v0, 3, 33f
	nop
	li	v0, 3
	b	.ddrtype1
	nop
10:	bne	t1, 1, 11f
	nop
	bne	v0, 3, 20f
	nop
	li	v0, 4
	b	.ddrtype1
	nop
20:	bne	v0, 2, 21f
	nop
	li	v0, 5
	b	.ddrtype1
	nop
21:	bne	v0, 1, 22f
	nop
	li	v0, 6
	b	.ddrtype1
	nop
22:	bne	v0, 4, 33f
	nop
	li	v0, 7
	b	.ddrtype1
	nop
11:	bne	t1, 2, 33f
	nop
	bne	v0, 4, 20f
	nop
	li	v0, 8
	b	.ddrtype1
	nop
20:	bne	v0, 3, 21f
	nop
	li	v0, 9
	b	.ddrtype1
	nop
21:	bne	v0, 2, 33f
	nop
	li	v0, 10
	b	.ddrtype1
	nop
33:	PRINTSTR("DDR type not supported!\r\n");
34:	b	34b
	nop

.ddrtype1:
	#bit 25:22 is DDR type field
	sll	v0, 22 
	and	v0,0x03c00000
	or	sdCfg,v0
	
	/* read DDR RATE*/
	move	a0,t5
	li	a1,23
	bal 	i2cread 
	nop
	beq	v0,0xa0,40f
	nop
	beq	v0,0x75,41f
	nop
	beq	v0,0x60,42f
	nop
	b	42f
	nop
	
	/* config sdCfg bits [ 9:0 ] */
40:	/* ddr200 */
	or	sdCfg,0x0b1
	b	47f
	nop

41:     /* ddr266 */
    or      sdCfg,0x3ae	
	b     47f
    nop

42:     /* ddr333 */
    or      sdCfg,0x3df
	b     47f
    nop

#####################################################

 /* read DDR SDRAM Minimum Clock Cycle when CL is Derated by One Clock,config sdCfg [ 21:10 ]*/
47:
    move    a0,t5
    li      a1,25
    bal     i2cread
	nop
    beq     v0,0xa0,40f
    nop
    beq     v0,0x75,41f
    nop
    beq     v0,0x60,42f
    nop
    b     41f
	nop
	
40:     /* ddr200 */
 	move    a0,t5
    li      a1,12
    bal     i2cread
    nop
    bne     v0,0x82,10f
    nop
    or      sdCfg,780<<10
	b	45f
	nop
10:
    bne     v0,0x80,.nodimm1
    nop
    or      sdCfg,1560<<10
    b     45f
    nop

41:     /* ddr266 */
    move    a0,t5
    li      a1,12
    bal     i2cread
    nop
    bne     v0,0x82,11f
    nop
    or      sdCfg,1040<<10
	b	45f
	nop
11:
     bne     v0,0x80,.nodimm1
     nop
     or      sdCfg,2080<<10
     b     45f
     nop

42:     /* ddr333 */
    move    a0,t5
    li      a1,12
    bal     i2cread
    nop
    bne     v0,0x82,12f
    nop
    or      sdCfg,1300<<10
	b	45f
	nop
12:
     bne     v0,0x80,.nodimm1
     nop
     or      sdCfg,2600<<10
     b     45f
     nop


#####################################################


	/* Minimum Ras to Cas Delay (tRCD) 
	move	a0,t5
	bal 	i2cread
	li	a1,29
	or	*/
45:
2:	/* read DIMM number of blocks-per-ddrram */
    move      a0,t5   /* #zgj-11-17 */
	li	a1,17
	bal	i2cread
	nop
	beq	v0,2,2f
	nop
	bne	v0,4,.nodimm1
	nop
	PRINTSTR("read blocks per ddrram\r\n");

2:	/* read DIMM number of sides (banks) */
    move      a0,t5   /* #zgj-11-17 */
	li	a1,5
	bal	i2cread
	nop
	beq	v0,1,2f
	nop
	bne	v0,2,.nodimm1
	nop

	or  sdCfg, 0x1<<27
	PRINTSTR("read number of sides\r\n") ;

2:	/* read DIMM width */
    move      a0,t5   /* #zgj-11-17 */
	li	a1,6
	bal	i2cread
	nop
	bleu	v0,36,2f
	nop
	bgtu	v0,72,.nodimm1
	nop
	PRINTSTR("read width\r\n") ;
2:
   move      a0,t5
   li      a1,31
   bal     i2cread
   nop
   beqz    v0,.nodimm1
   nop
   sll     tmpsize,v0,22 # multiply by 4M
	addu	msize,tmpsize
    move      a0,t5 
	li	a1,5
	bal	i2cread
	nop
	beq	v0,1,1f
	nop
	addu	msize,tmpsize
	b 1f
	nop
	
.nodimm1:
	PRINTSTR ("\r\nNo DIMM in this slot ");
1:
	jr t6
	nop
END(ii2c_cfg)

LEAF(i2cread)
        /* set device address */
        li  v0, 0xbfd00000 + SMBUS_HOST_ADDRESS

        sb  a0, 0(v0);

        /* store register offset */
        li  v0, 0xbfd00000 + SMBUS_HOST_COMMAND
        sb  a1, 0(v0);

        /* read byte data protocol */
        li  v0, 0x08
        li  v1, 0xbfd00000 + SMBUS_HOST_CONTROL
        sb  v0, 0(v1);

        /* make sure SMB host ready to start, important!--zfx */
        li  v1, 0xbfd00000 + SMBUS_HOST_STATUS
        lbu v0, 0(v1)
        andi v0,v0, 0x1f
        beqz  v0,1f
        nop
        sb  v0, 0(v1)
        lbu v0, 0(v1)   #flush the write
1:

        /* start */
        li  v1, 0xbfd00000 + SMBUS_HOST_CONTROL
        lbu v0, 0(v1)
        ori v0, v0, 0x40
        sb  v0, 0(v1);

        /* wait */
        li  v1, 0xbfd00000 + SMBUS_HOST_STATUS
1:

#if 0
        /* delay */
        li a0, 0x1000
2:
        bnez    a0,2b
        addiu   a0, -1
#endif

        lbu  v0, 0(v1)
        andi v0, SMBUS_HOST_STATUS_BUSY
        bnez  v0, 1b  #IDEL ?
        nop

        li  v1, 0xbfd00000 + SMBUS_HOST_STATUS
        lbu v0, 0(v1)
        andi v0,v0, 0x1f
        beqz  v0,1f
        nop
        sb  v0, 0(v1)   #reset
        lbu v0, 0(v1)   #flush the write
1:

        li  v1, 0xbfd00000 + SMBUS_HOST_DATA0
        lbu  v0, 0(v1)

        jr      ra
        nop
END(i2cread)
211:
#undef i2cread
######################################################################################################
