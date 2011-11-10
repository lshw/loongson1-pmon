/* $Id: mv64360reg.h,v 1.1.1.1 2006/09/14 01:59:08 root Exp $ */

/*
 * Copyright (c) 2002 Galileo Technology.
 * Copyright (c) 2002 Opsycon AB  (www.opsycon.se)
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
 *	This product includes software developed by Opsycon AB, Sweden.
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

#ifndef _MV64360REG_H_
#define _MV64360REG_H_

#include <machine/asm.h>

#ifndef _LOCORE
#include <machine/pio.h>

#define GT_WRITE(offs, data) \
	out32rb(GT_BASE_ADDR+(offs), data)

#define GT_READ(offs) \
	in32rb(GT_BASE_ADDR+(offs))
#endif

/* Always move internal space up! */
#define	GT_BASE_ADDR			0xf1000000
#define	GT_BASE_ADDR_DEFAULT		0x14000000 


#if 0
/* Supported by the Atlantis */
#define INCLUDE_PCI_1
#define INCLUDE_PCI_0_ARBITER
#define INCLUDE_PCI_1_ARBITER
#define INCLUDE_SNOOP_SUPPORT
#define INCLUDE_P2P
#define INCLUDE_ETH_PORT_2
#define INCLUDE_CPU_MAPPING 
#define INCLUDE_MPSC

/* Not supported features */ 
#undef  INCLUDE_CNTMR_4_7
#undef  INCLUDE_DMA_4_7
#endif

#define	GT_IPCI_CFGADDR_ConfigEn	(1<<31)

/* Aliases */

#define GT_BOOT_PAR 		DEVICE_BOOT_BANK_PARAMETERS

/****************************************/
/* Processor Address Space              */
/****************************************/

/* DDR SDRAM BAR and size registers */

#define CS_0_BASE_ADDR                                      0x008
#define CS_0_SIZE                                           0x010
#define CS_1_BASE_ADDR                                      0x208
#define CS_1_SIZE                                           0x210
#define CS_2_BASE_ADDR                                      0x018
#define CS_2_SIZE                                           0x020
#define CS_3_BASE_ADDR                                      0x218
#define CS_3_SIZE                                           0x220

/* Devices BAR and size registers */

#define DEV_CS0_BASE_ADDR                                   0x028
#define DEV_CS0_SIZE                                        0x030
#define DEV_CS1_BASE_ADDR                                   0x228
#define DEV_CS1_SIZE                                        0x230
#define DEV_CS2_BASE_ADDR                                   0x248
#define DEV_CS2_SIZE                                        0x250
#define DEV_CS3_BASE_ADDR                                   0x038
#define DEV_CS3_SIZE                                        0x040
#define BOOTCS_BASE_ADDR                                    0x238
#define BOOTCS_SIZE                                         0x240

/* PCI 0 BAR and size registers */

#define PCI_0_IO_BASE_ADDR                                  0x048
#define PCI_0_IO_SIZE                                       0x050
#define PCI_0_MEMORY0_BASE_ADDR                             0x058
#define PCI_0_MEMORY0_SIZE                                  0x060
#define PCI_0_MEMORY1_BASE_ADDR                             0x080
#define PCI_0_MEMORY1_SIZE                                  0x088
#define PCI_0_MEMORY2_BASE_ADDR                             0x258
#define PCI_0_MEMORY2_SIZE                                  0x260
#define PCI_0_MEMORY3_BASE_ADDR                             0x280
#define PCI_0_MEMORY3_SIZE                                  0x288

/* PCI 1 BAR and size registers */
#define PCI_1_IO_BASE_ADDR                                  0x090
#define PCI_1_IO_SIZE                                       0x098
#define PCI_1_MEMORY0_BASE_ADDR                             0x0a0
#define PCI_1_MEMORY0_SIZE                                  0x0a8
#define PCI_1_MEMORY1_BASE_ADDR                             0x0b0
#define PCI_1_MEMORY1_SIZE                                  0x0b8
#define PCI_1_MEMORY2_BASE_ADDR                             0x2a0
#define PCI_1_MEMORY2_SIZE                                  0x2a8
#define PCI_1_MEMORY3_BASE_ADDR                             0x2b0
#define PCI_1_MEMORY3_SIZE                                  0x2b8

/* SRAM base address */
#define INTEGRATED_SRAM_BASE_ADDR                           0x268

/* internal registers space base address */
#define INTERNAL_SPACE_BASE_ADDR                            0x068

/* Enables the CS , DEV_CS , PCI 0 and PCI 1 
   windows above */
#define BASE_ADDR_ENABLE                                    0x278

/****************************************/
/* PCI remap registers                  */
/****************************************/
      /* PCI 0 */
#define PCI_0_IO_ADDR_REMAP                                 0x0f0
#define PCI_0_MEMORY0_LOW_ADDR_REMAP                        0x0f8
#define PCI_0_MEMORY0_HIGH_ADDR_REMAP                       0x320
#define PCI_0_MEMORY1_LOW_ADDR_REMAP                        0x100
#define PCI_0_MEMORY1_HIGH_ADDR_REMAP                       0x328
#define PCI_0_MEMORY2_LOW_ADDR_REMAP                        0x2f8
#define PCI_0_MEMORY2_HIGH_ADDR_REMAP                       0x330
#define PCI_0_MEMORY3_LOW_ADDR_REMAP                        0x300
#define PCI_0_MEMORY3_HIGH_ADDR_REMAP                       0x338
      /* PCI 1 */
#define PCI_1_IO_ADDR_REMAP                                 0x108
#define PCI_1_MEMORY0_LOW_ADDR_REMAP                        0x110
#define PCI_1_MEMORY0_HIGH_ADDR_REMAP                       0x340
#define PCI_1_MEMORY1_LOW_ADDR_REMAP                        0x118
#define PCI_1_MEMORY1_HIGH_ADDR_REMAP                       0x348
#define PCI_1_MEMORY2_LOW_ADDR_REMAP                        0x310
#define PCI_1_MEMORY2_HIGH_ADDR_REMAP                       0x350
#define PCI_1_MEMORY3_LOW_ADDR_REMAP                        0x318
#define PCI_1_MEMORY3_HIGH_ADDR_REMAP                       0x358
 
#define CPU_PCI_0_HEADERS_RETARGET_CONTROL                  0x3b0
#define CPU_PCI_0_HEADERS_RETARGET_BASE                     0x3b8
#define CPU_PCI_1_HEADERS_RETARGET_CONTROL                  0x3c0
#define CPU_PCI_1_HEADERS_RETARGET_BASE                     0x3c8
#define CPU_GE_HEADERS_RETARGET_CONTROL                     0x3d0
#define CPU_GE_HEADERS_RETARGET_BASE                        0x3d8
#define CPU_IDMA_HEADERS_RETARGET_CONTROL                   0x3e0
#define CPU_IDMA_HEADERS_RETARGET_BASE                      0x3e8

/****************************************/
/*         CPU Control Registers        */
/****************************************/

