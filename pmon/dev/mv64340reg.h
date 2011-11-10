/* $Id: mv64340reg.h,v 1.1.1.1 2006/09/14 01:59:08 root Exp $ */

/*
 * Copyright (c) 2001-2002 Galileo Technology.
 * Copyright (c) 2002 Momentum Computer.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by Galileo Technology.
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 */
/* mv64340reg.h - MV-64340 Internal registers definition file */

#ifndef _MV64340REG_H_
#define _MV64340REG_H_

#ifdef __MIPSEB__
#define HTOLE32(v)      ((((v) & 0xff) << 24) | (((v) & 0xff00) << 8) | \
                        (((v) >> 24) & 0xff) | (((v) >> 8) & 0xff00))
#else
#ifdef __MIPSEL__
#define HTOLE32(v)      (v)
#else
#error ENDIAN NOT DEFINED!
#endif
#endif

#define GT_WRITE(ofs, data) \
    *(volatile u_int32_t *)(GT_BASE_ADDR+ofs) = HTOLE32(data)

#define GT_READ(ofs) \
    HTOLE32(*(volatile u_int32_t *)(GT_BASE_ADDR+ofs))

#define GT_BASE_ADDR			PHYS_TO_IOSPACE(0x14000000)
#define GT_BASE_ADDR_DEFAULT		PHYS_TO_UNCACHED(0x14000000)


/************************************************/
#define GT_DEVPAR_TurnOff(x)            ((((x) & 7) << 0) | (((x) & 8) << 19))
#define GT_DEVPAR_AccToFirst(x)         ((((x) & 15) << 3) | (((x) & 16) << 19))
#define GT_DEVPAR_AccToNext(x)          ((((x) & 15) << 7) | (((x) & 16) << 20))
#define GT_DEVPAR_ALEtoWr(x)            ((((x) & 7) << 11) | (((x) & 8) << 22))
#define GT_DEVPAR_WrActive(x)           ((((x) & 7) << 14) | (((x) & 8) << 23))
#define GT_DEVPAR_WrHigh(x)             ((((x) & 7) << 17) | (((x) & 8) << 24))
#define GT_DEVPAR_DevWidth8             (0<<20)
#define GT_DEVPAR_DevWidth16            (1<<20)
#define GT_DEVPAR_DevWidth32            (2<<20)
#define	GT_DEVPAR_DevWidthMASK		(3<<20)
#define GT_DEVPAR_BaddrSkew0            (0<<28)
#define GT_DEVPAR_BaddrSkew1            (1<<28)
#define GT_DEVPAR_BaddrSkew2            (2<<28)
#define GT_DEVPAR_BaddrSkewMASK         (3<<28)
#define GT_DEVPAR_DPEnDisable           (0<<30)
#define GT_DEVPAR_DPEnEnable            (1<<30)
#define GT_DEVPAR_ReservedMASK          0x80000000
#define GT_DEVPAR_Reserved              0x80000000 

/* MDD: Done to here */

/************************************************/
#define GT_PCI0_MAP10                   0x0010
#define GT_PCI0_MAP14                   0x0014
#define GT_PCI1_MAP10                   0x0090
#define GT_PCI1_MAP14                   0x0094
/************************************************/
#define GT_IPCI_CFGADDR_ConfigEn        (1<<31)
/************************************************/
#define GT_SMIR_REG			0x2010
#define CPU_CONF        		0x0000
#define SDRAM_CNFG      		0x0448
#define SDRAM_PARA0     		0x044c
#define SDRAM_PARA1     		0x0450
#define SDRAM_PARA2     		0x0454
#define SDRAM_PARA3     		0x0458
#define GT_DEV0_PAR     		0x045c
#define GT_DEV1_PAR     		0x0460
#define GT_DEV2_PAR     		0x0464
#define GT_DEV3_PAR     		0x0468
#define GT_BOOT_PAR     		0x046c
#define PCI0_COMMAND    		0x0c00
#define PCI1_COMMAND    		0x0c80
#define PCI0_IO_LO      		0x0048
#define PCI0_IO_HI      		0x0050
#define PCI0_MEM0_LO    		0x0058
#define PCI0_MEM0_HI    		0x0060
#define PCI1_IO_LO     			0x0090
#define PCI1_IO_HI      		0x0098
#define PCI1_MEM0_LO    		0x00a0
#define PCI1_MEM0_HI    		0x00a8
#define CPU_CONFIG      		0x0000
#define SCS0_LOW 	     		0x0008
#define SCS0_HIGH 	    		0x0010
#define SCS2_LOW 	     		0x0018
#define SCS2_HIGH 	    		0x0020
#define SCS1_LOW        		0x0208
#define SCS1_HIGH       		0x0210
#define SCS3_LOW        		0x0218
#define SCS3_HIGH       		0x0220
#define PCI_1_IO_LOW    		0x0090
#define PCI_1_IO_HIGH   		0x0098
#define PCI1_MEM0_LOW   		0x00a0
#define PCI1_MEM0_HIGH  		0x00a8
#define PCI_1_MEM1_LOW  		0x00b0
#define PCI_1_MEM1_HIGH 		0x00b8
#define PCI_0_TIME_OUT  		0x0c04
#define PCI_1_TIME_OUT  		0x0c84
#define PCI_0_BAR_EN    		0x0c3c
#define PCI_1_BAR_EN    		0x0cbc
#define PCI_0_ARBITER   		0x1d00
#define PCI_1_ARBITER   		0x1d80
#define MPP_CNTRL0      		0xf000
#define MPP_CNTRL1      		0xf004
#define MPP_CNTRL2      		0xf008
#define MPP_CNTRL3      		0xf00c
#define SER_PORTS_MUX   		0xf010

/****************************************/
/* Processor Address Space		*/
/****************************************/

/* MDD: This section done */

/* BAR Enables */
#define CPU_BASE_ADDRESS_ENABLE				0x278

/* Sdram's BAR'S */
#define SCS_0_BASE_ADDRESS				0x008
#define SCS_0_SIZE					0x010
#define SCS_1_BASE_ADDRESS				0x208
#define SCS_1_SIZE					0x210
#define SCS_2_BASE_ADDRESS				0x018
#define SCS_2_SIZE					0x020
#define SCS_3_BASE_ADDRESS				0x218
#define SCS_3_SIZE					0x220

/* Devices BAR'S */
#define CS_0_BASE_ADDRESS				0x028
#define CS_0_SIZE					0x030
#define CS_1_BASE_ADDRESS			    	0x228
#define CS_1_SIZE					0x230
#define CS_2_BASE_ADDRESS			    	0x248
#define CS_2_SIZE					0x250
#define CS_3_BASE_ADDRESS				0x038
#define CS_3_SIZE					0x040
#define BOOTCS_BASE_ADDRESS				0x238
#define BOOTCS_SIZE					0x240

#define PCI_0I_O_BASE_ADDRESS				0x048
#define PCI_0I_O_SIZE					0x050
#define PCI_0MEMORY0_BASE_ADDRESS			0x058
#define PCI_0MEMORY0_SIZE				0x060
#define PCI_0MEMORY1_BASE_ADDRESS			0x080
#define PCI_0MEMORY1_SIZE				0x088
#define PCI_0MEMORY2_BASE_ADDRESS			0x258
#define PCI_0MEMORY2_SIZE				0x260
#define PCI_0MEMORY3_BASE_ADDRESS			0x280
#define PCI_0MEMORY3_SIZE				0x288

#define PCI_1I_O_BASE_ADDRESS				0x090
#define PCI_1I_O_SIZE					0x098
#define PCI_1MEMORY0_BASE_ADDRESS			0x0a0
#define PCI_1MEMORY0_SIZE				0x0a8
#define PCI_1MEMORY1_BASE_ADDRESS			0x0b0
#define PCI_1MEMORY1_SIZE				0x0b8
#define PCI_1MEMORY2_BASE_ADDRESS			0x2a0
#define PCI_1MEMORY2_SIZE				0x2a8
#define PCI_1MEMORY3_BASE_ADDRESS			0x2b0
#define PCI_1MEMORY3_SIZE				0x2b8

#define INTERNAL_SPACE_BASE				0x068
#define INTERNAL_SRAM_BASE				0x268

#define PCI_0I_O_ADDRESS_REMAP				0x0f0
#define PCI_0MEMORY0_ADDRESS_REMAP  			0x0f8
#define PCI_0MEMORY0_HIGH_ADDRESS_REMAP			0x320
#define PCI_0MEMORY1_ADDRESS_REMAP  			0x100
#define PCI_0MEMORY1_HIGH_ADDRESS_REMAP			0x328
#define PCI_0MEMORY2_ADDRESS_REMAP  			0x2f8
#define PCI_0MEMORY2_HIGH_ADDRESS_REMAP			0x330
#define PCI_0MEMORY3_ADDRESS_REMAP  			0x300
#define PCI_0MEMORY3_HIGH_ADDRESS_REMAP			0x338

