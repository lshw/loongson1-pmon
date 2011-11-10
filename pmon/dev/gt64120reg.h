/*	$Id: gt64120reg.h,v 1.1.1.1 2006/09/14 01:59:08 root Exp $ */

/*
 * Copyright (c) 2001-2002 Opsycon AB  (www.opsycon.se / www.opsycon.com)
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
/*
 * gt64120.h:  Galileo Technology 64-bit system controller with SDRAM & PCI
 *
 *  modified from gt64011.h by Laurin McLaurin, Quantum Effect Design, Inc.
 *  on April/May 1998.
 *
 */

#define	GT64120_BASE_DEFAULT 0x14000000

#ifndef _LOCORE
/* offsets from base pointer, this construct allows optimisation */
static char * const _gt64120p = (char *)PHYS_TO_UNCACHED(GT64120_BASE);

#define GT64120(x) *(volatile unsigned long *)(_gt64120p + (x))
#else
#define GT64120(x) (x)
#endif

/* 
 *  CPU configuration 
 */

#define GT_CPU_CFG	      GT64120(0x000)

#define GT_CPU_CFG_CacheOpMap   0x1FF
#define GT_CPU_CFG_CachePres	(1<<9)
#define GT_CPU_CFG_VRXMode	(1<<10)
#define GT_CPU_CFG_WriteMode	(1<<11)
#define GT_CPU_CFG_Endianess	(1<<12)
#define GT_CPU_CFG_IntRegEnd	(1<<13)
#define GT_CPU_CFG_ExtL2Pres	(1<<14)
#define GT_CPU_CFG_ExtHitDLY	(1<<15)
#define GT_CPU_CFG_CPUWrRate	(1<<16)
#define GT_CPU_CFG_StopRetry	(1<<17)
#define GT_CPU_CFG_MultiGT	(1<<18)
#define GT_CPU_CFG_SysADCVld	(1<<19)

                                      /* Processor Address Space */
#define GT_PAS_RAS10LO	      GT64120(0x008)
#define GT_PAS_RAS10HI	      GT64120(0x010)
#define GT_PAS_RAS32LO	      GT64120(0x018)
#define GT_PAS_RAS32HI	      GT64120(0x020)
#define GT_PAS_CS20LO	      GT64120(0x028)
#define GT_PAS_CS20HI	      GT64120(0x030)
#define GT_PAS_CS3BOOTLO      GT64120(0x038)
#define GT_PAS_CS3BOOTHI      GT64120(0x040)

#define GT_PAS_PCI0IOLO	      GT64120(0x048)
#define GT_PAS_PCI0IOHI	      GT64120(0x050)
#define GT_PAS_PCI0MEM0LO     GT64120(0x058)
#define GT_PAS_PCI0MEM0HI     GT64120(0x060)
#define GT_PAS_PCI0MEM1LO     GT64120(0x080)
#define GT_PAS_PCI0MEM1HI     GT64120(0x088)

#define GT_PAS_PCI1IOLO	      GT64120(0x090)
#define GT_PAS_PCI1IOHI	      GT64120(0x098)
#define GT_PAS_PCI1MEM0LO     GT64120(0x0a0)
#define GT_PAS_PCI1MEM0HI     GT64120(0x0a8)
#define GT_PAS_PCI1MEM1LO     GT64120(0x0b0)
#define GT_PAS_PCI1MEM1HI     GT64120(0x0b8)

#define GT_PAS_INTDEC	      GT64120(0x068)
#define GT_PAS_BUSERRLO	      GT64120(0x070)
#define GT_PAS_BUSERRHI       GT64120(0x078)

#define GT_PAS_LOMASK_Low	0x7fff
#define GT_PAS_LOSHIFT_Low	0
#define GT_PAS_HIMASK_High	0x07f
#define GT_PAS_HISHIFT_High	0

#define GT_PAS_PCI0SBVR	      GT64120(0x0c0)
#define GT_PAS_PCI1SBVR	      GT64120(0x0c8)
#define GT_PAS_Ras10Rmap      GT64120(0x0d0)
#define GT_PAS_Ras32Rmap      GT64120(0x0d8)
#define GT_PAS_Cas20Rmap      GT64120(0x0e0)
#define GT_PAS_Cas3BootRmap   GT64120(0x0e8)
#define GT_PAS_PCI0IORmap     GT64120(0x0f0)
#define GT_PAS_PCI0MEM0Rmap   GT64120(0x0f8)
#define GT_PAS_PCI0MEM1Rmap   GT64120(0x100)
#define GT_PAS_PCI1IORmap     GT64120(0x108)
#define GT_PAS_PCI1MEM0Rmap   GT64120(0x110)
#define GT_PAS_PCI1MEM1Rmap   GT64120(0x118)