#define CPU_CONFIG                                          0x000
#define CPU_MODE                                            0x120
#define CPU_MASTER_CONTROL                                  0x160
#define CPU_CROSS_BAR_CONTROL_LOW                           0x150
#define CPU_CROSS_BAR_CONTROL_HIGH                          0x158
#define CPU_CROSS_BAR_TIMEOUT                               0x168

/****************************************/
/* SMP RegisterS                        */
/****************************************/

#define SMP_WHO_AM_I                                        0x200
#define SMP_CPU0_DOORBELL                                   0x214
#define SMP_CPU0_DOORBELL_CLEAR                             0x21C
#define SMP_CPU1_DOORBELL                                   0x224
#define SMP_CPU1_DOORBELL_CLEAR                             0x22C
#define SMP_CPU0_DOORBELL_MASK                              0x234
#define SMP_CPU1_DOORBELL_MASK                              0x23C
#define SMP_SEMAPHOR0                                       0x244
#define SMP_SEMAPHOR1                                       0x24c
#define SMP_SEMAPHOR2                                       0x254
#define SMP_SEMAPHOR3                                       0x25c
#define SMP_SEMAPHOR4                                       0x264
#define SMP_SEMAPHOR5                                       0x26c
#define SMP_SEMAPHOR6                                       0x274
#define SMP_SEMAPHOR7                                       0x27c

/****************************************/
/*  CPU Sync Barrier Register           */
/****************************************/

#define CPU_0_SYNC_BARRIER_TRIGGER                          0x0c0
#define CPU_0_SYNC_BARRIER_VIRTUAL                          0x0c8
#define CPU_1_SYNC_BARRIER_TRIGGER                          0x0d0
#define CPU_1_SYNC_BARRIER_VIRTUAL                          0x0d8

/****************************************/
/* CPU Access Protect                   */
/****************************************/

#define CPU_PROTECT_WINDOW_0_BASE_ADDR                      0x180
#define CPU_PROTECT_WINDOW_0_SIZE                           0x188
#define CPU_PROTECT_WINDOW_1_BASE_ADDR                      0x190
#define CPU_PROTECT_WINDOW_1_SIZE                           0x198
#define CPU_PROTECT_WINDOW_2_BASE_ADDR                      0x1a0
#define CPU_PROTECT_WINDOW_2_SIZE                           0x1a8
#define CPU_PROTECT_WINDOW_3_BASE_ADDR                      0x1b0
#define CPU_PROTECT_WINDOW_3_SIZE                           0x1b8


/****************************************/
/*          CPU Error Report            */
/****************************************/

#define CPU_ERROR_ADDR_LOW                                  0x070
#define CPU_ERROR_ADDR_HIGH                                 0x078
#define CPU_ERROR_DATA_LOW                                  0x128
#define CPU_ERROR_DATA_HIGH                                 0x130
#define CPU_ERROR_PARITY                                    0x138
#define CPU_ERROR_CAUSE                                     0x140
#define CPU_ERROR_MASK                                      0x148

/****************************************/
/*      CPU Interface Debug Registers 	*/
/****************************************/

#define PUNIT_SLAVE_DEBUG_LOW                               0x360
#define PUNIT_SLAVE_DEBUG_HIGH                              0x368
#define PUNIT_MASTER_DEBUG_LOW                              0x370
#define PUNIT_MASTER_DEBUG_HIGH                             0x378
#define PUNIT_MMASK                                         0x3e4

/****************************************/
/*  Integrated SRAM Registers           */
/****************************************/

#define SRAM_CONFIG                                         0x380
#define SRAM_TEST_MODE                                      0X3F4
#define SRAM_ERROR_CAUSE                                    0x388
#define SRAM_ERROR_ADDR                                     0x390
#define SRAM_ERROR_ADDR_HIGH                                0X3F8
#define SRAM_ERROR_DATA_LOW                                 0x398
#define SRAM_ERROR_DATA_HIGH                                0x3a0
#define SRAM_ERROR_DATA_PARITY                              0x3a8

/****************************************/
/* SDRAM Configuration                  */
/****************************************/

#define SDRAM_CONFIG                                        0x1400
#define D_UNIT_CONTROL_LOW                                  0x1404
#define D_UNIT_CONTROL_HIGH                                 0x1424
#define SDRAM_TIMING_CONTROL_LOW                            0x1408
#define SDRAM_TIMING_CONTROL_HIGH                           0x140c
#define SDRAM_ADDR_CONTROL                                  0x1410
#define SDRAM_OPEN_PAGES_CONTROL                            0x1414
#define SDRAM_OPERATION                                     0x1418
#define SDRAM_MODE                                          0x141c
#define EXTENDED_DRAM_MODE                                  0x1420
#define SDRAM_CROSS_BAR_CONTROL_LOW                         0x1430
#define SDRAM_CROSS_BAR_CONTROL_HIGH                        0x1434
#define SDRAM_CROSS_BAR_TIMEOUT                             0x1438

#define	SDRAM_DATA_PADS_CALIBRATION			0x14c4

/****************************************/
/* SDRAM Error Report                   */
/****************************************/

#define SDRAM_ERROR_DATA_LOW                                0x1444
#define SDRAM_ERROR_DATA_HIGH                               0x1440
#define SDRAM_ERROR_ADDR                                    0x1450
#define SDRAM_RECEIVED_ECC                                  0x1448
#define SDRAM_CALCULATED_ECC                                0x144c
#define SDRAM_ECC_CONTROL                                   0x1454
#define SDRAM_ECC_ERROR_COUNTER                             0x1458

/******************************************/
/*  Controlled Delay Line (CDL) Registers */
/******************************************/

#define DFCDL_CONFIG0                                       0x1480
#define DFCDL_CONFIG1                                       0x1484
#define DLL_WRITE                                           0x1488
#define DLL_READ                                            0x148c
#define SRAM_ADDR                                           0x1490
#define SRAM_DATA0                                          0x1494
#define SRAM_DATA1                                          0x1498
#define SRAM_DATA2                                          0x149c
#define DFCL_PROBE                                          0x14a0

/******************************************/
/*   Debug Registers                      */
/******************************************/

#define DUNIT_DEBUG_LOW                                     0x1460
#define DUNIT_DEBUG_HIGH                                    0x1464
#define DUNIT_MMASK                                         0X1b40

/****************************************/
/* Device Parameters					*/
/****************************************/

#define DEVICE_BANK0_PARAMETERS				0x45c
#define DEVICE_BANK1_PARAMETERS				0x460
#define DEVICE_BANK2_PARAMETERS				0x464
#define DEVICE_BANK3_PARAMETERS				0x468
#define DEVICE_BOOT_BANK_PARAMETERS			0x46c
#define DEVICE_INTERFACE_CONTROL                        0x4c0
#define DEVICE_INTERFACE_CROSS_BAR_CONTROL_LOW          0x4c8
#define DEVICE_INTERFACE_CROSS_BAR_CONTROL_HIGH         0x4cc
#define DEVICE_INTERFACE_CROSS_BAR_TIMEOUT              0x4c4

