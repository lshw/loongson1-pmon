#define GT_REGWR_R(offs, reg)		\
	sw	reg, GT_BASE_ADDR+(offs)

#define GT_REGRD_R(offs,reg)			\
	lw	reg, GT_BASE_ADDR+(offs)
	
#define GT_REGWR(offs, value)		\
	li	v0, HTOLE32(value);	\
	sw	v0, GT_BASE_ADDR+(offs)

#define MVREGREAD GT_REGRD_R
#define MV_REG_WRITE GT_REGWR_R
#define MV_REG_WRITEC GT_REGWR

#define udelay(ms) \
	li v0,(ms)*0x10000; \
1: \
	bnez v0,1b; \
	addi v0,-1 


#define brg_disable(CHANNEL) \
	MVREGREAD(GALBRG_0_CONFREG+(CHANNEL*GALBRG_REG_GAP),v0); \
	li v1,0xFFFEFFFF; \
	and v0,v1; \
	MV_REG_WRITE(GALBRG_0_CONFREG+(CHANNEL*GALBRG_REG_GAP), v0); 

#define brg_enable(CHANNEL) \
	MVREGREAD(GALBRG_0_CONFREG+(CHANNEL*GALBRG_REG_GAP),v0); \
	li v1,0x00010000; \
	or v0,v1; \
	MV_REG_WRITE(GALBRG_0_CONFREG+(CHANNEL*GALBRG_REG_GAP), v0); 

#define brg_set_CDV( CHANNEL,CLOCK) \
	MVREGREAD(GALBRG_0_CONFREG+(CHANNEL*GALBRG_REG_GAP),v0); \
	li v1,0xFFFF0000; \
	and v0,v1; \
	li v1,(0x0000FFFF & CLOCK); \
	or v0,v1; \
	MV_REG_WRITE(GALBRG_0_CONFREG+(CHANNEL*GALBRG_REG_GAP),v0); 

#define brg_set_baudrate(CHANNEL,CONS_BAUD) \
	brg_disable(CHANNEL); \
	brg_set_CDV(CHANNEL,((CFG_BCLK_IN/(16*CONS_BAUD)) - 1))	/* set timer Reg. for BRG*/  \
	brg_enable(CHANNEL);



#define brg_set_clksrc(CHANNEL,VALUE) \
	MVREGREAD(GALBRG_0_CONFREG+(CHANNEL*GALBRG_REG_GAP),v0); \
	li v1,0xFFC3FFFF; \
	and v0,v1; \
	li v1,(VALUE<<18); \
	or v0,v1; \
	MV_REG_WRITE(GALBRG_0_CONFREG+(CHANNEL*GALBRG_REG_GAP),v0);

#define mpsc_set_brkcnt(MPSC,VALUE) /* mpsc_set_tcschar(value) */ \
	MVREGREAD(GALMPSC_CHANNELREG_1+(MPSC*GALMPSC_REG_GAP),v0); \
	and  v0,0x0000FFFF; \
	li v1,(VALUE<<16); \
	or v0,v1; \
	MV_REG_WRITE(GALMPSC_CHANNELREG_1+(MPSC*GALMPSC_REG_GAP),v0); 

#define mpsc_set_tcschar(MPSC,VALUE) /* mpsc_set_tcschar(value) */ \
	MVREGREAD(GALMPSC_CHANNELREG_1+(MPSC*GALMPSC_REG_GAP),v0); \
	li v1, 0xFFFF0000; \
	and v0,v1; \
	or v0,VALUE; \
	MV_REG_WRITE(GALMPSC_CHANNELREG_1+(MPSC*GALMPSC_REG_GAP),v0); 

#define mpsc_set_tcschar_a0(MPSC) /* mpsc_set_tcschar(value) */ \
	MVREGREAD(GALMPSC_CHANNELREG_1+(MPSC*GALMPSC_REG_GAP),v0); \
	li v1, 0xFFFF0000; \
	and v0,v1; \
	or v0,a0; \
	MV_REG_WRITE(GALMPSC_CHANNELREG_1+(MPSC*GALMPSC_REG_GAP),v0); 