#define GT_PAS_RmapMASK	        0x7ff
#define GT_PAS_RmapSHIFT	0

                                      /* Multiple GT-64120 Support */
#define GT_PAS_MGTActive      GT64120(0x120)

#define GT_PAS_MGTActMask       0x3

                                      /* SDRAM and  Device Address Space */
#define GT_DDAS_RAS0LO	      GT64120(0x400)
#define GT_DDAS_RAS0HI	      GT64120(0x404)
#define GT_DDAS_RAS1LO	      GT64120(0x408)
#define GT_DDAS_RAS1HI	      GT64120(0x40c)
#define GT_DDAS_RAS2LO	      GT64120(0x410)
#define GT_DDAS_RAS2HI	      GT64120(0x414)
#define GT_DDAS_RAS3LO	      GT64120(0x418)
#define GT_DDAS_RAS3HI	      GT64120(0x41c)
#define GT_DDAS_CS0LO	      GT64120(0x420)
#define GT_DDAS_CS0HI	      GT64120(0x424)
#define GT_DDAS_CS1LO	      GT64120(0x428)
#define GT_DDAS_CS1HI	      GT64120(0x42c)
#define GT_DDAS_CS2LO	      GT64120(0x430)
#define GT_DDAS_CS2HI	      GT64120(0x434)
#define GT_DDAS_CS3LO	      GT64120(0x438)
#define GT_DDAS_CS3HI	      GT64120(0x43c)
#define GT_DDAS_BOOTCSLO      GT64120(0x440)
#define GT_DDAS_BOOTCSHI      GT64120(0x444)
#define GT_DDAS_ERROR	      GT64120(0x470)

#define GT_DDAS_LOMASK_Low	0xff
#define GT_DDAS_LOSHIFT_Low	0
#define GT_DDAS_HIMASK_High	0xff
#define GT_DDAS_HISHIFT_High	0

                                            /* SDRAM Configuration */
#define GT_DRAM_CFG	      GT64120(0x448)

#define GT_DRAM_CFG_RefIntCntMASK	0x00003fff
#define GT_DRAM_CFG_RefIntCntSHIFT	0
#define GT_DRAM_CFG_RefIntCnt(x)	(((x)<<GT_DRAM_CFG_RefIntCntSHIFT)&\
					 GT_DRAM_CFG_RefIntCntMASK)
#define GT_DRAM_CFG_Interleave		(1<<14)
#define GT_DRAM_CFG_InterleaveOn        0
#define GT_DRAM_CFG_InterleaveOff       GT_DRAM_CFG_Interleave
#define GT_DRAM_CFG_RdModWrt		(1<<15)
#define GT_DRAM_CFG_RdModWrtOff         0
#define GT_DRAM_CFG_RdModWrtOn          GT_DRAM_CFG_RdModWrt          
#define GT_DRAM_CFG_StagRef		(1<<16)
#define GT_DRAM_CFG_StagRefOn		0
#define GT_DRAM_CFG_StagRefAll		GT_DRAM_CFG_StagRef
#define GT_DRAM_CFG_CPUtoDRAMErr	(1<<17)
#define GT_DRAM_CFG_CPUtoDRAMErrOn	0
#define GT_DRAM_CFG_CPUtoDRAMErrOff	GT_DRAM_CFG_CPUtoDRAMErr
#define GT_DRAM_CFG_ECCInt		(1<<18)
#define GT_DRAM_CFG_ECCIntOn1		GT_DRAM_CFG_ECCInt
#define GT_DRAM_CFG_ECCIntOn2		0
#define GT_DRAM_CFG_DupCntl		(1<<19)
#define GT_DRAM_CFG_DupCntlOff		0
#define GT_DRAM_CFG_DupCntOn		GT_DRAM_CFG_DupCntl
#define GT_DRAM_CFG_DupBA		(1<<20)
#define GT_DRAM_CFG_DupBAOff		0
#define GT_DRAM_CFG_DupBAOn		GT_DRAM_CFG_DupBA
#define GT_DRAM_CFG_DupEOT0		(1<<21)
#define GT_DRAM_CFG_DupEOT0Off		0
#define GT_DRAM_CFG_DupEOT0On		GT_DRAM_CFG_DupEOT0
#define GT_DRAM_CFG_DupEOT1		(1<<22)
#define GT_DRAM_CFG_DupEOT1Off		0
#define GT_DRAM_CFG_DupEOT1On		GT_DRAM_CFG_DupEOT1
#define GT_DRAM_CFG_RegSDRAM		(1<<23)
#define GT_DRAM_CFG_RegSDRAMOn		GT_DRAM_CFG_RegSDRAM
#define GT_DRAM_CFG_RegSDRAMOff		0
#define GT_DRAM_CFG_DAdr12		(1<<24)
#define GT_DRAM_CFG_DAdr12ADP		0
#define GT_DRAM_CFG_DAdr12DMA		GT_DRAM_CFG_DAdr12

