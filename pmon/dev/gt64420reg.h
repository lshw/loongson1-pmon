/*	$Id: gt64420reg.h,v 1.1.1.1 2006/09/14 01:59:08 root Exp $ */

/*
 * Copyright (c) 2001-2002 Galileo Technology.
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
/* gt64420reg.h - GT64420 Internal registers definition file */

#ifndef _GT64420REG_H_
#define _GT64420REG_H_

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

#ifdef __MIPSEB__
#define GT_WRITE(ofs, data)	out32rb(GT_BASE_ADDR+(ofs), data)
#define GT_READ(ofs)		in32rb(GT_BASE_ADDR+(ofs))
#else
#define GT_WRITE(ofs, data)		out32(GT_BASE_ADDR+(ofs), data)
#define GT_WRITE_WORD(ofs, data)	out16(GT_BASE_ADDR+(ofs), data)
#define GT_WRITE_BYTE(ofs, data)	out8(GT_BASE_ADDR+(ofs), data)
#define GT_READ(ofs)			in32(GT_BASE_ADDR+(ofs))
#define GT_READ_WORD(ofs)		in16(GT_BASE_ADDR+(ofs))
#define GT_READ_BYTE(ofs)		in8(GT_BASE_ADDR+(ofs))
#endif

#ifdef CONFIG_PCI0_GAINT_MEM
#define GT_BASE_ADDR			PHYS_TO_IOSPACE(0x1e000000)
#endif

#if !defined(GT_BASE_ADDR)
#define GT_BASE_ADDR			PHYS_TO_IOSPACE(0x14000000)
#endif

#define GT_BASE_ADDR_DEFAULT		PHYS_TO_UNCACHED(0x14000000)


/************************************************/
#define GT_DEVPAR_TurnOff(x)            ((((x) & 7) << 0) & (((x) & 8) << 19))
#define GT_DEVPAR_AccToFirst(x)         ((((x) & 15) << 3) | (((x) & 16) << 19))
#define GT_DEVPAR_AccToNext(x)          ((((x) & 15) << 7) | (((x) & 16) << 20))
#define GT_DEVPAR_ALEtoWr(x)            ((((x) & 7) << 11) | (((x) & 8) << 22))
#define GT_DEVPAR_WrActive(x)           ((((x) & 7) << 14) | (((x) & 8) << 23))
#define GT_DEVPAR_WrHigh(x)             ((((x) & 7) << 17) | (((x) & 8) << 24))
#define GT_DEVPAR_DevWidth8             (0<<20)
#define GT_DEVPAR_DevWidth16            (1<<20)
#define GT_DEVPAR_DevWidth32            (2<<20)
#define GT_DEVPAR_DevWidth64            (3<<20)
#define	GT_DEVPAR_DevWidthMASK		(3<<20)
#define GT_DEVPAR_ReservedMASK          0xf0000000
#define GT_DEVPAR_Reserved              0x00000000 
/************************************************/
#define GT_PCI0_MAP10                   0x0010
#define GT_PCI0_MAP14                   0x0014
/************************************************/
#define GT_IPCI_CFGADDR_ConfigEn        (1<<31)
/************************************************/
#define GT_SMIR_REG			0x2010
#define CPU_CONF        		0x0000
#define SDRAM_CNFG      		0x1400
#define GT_DEV0_PAR     		0x045c
#define GT_DEV1_PAR     		0x0460
#define GT_DEV2_PAR     		0x0464
#define GT_DEV3_PAR     		0x0468
#define GT_BOOT_PAR     		0x046c
//#define PCI_COMMAND    			0x0c00
#define PCI_IO_BASE      		0x0048
#define PCI_IO_SIZE      		0x0050
#define PCI_MEM0_BASE    		0x0058
#define PCI_MEM0_SIZE    		0x0060
#define CPU_CONFIG      		0x0000
#define CS0_BASE 	     		0x0008
#define CS0_SIZE 	    		0x0010
#define CS2_BASE 	     		0x0018
#define CS2_SIZE 	    		0x0020
#define CS1_BASE        		0x0208
#define CS1_SIZE       		0x0210
#define CS3_BASE        		0x0218
#define CS3_SIZE       		0x0220
#define PCI_TIME_OUT  		0x0c04
#define PCI_BAR_EN    		0x0c3c
#define PCI_ARBITER   		0x1d00
#define MPP_CNTRL0      		0xf000
#define MPP_CNTRL1      		0xf004
#define MPP_CNTRL2      		0xf008
#define MPP_CNTRL3      		0xf00c
#define SER_PORTS_MUX   		0xf010
#define GPP_LEVEL_CNTRL 		0xf110


#define MINIMUM_MEM_BANK_SIZE        	0x10000
#define MINIMUM_DEVICE_WINDOW_SIZE   	0x10000
#define MINIMUM_PCI_WINDOW_SIZE      	0x10000
#define MINIMUM_ACCESS_WIN_SIZE      	0x10000

/****************************************/
/* Processor Address Space		*/
/****************************************/

/* Sdram's BAR'S */
#define CS_0_BASE_ADDRESS				0x008
#define CS_0_SIZE						0x010
#define CS_1_BASE_ADDRESS			    0x208
#define CS_1_SIZE						0x210
#define CS_2_BASE_ADDRESS			    0x018
#define CS_2_SIZE						0x020
#define CS_3_BASE_ADDRESS				0x218
#define CS_3_SIZE						0x220

/* Devices BAR'S */
#define DEVCS_0_BASE_ADDRESS				0x028
#define DEVCS_0_SIZE						0x030
#define DEVCS_1_BASE_ADDRESS		    	0x228
#define DEVCS_1_SIZE						0x230
#define DEVCS_2_BASE_ADDRESS		    	0x248
#define DEVCS_2_SIZE						0x250
#define DEVCS_3_BASE_ADDRESS				0x038
#define DEVCS_3_SIZE						0x040
#define BOOTCS_BASE_ADDRESS				0x238
#define BOOTCS_SIZE						0x240