#define mpsc_set_char_length(MPSC,VALUE) /*mpsc_set_char_length(value)*/ \
	MVREGREAD(GALMPSC_PROTOCONF_REG+(MPSC*GALMPSC_REG_GAP),v0); \
	li v1,0xFFFFCFFF; \
	and v0,v1; \
	li v1,(VALUE<<12); \
	or v0,v1; \
	MV_REG_WRITE(GALMPSC_PROTOCONF_REG+(MPSC*GALMPSC_REG_GAP),v0); 

#define mpsc_set_parity(MPSC,VALUE) /* mpsc_set_parity(int value) */ \
	MVREGREAD(GALMPSC_CHANNELREG_2+(MPSC*GALMPSC_REG_GAP),v0); \
	li v1,-1; \
	beq v0,v1,1f; \
	nop;\
	li v1,0xFFF3FFF3; \
	and v0,v1; \
	li v1,((VALUE<<18)|(VALUE<<2)|(VALUE<<17)|(VALUE<<1)); \
	or v0,v1; \
	b 2f; \
	nop; \
1: \
	li v1,0xFFF1FFF1; \
	and v0,v1; \
2: \
	MV_REG_WRITE(GALMPSC_CHANNELREG_2+(MPSC*GALMPSC_REG_GAP),v0); 

#define mpsc_set_stop_bit_length(MPSC,VALUE) /* mpsc_set_stop_bit_length(int value) */ \
	MVREGREAD(GALMPSC_PROTOCONF_REG+(MPSC*GALMPSC_REG_GAP),v0); \
	li v1,0xFFFFBFFF; \
    and v0,v1; \
	li v1,(VALUE<<14); \
	or v0,v1; \
	MV_REG_WRITE(GALMPSC_PROTOCONF_REG+(MPSC*GALMPSC_REG_GAP),v0); 

#define mpsc_connect(CHANNEL) \
	MVREGREAD(GALMPSC_ROUTING_REGISTER,v0); \
	li v1,CHANNEL; \
	beqz v1,1f; \
	nop; \
	li v1,~(0x00000007 << 6); \
	b 2f;\
	nop;\
1: \
	li v1,~0x00000007; \
2: \
	and v0,v1; \
   li v1, 0x3fffffff; \
   and v0,v1; \
   MV_REG_WRITE(GALMPSC_ROUTING_REGISTER, v0); 


#define mpsc_disconnect(CHANNEL) \
	MVREGREAD(GALMPSC_ROUTING_REGISTER,v0); \
	li v1,CHANNEL; \
	beqz v1,1f; \
	nop;\
	li v1,(0x00000007 << 6); \
	b 2f; \
	nop; \
1: \
	li v1,0x00000007; \
2: \
	or v0,v1; \
   li v1, 0x3fffffff; \
   and v0,v1; \
   MV_REG_WRITE(GALMPSC_ROUTING_REGISTER, v0); 
   
#define mpsc_route_rx_clock(CHANNEL,BRG)   /*mpsc_route_rx_clock(int brg)*/ \
	MVREGREAD(GALMPSC_RxC_ROUTE,v0); \
	li a0,~0x00000F00; \
	li a1,(BRG<<8); \
	li v1,CHANNEL; \
	bnez v1,1f; \
	nop; \
    li a0,~0x0000000F; \
	li a1,BRG; \
1: \
	and v0,a0; \
	or v0,a1; \
	MV_REG_WRITE(GALMPSC_RxC_ROUTE,v0); 

#define mpsc_route_tx_clock(CHANNEL,BRG)   /*mpsc_route_rx_clock(int brg)*/ \
	MVREGREAD(GALMPSC_TxC_ROUTE,v0); \
	li a0,~0x00000F00; \
	li a1,(BRG<<8); \
	li v1,CHANNEL; \
	bnez v1,1f; \
	nop; \
    li a0,~0x0000000F; \
	li a1,BRG; \
1: \
	and v0,a0; \
	or v0,a1; \
	MV_REG_WRITE(GALMPSC_TxC_ROUTE,v0); 