#define PCI_1I_O_ADDRESS_REMAP				0x108
#define PCI_1MEMORY0_ADDRESS_REMAP  			0x110
#define PCI_1MEMORY0_HIGH_ADDRESS_REMAP			0x340
#define PCI_1MEMORY1_ADDRESS_REMAP  			0x118
#define PCI_1MEMORY1_HIGH_ADDRESS_REMAP			0x348
#define PCI_1MEMORY2_ADDRESS_REMAP  			0x310
#define PCI_1MEMORY2_HIGH_ADDRESS_REMAP			0x350
#define PCI_1MEMORY3_ADDRESS_REMAP  			0x318
#define PCI_1MEMORY3_HIGH_ADDRESS_REMAP			0x358


/****************************************/
/* CPU Sync Barrier             	*/
/****************************************/

#define PCI_0SYNC_BARIER_VIRTUAL_REGISTER		0x0c0
#define PCI_1SYNC_BARIER_VIRTUAL_REGISTER		0x0c8


/****************************************/
/* CPU Access Protect             	*/
/****************************************/

#define CPU_LOW_PROTECT_ADDRESS_0                     	0x180
#define CPU_HIGH_PROTECT_ADDRESS_0                      0x188
#define CPU_LOW_PROTECT_ADDRESS_1                       0x190
#define CPU_HIGH_PROTECT_ADDRESS_1                      0x198
#define CPU_LOW_PROTECT_ADDRESS_2                       0x1a0
#define CPU_HIGH_PROTECT_ADDRESS_2                      0x1a8
#define CPU_LOW_PROTECT_ADDRESS_3                       0x1b0
#define CPU_HIGH_PROTECT_ADDRESS_3                      0x1b8

/****************************************/
/*          CPU Error Report       	*/
/****************************************/

#define CPU_BUS_ERROR_LOW_ADDRESS 			0x070
#define CPU_BUS_ERROR_HIGH_ADDRESS 			0x078
#define CPU_BUS_ERROR_LOW_DATA                          0x128
#define CPU_BUS_ERROR_HIGH_DATA                         0x130
#define CPU_BUS_ERROR_LOW_PARITY                        0x138
#define CPU_BUS_ERROR_HIGH_PARITY                       0x140
#define CPU_BUS_ERROR_MASK                              0x148

/****************************************/
/*          Pslave Debug           	*/
/****************************************/

#define X_0_ADDRESS                                   	0x360
#define X_0_COMMAND_ID                                  0x368
#define X_1_ADDRESS                                     0x370
#define X_1_COMMAND_ID                                  0x378
#define WRITE_DATA_LOW                                  0x3c0
#define WRITE_DATA_HIGH                                 0x3c8
#define WRITE_BYTE_ENABLE                               0X3e0
#define READ_DATA_LOW                                   0x3d0
#define READ_DATA_HIGH                                  0x3d8
#define READ_ID                                         0x3e8


/****************************************/
/* SDRAM and Device Address Space	*/
/****************************************/
	

/****************************************/
/* SDRAM Configuration			*/
/****************************************/

/* MDD This section done */

#define SDRAM_CONFIGURATION	 			0x1400
#define DUNIT_CONTROL_LOW				0x1404
#define DUNIT_CONTROL_HIGH				0x1424
#define SDRAM_TIMING_LOW				0x1408
#define SDRAM_TIMING_HIGH				0x140c
#define SDRAM_ADDRESS_CONTROL				0x1410
#define SDRAM_OPEN_PAGES_CONTROL			0x1414
#define SDRAM_OPERATION					0x1418
#define SDRAM_MODE					0x141c
#define SDRAM_XTENDED_MODE				0x1420
#define SDRAM_CROSSBAR_CONTROL_LOW			0x1430
#define SDRAM_CROSSBAR_CONTROL_HIGH			0x1434
#define SDRAM_CROSSBAR_TIMEOUT				0x1438
#define SDRAM_ADDRESS_PADS_CALIBRATION			0x14c0
#define SDRAM_DATA_PADS_CALIBRATION			0x14c4

#define DFCDL_CONFIGURATION0				0x1480
#define DFCDL_CONFIGURATION1				0x1484
#define SRAM_ADDRESS					0x1490
#define SRAM_DATA0					0x1494
#define DFCDL_PROBE					0x14a0

/****************************************/
/* SDRAM Error Report 			*/
/****************************************/
			
#define SDRAM_ERROR_DATA_LOW                            0x484
#define SDRAM_ERROR_DATA_HIGH                           0x480
#define SDRAM_AND_DEVICE_ERROR_ADDRESS                  0x490
#define SDRAM_RECEIVED_ECC                              0x488
#define SDRAM_CALCULATED_ECC                            0x48c
#define SDRAM_ECC_CONTROL                               0x494
#define SDRAM_ECC_ERROR_COUNTER                         0x498


/****************************************/
/* SDunit Debug (for internal use)	*/
/****************************************/

#define X0_ADDRESS                                      0x500
#define X0_COMMAND_AND_ID                               0x504
#define X0_WRITE_DATA_LOW                               0x508
#define X0_WRITE_DATA_HIGH                              0x50c
#define X0_WRITE_BYTE_ENABLE                            0x518
#define X0_READ_DATA_LOW                                0x510
#define X0_READ_DATA_HIGH                               0x514
#define X0_READ_ID                                      0x51c
#define X1_ADDRESS                                      0x520
#define X1_COMMAND_AND_ID                               0x524
#define X1_WRITE_DATA_LOW                               0x528
#define X1_WRITE_DATA_HIGH                              0x52c
#define X1_WRITE_BYTE_ENABLE                            0x538
#define X1_READ_DATA_LOW                                0x530
#define X1_READ_DATA_HIGH                               0x534
#define X1_READ_ID                                      0x53c
#define X0_SNOOP_ADDRESS                                0x540
#define X0_SNOOP_COMMAND                                0x544
#define X1_SNOOP_ADDRESS                                0x548
#define X1_SNOOP_COMMAND                                0x54c

			

/****************************************/
/* Device Parameters			*/
/****************************************/

/* MDD: This section done */

#define DEVICE_BANK0PARAMETERS				0x45c
#define DEVICE_BANK1PARAMETERS				0x460
#define DEVICE_BANK2PARAMETERS				0x464
#define DEVICE_BANK3PARAMETERS				0x468
#define DEVICE_BOOT_BANK_PARAMETERS			0x46c
#define DEVICE_CONTROL                                  0x4c0
#define DEVICE_CROSS_BAR_CONTROL_LOW                    0x4c8
#define DEVICE_CROSS_BAR_CONTROL_HIGH                   0x4cc
#define DEVICE_CROSS_BAR_TIMEOUT                        0x4c4

/****************************************/
/* Device Interrupt 			*/
/****************************************/

#define DEVICE_INTERRUPT_CAUSE                          0x4d0
#define DEVICE_INTERRUPT_MASK                           0x4d4
#define DEVICE_ERROR_ADDRESS                            0x4d8

/****************************************/
/* DMA Record				*/
/****************************************/

#define CHANNEL0_DMA_BYTE_COUNT				0x800
#define CHANNEL1_DMA_BYTE_COUNT	 			0x804
#define CHANNEL2_DMA_BYTE_COUNT	 			0x808
#define CHANNEL3_DMA_BYTE_COUNT	 			0x80C
#define CHANNEL0_DMA_SOURCE_ADDRESS			0x810
#define CHANNEL1_DMA_SOURCE_ADDRESS			0x814
#define CHANNEL2_DMA_SOURCE_ADDRESS			0x818
#define CHANNEL3_DMA_SOURCE_ADDRESS			0x81C
#define CHANNEL0_DMA_DESTINATION_ADDRESS		0x820
#define CHANNEL1_DMA_DESTINATION_ADDRESS		0x824
#define CHANNEL2_DMA_DESTINATION_ADDRESS		0x828
#define CHANNEL3_DMA_DESTINATION_ADDRESS		0x82C
#define CHANNEL0NEXT_RECORD_POINTER			0x830
#define CHANNEL1NEXT_RECORD_POINTER			0x834
#define CHANNEL2NEXT_RECORD_POINTER			0x838
#define CHANNEL3NEXT_RECORD_POINTER			0x83C
#define CHANNEL0CURRENT_DESCRIPTOR_POINTER		0x870
#define CHANNEL1CURRENT_DESCRIPTOR_POINTER		0x874
#define CHANNEL2CURRENT_DESCRIPTOR_POINTER		0x878
#define CHANNEL3CURRENT_DESCRIPTOR_POINTER		0x87C

/****************************************/
/* DMA Channel Control			*/
/****************************************/