/****************************************/
/* Device interrupt registers			*/
/****************************************/

#define DEVICE_INTERRUPT_CAUSE				0x4d0
#define DEVICE_INTERRUPT_MASK				0x4d4
#define DEVICE_ERROR_ADDR				0x4d8
#define DEVICE_ERROR_DATA   				0x4dc
#define DEVICE_ERROR_PARITY     			0x4e0

/****************************************/
/* Device debug registers   			*/
/****************************************/

#define DEVICE_DEBUG_LOW     			        0x4e4
#define DEVICE_DEBUG_HIGH     			        0x4e8
#define RUNIT_MMASK					0x4f0

/****************************************/
/* PCI Slave Address Decoding registers */
/****************************************/

#define PCI_0_CS_0_BANK_SIZE                            0xc08
#define PCI_1_CS_0_BANK_SIZE                            0xc88
#define PCI_0_CS_1_BANK_SIZE                            0xd08
#define PCI_1_CS_1_BANK_SIZE                            0xd88
#define PCI_0_CS_2_BANK_SIZE                            0xc0c
#define PCI_1_CS_2_BANK_SIZE                            0xc8c
#define PCI_0_CS_3_BANK_SIZE                            0xd0c
#define PCI_1_CS_3_BANK_SIZE                            0xd8c
#define PCI_0_DEVCS_0_BANK_SIZE                         0xc10
#define PCI_1_DEVCS_0_BANK_SIZE                         0xc90
#define PCI_0_DEVCS_1_BANK_SIZE                         0xd10
#define PCI_1_DEVCS_1_BANK_SIZE                         0xd90
#define PCI_0_DEVCS_2_BANK_SIZE                         0xd18
#define PCI_1_DEVCS_2_BANK_SIZE                         0xd98
#define PCI_0_DEVCS_3_BANK_SIZE                         0xc14
#define PCI_1_DEVCS_3_BANK_SIZE                         0xc94
#define PCI_0_DEVCS_BOOT_BANK_SIZE                      0xd14
#define PCI_1_DEVCS_BOOT_BANK_SIZE                      0xd94
#define PCI_0_P2P_MEM0_BAR_SIZE                         0xd1c
#define PCI_1_P2P_MEM0_BAR_SIZE                         0xd9c
#define PCI_0_P2P_MEM1_BAR_SIZE                         0xd20
#define PCI_1_P2P_MEM1_BAR_SIZE                         0xda0
#define PCI_0_P2P_I_O_BAR_SIZE                          0xd24
#define PCI_1_P2P_I_O_BAR_SIZE                          0xda4
#define PCI_0_CPU_BAR_SIZE                              0xd28
#define PCI_1_CPU_BAR_SIZE                              0xda8
#define PCI_0_INTEGRATED_SRAM_BAR_SIZE                  0xe00
#define PCI_1_INTEGRATED_SRAM_BAR_SIZE                  0xe80
#define PCI_0_EXPANSION_ROM_BAR_SIZE                    0xd2c
#define PCI_1_EXPANSION_ROM_BAR_SIZE                    0xd9c
#define PCI_0_BASE_ADDR_REG_ENABLE                      0xc3c
#define PCI_1_BASE_ADDR_REG_ENABLE                      0xcbc
#define PCI_0_CS_0_BASE_ADDR_REMAP		        0xc48
#define PCI_1_CS_0_BASE_ADDR_REMAP		        0xcc8
#define PCI_0_CS_1_BASE_ADDR_REMAP		        0xd48
#define PCI_1_CS_1_BASE_ADDR_REMAP		        0xdc8
#define PCI_0_CS_2_BASE_ADDR_REMAP		        0xc4c
#define PCI_1_CS_2_BASE_ADDR_REMAP		        0xccc
#define PCI_0_CS_3_BASE_ADDR_REMAP		        0xd4c
#define PCI_1_CS_3_BASE_ADDR_REMAP		        0xdcc
#define PCI_0_CS_0_BASE_HIGH_ADDR_REMAP		        0xF04
#define PCI_1_CS_0_BASE_HIGH_ADDR_REMAP		        0xF84
#define PCI_0_CS_1_BASE_HIGH_ADDR_REMAP		        0xF08
#define PCI_1_CS_1_BASE_HIGH_ADDR_REMAP		        0xF88
#define PCI_0_CS_2_BASE_HIGH_ADDR_REMAP		        0xF0C
#define PCI_1_CS_2_BASE_HIGH_ADDR_REMAP		        0xF8C
#define PCI_0_CS_3_BASE_HIGH_ADDR_REMAP		        0xF10
#define PCI_1_CS_3_BASE_HIGH_ADDR_REMAP		        0xF90
#define PCI_0_DEVCS_0_BASE_ADDR_REMAP		        0xc50
#define PCI_1_DEVCS_0_BASE_ADDR_REMAP		        0xcd0
#define PCI_0_DEVCS_1_BASE_ADDR_REMAP		        0xd50
#define PCI_1_DEVCS_1_BASE_ADDR_REMAP		        0xdd0
#define PCI_0_DEVCS_2_BASE_ADDR_REMAP		        0xd58
#define PCI_1_DEVCS_2_BASE_ADDR_REMAP		        0xdd8
#define PCI_0_DEVCS_3_BASE_ADDR_REMAP           	0xc54
#define PCI_1_DEVCS_3_BASE_ADDR_REMAP           	0xcd4
#define PCI_0_DEVCS_BOOTCS_BASE_ADDR_REMAP      	0xd54
#define PCI_1_DEVCS_BOOTCS_BASE_ADDR_REMAP      	0xdd4
#define PCI_0_P2P_MEM0_BASE_ADDR_REMAP_LOW              0xd5c
#define PCI_1_P2P_MEM0_BASE_ADDR_REMAP_LOW              0xddc
#define PCI_0_P2P_MEM0_BASE_ADDR_REMAP_HIGH             0xd60
#define PCI_1_P2P_MEM0_BASE_ADDR_REMAP_HIGH             0xde0
#define PCI_0_P2P_MEM1_BASE_ADDR_REMAP_LOW              0xd64
#define PCI_1_P2P_MEM1_BASE_ADDR_REMAP_LOW              0xde4
#define PCI_0_P2P_MEM1_BASE_ADDR_REMAP_HIGH             0xd68
#define PCI_1_P2P_MEM1_BASE_ADDR_REMAP_HIGH             0xde8
#define PCI_0_P2P_I_O_BASE_ADDR_REMAP                   0xd6c
#define PCI_1_P2P_I_O_BASE_ADDR_REMAP                   0xdec 
#define PCI_0_CPU_BASE_ADDR_REMAP_LOW                   0xd70
#define PCI_1_CPU_BASE_ADDR_REMAP_LOW                   0xdf0
#define PCI_0_CPU_BASE_ADDR_REMAP_HIGH                  0xd74
#define PCI_1_CPU_BASE_ADDR_REMAP_HIGH                  0xdf4
#define PCI_0_INTEGRATED_SRAM_BASE_ADDR_REMAP           0xf00
#define PCI_1_INTEGRATED_SRAM_BASE_ADDR_REMAP           0xf80
#define PCI_0_EXPANSION_ROM_BASE_ADDR_REMAP             0xf38
#define PCI_1_EXPANSION_ROM_BASE_ADDR_REMAP             0xfb8
#define PCI_0_ADDR_DECODE_CONTROL                       0xd3c
#define PCI_1_ADDR_DECODE_CONTROL                       0xdbc
#define PCI_0_HEADERS_RETARGET_CONTROL                  0xF40
#define PCI_1_HEADERS_RETARGET_CONTROL                  0xFc0
#define PCI_0_HEADERS_RETARGET_BASE                     0xF44
#define PCI_1_HEADERS_RETARGET_BASE                     0xFc4
#define PCI_0_HEADERS_RETARGET_HIGH                     0xF48
#define PCI_1_HEADERS_RETARGET_HIGH                     0xFc8