#define mpsc_shutdown(MPSC) \
	/* cause RX abort (clears RX) */ \
	MVREGREAD(GALMPSC_CHANNELREG_2+(MPSC*GALMPSC_REG_GAP),v0); \
	or v0, MPSC_RX_ABORT | MPSC_TX_ABORT; \
	and v0, ~MPSC_ENTER_HUNT; \
	MV_REG_WRITE(GALMPSC_CHANNELREG_2+(MPSC*GALMPSC_REG_GAP),v0); \
	MV_REG_WRITE(GALSDMA_0_COM_REG, zero); \
	MV_REG_WRITEC(GALSDMA_0_COM_REG, SDMA_TX_ABORT | SDMA_RX_ABORT); \
\
	/* shut down the MPSC */ \
	MV_REG_WRITE(GALMPSC_MCONF_LOW, zero); \
	MV_REG_WRITE(GALMPSC_MCONF_HIGH, zero); \
	MV_REG_WRITE(GALMPSC_PROTOCONF_REG+(MPSC*GALMPSC_REG_GAP),zero); \
\
	udelay(100); \
\
	/* shut down the sdma engines. */ \
	/* reset config to default */	\
	MV_REG_WRITEC(GALSDMA_0_CONF_REG, 0x000000fc); \
\
	udelay(100); \
\
	/* clear the SDMA current and first TX and RX pointers */ \
	MV_REG_WRITE(GALSDMA_0_CUR_RX_PTR, zero); \
	MV_REG_WRITE(GALSDMA_0_CUR_TX_PTR, zero); \
	MV_REG_WRITE(GALSDMA_0_FIR_TX_PTR, zero); \
\
	udelay(100);

#define mpsc_write_config_regs(MPSC) /* mpsc_write_config_regs  for uart*/ \
		/* Main config reg Low (Null modem, Enable Tx/Rx, UART mode) */ \
		MV_REG_WRITEC(GALMPSC_MCONF_LOW+(MPSC*GALMPSC_REG_GAP), 0x000004c4); \
\
		/* Main config reg High (8x Rx/Tx clock mode, width=8bits */ \
		/*MV_REG_WRITEC(GALMPSC_MCONF_HIGH+(MPSC*GALMPSC_REG_GAP), 0x024003f8); */\
		MV_REG_WRITEC(GALMPSC_MCONF_HIGH+(MPSC*GALMPSC_REG_GAP), 0x02400200); \
		/*        22 2222 1111*/ \
		/*        54 3210 9876*/ \
		/* 0000 0010 0100 0000*/ \
		/*       1*/ \
		/*       098 7654 3210*/ \
		/* 0000 0010 0000 0000*/ 

#define mpsc_config_channel_regs(MPSC) \
	MV_REG_WRITE(GALMPSC_CHANNELREG_1+(MPSC*GALMPSC_REG_GAP), zero); \
	MV_REG_WRITE(GALMPSC_CHANNELREG_2+(MPSC*GALMPSC_REG_GAP), zero); \
	MV_REG_WRITEC(GALMPSC_CHANNELREG_3+(MPSC*GALMPSC_REG_GAP), 1); \
	MV_REG_WRITE(GALMPSC_CHANNELREG_4+(MPSC*GALMPSC_REG_GAP), zero); \
	MV_REG_WRITE(GALMPSC_CHANNELREG_5+(MPSC*GALMPSC_REG_GAP), zero); \
	MV_REG_WRITE(GALMPSC_CHANNELREG_6+(MPSC*GALMPSC_REG_GAP), zero); \
	MV_REG_WRITE(GALMPSC_CHANNELREG_7+(MPSC*GALMPSC_REG_GAP), zero); \
	MV_REG_WRITE(GALMPSC_CHANNELREG_8+(MPSC*GALMPSC_REG_GAP), zero); \
	MV_REG_WRITE(GALMPSC_CHANNELREG_9+(MPSC*GALMPSC_REG_GAP), zero); \
	MV_REG_WRITE(GALMPSC_CHANNELREG_10+(MPSC*GALMPSC_REG_GAP), zero); \
\
	mpsc_set_brkcnt(MPSC,0x3); \
	mpsc_set_tcschar(MPSC,0xab);