#define GT_DRAM_OpMode	      GT64120(0x474)

#define GT_DRAM_OpMode_MASK	0x3
#define GT_DRAM_OpMode_SHIFT	0

#define GT_DRAM_ADecode       GT64120(0x47c)

#define GT_DRAM_ADecode_MASK	0x3
#define GT_DRAM_ADecode_SHIFT   0

                                             /* DRAM Parameters */
#define GT_DRAMPAR_BANK0      GT64120(0x44c)
#define GT_DRAMPAR_BANK1      GT64120(0x450)
#define GT_DRAMPAR_BANK2      GT64120(0x454)
#define GT_DRAMPAR_BANK3      GT64120(0x458)

#define GT_DRAMPAR_CASLatSHIFT		0
#define GT_DRAMPAR_CASLatMASK		(3<<0)
#define GT_DRAMPAR_CASLat2		(1<<0)
#define GT_DRAMPAR_CASLat3		(2<<0)
#define GT_DRAMPAR_Sample		(1<<2)
#define GT_DRAMPAR_Sample1		0
#define GT_DRAMPAR_SampleNo		GT_DRAMPAR_Sample
#define GT_DRAMPAR_RASPrchg		(1<<3)
#define GT_DRAMPAR_RASPrchg1		0
#define GT_DRAMPAR_RASPrchg2		GT_DRAMPAR_RASPrchg
#define GT_DRAMPAR_64bitInt		(1<<5)
#define GT_DRAMPAR_64bitInt2		0
#define GT_DRAMPAR_64bitInt4		GT_DRAMPAR_64bitInt
#define GT_DRAMPAR_BankWidth		(1<<6)
#define GT_DRAMPAR_BankWidth32		0
#define GT_DRAMPAR_BankWidth64		GT_DRAMPAR_BankWidth
#define GT_DRAMPAR_BankLoc		(1<<7)
#define GT_DRAMPAR_BankLocEven		0
#define GT_DRAMPAR_BankLocOdd		GT_DRAMPAR_BankLoc
#define GT_DRAMPAR_Parity		(1<<8)
#define GT_DRAMPAR_ParityDisable	0
#define GT_DRAMPAR_ParityEnable		GT_DRAMPAR_Parity
#define GT_DRAMPAR_ByPass		(1<<9)
#define GT_DRAMPAR_ByPassNo		0
#define GT_DRAMPAR_ByPassYes		GT_DRAMPAR_ByPass
#define GT_DRAMPAR_SRAStoSCAS		(1<<10)
#define GT_DRAMPAR_SRAStoSCAS2  	0
#define GT_DRAMPAR_SRAStoSCAS3	        GT_DRAMPAR_SRAStoSCAS
#define GT_DRAMPAR_SDRAMSize		(1<<11)
#define GT_DRAMPAR_SDRAMSize16MB       	0
#define GT_DRAMPAR_SDRAMSize64MB       	GT_DRAMPAR_SDRAMSize
#define GT_DRAMPAR_ExtParity		(1<<12)
#define GT_DRAMPAR_ExtParityNo		0
#define GT_DRAMPAR_ExtParitySi		GT_DRAMPAR_ExtParity
#define GT_DRAMPAR_BrstLen		(1<<13)
#define GT_DRAMPAR_BrstLen8		0
#define GT_DRAMPAR_BrstLen4		GT_DRAMPAR_BrstLen


                                             /* Device Parameters */
#define GT_DEVPAR_BANK0	      GT64120(0x45c)
#define GT_DEVPAR_BANK1	      GT64120(0x460)
#define GT_DEVPAR_BANK2	      GT64120(0x464)
#define GT_DEVPAR_BANK3	      GT64120(0x468)
#define GT_DEVPAR_BOOT	      GT64120(0x46c)

