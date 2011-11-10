	la	s0, start		/* RA set from BAL above! */
	subu	s0, ra, s0		/* s0 is now load vs. link offset */
	and	s0, 0xffff0000		/* Mask off lower bits */
	
/*
 *  Turn off all high decoders to avoid address conflicts.
 */
	la	s2, GT_BASE_ADDR_DEFAULT
/*在初始化完成前,禁掉不用的设备*/
	INIT_CPU_WINDOW	
/*
 *  Relocate the Galileo to HIGH memory.
 */
	li	t0, HTOLE32((UNCACHED_TO_PHYS(GT_BASE_ADDR) >> 16) | 0x01000000)
	sw	t0, INTERNAL_SPACE_DECODE(s2)

	li	t0, 0x100		/* Need a small delay here */
1:
	bnez	t0, 1b
	addiu	t0, -1

	la	s2, GT_BASE_ADDR	/* From now this is the GT base */

	GT_REGWR(DEVCS_0_BASE_ADDRESS, UNCACHED_TO_PHYS(SRAM_BASE) >>16)
	GT_REGWR(DEVCS_0_SIZE, (SRAM_SIZE - 1) >> 16)
	
	ENABLE_CPU_WINDOW(DEVCS0_EN);
	
	li v0,SRAM_BASE
1:
	lw v1,(v0)
	b 1b
	nop

/*led init*/
GT_REGWR(GPP_IO_CONTROL,MPP_UART_TX|DBG_LED0|DBG_LED1|DBG_LED2);
1:
GT_REGSET(GPP_VALUE,DBG_LED0|DBG_LED1|DBG_LED2);
DELAY(0x10000);
GT_REGCLR(GPP_VALUE,DBG_LED0|DBG_LED1|DBG_LED2);
b 1b
nop