#define PCI_I_O_BASE_ADDRESS			0x048
#define PCI_I_O_SIZE					0x050
#define PCI_MEMORY0_BASE_ADDRESS		0x058
#define PCI_MEMORY0_SIZE				0x060
#define PCI_MEMORY1_BASE_ADDRESS				0x080
#define PCI_MEMORY1_SIZE				0x088
#define PCI_MEMORY2_BASE_ADDRESS				0x258
#define PCI_MEMORY2_SIZE				0x260
#define PCI_MEMORY3_BASE_ADDRESS				0x280
#define PCI_MEMORY3_SIZE				0x288

#define INTERNAL_SPACE_DECODE				0x068


#define PCI_I_O_ADDRESS_REMAP				0x0f0
#define PCI_MEMORY0_ADDRESS_REMAP  			0x0f8
#define PCI_MEMORY0_HIGH_ADDRESS_REMAP			0x320
#define PCI_MEMORY1_ADDRESS_REMAP  			0x100
#define PCI_MEMORY1_HIGH_ADDRESS_REMAP			0x328
#define PCI_MEMORY2_ADDRESS_REMAP  			0x2f8
#define PCI_MEMORY2_HIGH_ADDRESS_REMAP			0x330
#define PCI_MEMORY3_ADDRESS_REMAP  			0x300
#define PCI_MEMORY3_HIGH_ADDRESS_REMAP			0x338

#define BASE_ADDRESS_ENABLE_REG 0x278


/****************************************/
/* CPU Sync Barrier             	*/
/****************************************/

#define CPU_SYNC_BARIER_TRIGER_REGISTER		0x0c0
#define CPU_SYNC_BARIER_VIRTUAL_REGISTER	0x0c8


/****************************************/
/* CPU Access Protect             	*/
/****************************************/

#define CPU_PROTECT_BASE_ADDRESS_0                     	0x180
#define CPU_PROTECT_SIZE_0                      		0x188
#define CPU_PROTECT_BASE_ADDRESS_1                      0x190
#define CPU_PROTECT_SIZE_1                      		0x198
#define CPU_PROTECT_BASE_ADDRESS_2                      0x1a0
#define CPU_PROTECT_SIZE_2                      		0x1a8
#define CPU_PROTECT_BASE_ADDRESS_3                      0x1b0
#define CPU_PROTECT_SIZE_3                      		0x1b8

                                                                  

/****************************************/
/*          CPU Error Report       	*/
/****************************************/

#define CPU_BUS_ERROR_LOW_ADDRESS 			0x070
#define CPU_BUS_ERROR_HIGH_ADDRESS 			0x078
#define CPU_BUS_ERROR_LOW_DATA                          0x128
#define CPU_BUS_ERROR_HIGH_DATA                         0x130
#define CPU_BUS_ERROR_PARITY                        0x138
#define CPU_BUS_ERROR_CAUSE                       	0x140
#define CPU_BUS_ERROR_MASK                              0x148


/****************************************/
/* SDRAM and Device Address Space	*/
/****************************************/
	

/****************************************/
/* SDRAM Configuration			*/
/****************************************/

#define SDRAM_CONFIGURATION	 				0x1400
#define SDRAM_OPERATION_MODE				0x1418
#define SDRAM_ADDRESS_DECODE				0x1410
#define SDRAM_CROSS_BAR_CONTROL_LOW         0x1430
#define SDRAM_CROSS_BAR_CONTROL_HIGH        0x1434
#define SDRAM_CROSS_BAR_TIMEOUT             0x1438
#define	SDRAM_TIMING_LOW			0x1408
#define	SDRAM_TIMING_HIGH			0x140C

/****************************************/
/* SDRAM Error Report 			*/
/****************************************/
			