#define GT_DEVPAR_TurnOffMASK		(7<<0)
#define GT_DEVPAR_TurnOffSHIFT		0
#define GT_DEVPAR_TurnOff(x)		((x)<<0)
#define GT_DEVPAR_AccToFirstMASK	(15<<3)
#define GT_DEVPAR_AccToFirstSHIFT	3
#define GT_DEVPAR_AccToFirst(x)		((x)<<3)
#define GT_DEVPAR_AccToNextMASK		(15<<7)
#define GT_DEVPAR_AccToNextSHIFT	7
#define GT_DEVPAR_AccToNext(x)		((x)<<7)
#define GT_DEVPAR_ALEtoWrMASK		(7<<11)
#define GT_DEVPAR_ALEtoWrSHIFT		11
#define GT_DEVPAR_ALEtoWr(x)		((x)<<11)
#define GT_DEVPAR_WrActiveMASK		(7<<14)
#define GT_DEVPAR_WrActiveSHIFT		14
#define GT_DEVPAR_WrActive(x)		((x)<<14)
#define GT_DEVPAR_WrHighMASK		(7<<17)
#define GT_DEVPAR_WrHighSHIFT		17
#define GT_DEVPAR_WrHigh(x)		((x)<<17)
#define GT_DEVPAR_DevWidthMASK		(3<<20)
#define GT_DEVPAR_DevWidthSHIFT		20	
#define GT_DEVPAR_DevWidth8		(0<<20)
#define GT_DEVPAR_DevWidth16		(1<<20)
#define GT_DEVPAR_DevWidth32		(2<<20)
#define GT_DEVPAR_DevWidth64		(3<<20)
#define GT_DEVPAR_DevLoc		(1<<23)
#define GT_DEVPAR_DevLocEven		0
#define GT_DEVPAR_DevLocOdd		GT_DEVPAR_DevLoc
#define GT_DEVPAR_LatchFunct		(1<<25)
#define GT_DEVPAR_LatchFunctTransparent 0
#define GT_DEVPAR_LatchFunctEnable	GT_DEVPAR_LatchFunct
#define GT_DEVPAR_Parity		(1<<30)
#define GT_DEVPAR_ParityDisable		0
#define GT_DEVPAR_ParityEnable		GT_DEVPAR_Parity
#define GT_DEVPAR_ReservedMASK		0xc3000000
#define GT_DEVPAR_Reserved		0x00000000

  /* No definition for the DMAs */
#define GT_DEVPAR_DMAFlyBy_BCS(x)	(x<<22)
#define GT_DEVPAR_DMAFlyBy_CS(x)	(x<<26)

  /* PCI Internal */

#define GT_PCI0_CMD           GT64120(0xc00)
#define GT_PCI1_CMD           GT64120(0xc80)

#define GT_IPCI_CMD_ByteSwap		(1<<0)
#define GT_IPCI_CMD_ByteSwapOn		0
#define GT_IPCI_CMD_ByteSwapOff		GT_INTPCI_CMD_ByteSwap
#define GT_IPCI_CMD_SyncModeMASK	(7<<1)
#define GT_IPCI_CMD_SyncModeSHIFT	1
#define GT_IPCI_CMD_SyncModeStd		(0<<1)
#define GT_IPCI_CMD_SyncMode1		(1<<1)
#define GT_IPCI_CMD_SyncMode2		(2<<1)
#define GT_IPCI_CMD_SyncMode5		(5<<1)
#define GT_IPCI_CMD_SyncMode6		(6<<1)

#define GT_PCI0_TOR	      GT64120(0xc04)
#define GT_PCI1_TOR	      GT64120(0xc84)

#define GT_IPCI_TOR_Timeout0MASK	(255<<0)
#define GT_IPCI_TOR_Timeout0SHIFT	0
#define GT_IPCI_TOR_Timeout0(x)	        ((x)<<0)
#define GT_IPCI_TOR_Timeout1MASK	(255<<8)
#define GT_IPCI_TOR_Timeout1SHIFT	8
#define GT_IPCI_TOR_Timeout1(x)	        ((x)<<8)
#define GT_IPCI_TOR_RetryCtrMASK	(255<<16)
#define GT_IPCI_TOR_RetryCtrSHIFT	16
#define GT_IPCI_TOR_RetryCtr(x)		((x)<<16)

#define GT_PCI0_RAS10SIZE     GT64120(0xc08)
#define GT_PCI0_RAS32SIZE     GT64120(0xc0c)
#define GT_PCI0_CS20SIZE      GT64120(0xc10)
#define GT_PCI0_CS3BOOTSIZE   GT64120(0xc14)
#define GT_PCI1_RAS10SIZE     GT64120(0xc88)
#define GT_PCI1_RAS32SIZE     GT64120(0xc8c)
#define GT_PCI1_CS20SIZE      GT64120(0xc90)
#define GT_PCI1_CS3BOOTSIZE   GT64120(0xc94)