#define CHANNEL0CONTROL 				0x840
#define CHANNEL0CONTROL_HIGH				0x880
#define CHANNEL1CONTROL 				0x844
#define CHANNEL1CONTROL_HIGH				0x884
#define CHANNEL2CONTROL 				0x848
#define CHANNEL2CONTROL_HIGH				0x888
#define CHANNEL3CONTROL 				0x84C
#define CHANNEL3CONTROL_HIGH				0x88C

/****************************************/
/* DMA Arbiter				*/
/****************************************/

#define ARBITER_CONTROL_0_3				0x860
#define ARBITER_CONTROL_4_7				0x960


/****************************************/
/* DMA Interrupt			*/
/****************************************/

#define CHANELS0_3_INTERRUPT_CAUSE                      0x8c0
#define CHANELS0_3_INTERRUPT_MASK                       0x8c4
#define CHANELS0_3_ERROR_ADDRESS                        0x8c8
#define CHANELS0_3_ERROR_SELECT                         0x8cc
#define CHANELS4_7_INTERRUPT_CAUSE                      0x9c0
#define CHANELS4_7_INTERRUPT_MASK                       0x9c4
#define CHANELS4_7_ERROR_ADDRESS                        0x9c8
#define CHANELS4_7_ERROR_SELECT                         0x9cc


/****************************************/
/* DMA Debug (for internal use)         */
/****************************************/

#define DMA_X0_ADDRESS                                  0x8e0
#define DMA_X0_COMMAND_AND_ID                           0x8e4
#define DMA_X0_WRITE_DATA_LOW                           0x8e8
#define DMA_X0_WRITE_DATA_HIGH                          0x8ec
#define DMA_X0_WRITE_BYTE_ENABLE                        0x8f8
#define DMA_X0_READ_DATA_LOW                            0x8f0
#define DMA_X0_READ_DATA_HIGH                           0x8f4
#define DMA_X0_READ_ID                                  0x8fc
#define DMA_X1_ADDRESS                                  0x9e0
#define DMA_X1_COMMAND_AND_ID                           0x9e4
#define DMA_X1_WRITE_DATA_LOW                           0x9e8
#define DMA_X1_WRITE_DATA_HIGH                          0x9ec
#define DMA_X1_WRITE_BYTE_ENABLE                        0x9f8
#define DMA_X1_READ_DATA_LOW                            0x9f0
#define DMA_X1_READ_DATA_HIGH                           0x9f4
#define DMA_X1_READ_ID                                  0x9fc

/****************************************/
/* Timer_Counter 			*/
/****************************************/

#define TIMER_COUNTER0					0x850
#define TIMER_COUNTER1					0x854
#define TIMER_COUNTER2					0x858
#define TIMER_COUNTER3					0x85C
#define TIMER_COUNTER_0_3_CONTROL			0x864
#define TIMER_COUNTER_0_3_INTERRUPT_CAUSE		0x868
#define TIMER_COUNTER_0_3_INTERRUPT_MASK      		0x86c

/****************************************/
/* PCI Slave Address Decoding           */
/****************************************/

/* MDD This section done */
        
#define PCI_0SCS_0_BANK_SIZE				0xc08
#define PCI_1SCS_0_BANK_SIZE				0xc88
#define PCI_0SCS_1_BANK_SIZE				0xd08
#define PCI_1SCS_1_BANK_SIZE				0xd88
#define PCI_0SCS_2_BANK_SIZE				0xc0c
#define PCI_1SCS_2_BANK_SIZE				0xc8c
#define PCI_0SCS_3_BANK_SIZE				0xd0c
#define PCI_1SCS_3_BANK_SIZE				0xd8c
#define PCI_0CS_0_BANK_SIZE				0xc10
#define PCI_1CS_0_BANK_SIZE				0xc90
#define PCI_0CS_1_BANK_SIZE				0xd10
#define PCI_1CS_1_BANK_SIZE				0xd90
#define PCI_0CS_2_BANK_SIZE				0xd18
#define PCI_1CS_2_BANK_SIZE				0xd98
#define PCI_0CS_3_BANK_SIZE				0xc14
#define PCI_1CS_3_BANK_SIZE				0xc94
#define PCI_0CS_BOOT_BANK_SIZE				0xd14
#define PCI_1CS_BOOT_BANK_SIZE				0xd94
#define PCI_0P2P_MEM0_BAR_SIZE                          0xd1c
#define PCI_1P2P_MEM0_BAR_SIZE                          0xd9c
#define PCI_0P2P_MEM1_BAR_SIZE                          0xd20
#define PCI_1P2P_MEM1_BAR_SIZE                          0xda0
#define PCI_0P2P_I_O_BAR_SIZE                           0xd24
#define PCI_1P2P_I_O_BAR_SIZE                           0xda4
#define PCI_0INTERNAL_SRAM_BAR_SIZE                     0xe00
#define PCI_1INTERNAL_SRAM_BAR_SIZE                     0xe80
#define PCI_0EXPANSION_ROM_BAR_SIZE                     0xd2c
#define PCI_1EXPANSION_ROM_BAR_SIZE                     0xdac
#define PCI_0BASE_ADDRESS_REGISTERS_ENABLE 		0xc3c
#define PCI_1BASE_ADDRESS_REGISTERS_ENABLE 		0xcbc
#define PCI_0SCS_0_BASE_ADDRESS_REMAP			0xc48
#define PCI_1SCS_0_BASE_ADDRESS_REMAP			0xcc8
#define PCI_0SCS_1_BASE_ADDRESS_REMAP			0xd48
#define PCI_1SCS_1_BASE_ADDRESS_REMAP			0xdc8
#define PCI_0SCS_2_BASE_ADDRESS_REMAP			0xc4c
#define PCI_1SCS_2_BASE_ADDRESS_REMAP			0xccc
#define PCI_0SCS_3_BASE_ADDRESS_REMAP			0xd4c
#define PCI_1SCS_3_BASE_ADDRESS_REMAP			0xdcc
#define PCI_0CS_0_BASE_ADDRESS_REMAP			0xc50
#define PCI_1CS_0_BASE_ADDRESS_REMAP			0xcd0
#define PCI_0CS_1_BASE_ADDRESS_REMAP			0xd50
#define PCI_1CS_1_BASE_ADDRESS_REMAP			0xdd0
#define PCI_0CS_2_BASE_ADDRESS_REMAP			0xd58
#define PCI_1CS_2_BASE_ADDRESS_REMAP			0xdd8
#define PCI_0CS_3_BASE_ADDRESS_REMAP           		0xc54
#define PCI_1CS_3_BASE_ADDRESS_REMAP           		0xcd4
#define PCI_0CS_BOOTCS_BASE_ADDRESS_REMAP      		0xd54
#define PCI_1CS_BOOTCS_BASE_ADDRESS_REMAP      		0xdd4
#define PCI_0P2P_MEM0_BASE_ADDRESS_REMAP_LOW            0xd5c
#define PCI_1P2P_MEM0_BASE_ADDRESS_REMAP_LOW            0xddc
#define PCI_0P2P_MEM0_BASE_ADDRESS_REMAP_HIGH           0xd60
#define PCI_1P2P_MEM0_BASE_ADDRESS_REMAP_HIGH           0xde0
#define PCI_0P2P_MEM1_BASE_ADDRESS_REMAP_LOW            0xd64
#define PCI_1P2P_MEM1_BASE_ADDRESS_REMAP_LOW            0xde4
#define PCI_0P2P_MEM1_BASE_ADDRESS_REMAP_HIGH           0xd68
#define PCI_1P2P_MEM1_BASE_ADDRESS_REMAP_HIGH           0xde8
#define PCI_0P2P_I_O_BASE_ADDRESS_REMAP                 0xd6c
#define PCI_1P2P_I_O_BASE_ADDRESS_REMAP                 0xdec
#define PCI_0INTERNAL_SRAM_BASE_ADDRESS_REMAP		    0xf00
#define PCI_1INTERNAL_SRAM_BASE_ADDRESS_REMAP		    0xf80
#define PCI_0EXPANSION_ROM_BASE_ADDRESS_REMAP               0xf38
#define PCI_1EXPANSION_ROM_BASE_ADDRESS_REMAP               0xfb8
#define PCI_0ADDRESS_DECODE_CONTROL                         0xd3c
#define PCI_1ADDRESS_DECODE_CONTROL                         0xdbc

/****************************************/
/* PCI Control                          */
/****************************************/

#define PCI0_DLL_STATUS_CONTROL					0x1d20
#define PCI1_DLL_STATUS_CONTROL					0x1da0
#define PCI0_PADS_CALIBRATION					0x1d1c
#define PCI1_PADS_CALIBRATION					0x1d9c