#define SDRAM_ERROR_DATA_LOW                            0x1444
#define SDRAM_ERROR_DATA_HIGH                           0x1440
#define SDRAM_AND_DEVICE_ERROR_ADDRESS                  0x1450
#define SDRAM_RECEIVED_ECC                              0x1448
#define SDRAM_CALCULATED_ECC                            0x144c
#define SDRAM_ECC_CONTROL                               0x1454


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
#define CHANNEL4_DMA_BYTE_COUNT				0x900
#define CHANNEL5_DMA_BYTE_COUNT	 			0x904
#define CHANNEL6_DMA_BYTE_COUNT	 			0x908
#define CHANNEL7_DMA_BYTE_COUNT	 			0x90C
#define CHANNEL0_DMA_SOURCE_ADDRESS			0x810
#define CHANNEL1_DMA_SOURCE_ADDRESS			0x814
#define CHANNEL2_DMA_SOURCE_ADDRESS			0x818
#define CHANNEL3_DMA_SOURCE_ADDRESS			0x81C
#define CHANNEL4_DMA_SOURCE_ADDRESS			0x910
#define CHANNEL5_DMA_SOURCE_ADDRESS			0x914
#define CHANNEL6_DMA_SOURCE_ADDRESS			0x918
#define CHANNEL7_DMA_SOURCE_ADDRESS			0x91C
#define CHANNEL0_DMA_DESTINATION_ADDRESS		0x820
#define CHANNEL1_DMA_DESTINATION_ADDRESS		0x824
#define CHANNEL2_DMA_DESTINATION_ADDRESS		0x828
#define CHANNEL3_DMA_DESTINATION_ADDRESS		0x82C
#define CHANNEL4_DMA_DESTINATION_ADDRESS		0x920
#define CHANNEL5_DMA_DESTINATION_ADDRESS		0x924
#define CHANNEL6_DMA_DESTINATION_ADDRESS		0x928
#define CHANNEL7_DMA_DESTINATION_ADDRESS		0x92C
#define CHANNEL0NEXT_RECORD_POINTER			0x830
#define CHANNEL1NEXT_RECORD_POINTER			0x834
#define CHANNEL2NEXT_RECORD_POINTER			0x838
#define CHANNEL3NEXT_RECORD_POINTER			0x83C
#define CHANNEL4NEXT_RECORD_POINTER			0x930
#define CHANNEL5NEXT_RECORD_POINTER			0x934
#define CHANNEL6NEXT_RECORD_POINTER			0x938
#define CHANNEL7NEXT_RECORD_POINTER			0x93C
#define CHANNEL0CURRENT_DESCRIPTOR_POINTER		0x870
#define CHANNEL1CURRENT_DESCRIPTOR_POINTER		0x874
#define CHANNEL2CURRENT_DESCRIPTOR_POINTER		0x878
#define CHANNEL3CURRENT_DESCRIPTOR_POINTER		0x87C
#define CHANNEL4CURRENT_DESCRIPTOR_POINTER		0x970
#define CHANNEL5CURRENT_DESCRIPTOR_POINTER		0x974
#define CHANNEL6CURRENT_DESCRIPTOR_POINTER		0x978
#define CHANNEL7CURRENT_DESCRIPTOR_POINTER		0x97C
#define CHANNEL0_DMA_SOURCE_HIGH_PCI_ADDRESS            0x890
#define CHANNEL1_DMA_SOURCE_HIGH_PCI_ADDRESS            0x894
#define CHANNEL2_DMA_SOURCE_HIGH_PCI_ADDRESS            0x898
#define CHANNEL3_DMA_SOURCE_HIGH_PCI_ADDRESS            0x89c
#define CHANNEL4_DMA_SOURCE_HIGH_PCI_ADDRESS            0x990
#define CHANNEL5_DMA_SOURCE_HIGH_PCI_ADDRESS            0x994
#define CHANNEL6_DMA_SOURCE_HIGH_PCI_ADDRESS            0x998
#define CHANNEL7_DMA_SOURCE_HIGH_PCI_ADDRESS            0x99c
#define CHANNEL0_DMA_DESTINATION_HIGH_PCI_ADDRESS       0x8a0
#define CHANNEL1_DMA_DESTINATION_HIGH_PCI_ADDRESS       0x8a4
#define CHANNEL2_DMA_DESTINATION_HIGH_PCI_ADDRESS       0x8a8
#define CHANNEL3_DMA_DESTINATION_HIGH_PCI_ADDRESS       0x8ac
#define CHANNEL4_DMA_DESTINATION_HIGH_PCI_ADDRESS       0x9a0
#define CHANNEL5_DMA_DESTINATION_HIGH_PCI_ADDRESS       0x9a4
#define CHANNEL6_DMA_DESTINATION_HIGH_PCI_ADDRESS       0x9a8
#define CHANNEL7_DMA_DESTINATION_HIGH_PCI_ADDRESS       0x9ac
#define CHANNEL0_DMA_NEXT_RECORD_POINTER_HIGH_PCI_ADDRESS   0x8b0
#define CHANNEL1_DMA_NEXT_RECORD_POINTER_HIGH_PCI_ADDRESS   0x8b4
#define CHANNEL2_DMA_NEXT_RECORD_POINTER_HIGH_PCI_ADDRESS   0x8b8
#define CHANNEL3_DMA_NEXT_RECORD_POINTER_HIGH_PCI_ADDRESS   0x8bc
#define CHANNEL4_DMA_NEXT_RECORD_POINTER_HIGH_PCI_ADDRESS   0x9b0
#define CHANNEL5_DMA_NEXT_RECORD_POINTER_HIGH_PCI_ADDRESS   0x9b4
#define CHANNEL6_DMA_NEXT_RECORD_POINTER_HIGH_PCI_ADDRESS   0x9b8
#define CHANNEL7_DMA_NEXT_RECORD_POINTER_HIGH_PCI_ADDRESS   0x9bc

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

#define CHANNEL4CONTROL 				0x940
#define CHANNEL4CONTROL_HIGH				0x980
#define CHANNEL5CONTROL 				0x944
#define CHANNEL5CONTROL_HIGH				0x984
#define CHANNEL6CONTROL 				0x948
#define CHANNEL6CONTROL_HIGH				0x988
#define CHANNEL7CONTROL 				0x94C
#define CHANNEL7CONTROL_HIGH				0x98C


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
#define TIMER_COUNTER4					0x950
#define TIMER_COUNTER5					0x954
#define TIMER_COUNTER6					0x958
#define TIMER_COUNTER7					0x95C
#define TIMER_COUNTER_4_7_CONTROL			0x964
#define TIMER_COUNTER_4_7_INTERRUPT_CAUSE		0x968
#define TIMER_COUNTER_4_7_INTERRUPT_MASK      		0x96c

/****************************************/
/* PCI Slave Address Decoding           */
/****************************************/
        
#define PCI_CS_0_BANK_SIZE				0xc08
#define PCI_CS_1_BANK_SIZE				0xd08
#define PCI_CS_2_BANK_SIZE				0xc0c
#define PCI_CS_3_BANK_SIZE				0xd0c
#define PCI_DEVCS_0_BANK_SIZE				0xc10
#define PCI_DEVCS_1_BANK_SIZE				0xd10
#define PCI_DEVCS_2_BANK_SIZE				0xd18
#define PCI_DEVCS_3_BANK_SIZE				0xc14
#define PCI_DEVCS_BOOT_BANK_SIZE				0xd14
#define PCI_EXPANSION_ROM_BAR_SIZE                     0xd2c
#define PCI_BASE_ADDRESS_REGISTERS_ENABLE 		0xc3c
#define PCI_CS_0_BASE_ADDRESS_REMAP			0xc48
#define PCI_CS_1_BASE_ADDRESS_REMAP			0xd48
#define PCI_CS_2_BASE_ADDRESS_REMAP			0xc4c
#define PCI_CS_3_BASE_ADDRESS_REMAP			0xd4c
#define PCI_DEVCS_0_BASE_ADDRESS_REMAP			0xc50
#define PCI_DEVCS_1_BASE_ADDRESS_REMAP			0xd50
#define PCI_DEVCS_2_BASE_ADDRESS_REMAP			0xd58
#define PCI_DEVCS_3_BASE_ADDRESS_REMAP           		0xc54
#define PCI_DEVCS_BOOTCS_BASE_ADDRESS_REMAP      		0xd54
#define PCI_EXPANSION_ROM_BASE_ADDRESS_REMAP               0xf38
#define PCI_ADDRESS_DECODE_CONTROL                         0xd3c

