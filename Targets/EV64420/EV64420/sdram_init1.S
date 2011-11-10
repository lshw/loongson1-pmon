/*
rules:
use #define to define varible
the varible is valid between #define and #undef. 
*/
#define SRAM_DATA0 0x1494
#define SDRAM_OPERATION                                     0x1418
#define DFCDL_CONFIG0                                       0x1480
#define SDRAM_MODE                                          0x141c
#define SDRAM_CONFIG	 				0x1400
#define SDRAM_OPEN_PAGES_CONTROL	0x1414
#define D_UNIT_CONTROL_LOW	0x1404
#define SDRAM_ADDR_CONTROL  0x1410
#define SDRAM_TIMING_CONTROL_LOW 0x1408
#define SDRAM_TIMING_CONTROL_HIGH 0x140c

#define GT_REGWR_R(offs, reg)		\
	sw	reg, GT_BASE_ADDR+(offs)

#define GT_REGRD(offs)			\
	lw	v0, GT_BASE_ADDR+(offs)

#define GT_REGWR(offs, value)		\
	li	v0, HTOLE32(value);	\
	sw	v0, GT_BASE_ADDR+(offs)



#define I2C_RD_SPD(addr) \
    li  a0, 0x000|(addr); \
  	bal boot_i2c_read; \
	nop;

/*	
可能需要预先设置StBurstDel
*/
sdram_init:
	#define dfcdl_word t1
	#define tmp t0
	 li dfcdl_word,0x391
	 li tmp,64
1:
	 GT_REGWR_R(SRAM_DATA0, dfcdl_word);
	 addiu dfcdl_word,0x41
	 addi tmp,-1
	 bnez tmp,1b
	 nop
	#undef dfcdl_word
	#undef tmp
	
	GT_REGWR(DFCDL_CONFIG0, 0x00300000);	/* enable dynamic delay line updating */

	
	/* set SDRAM mode NOP */ /* To_do check it */
	GT_REGWR(SDRAM_OPERATION, 0x5);


	1:
	GT_REGRD (SDRAM_OPERATION)
	beqz v0,1f;	
	nop
	TTYDBG("\n*** SDRAM_OPERATION 1418: Module still busy ... please wait... ***\n");
	b 1b
	nop
1:
#define DDR_REFRESH_COUNTER 0x400
#define DDR_RegDRAM 	0     /*Enable Registered SDRAM
								0 = Non-registered SDRAM
								1 = Registered SDRAM
								*/
#define DDR_ECC    		0	    /*Enable ECC
								0 = ECC not enabled
								1 = ECC enabled
								When ECC is enabled, every partial write to the DRAM results in RMW.
								*/
#define DDR_IEER        1	    /*Ignore ECC errors.
								0 = ECC errors are reported
								1 = ECC errors are ignore. However, RMW is still performed per partial
								writes to DRAM
								*/
#define DDR_DQS         2          /*Data M_DQSn Pins
								0x0 = x32 devices (eight DQS pins)
								0x1 = Reserved
								0x2 = x8/x16 devices (eight DQS pins)
								0x3 = x4 devices (16 DQS pins)
								NOTE: When using x4 devices, data mask (DM) is not supported.
								When using x4 devices, use M_DQSn[17,8] as the data strobe for
								the ECC byte. In all other configurations, use M_DQSn[8].
								*/
#define DDR_RFMode      0      /*Refresh mode. How many refresh cycles every time the refresh counters
								expires
								0x0 = Single refresh cycle
								0x1 = Four refresh cycles
								0x2 = Eight refresh cycles
								0x3 = Reserved
								*/
#define DDR_SRMode 		0  	   /*Self Refresh Mode
								0x0 = Once entered self refresh, exit only upon power on reset
								*/
#define DDR_RdBuff      1      /*Read buffer assignment per each interface.
								If set to 0, the corresponding unit receives read data from read buffer 0.
								If set to 1, it receives read data from read buffer 1.
								[27] = PCI read
								[28] = Reserved
								[29] = MPSC read
								[30] = IDMA read
								[31] = Gigabit Ethernet read
								*/
#define SDRAM_CONFIG_DATA ((DDR_REFRESH_COUNTER)|(DDR_RegDRAM<<17)|(DDR_ECC<<18)|(DDR_IEER<<19)|(DDR_DQS<<20)|(DDR_RFMode<<22)|(DDR_SRMode<<24)|(DDR_RdBuff<<27))
#define sdram_config s1
	li sdram_config,SDRAM_CONFIG_DATA
	sw	sdram_config, GT_BASE_ADDR+(SDRAM_CONFIG)
#undef sdram_config
	GT_REGWR (SDRAM_OPEN_PAGES_CONTROL, 0x1);/*keep bank open if possible*/
	
#define DDR_BL   0x2    /* Burst Length Must be set to 0x2 (BL = 4).  */
#define DDR_CL   0x3    /* CAS Latency
						0x0-0x1 = Reserved
						0x2 = CL = 2
						0x3 = CL = 3
						0x4 = CL = 4
						0x5 = CL = 1.5
						0x6 = CL = 2.5
						0x7 = Reserved
						*/
#define DDR_OM    0    /*Operation Mode
						0x0 = Normal operation
						0x1 = Reserved.
						0x2 = Reset DLL. Relevant only to standard DDR DRAM
						0x3 - FF = Reserved
						*/

#define SDRAM_MODE_DATA DDR_BL|(DDR_CL<<4)|(DDR_OM<<7)

#define DDR_SYNC  0 /*
						Crossbar to DRAM Controller Synchronization
						0 = Crossbar requests are synchronized
						1 = Crossbar synchronizers are bypassed
						NOTE: Synchronizers bypass is only allowed when the core and the Dunit
						run with the same clock.
						*/