/***********************************/
/*   PCI Control Register Map      */
/***********************************/

#define PCI_0_DLL_STATUS_AND_COMMAND                    0x1d20
#define PCI_1_DLL_STATUS_AND_COMMAND                    0x1da0
#define PCI_0_MPP_PADS_DRIVE_CONTROL                    0x1d1C
#define PCI_1_MPP_PADS_DRIVE_CONTROL                    0x1d9C
#define PCI_0_COMMAND					0xc00
#define PCI_1_COMMAND					0xc80
#define PCI_0_MODE                                      0xd00
#define PCI_1_MODE                                      0xd80
#define PCI_0_RETRY	        			0xc04
#define PCI_1_RETRY				       	0xc84
#define PCI_0_READ_BUFFER_DISCARD_TIMER                 0xd04
#define PCI_1_READ_BUFFER_DISCARD_TIMER                 0xd84
#define PCI_0_MSI_TRIGGER_TIMER                         0xc38
#define PCI_1_MSI_TRIGGER_TIMER                         0xcb8
#define PCI_0_ARBITER_CONTROL                           0x1d00
#define PCI_1_ARBITER_CONTROL                           0x1d80
#define PCI_0_CROSS_BAR_CONTROL_LOW                     0x1d08
#define PCI_1_CROSS_BAR_CONTROL_LOW                     0x1d88
#define PCI_0_CROSS_BAR_CONTROL_HIGH                    0x1d0c
#define PCI_1_CROSS_BAR_CONTROL_HIGH                    0x1d8c
#define PCI_0_CROSS_BAR_TIMEOUT                         0x1d04
#define PCI_1_CROSS_BAR_TIMEOUT                         0x1d84
#define PCI_0_SYNC_BARRIER_TRIGGER_REG                  0x1D18
#define PCI_1_SYNC_BARRIER_TRIGGER_REG                  0x1D98
#define PCI_0_SYNC_BARRIER_VIRTUAL_REG                  0x1d10
#define PCI_1_SYNC_BARRIER_VIRTUAL_REG                  0x1d90
#define PCI_0_P2P_CONFIG                                0x1d14
#define PCI_1_P2P_CONFIG                                0x1d94

#define PCI_0_ACCESS_CONTROL_BASE_0_LOW                 0x1e00
#define PCI_0_ACCESS_CONTROL_BASE_0_HIGH                0x1e04
#define PCI_0_ACCESS_CONTROL_SIZE_0                     0x1e08
#define PCI_0_ACCESS_CONTROL_BASE_1_LOW                 0x1e10
#define PCI_0_ACCESS_CONTROL_BASE_1_HIGH                0x1e14
#define PCI_0_ACCESS_CONTROL_SIZE_1                     0x1e18
#define PCI_0_ACCESS_CONTROL_BASE_2_LOW                 0x1e20
#define PCI_0_ACCESS_CONTROL_BASE_2_HIGH                0x1e24
#define PCI_0_ACCESS_CONTROL_SIZE_2                     0x1e28
#define PCI_0_ACCESS_CONTROL_BASE_3_LOW                 0x1e30
#define PCI_0_ACCESS_CONTROL_BASE_3_HIGH                0x1e34
#define PCI_0_ACCESS_CONTROL_SIZE_3                     0x1e38
#define PCI_0_ACCESS_CONTROL_BASE_4_LOW                 0x1e40
#define PCI_0_ACCESS_CONTROL_BASE_4_HIGH                0x1e44
#define PCI_0_ACCESS_CONTROL_SIZE_4                     0x1e48
#define PCI_0_ACCESS_CONTROL_BASE_5_LOW                 0x1e50
#define PCI_0_ACCESS_CONTROL_BASE_5_HIGH                0x1e54
#define PCI_0_ACCESS_CONTROL_SIZE_5                     0x1e58

#define PCI_1_ACCESS_CONTROL_BASE_0_LOW                 0x1e80
#define PCI_1_ACCESS_CONTROL_BASE_0_HIGH                0x1e84
#define PCI_1_ACCESS_CONTROL_SIZE_0                     0x1e88
#define PCI_1_ACCESS_CONTROL_BASE_1_LOW                 0x1e90
#define PCI_1_ACCESS_CONTROL_BASE_1_HIGH                0x1e94
#define PCI_1_ACCESS_CONTROL_SIZE_1                     0x1e98
#define PCI_1_ACCESS_CONTROL_BASE_2_LOW                 0x1ea0
#define PCI_1_ACCESS_CONTROL_BASE_2_HIGH                0x1ea4
#define PCI_1_ACCESS_CONTROL_SIZE_2                     0x1ea8
#define PCI_1_ACCESS_CONTROL_BASE_3_LOW                 0x1eb0
#define PCI_1_ACCESS_CONTROL_BASE_3_HIGH                0x1eb4
#define PCI_1_ACCESS_CONTROL_SIZE_3                     0x1eb8
#define PCI_1_ACCESS_CONTROL_BASE_4_LOW                 0x1ec0
#define PCI_1_ACCESS_CONTROL_BASE_4_HIGH                0x1ec4
#define PCI_1_ACCESS_CONTROL_SIZE_4                     0x1ec8
#define PCI_1_ACCESS_CONTROL_BASE_5_LOW                 0x1ed0
#define PCI_1_ACCESS_CONTROL_BASE_5_HIGH                0x1ed4
#define PCI_1_ACCESS_CONTROL_SIZE_5                     0x1ed8

/****************************************/
/*   PCI Configuration Access Registers */
/****************************************/

#define PCI_0_CONFIG_ADDR 		                0xcf8
#define PCI_0_CONFIG_DATA_VIRTUAL_REG                   0xcfc
#define PCI_1_CONFIG_ADDR 		                0xc78
#define PCI_1_CONFIG_DATA_VIRTUAL_REG                   0xc7c
#define PCI_0_INTERRUPT_ACKNOWLEDGE_VIRTUAL_REG	        0xc34
#define PCI_1_INTERRUPT_ACKNOWLEDGE_VIRTUAL_REG	        0xcb4