/****************************************/
/* PCI Control                          */
/****************************************/

#define PCI_COMMAND					    0xc00
#define PCI_MODE                                           0xd00
#define PCI_TIMEOUT_RETRY				    0xc04
#define PCI_READ_BUFFER_DISCARD_TIMER                      0xd04
#define MSI_TRIGGER_TIMER                                  0xc38
#define PCI_ARBITER_CONTROL                                0x1d00
/* changing untill here */
#define PCI_CROSS_BAR_CONTROL_LOW                           0x1d08
#define PCI_CROSS_BAR_CONTROL_HIGH                          0x1d0c
#define PCI_CROSS_BAR_TIMEOUT                               0x1d04
#define PCI_SYNC_BARRIER_TRIGER_REGISTER                   0x1d18
#define PCI_SYNC_BARRIER_VIRTUAL_REGISTER                   0x1d10
#define PCI_ACCESS_CONTROL_BASE_0_LOW                       0x1e00
#define PCI_ACCESS_CONTROL_BASE_0_HIGH                      0x1e04
#define PCI_ACCESS_CONTROL_SIZE_0                            0x1e08
#define PCI_ACCESS_CONTROL_BASE_1_LOW                       0c1e10
#define PCI_ACCESS_CONTROL_BASE_1_HIGH                      0x1e14
#define PCI_ACCESS_CONTROL_SIZE_1                            0x1e18
#define PCI_ACCESS_CONTROL_BASE_2_LOW                       0c1e20
#define PCI_ACCESS_CONTROL_BASE_2_HIGH                      0x1e24
#define PCI_ACCESS_CONTROL_SIZE_2                            0x1e28
#define PCI_ACCESS_CONTROL_BASE_3_LOW                       0c1e30
#define PCI_ACCESS_CONTROL_BASE_3_HIGH                      0x1e34
#define PCI_ACCESS_CONTROL_SIZE_3                            0x1e38
#define PCI_ACCESS_CONTROL_BASE_4_LOW                       0c1e40
#define PCI_ACCESS_CONTROL_BASE_4_HIGH                      0x1e44
#define PCI_ACCESS_CONTROL_SIZE_4                            0x1e48
#define PCI_ACCESS_CONTROL_BASE_5_LOW                       0c1e50
#define PCI_ACCESS_CONTROL_BASE_5_HIGH                      0x1e54
#define PCI_ACCESS_CONTROL_SIZE_5                            0x1e58


/****************************************/
/* PCI Configuration Address            */
/****************************************/

#define PCI_CONFIGURATION_ADDRESS 				0xcf8
#define PCI_CONFIGURATION_DATA_VIRTUAL_REGISTER           	0xcfc
#define PCI_INTERRUPT_ACKNOWLEDGE_VIRTUAL_REGISTER		0xc34

/****************************************/
/* PCI Error Report                     */
/****************************************/

#define PCI_SERR_MASK					 	0xc28
#define PCI_ERROR_ADDRESS_LOW                               0x1d40
#define PCI_ERROR_ADDRESS_HIGH                              0x1d44
#define PCI_ERROR_DATA_LOW                                  0x1d48
#define PCI_ERROR_DATA_HIGH                                 0x1d4c
#define PCI_ERROR_COMMAND                                   0x1d50
#define PCI_ERROR_CAUSE                                     0x1d58
#define PCI_ERROR_MASK                                      0x1d5c



/****************************************/
/* PCI Configuration Function 0         */
/****************************************/

#define PCI_DEVICE_AND_VENDOR_ID 				0x000
#define PCI_STATUS_AND_COMMAND					0x004
//#define PCI_COMMAND_STATUS_REG        				0x004
#define PCI_CLASS_CODE_AND_REVISION_ID			        0x008
#define PCI_BIST_HEADER_TYPE_LATENCY_TIMER_CACHE_LINE 		0x00C
#define PCI_CS_0_BASE_ADDRESS_LOW	    				0x010
#define PCI_CS_0_BASE_ADDRESS_HIGH 				    	0x014
#define PCI_CS_1_BASE_ADDRESS_LOW 					0x018
#define PCI_CS_1_BASE_ADDRESS_HIGH      				0x01C
#define PCI_INTERNAL_REGISTERS_MEMORY_MAPPED_BASE_ADDRESS_LOW	0x020
#define PCI_INTERNAL_REGISTERS_MEMORY_MAPPED_BASE_ADDRESS_HIGH	0x024
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
#define PCI_COMPACT_PCI_HOT_SWAP_CAPABILITY                 0x058

/****************************************/
/* PCI Configuration Function 1         */
/****************************************/

#define PCI_CS_2_BASE_ADDRESS_LOW	    			0x110
#define PCI_CS_2_BASE_ADDRESS_HIGH			   		0x114
#define PCI_CS_3_BASE_ADDRESS_LOW 					0x118
#define PCI_CS_3_BASE_ADDRESS_HIGH     				0x11c

/****************************************/
/* PCI Configuration Function 2         */
/****************************************/

#define PCI_DEVCS_0_BASE_ADDRESS_LOW	    				0x210
#define PCI_DEVCS_0_BASE_ADDRESS_HIGH 				    	0x214
#define PCI_DEVCS_1_BASE_ADDRESS_LOW 					0x218
#define PCI_DEVCS_1_BASE_ADDRESS_HIGH      				0x21C
#define PCI_DEVCS_2_BASE_ADDRESS_LOW 					0x220
#define PCI_DEVCS_2_BASE_ADDRESS_HIGH      				0x224
/****************************************/
/* PCI Configuration Function 3         */
/****************************************/
#define PCI_DEVCS_3_BASE_ADDRESS_LOW	    				0x310
#define PCI_DEVCS_3_BASE_ADDRESS_HIGH 				    	0x314
#define PCI_BOOTCS_BASE_ADDRESS_LOW                     	    0x318
#define PCI_BOOTCS_BASE_ADDRESS_HIGH                     	    0x31c