#define mpsc_debug_init(CHANNEL) \
	/* Clear the CFR  (CHR4) */ \
	/* Write random 'Z' bit (bit 29) of CHR4 to enable debug uart *UNDOCUMENTED FEATURE* */ \
	MVREGREAD(GALMPSC_CHANNELREG_4+(CHANNEL*GALMPSC_REG_GAP),v0); \
	li v1,0xffffff00; \
	and v0,v1; \
	li v1,(1<<29); \
	or v0,v1; \
	MV_REG_WRITE(GALMPSC_CHANNELREG_4+(CHANNEL*GALMPSC_REG_GAP),v0); \
	/* Set the Valid bit 'V' (bit 12) and int generation bit 'INT' (bit 15) */ \
	MVREGREAD(GALMPSC_CHANNELREG_5+(CHANNEL*GALMPSC_REG_GAP),v0); \
	li v1,(1<<12)|(1<<15); \
	or v0,v1; \
	MV_REG_WRITE(GALMPSC_CHANNELREG_5+(CHANNEL*GALMPSC_REG_GAP),v0); \
\
	/* Set int mask */ \
	MVREGREAD(GALMPSC_0_INT_MASK,v0); \
	li v1,(1<<6); \
	or v0,v1; \
	MV_REG_WRITE(GALMPSC_0_INT_MASK, v0); 


#define CONFIG_MPSC_PORT 0

/* special function for running out of flash.  doesn't modify any
 * global variables [josh] */
LEAF(tgt_putchar) /*mpsc_putchar_early(char ch)*/
	MVREGREAD(GALMPSC_CHANNELREG_2+(CHANNEL*GALMPSC_REG_GAP),a1);
	mpsc_set_tcschar_a0(CHANNEL)
	nop
	li v1,0x200
	or a1,v1
	MV_REG_WRITE(GALMPSC_CHANNELREG_2+(CHANNEL*GALMPSC_REG_GAP),a1);

	DELAY(0x200);
	#SET_LED(5)

	jr ra
	nop
END(tgt_putchar)

LEAF(tgt_getchar)
1:
	MVREGREAD(GALMPSC_0_INT_CAUSE,a0);
	li v1,(1<<6)
	and v0,a0,v1
	beqz v0,1b
	nop
	MVREGREAD(GALMPSC_CHANNELREG_10,v0);
	/* By writing 1's to the set bits, the register is cleared */
	MV_REG_WRITE(GALMPSC_CHANNELREG_10+(CHANNEL*GALMPSC_REG_GAP),v0);
	li v1,~(1<<6)
	and a0,v1
	MV_REG_WRITE(GALMPSC_0_INT_CAUSE,a0);
	srl v0,16
	andi v0,0xff
	jr ra
	nop
END(tgt_getchar)

LEAF(initserial)
	/* BRG CONFIG */

	brg_set_baudrate(CHANNEL,CONS_BAUD);/*baudrate*/
	#brg_set_clksrc(CHANNEL,8);	/* set source=Tclk */
	brg_set_clksrc(CHANNEL,0);	/* set source=bclk_in*/
	#brg_set_clksrc(CHANNEL,7);	/* set source=tsclk1*/
	brg_enable(CHANNEL);		/* Enable BRG */


	/* Set up clock routing */
	mpsc_connect(CHANNEL)	/* connect it */
	mpsc_route_rx_clock(CHANNEL,CHANNEL);		/* chosse BRG0 for Rx */
	mpsc_route_tx_clock(CHANNEL,CHANNEL);		/* chose BRG0 for Tx */


	/* reset MPSC state */
	#mpsc_shutdown(CHANNEL);


	/* MPSC CONFIG */
	mpsc_write_config_regs(CHANNEL);
	mpsc_config_channel_regs(CHANNEL);
	mpsc_set_char_length(CHANNEL,GALMPSC_CHAR_LENGTH_8);       /* 8 */
	mpsc_set_parity(CHANNEL,GALMPSC_PARITY_NONE);              /* N */
	mpsc_set_stop_bit_length(CHANNEL,GALMPSC_STOP_BITS_1)     /* 1 */


    mpsc_debug_init(CHANNEL)
	nop

	jr ra
	nop
END(initserial)