#define DDR_CtrlPos 0 /*Address/Control Output Timing
						0 = On falling edge of clock
						1 = On rising edge of clock
						NOTE: When interfacing single DRAM bank (meaning, address/control
						signals interface lite load), it is recommended to use falling edge,
						in order to prevent DRAM hold time violations.
						*/

#define DDR_CRdSyncEn ~DDR_SYNC /*
								DRAM to Crossbar Read Data Path Synchronization
								0 = Disabled
								1 = Enabled
								NOTE: Synchronizers bypass is only allowed when the core and the Dunit
								run with the same clock
								*/
#define DDR_prio 	1	/*CPU fast path priority over crossbar transactions:
						0x0 = One crossbar transaction per one CPU transaction (1/1)
						0x1 = One crossbar transaction per two CPU transactions (2/1)
						0x2 = Once crossbar transaction per four CPU transactions (4/1)
						0x3 = Crossbar "starvation". Always prefer CPU transaction (serve crossbar
						transaction only when there is no pending CPU transaction).
						*/

						/*Number of sample stages on StartBurstIn.
						  Program StBurstDel based on CL, registered/non-buffered DIMM.
						  */

#if ((DDR_CL==2)|(DDR_CL==3)|(DDR_CL==4))
#define DDR_StBurstDel (DDR_CL+1+DDR_RegDRAM)
#elif ((DDR_CL==5)|(DDR_CL==6))
#define DDR_StBurstDel (DDR_CL-2+DDR_RegDRAM)
#else
error
#endif



#define DDR_FPMode     3  /*Fast path mode.
						0x0, 0x2 = Reserved.
						0x1 = CPU fast path transactions bypass transaction queue, only when
						queue is empty.
						0x3 = CPU fast path transactions bypass transaction queue, only when
						queue is empty, or when address lookup (page + bank address) results
						in miss.
						Reserved for Marvell usage.
						*/

li dunit_ctrol_low,(DDR_SYNC|(DDR_CtrlPos<<6)|(DDR_CRdSyncEn<<8)|(DDR_prio<<10)|(DDR_StBurstDel<<24)|(DDR_FPMode<<30))


#define sdram_mode a2
#define dunit_ctrol_low a3
	/* Write results of CL detection procedure */
	/* set SDRAM mode reg. 0x141c */
	li sdram_mode,SDRAM_MODE_DATA
	GT_REGWR_R(SDRAM_MODE, sdram_mode);

	/* set SDRAM modeSet Command 0x1418 */
	GT_REGWR (SDRAM_OPERATION, 0x3);
1:
	GT_REGRD (SDRAM_OPERATION)
	beqz v0,1f;	
	nop
	TTYDBG("\n*** SDRAM_OPERATION 1418: Module still busy ... please wait... ***\n");
	b 1b
	nop
1:
/* SDRAM D_UNIT_CONTROL_LOW 0x1404 */
	GT_REGWR_R (D_UNIT_CONTROL_LOW, dunit_ctrol_low);

	/* set SDRAM modeSet Command 0x1418 */
	GT_REGWR (SDRAM_OPERATION, 0x3);
1:
	GT_REGRD (SDRAM_OPERATION)
	beqz v0,1f;	
	nop
	TTYDBG("\n*** SDRAM_OPERATION 1418: Module still busy ... please wait... ***\n");
	b 1b
	nop

#define DDR_AddrSel 2 /*
					SDRAM Address Select
					Determines what address bits to drive on M_DA[13:0] and M_BA[1:0] during
					activate and command phases (row and column addresses)
					*/
#define DDR_DCfg 1 /*
					SDRAM Address Select
					Determines what address bits to drive on M_DA[13:0] and M_BA[1:0] during
					activate and command phases (row and column addresses)
					*/

GT_REGWR (SDRAM_ADDR_CONTROL, (DDR_AddrSel|(DDR_DCfg<<4)));

/*the data bellow is very complex for caculation*/
GT_REGWR (SDRAM_TIMING_CONTROL_LOW, 0x01701440);
GT_REGWR (SDRAM_TIMING_CONTROL_HIGH, 0x9);

#define banksize a2
#define bankno a0
#define bankbase a1
#define nums_modules s1
#define memsize t8

#define DDR_MODULES 2
#definr DDR_MODULE_SIZE 128*(1<<20)

	li nums_modules,DDR_MODULES
	li banksize,DDR_MODULE_SIZE
	move memsize,banksize
1:
	bal map_bank
	nop
#define NUMS_MODULE_OFS 5
	beqz nums_modules,_map_end
	subu nums_modules,1
	add memsize,banksize
	add bankbase,bankbase,banksize
	addi bankno,bankno,1
	b 1b
	nop
	
mem_ofs:
.word  8,0x208,0x18,0x218

LEAF(map_bank)
#define bankno a0
#define bankbase a1
#define banksize a2
#define link_offs s0
#define register_base s2
#define relocation s0
	la t7,mem_ofs
	sll t1,bankno,2
	add t7,t1
	add t7,relocation
	lw t6,(t7)
	add t6,register_base
/*setup memory base address and size */
	srl t0,bankbase,16
	sw t0,(t6)
	addi t0,banksize,-1
	srl t0,16
	sw t0,4(t6)
/*enable ddr memory window*/
	li t1,1
	sll t1,bankno
	not t1
	lw t0,BASE_ADDRESS_ENABLE_REG(register_base);	
	and t0,t1;
	sw t0,BASE_ADDRESS_ENABLE_REG(register_base);	
	jr ra
	nop
END(map_bank)
	
_map_end:
	move s1,memsize
#undef memsize
#undef bankno
#undef bankbase
#undef banksize


//initdram先运行.
	


	
	
	

	
	