/****************************************/
/* PCI Configuration Function 4         */
/****************************************/
#define PCI_IO_BASE_ADDRESS           	        0x424

/****************************************/
/* Interrupts	  			*/
/****************************************/
			
#define LOW_INTERRUPT_CAUSE_REGISTER   				0x004
#define HIGH_INTERRUPT_CAUSE_REGISTER				0x00c
#define CPU_INTERRUPT_MASK_REGISTER_LOW				0x014
#define CPU_INTERRUPT_MASK_REGISTER_HIGH			0x01c
#define CPU_SELECT_CAUSE_REGISTER				0x024
#define INTERRUPT_MASK_REGISTER_LOW				0x054
#define INTERRUPT_MASK_REGISTER_HIGH			0x05c
#define SELECT_CAUSE_REGISTER				0x064

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
#define QUEUE_CONTROL_REGISTER_PCI_SIDE 					0x050
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
/* Fast Ethernet Unit Registers         */
/****************************************/

#define	ETHERNET_PORTS_DIFFERENCE_OFFSETS		0x0400
/* Ethernet */
#define ETH_PHY_ADDR_REG			0x2000
#define ETH_SMI_REG					0x2004
#define ETH_UNIT_DEFAULT_ADDR_REG		0x2008
#define ETH_UNIT_DEFAULTID_REG			0x200c
#define ETH_UNIT_INTERRUPT_CAUSE_REG	0x2080
#define ETH_UNIT_INTERRUPT_MASK_REG	0x2084
#define ETH_UNIT_INTERNAL_USE_REG		0x24fc
#define ETH_UNIT_ERROR_ADDR_REG			0x2094
#define ETH_BAR_0					0x2200
#define ETH_BAR_1					0x2208
#define ETH_BAR_2					0x2210
#define ETH_BAR_3					0x2218
#define ETH_BAR_4					0x2220
#define ETH_BAR_5					0x2228
#define ETH_SIZE_REG_0				0x2204
#define ETH_SIZE_REG_1				0x220c
#define ETH_SIZE_REG_2				0x2214
#define ETH_SIZE_REG_3				0x221c
#define ETH_SIZE_REG_4				0x2224
#define ETH_SIZE_REG_5					0x222c
#define ETH_HEADERS_RETARGET_BASE_REG			0x2230
#define ETH_HEADERS_RETARGET_CONTROL_REG	0x2234
#define ETH_HIGH_ADDR_REMAP_REG_0		0x2280
#define ETH_HIGH_ADDR_REMAP_REG_1		0x2284
#define ETH_HIGH_ADDR_REMAP_REG_2		0x2288
#define ETH_HIGH_ADDR_REMAP_REG_3			0x228c
#define ETH_BASE_ADDR_ENABLE_REG			0x2290
#define ETH_ACCESS_PROTECTION_REG(port)			(0x2294 + (port<<2))
#define ETH_MIB_COUNTERS_BASE(port)			(0x3000 + (port<<7))
#define ETH_PORT_CONFIG_REG(port)				(0x2400 + (port<<10))
#define ETH_PORT_CONFIG_EXTEND_REG(port)		(0x2404 + (port<<10))
#define ETH_MII_SERIAL_PARAMETRS_REG(port)		(0x2408 + (port<<10))
#define ETH_GMII_SERIAL_PARAMETRS_REG(port)		(0x240c + (port<<10))
#define ETH_VLAN_ETHERTYPE_REG(port)			(0x2410 + (port<<10))
#define ETH_MAC_ADDR_LOW(port)					(0x2414 + (port<<10))
#define ETH_MAC_ADDR_HIGH(port)					(0x2418 + (port<<10))
#define ETH_SDMA_CONFIG_REG(port)				(0x241c + (port<<10))
#define ETH_DSCP_0(port)					(0x2420 + (port<<10))
#define ETH_DSCP_1(port)					(0x2424 + (port<<10))
#define ETH_DSCP_2(port)					(0x2428 + (port<<10))
#define ETH_DSCP_3(port)					(0x242c + (port<<10))
#define ETH_DSCP_4(port)					(0x2430 + (port<<10))
#define ETH_DSCP_5(port)					(0x2434 + (port<<10))
#define ETH_DSCP_6(port)					(0x2438 + (port<<10))
#define ETH_PORT_SERIAL_CONTROL_REG(port)		(0x243c + (port<<10))
#define ETH_VLAN_PRIORITY_TAG_TO_PRIORITY(port)		(0x2440 + (port<<10))
#define ETH_PORT_STATUS_REG(port)				(0x2444 + (port<<10))
#define ETH_TRANSMIT_QUEUE_COMMAND_REG(port)	(0x2448 + (port<<10))
#define ETH_TX_QUEUE_FIXED_PRIORITY(port)		(0x244c + (port<<10))
#define ETH_PORT_TX_TOKEN_BUCKET_RATE_CONFIG(port)		(0x2450 + (port<<10))
#define ETH_MAXIMUM_TRANSMIT_UNIT(port)				(0x2458 + (port<<10))
#define ETH_PORT_MAXIMUM_TOKEN_BUCKET_SIZE(port)		(0x245c + (port<<10))
#define ETH_INTERRUPT_CAUSE_REG(port)				(0x2460 + (port<<10))
#define ETH_INTERRUPT_CAUSE_EXTEND_REG(port)			(0x2464 + (port<<10))
#define ETH_INTERRUPT_MASK_REG(port)				(0x2468 + (port<<10))
#define ETH_INTERRUPT_EXTEND_MASK_REG(port)			(0x246c + (port<<10))
#define ETH_RX_FIFO_URGENT_THRESHOLD_REG(port)			(0x2470 + (port<<10))
#define ETH_TX_FIFO_URGENT_THRESHOLD_REG(port)			(0x2474 + (port<<10))
#define ETH_RX_MINIMAL_FRAME_SIZE_REG(port)			(0x247c + (port<<10))
#define ETH_RX_DISCARDED_FRAMES_COUNTER(port)			(0x2484 + (port<<10)
#define ETH_PORT_DEBUG_0_REG(port)					(0x248c + (port<<10))
#define ETH_PORT_DEBUG_1_REG(port)					(0x2490 + (port<<10))
#define ETH_PORT_INTERNAL_ADDR_ERROR_REG(port)			(0x2494 + (port<<10))
#define ETH_INTERNAL_USE_REG(port)					(0x24fc + (port<<10))
#define ETH_RECEIVE_QUEUE_COMMAND_REG(port)			(0x2680 + (port<<10))
#define ETH_CURRENT_SERVED_TX_DESC_PTR(port)			(0x2684 + (port<<10))
#define ETH_RX_CURRENT_QUEUE_DESC_PTR_0(port)			(0x260c + (port<<10))
#define ETH_RX_CURRENT_QUEUE_DESC_PTR_1(port)			(0x261c + (port<<10))
#define ETH_RX_CURRENT_QUEUE_DESC_PTR_2(port)			(0x262c + (port<<10))
#define ETH_RX_CURRENT_QUEUE_DESC_PTR_3(port)			(0x263c + (port<<10))
#define ETH_RX_CURRENT_QUEUE_DESC_PTR_4(port)			(0x264c + (port<<10))
#define ETH_RX_CURRENT_QUEUE_DESC_PTR_5(port)			(0x265c + (port<<10))
#define ETH_RX_CURRENT_QUEUE_DESC_PTR_6(port)			(0x266c + (port<<10))
#define ETH_RX_CURRENT_QUEUE_DESC_PTR_7(port)			(0x267c + (port<<10))
#define ETH_TX_CURRENT_QUEUE_DESC_PTR_0(port)			(0x26c0 + (port<<10))
#define ETH_TX_CURRENT_QUEUE_DESC_PTR_1(port)			(0x26c4 + (port<<10))
#define ETH_TX_CURRENT_QUEUE_DESC_PTR_2(port)			(0x26c8 + (port<<10))
#define ETH_TX_CURRENT_QUEUE_DESC_PTR_3(port)			(0x26cc + (port<<10))
#define ETH_TX_CURRENT_QUEUE_DESC_PTR_4(port)			(0x26d0 + (port<<10))
#define ETH_TX_CURRENT_QUEUE_DESC_PTR_5(port)			(0x26d4 + (port<<10))
#define ETH_TX_CURRENT_QUEUE_DESC_PTR_6(port)			(0x26d8 + (port<<10))
#define ETH_TX_CURRENT_QUEUE_DESC_PTR_7(port)			(0x26dc + (port<<10))
#define ETH_TX_QUEUE_0_TOKEN_BUCKET_COUNT(port)			(0x2700 + (port<<10))
#define ETH_TX_QUEUE_1_TOKEN_BUCKET_COUNT(port)			(0x2710 + (port<<10))
#define ETH_TX_QUEUE_2_TOKEN_BUCKET_COUNT(port)			(0x2720 + (port<<10))
#define ETH_TX_QUEUE_3_TOKEN_BUCKET_COUNT(port)			(0x2730 + (port<<10))
#define ETH_TX_QUEUE_4_TOKEN_BUCKET_COUNT(port)			(0x2740 + (port<<10))
#define ETH_TX_QUEUE_5_TOKEN_BUCKET_COUNT(port)			(0x2750 + (port<<10))
#define ETH_TX_QUEUE_6_TOKEN_BUCKET_COUNT(port)			(0x2760 + (port<<10))
#define ETH_TX_QUEUE_7_TOKEN_BUCKET_COUNT(port)			(0x2770 + (port<<10))
#define ETH_TX_QUEUE_0_TOKEN_BUCKET_CONFIG(port)		(0x2704 + (port<<10))
#define ETH_TX_QUEUE_1_TOKEN_BUCKET_CONFIG(port)		(0x2714 + (port<<10))
#define ETH_TX_QUEUE_2_TOKEN_BUCKET_CONFIG(port)		(0x2724 + (port<<10))
#define ETH_TX_QUEUE_3_TOKEN_BUCKET_CONFIG(port)		(0x2734 + (port<<10))
#define ETH_TX_QUEUE_4_TOKEN_BUCKET_CONFIG(port)		(0x2744 + (port<<10))
#define ETH_TX_QUEUE_5_TOKEN_BUCKET_CONFIG(port)		(0x2754 + (port<<10))
#define ETH_TX_QUEUE_6_TOKEN_BUCKET_CONFIG(port)		(0x2764 + (port<<10))
#define ETH_TX_QUEUE_7_TOKEN_BUCKET_CONFIG(port)		(0x2774 + (port<<10))
#define ETH_TX_QUEUE_0_ARBITER_CONFIG(port)			(0x2708 + (port<<10))
#define ETH_TX_QUEUE_1_ARBITER_CONFIG(port)			(0x2718 + (port<<10))
#define ETH_TX_QUEUE_2_ARBITER_CONFIG(port)			(0x2728 + (port<<10))
#define ETH_TX_QUEUE_3_ARBITER_CONFIG(port)			(0x2738 + (port<<10))
#define ETH_TX_QUEUE_4_ARBITER_CONFIG(port)			(0x2748 + (port<<10))
#define ETH_TX_QUEUE_5_ARBITER_CONFIG(port)			(0x2758 + (port<<10))
#define ETH_TX_QUEUE_6_ARBITER_CONFIG(port)			(0x2768 + (port<<10))
#define ETH_TX_QUEUE_7_ARBITER_CONFIG(port)			(0x2778 + (port<<10))
#define ETH_PORT_TX_TOKEN_BUCKET_COUNT(port)			(0x2780 + (port<<10))
#define ETH_DA_FILTER_SPECIAL_MULTICAST_TABLE_BASE(port)	(0x3400 + (port<<10))
#define ETH_DA_FILTER_OTHER_MULTICAST_TABLE_BASE(port)		(0x3500 + (port<<10))
#define ETH_DA_FILTER_UNICAST_TABLE_BASE(port)			(0x3600 + (port<<10))
/* ETHERNET 0*/
#define ETH0_ACCESS_PROTECTION_REG			(0x2294 + (0<<2))
#define ETH0_MIB_COUNTERS_BASE			(0x3000 + (0<<7))
#define ETH0_PORT_CONFIG_REG				(0x2400 + (0<<10))
#define ETH0_PORT_CONFIG_EXT_REG		(0x2404 + (0<<10))
#define ETH0_MII_SERIAL_PARAMETRS_REG		(0x2408 + (0<<10))
#define ETH0_GMII_SERIAL_PARAMETRS_REG		(0x240c + (0<<10))
#define ETH0_VLAN_ETHERTYPE_REG			(0x2410 + (0<<10))
#define ETH0_MAC_ADDR_LOW					(0x2414 + (0<<10))
#define ETH0_MAC_ADDR_HIGH					(0x2418 + (0<<10))
#define ETH0_SDMA_CONFIG_REG				(0x241c + (0<<10))
#define ETH0_DSCP_0					(0x2420 + (0<<10))
#define ETH0_DSCP_1					(0x2424 + (0<<10))
#define ETH0_DSCP_2					(0x2428 + (0<<10))
#define ETH0_DSCP_3					(0x242c + (0<<10))
#define ETH0_DSCP_4					(0x2430 + (0<<10))
#define ETH0_DSCP_5					(0x2434 + (0<<10))
#define ETH0_DSCP_6					(0x2438 + (0<<10))
#define ETH0_PORT_SERIAL_CONTROL_REG		(0x243c + (0<<10))
#define ETH0_VLAN_PRIORITY_TAG_TO_PRIORITY		(0x2440 + (0<<10))
#define ETH0_PORT_STATUS_REG				(0x2444 + (0<<10))
#define ETH0_TRANSMIT_QUEUE_COMMAND_REG	(0x2448 + (0<<10))
#define ETH0_TX_QUEUE_FIXED_PRIORITY		(0x244c + (0<<10))
#define ETH0_PORT_TX_TOKEN_BUCKET_RATE_CONFIG		(0x2450 + (0<<10))
#define ETH0_MAXIMUM_TRANSMIT_UNIT				(0x2458 + (0<<10))
#define ETH0_PORT_MAXIMUM_TOKEN_BUCKET_SIZE		(0x245c + (0<<10))
#define ETH0_INTERRUPT_CAUSE_REG				(0x2460 + (0<<10))
#define ETH0_INTERRUPT_CAUSE_EXTEND_REG			(0x2464 + (0<<10))
#define ETH0_INTERRUPT_MASK_REG				(0x2468 + (0<<10))
#define ETH0_INTERRUPT_EXTEND_MASK_REG			(0x246c + (0<<10))
#define ETH0_RX_FIFO_URGENT_THRESHOLD_REG			(0x2470 + (0<<10))
#define ETH0_TX_FIFO_URGENT_THRESHOLD_REG			(0x2474 + (0<<10))
#define ETH0_RX_MINIMAL_FRAME_SIZE_REG			(0x247c + (0<<10))
#define ETH0_RX_DISCARDED_FRAMES_COUNTER			(0x2484 + (0<<10)
#define ETH0_PORT_DEBUG_0_REG					(0x248c + (0<<10))
#define ETH0_PORT_DEBUG_1_REG					(0x2490 + (0<<10))
#define ETH0_PORT_INTERNAL_ADDR_ERROR_REG			(0x2494 + (0<<10))
#define ETH0_INTERNAL_USE_REG					(0x24fc + (0<<10))
#define ETH0_RECEIVE_QUEUE_COMMAND_REG			(0x2680 + (0<<10))
#define ETH0_CURRENT_SERVED_TX_DESC_PTR			(0x2684 + (0<<10))
#define ETH0_CURRENT_RX_DESC_PTR0			(0x260c + (0<<10))
#define ETH0_CURRENT_RX_DESC_PTR1			(0x261c + (0<<10))
#define ETH0_CURRENT_RX_DESC_PTR2			(0x262c + (0<<10))
#define ETH0_CURRENT_RX_DESC_PTR3			(0x263c + (0<<10))
#define ETH0_CURRENT_RX_DESC_PTR4			(0x264c + (0<<10))
#define ETH0_CURRENT_RX_DESC_PTR5			(0x265c + (0<<10))
#define ETH0_CURRENT_RX_DESC_PTR6			(0x266c + (0<<10))
#define ETH0_CURRENT_RX_DESC_PTR7			(0x267c + (0<<10))
#define ETH0_CURRENT_TX_DESC_PTR0			(0x26c0 + (0<<10))
#define ETH0_CURRENT_TX_DESC_PTR1			(0x26c4 + (0<<10))
#define ETH0_CURRENT_TX_DESC_PTR2			(0x26c8 + (0<<10))
#define ETH0_CURRENT_TX_DESC_PTR3			(0x26cc + (0<<10))
#define ETH0_CURRENT_TX_DESC_PTR4			(0x26d0 + (0<<10))
#define ETH0_CURRENT_TX_DESC_PTR5			(0x26d4 + (0<<10))
#define ETH0_CURRENT_TX_DESC_PTR6			(0x26d8 + (0<<10))
#define ETH0_CURRENT_TX_DESC_PTR7			(0x26dc + (0<<10))
#define ETH0_TX_QUEUE_0_TOKEN_BUCKET_COUNT			(0x2700 + (0<<10))
#define ETH0_TX_QUEUE_1_TOKEN_BUCKET_COUNT			(0x2710 + (0<<10))
#define ETH0_TX_QUEUE_2_TOKEN_BUCKET_COUNT			(0x2720 + (0<<10))
#define ETH0_TX_QUEUE_3_TOKEN_BUCKET_COUNT			(0x2730 + (0<<10))
#define ETH0_TX_QUEUE_4_TOKEN_BUCKET_COUNT			(0x2740 + (0<<10))
#define ETH0_TX_QUEUE_5_TOKEN_BUCKET_COUNT			(0x2750 + (0<<10))
#define ETH0_TX_QUEUE_6_TOKEN_BUCKET_COUNT			(0x2760 + (0<<10))
#define ETH0_TX_QUEUE_7_TOKEN_BUCKET_COUNT			(0x2770 + (0<<10))
#define ETH0_TX_QUEUE_0_TOKEN_BUCKET_CONFIG		(0x2704 + (0<<10))
#define ETH0_TX_QUEUE_1_TOKEN_BUCKET_CONFIG		(0x2714 + (0<<10))
#define ETH0_TX_QUEUE_2_TOKEN_BUCKET_CONFIG		(0x2724 + (0<<10))
#define ETH0_TX_QUEUE_3_TOKEN_BUCKET_CONFIG		(0x2734 + (0<<10))
#define ETH0_TX_QUEUE_4_TOKEN_BUCKET_CONFIG		(0x2744 + (0<<10))
#define ETH0_TX_QUEUE_5_TOKEN_BUCKET_CONFIG		(0x2754 + (0<<10))
#define ETH0_TX_QUEUE_6_TOKEN_BUCKET_CONFIG		(0x2764 + (0<<10))
#define ETH0_TX_QUEUE_7_TOKEN_BUCKET_CONFIG		(0x2774 + (0<<10))
#define ETH0_TX_QUEUE_0_ARBITER_CONFIG			(0x2708 + (0<<10))
#define ETH0_TX_QUEUE_1_ARBITER_CONFIG			(0x2718 + (0<<10))
#define ETH0_TX_QUEUE_2_ARBITER_CONFIG			(0x2728 + (0<<10))
#define ETH0_TX_QUEUE_3_ARBITER_CONFIG			(0x2738 + (0<<10))
#define ETH0_TX_QUEUE_4_ARBITER_CONFIG			(0x2748 + (0<<10))
#define ETH0_TX_QUEUE_5_ARBITER_CONFIG			(0x2758 + (0<<10))
#define ETH0_TX_QUEUE_6_ARBITER_CONFIG			(0x2768 + (0<<10))
#define ETH0_TX_QUEUE_7_ARBITER_CONFIG			(0x2778 + (0<<10))
#define ETH0_PORT_TX_TOKEN_BUCKET_COUNT			(0x2780 + (0<<10))
#define ETH0_DA_FILTER_SPECIAL_MULTICAST_TABLE_BASE	(0x3400 + (0<<10))
#define ETH0_DA_FILTER_OTHER_MULTICAST_TABLE_BASE		(0x3500 + (0<<10))
#define ETH0_DA_FILTER_UNICAST_TABLE_BASE			(0x3600 + (0<<10))
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