#define PCI_0COMMAND					    0xc00
#define PCI_1COMMAND					    0xc80
#define PCI_0MODE                                           0xd00
#define PCI_1MODE                                           0xd80
#define PCI_0TIMEOUT_RETRY				    0xc04
#define PCI_1TIMEOUT_RETRY				    0xc84
#define PCI_0READ_BUFFER_DISCARD_TIMER                      0xd04
#define PCI_1READ_BUFFER_DISCARD_TIMER                      0xd84
#define MSI_0TRIGGER_TIMER                                  0xc38
#define MSI_1TRIGGER_TIMER                                  0xcb8
#define PCI_0ARBITER_CONTROL                                0x1d00
#define PCI_1ARBITER_CONTROL                                0x1d80

/* changed until here */
#define PCI_0CROSS_BAR_CONTROL_LOW                           0x1d08
#define PCI_0CROSS_BAR_CONTROL_HIGH                          0x1d0c
#define PCI_0CROSS_BAR_TIMEOUT                               0x1d04
#define PCI_0READ_RESPONSE_CROSS_BAR_CONTROL_LOW             0x1d18
#define PCI_0SYNC_BARRIER_VIRTUAL_REGISTER                   0x1d10
#define PCI_0P2P_CONFIGURATION                               0x1d14
#define PCI_0ACCESS_CONTROL_BASE_0_LOW                       0x1e00
#define PCI_0ACCESS_CONTROL_BASE_0_HIGH                      0x1e04
#define PCI_0ACCESS_CONTROL_TOP_0                            0x1e08
#define PCI_0ACCESS_CONTROL_BASE_1_LOW                       0c1e10
#define PCI_0ACCESS_CONTROL_BASE_1_HIGH                      0x1e14
#define PCI_0ACCESS_CONTROL_TOP_1                            0x1e18
#define PCI_0ACCESS_CONTROL_BASE_2_LOW                       0c1e20
#define PCI_0ACCESS_CONTROL_BASE_2_HIGH                      0x1e24
#define PCI_0ACCESS_CONTROL_TOP_2                            0x1e28
#define PCI_0ACCESS_CONTROL_BASE_3_LOW                       0c1e30
#define PCI_0ACCESS_CONTROL_BASE_3_HIGH                      0x1e34
#define PCI_0ACCESS_CONTROL_TOP_3                            0x1e38
#define PCI_0ACCESS_CONTROL_BASE_4_LOW                       0c1e40
#define PCI_0ACCESS_CONTROL_BASE_4_HIGH                      0x1e44
#define PCI_0ACCESS_CONTROL_TOP_4                            0x1e48
#define PCI_0ACCESS_CONTROL_BASE_5_LOW                       0c1e50
#define PCI_0ACCESS_CONTROL_BASE_5_HIGH                      0x1e54
#define PCI_0ACCESS_CONTROL_TOP_5                            0x1e58
#define PCI_0ACCESS_CONTROL_BASE_6_LOW                       0c1e60
#define PCI_0ACCESS_CONTROL_BASE_6_HIGH                      0x1e64
#define PCI_0ACCESS_CONTROL_TOP_6                            0x1e68
#define PCI_0ACCESS_CONTROL_BASE_7_LOW                       0c1e70
#define PCI_0ACCESS_CONTROL_BASE_7_HIGH                      0x1e74
#define PCI_0ACCESS_CONTROL_TOP_7                            0x1e78
#define PCI_1CROSS_BAR_CONTROL_LOW                           0x1d88
#define PCI_1CROSS_BAR_CONTROL_HIGH                          0x1d8c
#define PCI_1CROSS_BAR_TIMEOUT                               0x1d84
#define PCI_1READ_RESPONSE_CROSS_BAR_CONTROL_LOW             0x1d98
#define PCI_1SYNC_BARRIER_VIRTUAL_REGISTER                   0x1d90
#define PCI_1P2P_CONFIGURATION                               0x1d94
#define PCI_1ACCESS_CONTROL_BASE_0_LOW                       0x1e80
#define PCI_1ACCESS_CONTROL_BASE_0_HIGH                      0x1e84
#define PCI_1ACCESS_CONTROL_TOP_0                            0x1e88
#define PCI_1ACCESS_CONTROL_BASE_1_LOW                       0c1e90
#define PCI_1ACCESS_CONTROL_BASE_1_HIGH                      0x1e94
#define PCI_1ACCESS_CONTROL_TOP_1                            0x1e98
#define PCI_1ACCESS_CONTROL_BASE_2_LOW                       0c1ea0
#define PCI_1ACCESS_CONTROL_BASE_2_HIGH                      0x1ea4
#define PCI_1ACCESS_CONTROL_TOP_2                            0x1ea8
#define PCI_1ACCESS_CONTROL_BASE_3_LOW                       0c1eb0
#define PCI_1ACCESS_CONTROL_BASE_3_HIGH                      0x1eb4
#define PCI_1ACCESS_CONTROL_TOP_3                            0x1eb8
#define PCI_1ACCESS_CONTROL_BASE_4_LOW                       0c1ec0
#define PCI_1ACCESS_CONTROL_BASE_4_HIGH                      0x1ec4
#define PCI_1ACCESS_CONTROL_TOP_4                            0x1ec8
#define PCI_1ACCESS_CONTROL_BASE_5_LOW                       0c1ed0
#define PCI_1ACCESS_CONTROL_BASE_5_HIGH                      0x1ed4
#define PCI_1ACCESS_CONTROL_TOP_5                            0x1ed8
#define PCI_1ACCESS_CONTROL_BASE_6_LOW                       0c1ee0
#define PCI_1ACCESS_CONTROL_BASE_6_HIGH                      0x1ee4
#define PCI_1ACCESS_CONTROL_TOP_6                            0x1ee8
#define PCI_1ACCESS_CONTROL_BASE_7_LOW                       0c1ef0
#define PCI_1ACCESS_CONTROL_BASE_7_HIGH                      0x1ef4
#define PCI_1ACCESS_CONTROL_TOP_7                            0x1ef8

/****************************************/
/* PCI Configuration Address            */
/****************************************/

/* MDD This section done */

#define PCI_0CONFIGURATION_ADDRESS 				0xcf8
#define PCI_0CONFIGURATION_DATA_VIRTUAL_REGISTER           	0xcfc
#define PCI_1CONFIGURATION_ADDRESS 				0xc78
#define PCI_1CONFIGURATION_DATA_VIRTUAL_REGISTER           	0xc7c
#define PCI_0INTERRUPT_ACKNOWLEDGE_VIRTUAL_REGISTER		0xc34
#define PCI_1INTERRUPT_ACKNOWLEDGE_VIRTUAL_REGISTER		0xcb4

/****************************************/
/* PCI Error Report                     */
/****************************************/

/* MDD This section done */

#define PCI_0SERR_MASK						0xc28
#define PCI_0ERROR_ADDRESS_LOW					0x1d40
#define PCI_0ERROR_ADDRESS_HIGH					0x1d44
#define PCI_0ERROR_ATTRIBUTE					0x1d48
#define PCI_0ERROR_COMMAND					0x1d50
#define PCI_0ERROR_CAUSE					0x1d58
#define PCI_0ERROR_MASK						0x1d5c

#define PCI_1SERR_MASK						0xca8
#define PCI_1ERROR_ADDRESS_LOW					0x1dc0
#define PCI_1ERROR_ADDRESS_HIGH					0x1dc4
#define PCI_1ERROR_ATTRIBUTE					0x1dc8
#define PCI_1ERROR_COMMAND					0x1dd0
#define PCI_1ERROR_CAUSE					0x1dd8
#define PCI_1ERROR_MASK						0x1ddc

/****************************************/
/* PCI Configuration Function 0         */
/****************************************/

/* MDD This section done */

#define PCI_DEVICE_AND_VENDOR_ID 				0x000
#define PCI_STATUS_AND_COMMAND					0x004
#define PCI0_COMMAND_STATUS_REG        				0x004
#define PCI1_COMMAND_STATUS_REG        				0x084
#define PCI_CLASS_CODE_AND_REVISION_ID			        0x008
#define PCI_BIST_HEADER_TYPE_LATENCY_TIMER_CACHE_LINE 		0x00C
#define PCI_SCS_0_BASE_ADDRESS_LOW    				0x010
#define PCI_SCS_0_BASE_ADDRESS_HIGH			    	0x014
#define PCI_SCS_1_BASE_ADDRESS_LOW				0x018
#define PCI_SCS_1_BASE_ADDRESS_HIGH   				0x01C
#define PCI_INTERNAL_REGISTERS_MEMORY_MAPPED_BASE_ADDRESS	0x020
#define PCI_INTERNAL_REGISTERS_I_OMAPPED_BASE_ADDRESS		0x024
#define PCI_SUBSYSTEM_ID_AND_SUBSYSTEM_VENDOR_ID		0x02C
#define PCI_EXPANSION_ROM_BASE_ADDRESS_REGISTER			0x030
#define PCI_CAPABILTY_LIST_POINTER                          	0x034
#define PCI_INTERRUPT_PIN_AND_LINE 			    	0x03C
#define PCI_POWER_MANAGEMENT_CAPABILITY                     0x040
#define PCI_POWER_MANAGEMENT_STATUS_AND_CONTROL             0x044
#define PCI_VPD_ADDRESS                                     0x048
#define PCI_VPD_DATA                                        0X04c
#define PCI_MSI_MESSAGE_CONTROL                             0x050
#define PCI_MSI_MESSAGE_ADDRESS                             0x054
#define PCI_MSI_MESSAGE_UPPER_ADDRESS                       0x058
#define PCI_MSI_MESSAGE_DATA                                0x05c
#define PCI_X_COMMAND					    0x060
#define PCI_X_STATUS					    0x064
#define PCI_COMPACT_PCI_HOT_SWAP_CAPABILITY                 0x068