/****************************************/
/*   PCI Error Report Registers         */
/****************************************/

#define PCI_0_SERR_MASK					0xc28
#define PCI_1_SERR_MASK					0xca8
#define PCI_0_ERROR_ADDR_LOW                            0x1d40
#define PCI_1_ERROR_ADDR_LOW                            0x1dc0
#define PCI_0_ERROR_ADDR_HIGH                           0x1d44
#define PCI_1_ERROR_ADDR_HIGH                           0x1dc4
#define PCI_0_ERROR_ATTRIBUTE                           0x1d48
#define PCI_1_ERROR_ATTRIBUTE                           0x1dc8
#define PCI_0_ERROR_COMMAND                             0x1d50
#define PCI_1_ERROR_COMMAND                             0x1dd0
#define PCI_0_ERROR_CAUSE                               0x1d58
#define PCI_1_ERROR_CAUSE                               0x1dd8
#define PCI_0_ERROR_MASK                                0x1d5c
#define PCI_1_ERROR_MASK                                0x1ddc

/****************************************/
/*   PCI Debug Registers                */
/****************************************/

#define PCI_0_MMASK                                     0X1D24
#define PCI_1_MMASK                                     0X1DA4

/*********************************************/
/* PCI Configuration, Function 0, Registers  */
/*********************************************/

#define PCI_DEVICE_AND_VENDOR_ID 			0x000
#define PCI_STATUS_AND_COMMAND				0x004
#define PCI_CLASS_CODE_AND_REVISION_ID		        0x008
#define PCI_BIST_HEADER_TYPE_LATENCY_TIMER_CACHE_LINE	0x00C

#define PCI_SCS_0_BASE_ADDR_LOW   		        0x010
#define PCI_SCS_0_BASE_ADDR_HIGH   		        0x014
#define PCI_SCS_1_BASE_ADDR_LOW  		        0x018
#define PCI_SCS_1_BASE_ADDR_HIGH  		        0x01C
#define PCI_INTERNAL_REG_MEM_MAPPED_BASE_ADDR_LO       	0x020
#define PCI_INTERNAL_REG_MEM_MAPPED_BASE_ADDR_HIGH     	0x024
#define PCI_SUBSYSTEM_ID_AND_SUBSYSTEM_VENDOR_ID	0x02c
#define PCI_EXPANSION_ROM_BASE_ADDR_REG		        0x030
#define PCI_CAPABILTY_LIST_POINTER                      0x034
#define PCI_INTERRUPT_PIN_AND_LINE 			0x03C
       /* capability list */
#define PCI_POWER_MANAGEMENT_CAPABILITY                 0x040
#define PCI_POWER_MANAGEMENT_STATUS_AND_CONTROL         0x044
#define PCI_VPD_ADDR                                    0x048
#define PCI_VPD_DATA                                    0x04c
#define PCI_MSI_MESSAGE_CONTROL                         0x050
#define PCI_MSI_MESSAGE_ADDR                            0x054
#define PCI_MSI_MESSAGE_UPPER_ADDR                      0x058
#define PCI_MSI_MESSAGE_DATA                            0x05c
#define PCI_X_COMMAND                                   0x060
#define PCI_X_STATUS                                    0x064
#define PCI_COMPACT_PCI_HOT_SWAP                        0x068

/***********************************************/
/*   PCI Configuration, Function 1, Registers  */
/***********************************************/

#define PCI_SCS_2_BASE_ADDR_LOW   			0x110
#define PCI_SCS_2_BASE_ADDR_HIGH			0x114
#define PCI_SCS_3_BASE_ADDR_LOW 			0x118
#define PCI_SCS_3_BASE_ADDR_HIGH			0x11c
#define PCI_INTERGRATED_SRAM_BASE_ADDR_LOW          	0x120
#define PCI_INTERGRATED_SRAM_BASE_ADDR_HIGH         	0x124

/***********************************************/
/*  PCI Configuration, Function 2, Registers   */
/***********************************************/

#define PCI_DEVCS_0_BASE_ADDR_LOW	    		0x210
#define PCI_DEVCS_0_BASE_ADDR_HIGH 			0x214
#define PCI_DEVCS_1_BASE_ADDR_LOW 			0x218
#define PCI_DEVCS_1_BASE_ADDR_HIGH      		0x21c
#define PCI_DEVCS_2_BASE_ADDR_LOW 			0x220
#define PCI_DEVCS_2_BASE_ADDR_HIGH      		0x224

/***********************************************/
/*  PCI Configuration, Function 3, Registers   */
/***********************************************/

#define PCI_DEVCS_3_BASE_ADDR_LOW	    		0x310
#define PCI_DEVCS_3_BASE_ADDR_HIGH 			0x314
#define PCI_BOOT_CS_BASE_ADDR_LOW			0x318
#define PCI_BOOT_CS_BASE_ADDR_HIGH      		0x31c
#define PCI_CPU_BASE_ADDR_LOW 				0x220
#define PCI_CPU_BASE_ADDR_HIGH      			0x224

/***********************************************/
/*  PCI Configuration, Function 4, Registers   */
/***********************************************/

#define PCI_P2P_MEM0_BASE_ADDR_LOW  			0x410
#define PCI_P2P_MEM0_BASE_ADDR_HIGH 			0x414
#define PCI_P2P_MEM1_BASE_ADDR_LOW   			0x418
#define PCI_P2P_MEM1_BASE_ADDR_HIGH 			0x41c
#define PCI_P2P_I_O_BASE_ADDR                 	        0x420
#define PCI_INTERNAL_REGS_I_O_MAPPED_BASE_ADDR          0x424

/****************************************/
/* Messaging Unit Registers (I20)   	*/
/****************************************/

#define I2O_INBOUND_MESSAGE_REG0_PCI_0_SIDE		0x010
#define I2O_INBOUND_MESSAGE_REG1_PCI_0_SIDE  		0x014
#define I2O_OUTBOUND_MESSAGE_REG0_PCI_0_SIDE 		0x018
#define I2O_OUTBOUND_MESSAGE_REG1_PCI_0_SIDE  		0x01C
#define I2O_INBOUND_DOORBELL_REG_PCI_0_SIDE  		0x020
#define I2O_INBOUND_INTERRUPT_CAUSE_REG_PCI_0_SIDE      0x024
#define I2O_INBOUND_INTERRUPT_MASK_REG_PCI_0_SIDE	0x028
#define I2O_OUTBOUND_DOORBELL_REG_PCI_0_SIDE 		0x02C
#define I2O_OUTBOUND_INTERRUPT_CAUSE_REG_PCI_0_SIDE     0x030
#define I2O_OUTBOUND_INTERRUPT_MASK_REG_PCI_0_SIDE      0x034
#define I2O_INBOUND_QUEUE_PORT_VIRTUAL_REG_PCI_0_SIDE   0x040
#define I2O_OUTBOUND_QUEUE_PORT_VIRTUAL_REG_PCI_0_SIDE  0x044
#define I2O_QUEUE_CONTROL_REG_PCI_0_SIDE 		0x050
#define I2O_QUEUE_BASE_ADDR_REG_PCI_0_SIDE 		0x054
#define I2O_INBOUND_FREE_HEAD_POINTER_REG_PCI_0_SIDE    0x060
#define I2O_INBOUND_FREE_TAIL_POINTER_REG_PCI_0_SIDE    0x064
#define I2O_INBOUND_POST_HEAD_POINTER_REG_PCI_0_SIDE    0x068
#define I2O_INBOUND_POST_TAIL_POINTER_REG_PCI_0_SIDE    0x06C
#define I2O_OUTBOUND_FREE_HEAD_POINTER_REG_PCI_0_SIDE   0x070
#define I2O_OUTBOUND_FREE_TAIL_POINTER_REG_PCI_0_SIDE   0x074
#define I2O_OUTBOUND_POST_HEAD_POINTER_REG_PCI_0_SIDE   0x0F8
#define I2O_OUTBOUND_POST_TAIL_POINTER_REG_PCI_0_SIDE   0x0FC