#define GPP_IO_CONTROL                                      0xf100
#define GPP_LEVEL_CONTROL                                   0xf110
#define GPP_VALUE                                           0xf104
#define GPP_INTERRUPT_CAUSE                                 0xf108
#define GPP_INTERRUPT_MASK                                  0xf10c

#define MPP_CONTROL0                                        0xf000
#define MPP_CONTROL1                                        0xf004
#define MPP_CONTROL2                                        0xf008
#define MPP_CONTROL3                                        0xf00c
#define DEBUG_PORT_MULTIPLEX                                0xf014
#define SERIAL_PORT_MULTIPLEX                               0xf010

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

        /*  MPSCs Clocks Routing Registers  */

#define MPSC_ROUTING_REG                                    0xb400
#define MPSC_RX_CLOCK_ROUTING_REG                           0xb404
#define MPSC_TX_CLOCK_ROUTING_REG                           0xb408

        /*  MPSCs Interrupts Registers    */

#define MPSC_CAUSE_REG(port)                               (0xb804 + (port<<3))
#define MPSC_MASK_REG(port)                                (0xb884 + (port<<3))
 
#define MPSC_MAIN_CONFIG_LOW(port)                         (0x8000 + (port<<12))
#define MPSC_MAIN_CONFIG_HIGH(port)                        (0x8004 + (port<<12))    
#define MPSC_PROTOCOL_CONFIG(port)                         (0x8008 + (port<<12))    
#define MPSC_CHANNEL_REG1(port)                            (0x800c + (port<<12))    
#define MPSC_CHANNEL_REG2(port)                            (0x8010 + (port<<12))    
#define MPSC_CHANNEL_REG3(port)                            (0x8014 + (port<<12))    
#define MPSC_CHANNEL_REG4(port)                            (0x8018 + (port<<12))    
#define MPSC_CHANNEL_REG5(port)                            (0x801c + (port<<12))    
#define MPSC_CHANNEL_REG6(port)                            (0x8020 + (port<<12))    
#define MPSC_CHANNEL_REG7(port)                            (0x8024 + (port<<12))    
#define MPSC_CHANNEL_REG8(port)                            (0x8028 + (port<<12))    
#define MPSC_CHANNEL_REG9(port)                            (0x802c + (port<<12))    
#define MPSC_CHANNEL_REG10(port)                           (0x8030 + (port<<12))    
/***************************************/
/*          SDMA Registers             */
/***************************************/

#define SDMA_CONFIG_REG(channel)                        (0x4000 + (channel<<13))        
#define SDMA_COMMAND_REG(channel)                       (0x4008 + (channel<<13))        
#define SDMA_CURRENT_RX_DESCRIPTOR_POINTER(channel)     (0x4810 + (channel<<13))        
#define SDMA_CURRENT_TX_DESCRIPTOR_POINTER(channel)     (0x4c10 + (channel<<13))        
#define SDMA_FIRST_TX_DESCRIPTOR_POINTER(channel)       (0x4c14 + (channel<<13)) 

#define SDMA_CAUSE_REG                                      0xb800
#define SDMA_MASK_REG                                       0xb880
         
/* BRG Interrupts */

#define BRG_CONFIG_REG(brg)                              (0xb200 + (brg<<3))
#define BRG_BAUDE_TUNING_REG(brg)                        (0xb208 + (brg<<3))
#define BRG_CAUSE_REG                                       0xb834
#define BRG_MASK_REG                                        0xb8b4

#endif /* _GT64420REG_H_ */