/****************************************/
/* PCI Configuration Function 1         */
/****************************************/

/* MDD This section done */

#define PCI_SCS_2_BASE_ADDRESS_LOW    				0x110
#define PCI_SCS_2_BASE_ADDRESS_HIGH    				0x114
#define PCI_SCS_3_BASE_ADDRESS_LOW    				0x118
#define PCI_SCS_3_BASE_ADDRESS_HIGH    				0x11c
#define PCI_INTEGRATED_SRAM_BASE_ADDRESS_LOW   			0x120
#define PCI_INTEGRATED_SRAM_BASE_ADDRESS_HIGH			0x124

/****************************************/
/* PCI Configuration Function 2         */
/****************************************/

/* MDD This section done */

#define PCI_CS_0_BASE_ADDRESS_LOW	    			0x210
#define PCI_CS_0_BASE_ADDRESS_HIGH 				0x214
#define PCI_CS_1_BASE_ADDRESS_LOW	    			0x218
#define PCI_CS_1_BASE_ADDRESS_HIGH 				0x21c
#define PCI_CS_2_BASE_ADDRESS_LOW	    			0x220
#define PCI_CS_2_BASE_ADDRESS_HIGH 				0x224

/****************************************/
/* PCI Configuration Function 3         */
/****************************************/

/* MDD This section done */

#define PCI_CS_3_BASE_ADDRESS_LOW	    			0x310
#define PCI_CS_3_BASE_ADDRESS_HIGH 				0x314
#define PCI_BOOTCS_BASE_ADDRESS_LOW	    			0x318
#define PCI_BOOTCS_BASE_ADDRESS_HIGH 				0x31c

/****************************************/
/* PCI Configuration Function 4         */
/****************************************/

/* MDD This section done */

#define PCI_P2P_MEM0_BASE_ADDRESS_LOW 				0x410
#define PCI_P2P_MEM0_BASE_ADDRESS_HIGH			    	0x414
#define PCI_P2P_MEM1_BASE_ADDRESS_LOW 				0x418
#define PCI_P2P_MEM1_BASE_ADDRESS_HIGH			    	0x41c
#define PCI_P2P_I_O_BASE_ADDRESS 				0x420
#define PCI_INTERNAL_REGS_ADDRESS_BASE			    	0x424

/****************************************/
/* Interrupts	  			*/
/****************************************/
			
#define LOW_INTERRUPT_CAUSE_REGISTER   				0xc18
#define HIGH_INTERRUPT_CAUSE_REGISTER				0xc68
#define CPU_INTERRUPT_MASK_REGISTER_LOW				0xc1c
#define CPU_INTERRUPT_MASK_REGISTER_HIGH			0xc6c
#define CPU_SELECT_CAUSE_REGISTER				0xc70
#define PCI_0INTERRUPT_CAUSE_MASK_REGISTER_LOW			0xc24
#define PCI_0INTERRUPT_CAUSE_MASK_REGISTER_HIGH			0xc64
#define PCI_0SELECT_CAUSE                                   	0xc74
#define PCI_1INTERRUPT_CAUSE_MASK_REGISTER_LOW			0xca4
#define PCI_1INTERRUPT_CAUSE_MASK_REGISTER_HIGH			0xce4
#define PCI_1SELECT_CAUSE                                   	0xcf4
#define CPU_INT_0_MASK                                      	0xe60
#define CPU_INT_1_MASK                                      	0xe64
#define CPU_INT_2_MASK                                      	0xe68
#define CPU_INT_3_MASK                                      	0xe6c

/****************************************/
/* I20 Support registers		*/
/****************************************/

#define INBOUND_MESSAGE_REGISTER0_PCI_SIDE			0x010
#define INBOUND_MESSAGE_REGISTER1_PCI_SIDE  			0x014
#define OUTBOUND_MESSAGE_REGISTER0_PCI_SIDE 			0x018
#define OUTBOUND_MESSAGE_REGISTER1_PCI_SIDE  			0x01C
#define INBOUND_DOORBELL_REGISTER_PCI_SIDE  			0x020
#define INBOUND_INTERRUPT_CAUSE_REGISTER_PCI_SIDE  		0x024
#define INBOUND_INTERRUPT_MASK_REGISTER_PCI_SIDE		0x028
#define OUTBOUND_DOORBELL_REGISTER_PCI_SIDE 			0x02C
#define OUTBOUND_INTERRUPT_CAUSE_REGISTER_PCI_SIDE   		0x030
#define OUTBOUND_INTERRUPT_MASK_REGISTER_PCI_SIDE   		0x034
#define INBOUND_QUEUE_PORT_VIRTUAL_REGISTER_PCI_SIDE  		0x040
#define OUTBOUND_QUEUE_PORT_VIRTUAL_REGISTER_PCI_SIDE   	0x044
#define QUEUE_CONTROL_REGISTER_PCI_SIDE 			0x050
#define QUEUE_BASE_ADDRESS_REGISTER_PCI_SIDE 			0x054
#define INBOUND_FREE_HEAD_POINTER_REGISTER_PCI_SIDE		0x060
#define INBOUND_FREE_TAIL_POINTER_REGISTER_PCI_SIDE  		0x064
#define INBOUND_POST_HEAD_POINTER_REGISTER_PCI_SIDE 		0x068
#define INBOUND_POST_TAIL_POINTER_REGISTER_PCI_SIDE 		0x06C
#define OUTBOUND_FREE_HEAD_POINTER_REGISTER_PCI_SIDE		0x070
#define OUTBOUND_FREE_TAIL_POINTER_REGISTER_PCI_SIDE		0x074
#define OUTBOUND_POST_HEAD_POINTER_REGISTER_PCI_SIDE		0x078
#define OUTBOUND_POST_TAIL_POINTER_REGISTER_PCI_SIDE		0x07C

#define INBOUND_MESSAGE_REGISTER0_CPU_SIDE			0X1C10
#define INBOUND_MESSAGE_REGISTER1_CPU_SIDE  			0X1C14
#define OUTBOUND_MESSAGE_REGISTER0_CPU_SIDE 			0X1C18
#define OUTBOUND_MESSAGE_REGISTER1_CPU_SIDE  			0X1C1C
#define INBOUND_DOORBELL_REGISTER_CPU_SIDE  			0X1C20
#define INBOUND_INTERRUPT_CAUSE_REGISTER_CPU_SIDE  		0X1C24
#define INBOUND_INTERRUPT_MASK_REGISTER_CPU_SIDE		0X1C28
#define OUTBOUND_DOORBELL_REGISTER_CPU_SIDE 			0X1C2C
#define OUTBOUND_INTERRUPT_CAUSE_REGISTER_CPU_SIDE   		0X1C30
#define OUTBOUND_INTERRUPT_MASK_REGISTER_CPU_SIDE   		0X1C34
#define INBOUND_QUEUE_PORT_VIRTUAL_REGISTER_CPU_SIDE  		0X1C40
#define OUTBOUND_QUEUE_PORT_VIRTUAL_REGISTER_CPU_SIDE   	0X1C44
#define QUEUE_CONTROL_REGISTER_CPU_SIDE 			0X1C50
#define QUEUE_BASE_ADDRESS_REGISTER_CPU_SIDE 			0X1C54
#define INBOUND_FREE_HEAD_POINTER_REGISTER_CPU_SIDE		0X1C60
#define INBOUND_FREE_TAIL_POINTER_REGISTER_CPU_SIDE  		0X1C64
#define INBOUND_POST_HEAD_POINTER_REGISTER_CPU_SIDE 		0X1C68
#define INBOUND_POST_TAIL_POINTER_REGISTER_CPU_SIDE 		0X1C6C
#define OUTBOUND_FREE_HEAD_POINTER_REGISTER_CPU_SIDE		0X1C70
#define OUTBOUND_FREE_TAIL_POINTER_REGISTER_CPU_SIDE		0X1C74
#define OUTBOUND_POST_HEAD_POINTER_REGISTER_CPU_SIDE		0X1C78
#define OUTBOUND_POST_TAIL_POINTER_REGISTER_CPU_SIDE		0X1C7C