#define GT_IPCI_SIZE_BankSizeMASK	(0xfffff<<12)
#define GT_IPCI_SIZE_BankSizeSHIFT	12

#define GT_PCI0_BAREN	      GT64120(0xc3c)
#define GT_PCI1_BAREN	      GT64120(0xcbc)

#define GT_IPCI_BAREN_SwCs3BootDis	(1<<0)
#define GT_IPCI_BAREN_SwRas32Dis	(1<<1)
#define GT_IPCI_BAREN_SwRas10Dis	(1<<2)
#define GT_IPCI_BAREN_IntIODis		(1<<3)
#define GT_IPCI_BAREN_IntMemDis		(1<<4)
#define GT_IPCI_BAREN_Cs3BootDis	(1<<5)
#define GT_IPCI_BAREN_Cs20Dis		(1<<6)
#define GT_IPCI_BAREN_Ras32Dis		(1<<7)
#define GT_IPCI_BAREN_Ras10Dis		(1<<8)

#define GT_PCI0_PMBS          GT64120(0xc40)
#define GT_PCI1_PMBS          GT64120(0xcc0)

#define GT_IPCI_PMBS_Ras10MBL           (1<<0)
#define GT_IPCI_PMBS_Ras32MBL           (1<<1)
#define GT_IPCI_PMBS_Cs20MBL            (1<<2)
#define GT_IPCI_PMBS_Cs3BootMBL         (1<<3)
#define GT_IPCI_PMBS_RdMenPREF          (1<<4)
#define GT_IPCI_PMBS_RdLnPREF           (1<<5)
#define GT_IPCI_PMBS_RdMuliPREF         (1<<6)

#define GT_PCI0_Ras10BARO     GT64120(0xc48)
#define GT_PCI0_SwapRAS10     GT64120(0xc58)
#define GT_PCI0_Ras32BARO     GT64120(0xc4c)
#define GT_PCI0_SwapRAS32     GT64120(0xc5c)
#define GT_PCI0_Cs20BARO      GT64120(0xc50)
#define GT_PCI0_Cs3BootBeARO  GT64120(0xc54)
#define GT_PCI0_SwapCs3Boot   GT64120(0xc64)
#define GT_PCI1_Ras10BARO     GT64120(0xcc8)
#define GT_PCI1_SwapRAS10     GT64120(0xcd8)
#define GT_PCI1_Ras32BARO     GT64120(0xccc)
#define GT_PCI1_SwapRAS32     GT64120(0xcdc)
#define GT_PCI1_Cs20BARO      GT64120(0xcd0)
#define GT_PCI1_Cs3BootBeARO  GT64120(0xcd4)
#define GT_PCI1_SwapCs3Boot   GT64120(0xce4)


#define GT_PCI0_CFGADDR	      GT64120(0xcf8)
#define GT_PCI0_CFGDATA	      GT64120(0xcfc)
#define GT_PCI1_CFGADDR	      GT64120(0xcf0)
#define GT_PCI1_CFGDATA	      GT64120(0xcf4)


#define GT_IPCI_CFGADDR_RegNumMASK	(0x3f<<2)
#define GT_IPCI_CFGADDR_RegNumSHIFT	2
#define GT_IPCI_CFGADDR_RegNum(x)	((x)<<2)
#define GT_IPCI_CFGADDR_FunctNumMASK	(0x7<<8)
#define GT_IPCI_CFGADDR_FunctNumSHIFT	8
#define GT_IPCI_CFGADDR_FunctNum(x)	((x)<<8)
#define GT_IPCI_CFGADDR_DevNumMASK	(0x1f<<11)
#define GT_IPCI_CFGADDR_DevNumSHIFT	11
#define GT_IPCI_CFGADDR_DevNum(x)	((x)<<11)
#define GT_IPCI_CFGADDR_BusNumMASK	(0xff<<16)
#define GT_IPCI_CFGADDR_BusNumSHIFT	16
#define GT_IPCI_CFGADDR_BusNum(x)	((x)<<16)
#define GT_IPCI_CFGADDR_ConfigEn	(1<<31)

/*
 *  Some PCI config space regs.
 */
#define	GT_PCI0_MAP10			0x10
#define	GT_PCI0_MAP14			0x14
#define	GT_PCI0_MAP20			0x20
#define	GT_PCI1_MAP10			0x90
#define	GT_PCI1_MAP14			0x94
#define	GT_PCI1_MAP20			0xa0