#define I2O_INBOUND_MESSAGE_REG0_PCI_1_SIDE		0x090
#define I2O_INBOUND_MESSAGE_REG1_PCI_1_SIDE  		0x094
#define I2O_OUTBOUND_MESSAGE_REG0_PCI_1_SIDE 		0x098
#define I2O_OUTBOUND_MESSAGE_REG1_PCI_1_SIDE  		0x09C
#define I2O_INBOUND_DOORBELL_REG_PCI_1_SIDE  		0x0A0
#define I2O_INBOUND_INTERRUPT_CAUSE_REG_PCI_1_SIDE      0x0A4
#define I2O_INBOUND_INTERRUPT_MASK_REG_PCI_1_SIDE	0x0A8
#define I2O_OUTBOUND_DOORBELL_REG_PCI_1_SIDE 		0x0AC
#define I2O_OUTBOUND_INTERRUPT_CAUSE_REG_PCI_1_SIDE     0x0B0
#define I2O_OUTBOUND_INTERRUPT_MASK_REG_PCI_1_SIDE      0x0B4
#define I2O_INBOUND_QUEUE_PORT_VIRTUAL_REG_PCI_1_SIDE   0x0C0
#define I2O_OUTBOUND_QUEUE_PORT_VIRTUAL_REG_PCI_1_SIDE  0x0C4
#define I2O_QUEUE_CONTROL_REG_PCI_1_SIDE 		0x0D0
#define I2O_QUEUE_BASE_ADDR_REG_PCI_1_SIDE 		0x0D4
#define I2O_INBOUND_FREE_HEAD_POINTER_REG_PCI_1_SIDE    0x0E0
#define I2O_INBOUND_FREE_TAIL_POINTER_REG_PCI_1_SIDE    0x0E4
#define I2O_INBOUND_POST_HEAD_POINTER_REG_PCI_1_SIDE    0x0E8
#define I2O_INBOUND_POST_TAIL_POINTER_REG_PCI_1_SIDE    0x0EC
#define I2O_OUTBOUND_FREE_HEAD_POINTER_REG_PCI_1_SIDE   0x0F0
#define I2O_OUTBOUND_FREE_TAIL_POINTER_REG_PCI_1_SIDE   0x0F4
#define I2O_OUTBOUND_POST_HEAD_POINTER_REG_PCI_1_SIDE   0x078
#define I2O_OUTBOUND_POST_TAIL_POINTER_REG_PCI_1_SIDE   0x07C

#define I2O_INBOUND_MESSAGE_REG0_CPU0_SIDE		0x1C10
#define I2O_INBOUND_MESSAGE_REG1_CPU0_SIDE  		0x1C14
#define I2O_OUTBOUND_MESSAGE_REG0_CPU0_SIDE 		0x1C18
#define I2O_OUTBOUND_MESSAGE_REG1_CPU0_SIDE  		0x1C1C
#define I2O_INBOUND_DOORBELL_REG_CPU0_SIDE  		0x1C20
#define I2O_INBOUND_INTERRUPT_CAUSE_REG_CPU0_SIDE  	0x1C24
#define I2O_INBOUND_INTERRUPT_MASK_REG_CPU0_SIDE	0x1C28
#define I2O_OUTBOUND_DOORBELL_REG_CPU0_SIDE 		0x1C2C
#define I2O_OUTBOUND_INTERRUPT_CAUSE_REG_CPU0_SIDE      0x1C30
#define I2O_OUTBOUND_INTERRUPT_MASK_REG_CPU0_SIDE       0x1C34
#define I2O_INBOUND_QUEUE_PORT_VIRTUAL_REG_CPU0_SIDE    0x1C40
#define I2O_OUTBOUND_QUEUE_PORT_VIRTUAL_REG_CPU0_SIDE   0x1C44
#define I2O_QUEUE_CONTROL_REG_CPU0_SIDE 		0x1C50
#define I2O_QUEUE_BASE_ADDR_REG_CPU0_SIDE 		0x1C54
#define I2O_INBOUND_FREE_HEAD_POINTER_REG_CPU0_SIDE     0x1C60
#define I2O_INBOUND_FREE_TAIL_POINTER_REG_CPU0_SIDE     0x1C64
#define I2O_INBOUND_POST_HEAD_POINTER_REG_CPU0_SIDE     0x1C68
#define I2O_INBOUND_POST_TAIL_POINTER_REG_CPU0_SIDE     0x1C6C
#define I2O_OUTBOUND_FREE_HEAD_POINTER_REG_CPU0_SIDE    0x1C70
#define I2O_OUTBOUND_FREE_TAIL_POINTER_REG_CPU0_SIDE    0x1C74
#define I2O_OUTBOUND_POST_HEAD_POINTER_REG_CPU0_SIDE    0x1CF8
#define I2O_OUTBOUND_POST_TAIL_POINTER_REG_CPU0_SIDE    0x1CFC
#define I2O_INBOUND_MESSAGE_REG0_CPU1_SIDE		0x1C90
#define I2O_INBOUND_MESSAGE_REG1_CPU1_SIDE  		0x1C94
#define I2O_OUTBOUND_MESSAGE_REG0_CPU1_SIDE 		0x1C98
#define I2O_OUTBOUND_MESSAGE_REG1_CPU1_SIDE  		0x1C9C
#define I2O_INBOUND_DOORBELL_REG_CPU1_SIDE  		0x1CA0
#define I2O_INBOUND_INTERRUPT_CAUSE_REG_CPU1_SIDE  	0x1CA4
#define I2O_INBOUND_INTERRUPT_MASK_REG_CPU1_SIDE	0x1CA8
#define I2O_OUTBOUND_DOORBELL_REG_CPU1_SIDE 		0x1CAC
#define I2O_OUTBOUND_INTERRUPT_CAUSE_REG_CPU1_SIDE      0x1CB0
#define I2O_OUTBOUND_INTERRUPT_MASK_REG_CPU1_SIDE       0x1CB4
#define I2O_INBOUND_QUEUE_PORT_VIRTUAL_REG_CPU1_SIDE    0x1CC0
#define I2O_OUTBOUND_QUEUE_PORT_VIRTUAL_REG_CPU1_SIDE   0x1CC4
#define I2O_QUEUE_CONTROL_REG_CPU1_SIDE 		0x1CD0
#define I2O_QUEUE_BASE_ADDR_REG_CPU1_SIDE 		0x1CD4
#define I2O_INBOUND_FREE_HEAD_POINTER_REG_CPU1_SIDE     0x1CE0
#define I2O_INBOUND_FREE_TAIL_POINTER_REG_CPU1_SIDE     0x1CE4
#define I2O_INBOUND_POST_HEAD_POINTER_REG_CPU1_SIDE     0x1CE8
#define I2O_INBOUND_POST_TAIL_POINTER_REG_CPU1_SIDE     0x1CEC
#define I2O_OUTBOUND_FREE_HEAD_POINTER_REG_CPU1_SIDE    0x1CF0
#define I2O_OUTBOUND_FREE_TAIL_POINTER_REG_CPU1_SIDE    0x1CF4
#define I2O_OUTBOUND_POST_HEAD_POINTER_REG_CPU1_SIDE    0x1C78
#define I2O_OUTBOUND_POST_TAIL_POINTER_REG_CPU1_SIDE    0x1C7C

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