/****************************************/
/*        Ethernet Unit Registers  		*/
/****************************************/

#define ETH_PHY_ADDR_REG                                     0x2000
#define ETH_SMI_REG                                          0x2004
#define ETH_UNIT_DEFAULT_ADDR_REG                            0x2008
#define ETH_UNIT_DEFAULTID_REG                               0x200c
#define ETH_UNIT_INTERRUPT_CAUSE_REG                         0x2080
#define ETH_UNIT_INTERRUPT_MASK_REG                          0x2084
#define ETH_UNIT_INTERNAL_USE_REG                            0x24fc
#define ETH_UNIT_ERROR_ADDR_REG                              0x2094
#define ETH_BAR_0                                            0x2200
#define ETH_BAR_1                                            0x2208
#define ETH_BAR_2                                            0x2210
#define ETH_BAR_3                                            0x2218
#define ETH_BAR_4                                            0x2220
#define ETH_BAR_5                                            0x2228
#define ETH_SIZE_REG_0                                       0x2204
#define ETH_SIZE_REG_1                                       0x220c
#define ETH_SIZE_REG_2                                       0x2214
#define ETH_SIZE_REG_3                                       0x221c
#define ETH_SIZE_REG_4                                       0x2224
#define ETH_SIZE_REG_5                                       0x222c
#define ETH_HEADERS_RETARGET_BASE_REG                        0x2230
#define ETH_HEADERS_RETARGET_CONTROL_REG                     0x2234
#define ETH_HIGH_ADDR_REMAP_REG_0                            0x2280
#define ETH_HIGH_ADDR_REMAP_REG_1                            0x2284
#define ETH_HIGH_ADDR_REMAP_REG_2                            0x2288
#define ETH_HIGH_ADDR_REMAP_REG_3                            0x228c
#define ETH_BASE_ADDR_ENABLE_REG                             0x2290
#define ETH_ACCESS_PROTECTION_REG(port)                    (0x2294 + (port<<2))
#define ETH_MIB_COUNTERS_BASE(port)                        (0x3000 + (port<<7))
#define ETH_PORT_CONFIG_REG(port)                          (0x2400 + (port<<10))
#define ETH_PORT_CONFIG_EXTEND_REG(port)                   (0x2404 + (port<<10))
#define ETH_MII_SERIAL_PARAMETRS_REG(port)                 (0x2408 + (port<<10))
#define ETH_GMII_SERIAL_PARAMETRS_REG(port)                (0x240c + (port<<10))
#define ETH_VLAN_ETHERTYPE_REG(port)                       (0x2410 + (port<<10))
#define ETH_MAC_ADDR_LOW(port)                             (0x2414 + (port<<10))
#define ETH_MAC_ADDR_HIGH(port)                            (0x2418 + (port<<10))
#define ETH_SDMA_CONFIG_REG(port)                          (0x241c + (port<<10))
#define ETH_DSCP_0(port)                                   (0x2420 + (port<<10))
#define ETH_DSCP_1(port)                                   (0x2424 + (port<<10))
#define ETH_DSCP_2(port)                                   (0x2428 + (port<<10))
#define ETH_DSCP_3(port)                                   (0x242c + (port<<10))
#define ETH_DSCP_4(port)                                   (0x2430 + (port<<10))
#define ETH_DSCP_5(port)                                   (0x2434 + (port<<10))
#define ETH_DSCP_6(port)                                   (0x2438 + (port<<10))
#define ETH_PORT_SERIAL_CONTROL_REG(port)                  (0x243c + (port<<10))
#define ETH_VLAN_PRIORITY_TAG_TO_PRIORITY(port)            (0x2440 + (port<<10))
#define ETH_PORT_STATUS_REG(port)                          (0x2444 + (port<<10))
#define ETH_TRANSMIT_QUEUE_COMMAND_REG(port)               (0x2448 + (port<<10))
#define ETH_TX_QUEUE_FIXED_PRIORITY(port)                  (0x244c + (port<<10))
#define ETH_PORT_TX_TOKEN_BUCKET_RATE_CONFIG(port)         (0x2450 + (port<<10))
#define ETH_MAXIMUM_TRANSMIT_UNIT(port)                    (0x2458 + (port<<10))
#define ETH_PORT_MAXIMUM_TOKEN_BUCKET_SIZE(port)           (0x245c + (port<<10))
#define ETH_INTERRUPT_CAUSE_REG(port)                      (0x2460 + (port<<10))
#define ETH_INTERRUPT_CAUSE_EXTEND_REG(port)               (0x2464 + (port<<10))
#define ETH_INTERRUPT_MASK_REG(port)                       (0x2468 + (port<<10))
#define ETH_INTERRUPT_EXTEND_MASK_REG(port)                (0x246c + (port<<10))
#define ETH_RX_FIFO_URGENT_THRESHOLD_REG(port)             (0x2470 + (port<<10))
#define ETH_TX_FIFO_URGENT_THRESHOLD_REG(port)             (0x2474 + (port<<10))
#define ETH_RX_MINIMAL_FRAME_SIZE_REG(port)                (0x247c + (port<<10))
#define ETH_RX_DISCARDED_FRAMES_COUNTER(port)              (0x2484 + (port<<10))
#define ETH_PORT_DEBUG_0_REG(port)                         (0x248c + (port<<10))
#define ETH_PORT_DEBUG_1_REG(port)                         (0x2490 + (port<<10))
#define ETH_PORT_INTERNAL_ADDR_ERROR_REG(port)             (0x2494 + (port<<10))
#define ETH_INTERNAL_USE_REG(port)                         (0x24fc + (port<<10))
#define ETH_RECEIVE_QUEUE_COMMAND_REG(port)                (0x2680 + (port<<10))
#define ETH_CURRENT_SERVED_TX_DESC_PTR(port)               (0x2684 + (port<<10))
#define ETH_RX_CURRENT_QUEUE_DESC_PTR_0(port)              (0x260c + (port<<10))
#define ETH_RX_CURRENT_QUEUE_DESC_PTR_1(port)              (0x261c + (port<<10))
#define ETH_RX_CURRENT_QUEUE_DESC_PTR_2(port)              (0x262c + (port<<10))
#define ETH_RX_CURRENT_QUEUE_DESC_PTR_3(port)              (0x263c + (port<<10))
#define ETH_RX_CURRENT_QUEUE_DESC_PTR_4(port)              (0x264c + (port<<10))
#define ETH_RX_CURRENT_QUEUE_DESC_PTR_5(port)              (0x265c + (port<<10))
#define ETH_RX_CURRENT_QUEUE_DESC_PTR_6(port)              (0x266c + (port<<10))
#define ETH_RX_CURRENT_QUEUE_DESC_PTR_7(port)              (0x267c + (port<<10))
#define ETH_TX_CURRENT_QUEUE_DESC_PTR_0(port)              (0x26c0 + (port<<10))
#define ETH_TX_CURRENT_QUEUE_DESC_PTR_1(port)              (0x26c4 + (port<<10))
#define ETH_TX_CURRENT_QUEUE_DESC_PTR_2(port)              (0x26c8 + (port<<10))
#define ETH_TX_CURRENT_QUEUE_DESC_PTR_3(port)              (0x26cc + (port<<10))
#define ETH_TX_CURRENT_QUEUE_DESC_PTR_4(port)              (0x26d0 + (port<<10))
#define ETH_TX_CURRENT_QUEUE_DESC_PTR_5(port)              (0x26d4 + (port<<10))
#define ETH_TX_CURRENT_QUEUE_DESC_PTR_6(port)              (0x26d8 + (port<<10))
#define ETH_TX_CURRENT_QUEUE_DESC_PTR_7(port)              (0x26dc + (port<<10))
#define ETH_TX_QUEUE_0_TOKEN_BUCKET_COUNT(port)            (0x2700 + (port<<10))
#define ETH_TX_QUEUE_1_TOKEN_BUCKET_COUNT(port)            (0x2710 + (port<<10))
#define ETH_TX_QUEUE_2_TOKEN_BUCKET_COUNT(port)            (0x2720 + (port<<10))
#define ETH_TX_QUEUE_3_TOKEN_BUCKET_COUNT(port)            (0x2730 + (port<<10))
#define ETH_TX_QUEUE_4_TOKEN_BUCKET_COUNT(port)            (0x2740 + (port<<10))
#define ETH_TX_QUEUE_5_TOKEN_BUCKET_COUNT(port)            (0x2750 + (port<<10))
#define ETH_TX_QUEUE_6_TOKEN_BUCKET_COUNT(port)            (0x2760 + (port<<10))
#define ETH_TX_QUEUE_7_TOKEN_BUCKET_COUNT(port)            (0x2770 + (port<<10))
#define ETH_TX_QUEUE_0_TOKEN_BUCKET_CONFIG(port)           (0x2704 + (port<<10))
#define ETH_TX_QUEUE_1_TOKEN_BUCKET_CONFIG(port)           (0x2714 + (port<<10))
#define ETH_TX_QUEUE_2_TOKEN_BUCKET_CONFIG(port)           (0x2724 + (port<<10))
#define ETH_TX_QUEUE_3_TOKEN_BUCKET_CONFIG(port)           (0x2734 + (port<<10))
#define ETH_TX_QUEUE_4_TOKEN_BUCKET_CONFIG(port)           (0x2744 + (port<<10))
#define ETH_TX_QUEUE_5_TOKEN_BUCKET_CONFIG(port)           (0x2754 + (port<<10))
#define ETH_TX_QUEUE_6_TOKEN_BUCKET_CONFIG(port)           (0x2764 + (port<<10))
#define ETH_TX_QUEUE_7_TOKEN_BUCKET_CONFIG(port)           (0x2774 + (port<<10))
#define ETH_TX_QUEUE_0_ARBITER_CONFIG(port)                (0x2708 + (port<<10))
#define ETH_TX_QUEUE_1_ARBITER_CONFIG(port)                (0x2718 + (port<<10))
#define ETH_TX_QUEUE_2_ARBITER_CONFIG(port)                (0x2728 + (port<<10))
#define ETH_TX_QUEUE_3_ARBITER_CONFIG(port)                (0x2738 + (port<<10))
#define ETH_TX_QUEUE_4_ARBITER_CONFIG(port)                (0x2748 + (port<<10))
#define ETH_TX_QUEUE_5_ARBITER_CONFIG(port)                (0x2758 + (port<<10))
#define ETH_TX_QUEUE_6_ARBITER_CONFIG(port)                (0x2768 + (port<<10))
#define ETH_TX_QUEUE_7_ARBITER_CONFIG(port)                (0x2778 + (port<<10))
#define ETH_PORT_TX_TOKEN_BUCKET_COUNT(port)               (0x2780 + (port<<10))
#define ETH_DA_FILTER_SPECIAL_MULTICAST_TABLE_BASE(port)   (0x3400 + (port<<10))
#define ETH_DA_FILTER_OTHER_MULTICAST_TABLE_BASE(port)     (0x3500 + (port<<10))
#define ETH_DA_FILTER_UNICAST_TABLE_BASE(port)             (0x3600 + (port<<10))

