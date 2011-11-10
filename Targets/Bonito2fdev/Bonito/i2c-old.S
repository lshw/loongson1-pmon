
	PRINTSTR("DIMM read\r\n")

	/* only one memory slot, slave address is 1010000b */

	li  a1, 0x0
1:
	li	a0,0xa1 
	bal	i2cread
	nop

    # save a1
	move t1, a1

	#print
	move a0, v0
	bal  hexserial
	nop

	PRINTSTR("\r\n")

    # restore a1
	move  a1,t1
	addiu a1,a1,1

	li   v0, 0x20
	bleu  a1, v0, 1b
	nop

	li	msize,0
	# set some parameters for DDR333
	# rank number and DDR type field will be filled later
	# to check: fix TCAS?
#	li	sdCfg, 0x341043df  

#	li	sdCfg, 0x255043df /*   zgj  */

	li	sdCfg,0x055043df         /* zgj-8-7-14-13 */
	

	li	a0,0xa1
	
	/* read DIMM memory type (must be DDRAM) */
	#li	a1,2
	#bal	i2cread
	#nop
	#bne	v0,7,.nodimm
	#nop
	PRINTSTR("read memory type\r\n") 

	/* read DIMM number of rows */
	li	a0,0xa1
	li	a1,3
	bal	i2cread
	nop	
	move	a0, v0
	subu	v0, 12
	move	s1, v0

	bgtu	v0, 2,.nodimm
	nop
	PRINTSTR("read number of rows\r\n")

2:	/* read DIMM number of cols */
	li	a0,0xa1
	li	a1,4
	bal	i2cread
	nop
	
	subu	v0, 8
	bgtu	v0, 4,.nodimm
	nop
	
	move	t1, s1

PRINTSTR("DDR type 10 bal\r\n");

	bne	t1, 0, 10f
	nop
PRINTSTR("DDR type 20 bal\r\n"); /* zgj */

	bne	v0, 2, 20f
	nop
	li	v0, 0
PRINTSTR("DDR type 2022 bal\r\n"); /* zgj */
	b	.ddrtype
	nop

PRINTSTR("DDR type 21 bal\r\n"); /* zgj */
20:	bne	v0, 1, 21f
	nop
PRINTSTR("DDR type 22 bal\r\n"); /* zgj */
	li	v0, 1
	b	.ddrtype
	nop
21:	bne	v0, 0, 22f
	nop

PRINTSTR("DDR type 33 bal\r\n"); /* zgj */

	li	v0, 2
	b	.ddrtype
	nop
22:	bne	v0, 3, 33f
	nop
	li	v0, 3
	b	.ddrtype
	nop
10:	bne	t1, 1, 11f
	nop
	bne	v0, 3, 20f
	nop
	li	v0, 4
	b	.ddrtype
	nop
20:	bne	v0, 2, 21f
	nop
	li	v0, 5
	b	.ddrtype
	nop
21:	bne	v0, 1, 22f
	nop
	li	v0, 6
	b	.ddrtype
	nop
22:	bne	v0, 4, 33f
	nop
	li	v0, 7
	b	.ddrtype
	nop
11:	bne	t1, 2, 33f
	nop
	bne	v0, 4, 20f
	nop
	li	v0, 8
	b	.ddrtype
	nop
20:	bne	v0, 3, 21f
	nop
	li	v0, 9
	b	.ddrtype
	nop
21:	bne	v0, 2, 33f
	nop
	li	v0, 10
	b	.ddrtype
	nop
33:	PRINTSTR("DDR type not supported!\r\n");
34:	b	34b
	nop

.ddrtype:
	#bit 25:22 is DDR type field
	sll	v0, 22 
	and	v0,0x03c00000
	or	sdCfg,v0


	PRINTSTR("sdcfg11=");
	move	a0,sdCfg
	bal	hexserial
	nop
	PRINTSTR("\r\n");

	/* read DIMM memory size per side */
	li	a0,0xa1
	li	a1,31
	bal	i2cread
	nop
	beqz	v0,.nodimm
	nop
	sll	tmpsize,v0,22		# multiply by 4M
	PRINTSTR("read memory size per side\r\n") 

2:	/* read DIMM number of blocks-per-ddrram */
	li	a1,17
	bal	i2cread
	nop
	beq	v0,2,2f
	nop
	bne	v0,4,.nodimm
	nop
	PRINTSTR("read blocks per ddrram\r\n")

2:	/* read DIMM number of sides (banks) */
	li	a1,5
	bal	i2cread
	nop
	beq	v0,1,2f
	nop
	bne	v0,2,.nodimm
	nop
	sll	tmpsize,1	# msize *= 2	
	or  sdCfg, 0x1<<27
	PRINTSTR("read number of sides\r\n") 

2:	/* read DIMM width */
	li	a1,6
	bal	i2cread
	nop
	bleu	v0,36,2f
	nop
	bgtu	v0,72,.nodimm
	nop
	PRINTSTR("read width\r\n") 

2:	addu	msize,tmpsize
	b	2f
	nop	