/*******************************************/
/*          CUNIT  Registers               */
/*******************************************/

         /* Address Decoding Register Map */
           
#define CUNIT_BASE_ADDR_REG0                                0xf200
#define CUNIT_BASE_ADDR_REG1                                0xf208
#define CUNIT_BASE_ADDR_REG2                                0xf210
#define CUNIT_BASE_ADDR_REG3                                0xf218
#define CUNIT_SIZE0                                         0xf204
#define CUNIT_SIZE1                                         0xf20c
#define CUNIT_SIZE2                                         0xf214
#define CUNIT_SIZE3                                         0xf21c
#define CUNIT_HIGH_ADDR_REMAP_REG0                          0xf240
#define CUNIT_HIGH_ADDR_REMAP_REG1                          0xf244
#define CUNIT_BASE_ADDR_ENABLE_REG                          0xf250
#define MPSC0_ACCESS_PROTECTION_REG                         0xf254
#define MPSC1_ACCESS_PROTECTION_REG                         0xf258
#define CUNIT_INTERNAL_SPACE_BASE_ADDR_REG                  0xf25C

        /*  Error Report Registers  */

#define CUNIT_INTERRUPT_CAUSE_REG                           0xf310
#define CUNIT_INTERRUPT_MASK_REG                            0xf314
#define CUNIT_ERROR_ADDR                                    0xf318

        /*  Cunit Control Registers */

#define CUNIT_ARBITER_CONTROL_REG                           0xf300
#define CUNIT_CONFIG_REG                                    0xb40c
#define CUNIT_CRROSBAR_TIMEOUT_REG                          0xf304

        /*  Cunit Debug Registers   */

#define CUNIT_DEBUG_LOW                                     0xf340
#define CUNIT_DEBUG_HIGH                                    0xf344
#define CUNIT_MMASK                                         0xf380

        /*  MPSCs Clocks Routing Registers  */

#define MPSC_ROUTING_REG                                    0xb400
#define MPSC_RX_CLOCK_ROUTING_REG                           0xb404
#define MPSC_TX_CLOCK_ROUTING_REG                           0xb408

        /*  MPSCs Interrupts Registers    */

#define MPSC_CAUSE_REG(port)                           (0xb804 + (port<<3))
#define MPSC_MASK_REG(port)                            (0xb884 + (port<<3))

#define MPSC_MAIN_CONFIG_LOW(port)                     (0x8000 + (port<<12))
#define MPSC_MAIN_CONFIG_HIGH(port)                    (0x8004 + (port<<12))
#define MPSC_PROTOCOL_CONFIG(port)                     (0x8008 + (port<<12))
#define MPSC_CHANNEL_REG1(port)                        (0x800c + (port<<12))
#define MPSC_CHANNEL_REG2(port)                        (0x8010 + (port<<12))
#define MPSC_CHANNEL_REG3(port)                        (0x8014 + (port<<12))
#define MPSC_CHANNEL_REG4(port)                        (0x8018 + (port<<12))
#define MPSC_CHANNEL_REG5(port)                        (0x801c + (port<<12))
#define MPSC_CHANNEL_REG6(port)                        (0x8020 + (port<<12))
#define MPSC_CHANNEL_REG7(port)                        (0x8024 + (port<<12))
#define MPSC_CHANNEL_REG8(port)                        (0x8028 + (port<<12))
#define MPSC_CHANNEL_REG9(port)                        (0x802c + (port<<12))
#define MPSC_CHANNEL_REG10(port)                       (0x8030 + (port<<12))
        
        /*  MPSC0 Registers      */


/***************************************/
/*          SDMA Registers             */
/***************************************/

#define SDMA_CONFIG_REG(channel)                       (0x4000 + (channel<<13))
#define SDMA_COMMAND_REG(channel)                      (0x4008 + (channel<<13))
#define SDMA_CURRENT_RX_DESCRIPTOR_POINTER(channel)    (0x4810 + (channel<<13))
#define SDMA_CURRENT_TX_DESCRIPTOR_POINTER(channel)    (0x4c10 + (channel<<13)) 
#define SDMA_FIRST_TX_DESCRIPTOR_POINTER(channel)      (0x4c14 + (channel<<13))

#define SDMA_CAUSE_REG                                  0xb800
#define SDMA_MASK_REG                                   0xb880
         
/* BRG Interrupts */

#define BRG_CONFIG_REG(brg)                             (0xb200 + (brg<<8))
#define BRG_BAUDE_TUNING_REG(brg)                       (0xb208 + (brg<<8))
#define BRG_CAUSE_REG                                   0xb834
#define BRG_MASK_REG                                    0xb8b4

/****************************************/
/* DMA Channel Control					*/
/****************************************/

#define DMA_CHANNEL0_CONTROL 				0x840
#define DMA_CHANNEL0_CONTROL_HIGH			0x880
#define DMA_CHANNEL1_CONTROL 				0x844
#define DMA_CHANNEL1_CONTROL_HIGH			0x884
#define DMA_CHANNEL2_CONTROL 				0x848
#define DMA_CHANNEL2_CONTROL_HIGH			0x888
#define DMA_CHANNEL3_CONTROL 				0x84C
#define DMA_CHANNEL3_CONTROL_HIGH			0x88C


/****************************************/
/*           IDMA Registers             */
/****************************************/