/****************************************/
/* Communication Unit Registers         */
/****************************************/

#define ETHERNET_0_ADDRESS_CONTROL_LOW			    0xf200
#define ETHERNET_0_ADDRESS_CONTROL_HIGH                     0xf204
#define ETHERNET_0_RECEIVE_BUFFER_PCI_HIGH_ADDRESS          0xf208
#define ETHERNET_0_TRANSMIT_BUFFER_PCI_HIGH_ADDRESS         0xf20c
#define ETHERNET_0_RECEIVE_DESCRIPTOR_PCI_HIGH_ADDRESS      0xf210
#define ETHERNET_0_TRANSMIT_DESCRIPTOR_PCI_HIGH_ADDRESS     0xf214
#define ETHERNET_0_HASH_TABLE_PCI_HIGH_ADDRESS              0xf218
#define ETHERNET_1_ADDRESS_CONTROL_LOW                      0xf220
#define ETHERNET_1_ADDRESS_CONTROL_HIGH                     0xf224
#define ETHERNET_1_RECEIVE_BUFFER_PCI_HIGH_ADDRESS          0xf228
#define ETHERNET_1_TRANSMIT_BUFFER_PCI_HIGH_ADDRESS         0xf22c
#define ETHERNET_1_RECEIVE_DESCRIPTOR_PCI_HIGH_ADDRESS      0xf230
#define ETHERNET_1_TRANSMIT_DESCRIPTOR_PCI_HIGH_ADDRESS     0xf234
#define ETHERNET_1_HASH_TABLE_PCI_HIGH_ADDRESS              0xf238
#define ETHERNET_2_ADDRESS_CONTROL_LOW                      0xf240
#define ETHERNET_2_ADDRESS_CONTROL_HIGH                     0xf244
#define ETHERNET_2_RECEIVE_BUFFER_PCI_HIGH_ADDRESS          0xf248
#define ETHERNET_2_TRANSMIT_BUFFER_PCI_HIGH_ADDRESS         0xf24c
#define ETHERNET_2_RECEIVE_DESCRIPTOR_PCI_HIGH_ADDRESS      0xf250
#define ETHERNET_2_TRANSMIT_DESCRIPTOR_PCI_HIGH_ADDRESS     0xf254
#define ETHERNET_2_HASH_TABLE_PCI_HIGH_ADDRESS              0xf258
#define MPSC_0_ADDRESS_CONTROL_LOW                          0xf280
#define MPSC_0_ADDRESS_CONTROL_HIGH                         0xf284
#define MPSC_0_RECEIVE_BUFFER_PCI_HIGH_ADDRESS              0xf288
#define MPSC_0_TRANSMIT_BUFFER_PCI_HIGH_ADDRESS             0xf28c
#define MPSC_0_RECEIVE_DESCRIPTOR_PCI_HIGH_ADDRESS          0xf290
#define MPSC_0_TRANSMIT_DESCRIPTOR_PCI_HIGH_ADDRESS         0xf294
#define MPSC_1_ADDRESS_CONTROL_LOW                          0xf2a0
#define MPSC_1_ADDRESS_CONTROL_HIGH                         0xf2a4
#define MPSC_1_RECEIVE_BUFFER_PCI_HIGH_ADDRESS              0xf2a8
#define MPSC_1_TRANSMIT_BUFFER_PCI_HIGH_ADDRESS             0xf2ac
#define MPSC_1_RECEIVE_DESCRIPTOR_PCI_HIGH_ADDRESS          0xf2b0
#define MPSC_1_TRANSMIT_DESCRIPTOR_PCI_HIGH_ADDRESS         0xf2b4
#define MPSC_2_ADDRESS_CONTROL_LOW                          0xf2c0
#define MPSC_2_ADDRESS_CONTROL_HIGH                         0xf2c4
#define MPSC_2_RECEIVE_BUFFER_PCI_HIGH_ADDRESS              0xf2c8
#define MPSC_2_TRANSMIT_BUFFER_PCI_HIGH_ADDRESS             0xf2cc
#define MPSC_2_RECEIVE_DESCRIPTOR_PCI_HIGH_ADDRESS          0xf2d0
#define MPSC_2_TRANSMIT_DESCRIPTOR_PCI_HIGH_ADDRESS         0xf2d4
#define SERIAL_INIT_PCI_HIGH_ADDRESS                        0xf320
#define SERIAL_INIT_LAST_DATA                               0xf324
#define SERIAL_INIT_STATUS_AND_CONTROL                      0xf328
#define COMM_UNIT_ARBITER_CONTROL                           0xf300
#define COMM_UNIT_CROSS_BAR_TIMEOUT                         0xf304
#define COMM_UNIT_INTERRUPT_CAUSE                           0xf310
#define COMM_UNIT_INTERRUPT_MASK                            0xf314
#define COMM_UNIT_ERROR_ADDRESS                             0xf314
           
/****************************************/
/* Cunit Debug  (for internal use)     */
/****************************************/

#define CUNIT_ADDRESS                                       0xf340
#define CUNIT_COMMAND_AND_ID                                0xf344
#define CUNIT_WRITE_DATA_LOW                                0xf348
#define CUNIT_WRITE_DATA_HIGH                               0xf34c
#define CUNIT_WRITE_BYTE_ENABLE                             0xf358
#define CUNIT_READ_DATA_LOW                                 0xf350
#define CUNIT_READ_DATA_HIGH                                0xf354
#define CUNIT_READ_ID                                       0xf35c

/****************************************/
/* SDMA Registers                       */
/****************************************/