#define DMA_CHANNEL0_BYTE_COUNT                         0x800
#define DMA_CHANNEL1_BYTE_COUNT                         0x804
#define DMA_CHANNEL2_BYTE_COUNT                         0x808
#define DMA_CHANNEL3_BYTE_COUNT                         0x80C
#define DMA_CHANNEL0_SOURCE_ADDR                        0x810
#define DMA_CHANNEL1_SOURCE_ADDR                        0x814
#define DMA_CHANNEL2_SOURCE_ADDR                        0x818
#define DMA_CHANNEL3_SOURCE_ADDR                        0x81c
#define DMA_CHANNEL0_DESTINATION_ADDR                   0x820
#define DMA_CHANNEL1_DESTINATION_ADDR                   0x824
#define DMA_CHANNEL2_DESTINATION_ADDR                   0x828
#define DMA_CHANNEL3_DESTINATION_ADDR                   0x82C
#define DMA_CHANNEL0_NEXT_DESCRIPTOR_POINTER            0x830
#define DMA_CHANNEL1_NEXT_DESCRIPTOR_POINTER            0x834
#define DMA_CHANNEL2_NEXT_DESCRIPTOR_POINTER            0x838
#define DMA_CHANNEL3_NEXT_DESCRIPTOR_POINTER            0x83C
#define DMA_CHANNEL0_CURRENT_DESCRIPTOR_POINTER         0x870
#define DMA_CHANNEL1_CURRENT_DESCRIPTOR_POINTER         0x874
#define DMA_CHANNEL2_CURRENT_DESCRIPTOR_POINTER         0x878
#define DMA_CHANNEL3_CURRENT_DESCRIPTOR_POINTER         0x87C

 /*  IDMA Address Decoding Base Address Registers  */
 
#define DMA_BASE_ADDR_REG0                              0xa00
#define DMA_BASE_ADDR_REG1                              0xa08
#define DMA_BASE_ADDR_REG2                              0xa10
#define DMA_BASE_ADDR_REG3                              0xa18
#define DMA_BASE_ADDR_REG4                              0xa20
#define DMA_BASE_ADDR_REG5                              0xa28
#define DMA_BASE_ADDR_REG6                              0xa30
#define DMA_BASE_ADDR_REG7                              0xa38

 /*  IDMA Address Decoding Size Address Register   */
 
#define DMA_SIZE_REG0                                   0xa04
#define DMA_SIZE_REG1                                   0xa0c
#define DMA_SIZE_REG2                                   0xa14
#define DMA_SIZE_REG3                                   0xa1c
#define DMA_SIZE_REG4                                   0xa24
#define DMA_SIZE_REG5                                   0xa2c
#define DMA_SIZE_REG6                                   0xa34
#define DMA_SIZE_REG7                                   0xa3C

 /* IDMA Address Decoding High Address Remap and Access 
                  Protection Registers                    */
                  
#define DMA_HIGH_ADDR_REMAP_REG0                        0xa60
#define DMA_HIGH_ADDR_REMAP_REG1                        0xa64
#define DMA_HIGH_ADDR_REMAP_REG2                        0xa68
#define DMA_HIGH_ADDR_REMAP_REG3                        0xa6C
#define DMA_BASE_ADDR_ENABLE_REG                        0xa80
#define DMA_CHANNEL0_ACCESS_PROTECTION_REG              0xa70
#define DMA_CHANNEL1_ACCESS_PROTECTION_REG              0xa74
#define DMA_CHANNEL2_ACCESS_PROTECTION_REG              0xa78
#define DMA_CHANNEL3_ACCESS_PROTECTION_REG              0xa7c
#define DMA_ARBITER_CONTROL                             0x860
#define DMA_CROSS_BAR_TIMEOUT                           0x8d0

 /*  IDMA Headers Retarget Registers   */

#define DMA_HEADERS_RETARGET_CONTROL                    0xa84
#define DMA_HEADERS_RETARGET_BASE                       0xa88

 /*  IDMA Interrupt Register  */

#define DMA_INTERRUPT_CAUSE_REG                         0x8c0
#define DMA_INTERRUPT_CAUSE_MASK                        0x8c4
#define DMA_ERROR_ADDR                                  0x8c8
#define DMA_ERROR_SELECT                                0x8cc

 /*  IDMA Debug Register ( for internal use )    */

#define DMA_DEBUG_LOW                                   0x8e0
#define DMA_DEBUG_HIGH                                  0x8e4
#define DMA_SPARE                                       0xA8C

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
/*         Watchdog registers     	*/
/****************************************/

#define WATCHDOG_CONFIG_REG                             0xb410
#define WATCHDOG_VALUE_REG                              0xb414

/****************************************/
/* I2C Registers                        */
/****************************************/

#define I2C_SLAVE_ADDR                                  0xc000
#define I2C_EXTENDED_SLAVE_ADDR                         0xc010
#define I2C_DATA                                        0xc004
#define I2C_CONTROL                                     0xc008
#define I2C_STATUS_BAUDE_RATE                           0xc00C
#define I2C_SOFT_RESET                                  0xc01c

/****************************************/
/* GPP Interface Registers              */
/****************************************/

#define GPP_IO_CONTROL                                  0xf100
#define GPP_LEVEL_CONTROL                               0xf110
#define GPP_VALUE                                       0xf104
#define GPP_INTERRUPT_CAUSE                             0xf108
#define GPP_INTERRUPT_MASK0                             0xf10c
#define GPP_INTERRUPT_MASK1                             0xf114
#define GPP_VALUE_SET                                   0xf118
#define GPP_VALUE_CLEAR                                 0xf11c

/****************************************/
/* Interrupt Controller Registers       */
/****************************************/

/****************************************/
/* Interrupts	  			*/
/****************************************/

#define MAIN_INTERRUPT_CAUSE_LOW                        0x004
#define MAIN_INTERRUPT_CAUSE_HIGH                       0x00c
#define CPU_INTERRUPT0_MASK_LOW                         0x014
#define CPU_INTERRUPT0_MASK_HIGH                        0x01c
#define CPU_INTERRUPT0_SELECT_CAUSE                     0x024
#define CPU_INTERRUPT1_MASK_LOW                         0x034
#define CPU_INTERRUPT1_MASK_HIGH                        0x03c
#define CPU_INTERRUPT1_SELECT_CAUSE                     0x044
#define INTERRUPT0_MASK_0_LOW                           0x054
#define INTERRUPT0_MASK_0_HIGH                          0x05c
#define INTERRUPT0_SELECT_CAUSE                         0x064
#define INTERRUPT1_MASK_0_LOW                           0x074
#define INTERRUPT1_MASK_0_HIGH                          0x07c
#define INTERRUPT1_SELECT_CAUSE                         0x084

/****************************************/
/*      MPP Interface Registers         */
/****************************************/

#define MPP_CONTROL0                                    0xf000
#define MPP_CONTROL1                                    0xf004
#define MPP_CONTROL2                                    0xf008
#define MPP_CONTROL3                                    0xf00c

/****************************************/
/*    Serial Initialization registers   */
/****************************************/

#define SERIAL_INIT_LAST_DATA                           0xf324
#define SERIAL_INIT_CONTROL                             0xf328
#define SERIAL_INIT_STATUS                              0xf32c


#endif /* _MV64360REG_H_ */