#define SDMA_GROUP_CONFIGURATION_REGISTER                   0xb1f0
#define CHANNEL0_CONFIGURATION_REGISTER                     0x4000
#define CHANNEL0_COMMAND_REGISTER                           0x4008
#define CHANNEL0_RX_CMD_STATUS                              0x4800
#define CHANNEL0_RX_PACKET_AND_BUFFER_SIZES                 0x4804
#define CHANNEL0_RX_BUFFER_POINTER                          0x4808
#define CHANNEL0_RX_NEXT_POINTER                            0x480c
#define CHANNEL0_CURRENT_RX_DESCRIPTOR_POINTER              0x4810
#define CHANNEL0_TX_CMD_STATUS                              0x4C00
#define CHANNEL0_TX_PACKET_SIZE                             0x4C04
#define CHANNEL0_TX_BUFFER_POINTER                          0x4C08
#define CHANNEL0_TX_NEXT_POINTER                            0x4C0c
#define CHANNEL0_CURRENT_TX_DESCRIPTOR_POINTER              0x4c10
#define CHANNEL0_FIRST_TX_DESCRIPTOR_POINTER                0x4c14
#define CHANNEL1_CONFIGURATION_REGISTER                     0x5000
#define CHANNEL1_COMMAND_REGISTER                           0x5008
#define CHANNEL1_RX_CMD_STATUS                              0x5800
#define CHANNEL1_RX_PACKET_AND_BUFFER_SIZES                 0x5804
#define CHANNEL1_RX_BUFFER_POINTER                          0x5808
#define CHANNEL1_RX_NEXT_POINTER                            0x580c
#define CHANNEL1_TX_CMD_STATUS                              0x5C00
#define CHANNEL1_TX_PACKET_SIZE                             0x5C04
#define CHANNEL1_TX_BUFFER_POINTER                          0x5C08
#define CHANNEL1_TX_NEXT_POINTER                            0x5C0c
#define CHANNEL1_CURRENT_RX_DESCRIPTOR_POINTER              0x5810
#define CHANNEL1_CURRENT_TX_DESCRIPTOR_POINTER              0x5c10
#define CHANNEL1_FIRST_TX_DESCRIPTOR_POINTER                0x5c14
#define CHANNEL2_CONFIGURATION_REGISTER                     0x6000
#define CHANNEL2_COMMAND_REGISTER                           0x6008
#define CHANNEL2_RX_CMD_STATUS                              0x6800
#define CHANNEL2_RX_PACKET_AND_BUFFER_SIZES                 0x6804
#define CHANNEL2_RX_BUFFER_POINTER                          0x6808
#define CHANNEL2_RX_NEXT_POINTER                            0x680c
#define CHANNEL2_CURRENT_RX_DESCRIPTOR_POINTER              0x6810
#define CHANNEL2_TX_CMD_STATUS                              0x6C00
#define CHANNEL2_TX_PACKET_SIZE                             0x6C04
#define CHANNEL2_TX_BUFFER_POINTER                          0x6C08
#define CHANNEL2_TX_NEXT_POINTER                            0x6C0c
#define CHANNEL2_CURRENT_RX_DESCRIPTOR_POINTER              0x6810
#define CHANNEL2_CURRENT_TX_DESCRIPTOR_POINTER              0x6c10
#define CHANNEL2_FIRST_TX_DESCRIPTOR_POINTER                0x6c14

/* SDMA Interrupt */

#define SDMA_CAUSE                                          0xb820
#define SDMA_MASK                                           0xb8a0


/****************************************/
/* Baude Rate Generators Registers      */
/****************************************/

/* BRG 0 */

#define BRG0_CFG_REG                         		    0xb200
#define BRG0_BAUD_TUNING_REG                                0xb204

/* BRG 1 */

#define BRG1_CFG_REG                                        0xb208
#define BRG1_BAUD_TUNING_REG                                0xb20c

/* BRG 2 */

#define BRG2_CFG_REGISTER                                   0xb210
#define BRG2_BAUD_TUNING_REG                                0xb214

/* BRG Interrupts */

#define BRG_CAUSE_REGISTER                                  0xb834
#define BRG_MASK_REGISTER                                   0xb8b4

/* MISC */

#define MAIN_ROUTING_REGISTER                               0xb400
#define RECEIVE_CLOCK_ROUTING_REGISTER                      0xb404
#define TRANSMIT_CLOCK_ROUTING_REGISTER                     0xb408
#define COMM_UNIT_ARBITER_CONFIGURATION_REGISTER            0xb40c
#define WATCHDOG_CONFIGURATION_REGISTER                     0xb410
#define WATCHDOG_VALUE_REGISTER                             0xb410


/****************************************/
/* Flex TDM Registers                   */
/****************************************/

/* FTDM Port */

#define FLEXTDM_TRANSMIT_READ_POINTER                       0xa800
#define FLEXTDM_RECEIVE_READ_POINTER                        0xa804
#define FLEXTDM_CONFIGURATION_REGISTER                      0xa808
#define FLEXTDM_AUX_CHANNELA_TX_REGISTER                    0xa80c
#define FLEXTDM_AUX_CHANNELA_RX_REGISTER                    0xa810
#define FLEXTDM_AUX_CHANNELB_TX_REGISTER                    0xa814
#define FLEXTDM_AUX_CHANNELB_RX_REGISTER                    0xa818

/* FTDM Interrupts */

#define FTDM_CAUSE_REGISTER                                 0xb830
#define FTDM_MASK_REGISTER                                  0xb8b0


/****************************************/
/* GPP Interface Registers              */
/****************************************/

/* MDD This section done */

#define GPP_IO_CONTROL                                      0xf100
#define GPP_LEVEL_CONTROL                                   0xf110
#define GPP_VALUE                                           0xf104
#define GPP_INTERRUPT_CAUSE                                 0xf108
#define GPP_INTERRUPT_MASK                                  0xf10c

#define MPP_CONTROL0                                        0xf000
#define MPP_CONTROL1                                        0xf004
#define MPP_CONTROL2                                        0xf008
#define MPP_CONTROL3                                        0xf00c

/****************************************/
/* I2C Registers                        */
/****************************************/

#define I2C_SLAVE_ADDRESS                                   0xc000
#define I2C_EXTENDED_SLAVE_ADDRESS                          0xc040
#define I2C_DATA                                            0xc004
#define I2C_CONTROL                                         0xc008
#define I2C_STATUS_BAUDE_RATE                               0xc00C
#define I2C_SOFT_RESET                                      0xc01c

/****************************************/ 
/* MPSC Registers                       */ 
/****************************************/ 

/* MPSC0  */

#define MPSC0_MAIN_CONFIGURATION_LOW                        0x8000
#define MPSC0_MAIN_CONFIGURATION_HIGH                       0x8004
#define MPSC0_PROTOCOL_CONFIGURATION                        0x8008
#define CHANNEL0_REGISTER1                                  0x800c
#define CHANNEL0_REGISTER2                                  0x8010 
#define CHANNEL0_REGISTER3                                  0x8014 
#define CHANNEL0_REGISTER4                                  0x8018 
#define CHANNEL0_REGISTER5                                  0x801c 
#define CHANNEL0_REGISTER6                                  0x8020 
#define CHANNEL0_REGISTER7                                  0x8024 
#define CHANNEL0_REGISTER8                                  0x8028 
#define CHANNEL0_REGISTER9                                  0x802c 
#define CHANNEL0_REGISTER10                                 0x8030 
#define CHANNEL0_REGISTER11                                 0x8034

/* MPSC1  */

#define MPSC1_MAIN_CONFIGURATION_LOW                        0x8840
#define MPSC1_MAIN_CONFIGURATION_HIGH                       0x8844
#define MPSC1_PROTOCOL_CONFIGURATION                        0x8848
#define CHANNEL1_REGISTER1                                  0x884c
#define CHANNEL1_REGISTER2                                  0x8850 
#define CHANNEL1_REGISTER3                                  0x8854 
#define CHANNEL1_REGISTER4                                  0x8858 
#define CHANNEL1_REGISTER5                                  0x885c 
#define CHANNEL1_REGISTER6                                  0x8860 
#define CHANNEL1_REGISTER7                                  0x8864 
#define CHANNEL1_REGISTER8                                  0x8868 
#define CHANNEL1_REGISTER9                                  0x886c 
#define CHANNEL1_REGISTER10                                 0x8870 
#define CHANNEL1_REGISTER11                                 0x8874
      
/* MPSC2  */

#define MPSC2_MAIN_CONFIGURATION_LOW                        0x9040
#define MPSC2_MAIN_CONFIGURATION_HIGH                       0x9044
#define MPSC2_PROTOCOL_CONFIGURATION                        0x9048
#define CHANNEL2_REGISTER1                                  0x904c
#define CHANNEL2_REGISTER2                                  0x9050 
#define CHANNEL2_REGISTER3                                  0x9054 
#define CHANNEL2_REGISTER4                                  0x9058 
#define CHANNEL2_REGISTER5                                  0x905c 
#define CHANNEL2_REGISTER6                                  0x9060 
#define CHANNEL2_REGISTER7                                  0x9064 
#define CHANNEL2_REGISTER8                                  0x9068 
#define CHANNEL2_REGISTER9                                  0x906c 
#define CHANNEL2_REGISTER10                                 0x9070 
#define CHANNEL2_REGISTER11                                 0x9074

/* MPSCs Interupts  */

#define MPSC0_CAUSE                                         0xb824
#define MPSC0_MASK                                          0xb8a4
#define MPSC1_CAUSE                                         0xb828
#define MPSC1_MASK                                          0xb8a8
#define MPSC2_CAUSE                                         0xb82c
#define MPSC2_MASK                                          0xb8ac

#endif /* _MV64340REG_H_ */
