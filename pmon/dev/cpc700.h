/*	$Id: cpc700.h,v 1.1.1.1 2006/09/14 01:59:08 root Exp $ */

/*
 * Copyright (c) 2001 IP Unplugged AB   (www.ipunplugged.com)
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
 *	This product includes software developed for IP Unplugged AB, by
 *	Patrik Lindergren.
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

#ifndef __CPC700_H__
#define __CPC700_H__

/*
 * CPC700 bit definitions
 */
#define CPC700_BBIT0   0x80
#define CPC700_BBIT1   0x40
#define CPC700_BBIT2   0x20
#define CPC700_BBIT3   0x10
#define CPC700_BBIT4   0x08
#define CPC700_BBIT5   0x04
#define CPC700_BBIT6   0x02
#define CPC700_BBIT7   0x01

#define CPC700_WBIT0   0x8000
#define CPC700_WBIT1   0x4000
#define CPC700_WBIT2   0x2000
#define CPC700_WBIT3   0x1000
#define CPC700_WBIT4   0x0800
#define CPC700_WBIT5   0x0400
#define CPC700_WBIT6   0x0200
#define CPC700_WBIT7   0x0100
#define CPC700_WBIT8   0x0080
#define CPC700_WBIT9   0x0040
#define CPC700_WBIT10  0x0020
#define CPC700_WBIT11  0x0010
#define CPC700_WBIT12  0x0008
#define CPC700_WBIT13  0x0004
#define CPC700_WBIT14  0x0002
#define CPC700_WBIT15  0x0001

#define CPC700_LBIT0   0x80000000
#define CPC700_LBIT1   0x40000000
#define CPC700_LBIT2   0x20000000
#define CPC700_LBIT3   0x10000000
#define CPC700_LBIT4   0x08000000
#define CPC700_LBIT5   0x04000000
#define CPC700_LBIT6   0x02000000
#define CPC700_LBIT7   0x01000000
#define CPC700_LBIT8   0x00800000
#define CPC700_LBIT9   0x00400000
#define CPC700_LBIT10  0x00200000
#define CPC700_LBIT11  0x00100000
#define CPC700_LBIT12  0x00080000
#define CPC700_LBIT13  0x00040000
#define CPC700_LBIT14  0x00020000
#define CPC700_LBIT15  0x00010000
#define CPC700_LBIT16  0x00008000
#define CPC700_LBIT17  0x00004000
#define CPC700_LBIT18  0x00002000
#define CPC700_LBIT19  0x00001000
#define CPC700_LBIT20  0x00000800
#define CPC700_LBIT21  0x00000400
#define CPC700_LBIT22  0x00000200
#define CPC700_LBIT23  0x00000100
#define CPC700_LBIT24  0x00000080
#define CPC700_LBIT25  0x00000040
#define CPC700_LBIT26  0x00000020
#define CPC700_LBIT27  0x00000010
#define CPC700_LBIT28  0x00000008
#define CPC700_LBIT29  0x00000004
#define CPC700_LBIT30  0x00000002
#define CPC700_LBIT31  0x00000001

/*
 * CPC700 Processor Interface Registers
 */
#define CPC700_PIFCFGADR	0xff500000	/* Processor Interface Configuration Address Register */
#define CPC700_PIFCFGDATA	0xff500004 	/* Processor Interface Configuration Data Register */
#define CPC700_PRIOPT1		0x00 		/* Processor Interface Options 1 */
#define CPC700_ERRDET1		0x04 		/* Error Detection 1 */
#define CPC700_ERREN1		0x08		/* Error Detect Enable 1 */
#define CPC700_CPUERAD		0x0c		/* Processor Error Address */
#define CPC700_CPUERAT		0x10		/* Processor Error Attributes */
#define CPC700_PLBMIFOPT	0x18		/* Processor-PLB Master Interface Options */
#define CPC700_PLBMTLSA1	0x20		/* Processor-PLB Master Byte Swap Region 1 Starting Address */
#define CPC700_PLBMTLEA1	0x24		/* Processor-PLB Master Byte Swap Region 1 Ending Address */
#define CPC700_PLBMTLSA2	0x28		/* Processor-PLB Master Byte Swap Region 2 Starting Address */
#define CPC700_PLBMTLEA2	0x2c		/* Processor-PLB Master Byte Swap Region 2 Ending Address */
#define CPC700_PLBMTLSA3	0x30		/* Processor-PLB Master Byte Swap Region 3 Starting Address */
#define CPC700_PLBMTLEA3	0x34		/* Processor-PLB Master Byte Swap Region 3 Ending Address */
#define CPC700_PLBSNSSA0	0x38		/* PLB Slave No Snoop Region Starting Address */
#define CPC700_PLBSNSEA0	0x3c		/* PLB Slave No Snoop Region Ending Address */
#define CPC700_BESR		0x40		/* PLB Bus Error Syndrome Register */
#define CPC700_BESRSET		0x44		/* PLB Bus Error Syndrome Register Set (for test/verification use) */
#define CPC700_BEAR		0x4c		/* PCI Bus Master Error Address Register */
#define CPC700_PLBSWRINT	0x80		/* Write Interrupt Region Base Address */

/*
 *  CPC700 PCI addresses.
 */
#define	CPC700_PCICFGADDR	0xfec00000	/* PCI Configuration Address Register */
#define	CPC700_PCICFGDATA	0xfec00004	/* PCI Configuration Data Register */
#define	CPC700_PCI_CMD		0x04		/* Really pci config space */
#define	CPC700_PCI_STAT		0x06		/* Status Register */
#define	CPC700_PCI_REV		0x08		/* Revision ID */
#define CPC700_PCIPTM1BAR	0x14		/* PTM 1 BAR */
#define CPC700_PCIPTM2BAR	0x18		/* PTM 2 BAR */
#define CPC700_PCIBUSNUM	0x40
#define CPC700_PCISUBBUSNUM	0x41
#define CPC700_PCIDSCCNT	0x42		/* Disconnect Counter */
#define CPC700_PCIARBCNTL	0x44		/* PCI Arbiter Control */
#define CPC700_PCIERREN		0x48
#define CPC700_PCIERRSTS	0x49
#define CPC700_PCIBRDGOPT1	0x4a
#define CPC700_SESR		0x4c
#define CPC700_SEAR0		0x50
#define CPC700_SEAR1		0x54
#define CPC700_PCIBRDGOPT2	0x60

/*
 * CPC700 Memory Controller addresses
 */
#define	CPC700_MEMCFGADDR	0xff500008	/* Memory Controller Address */
#define	CPC700_MEMCFGDATA	0xff50000c	/* Memory Controller Data */
#define	CPC700_MCOPT1		0x20		/* Memory Contoller Options 1 */
#define	CPC700_MBEN		0x24		/* Memory Bank Enable */
#define	CPC700_MEMTYPE		0x28		/* Installed Memory Type */
#define CPC700_RWD		0x2c		/* Bank Active Watchdog Timer */
#define	CPC700_RTR		0x30		/* Refresh Timer Register */
#define	CPC700_DAM		0x34		/* DRAM Addressing Mode */
#define	CPC700_MB0SA		0x38		/* Memory Bank 0 Starting Address */
#define	CPC700_MB1SA		0x3c		/* Memory Bank 1 Stating Address */
#define	CPC700_MB2SA		0x40		/* Memory Bank 2 Starting Address */
#define	CPC700_MB3SA		0x44		/* Memory Bank 3 Starting Address */
#define	CPC700_MB4SA		0x48		/* Memory Bank 4 Starting Address */
#define	CPC700_MB0EA		0x58		/* Memory Bank 0 Ending Address */
#define	CPC700_MB1EA		0x5c		/* Memory Bank 1 Ending Address */
#define	CPC700_MB2EA		0x60		/* Memory Bank 2 Ending Address */
#define	CPC700_MB3EA		0x64		/* Memory Bank 3 Ending Address */
#define	CPC700_MB4EA		0x68		/* Memory Bank 4 Ending Address */
#define	CPC700_SDTR1		0x80		/* SDRAM Timing Register 1 */
#define	CPC700_RBW		0x88		/* ROM Bank Width */
#define	CPC700_FWEN		0x90		/* Flash Write Enable */
#define	CPC700_ECCCF		0x94		/* ECC Configuration */
#define CPC700_ECCERR		0x98		/* ECC Error */
#define CPC700_RPB0P		0xe0		/* ROM / Peripheral Bank 0 Parameters */
#define	CPC700_RPB1P		0xe4		/* ROM / Peripheral Bank 1 Parameters */
#define CPC700_RPB2P		0xe8		/* ROM / Peripheral Bank 2 Parameters */
#define CPC700_RPB3P		0xec		/* ROM / Peripheral Bank 3 Parameters */
#define	CPC700_RPB4P		0xf0		/* ROM / Peripheral Bank 4 Parameters */

/* PCI to local bus configuration */
#define	CPC700_PMM0ADDR		0xff400000	/* PMM 0 Local Address */
#define	CPC700_PMM0MASK		0xff400004	/* PMM 0 Mask/Attribute */
#define	CPC700_PMM0LO		0xff400008	/* PMM 0 PCI Low Address */
#define	CPC700_PMM0HIGH		0xff40000c	/* PMM 0 PCI High Address */
#define	CPC700_PMM1ADDR		0xff400010	/* PMM 1 Local Address */
#define	CPC700_PMM1MASK		0xff400014	/* PMM 1 Mask/Attribute */
#define	CPC700_PMM1LO		0xff400018	/* PMM 1 PCI Low Address */
#define	CPC700_PMM1HIGH		0xff40001c	/* PMM 1 PCI High Address */
#define	CPC700_PMM2ADDR		0xff400020	/* PMM 2 Local Address */
#define	CPC700_PMM2MASK		0xff400024	/* PMM 2 Mask/Attribute */
#define	CPC700_PMM2LO		0xff400028	/* PMM 2 PCI Low Address */
#define	CPC700_PMM2HIGH		0xff40002c	/* PMM 2 PCI High Address */
#define	CPC700_PTM1SIZE		0xff400030	/* PTM 1 Memory Size */
#define	CPC700_PTM1ADDR		0xff400034	/* PTM 1 Local Address */
#define	CPC700_PTM2SIZE		0xff400038	/* PTM 2 Memory Size */
#define	CPC700_PTM2ADDR		0xff40003c	/* PTM 2 Local Address */

/*
 * IIC interface.
 */
#define	CPC700_IIC0		0xff620000	/* IIC0 Base Address */
#define	CPC700_IIC1		0xff630000	/* IIC1 Base Address */
#define	CPC700_IIC0_MDBUF	0xff620000	/* IIC0 Master Data Buffer */
#define CPC700_IIC0_SDBUF	0xff620002	/* IIC0 Slave Data Buffer */
#define CPC700_IIC0_LMADR	0xff620004	/* IIC0 Low Master Address */
#define CPC700_IIC0_HMADR	0xff620005	/* IIC0 High Master Address */
#define CPC700_IIC0_CNTL	0xff620006	/* IIC0 Control */
#define CPC700_IIC0_MDCNTL	0xff620007	/* IIC0 Mode Control */
#define CPC700_IIC0_STS		0xff620008	/* IIC0 Status */
#define CPC700_IIC0_EXTSTS	0xff620009	/* IIC0 Extended Status */
#define CPC700_IIC0_LSADR	0xff62000a	/* IIC0 Low Slave Address */
#define CPC700_IIC0_HSADR	0xff62000b	/* IIC0 High Slave Address */
#define CPC700_IIC0_CLKDIV	0xff62000c	/* IIC0 Clock Divide */
#define CPC700_IIC0_INTRMASK	0xff62000d	/* IIC0 Interrupt Mask */
#define CPC700_IIC0_XFRCNT	0xff62000e	/* IIC0 Transfer Count */
#define CPC700_IIC0_XTCNTLSS	0xff62000f	/* IIC0 Extended Control & Slave Status */
#define CPC700_IIC0_DIRECTCNTL	0xff620010	/* IIC0 Direct Control */
#define CPC700_IIC1_MDBUF       0xff630000      /* IIC1 Master Data Buffer */
#define CPC700_IIC1_SDBUF       0xff630002      /* IIC1 Slave Data Buffer */
#define CPC700_IIC1_LMADR       0xff630004      /* IIC1 Low Master Address */
#define CPC700_IIC1_HMADR       0xff630005      /* IIC1 High Master Address */
#define CPC700_IIC1_CNTL        0xff630006      /* IIC1 Control */
#define CPC700_IIC1_MDCNTL      0xff630007      /* IIC1 Mode Control */
#define CPC700_IIC1_STS         0xff630008      /* IIC1 Status */
#define CPC700_IIC1_EXTSTS      0xff630009      /* IIC1 Extended Status */
#define CPC700_IIC1_LSADR       0xff63000a      /* IIC1 Low Slave Address */
#define CPC700_IIC1_HSADR       0xff63000b      /* IIC1 High Slave Address */
#define CPC700_IIC1_CLKDIV      0xff63000c      /* IIC1 Clock Divide */
#define CPC700_IIC1_INTRMASK    0xff63000d      /* IIC1 Interrupt Mask */
#define CPC700_IIC1_XFRCNT      0xff63000e      /* IIC1 Transfer Count */
#define CPC700_IIC1_XTCNTLSS    0xff63000f      /* IIC1 Extended Control & Slave Status */
#define CPC700_IIC1_DIRECTCNTL  0xff630010      /* IIC1 Direct Control */

#define	CPC700_IIC_MDBUF	0x00
#define	CPC700_IIC_SDBUF	0x02
#define	CPC700_IIC_LMADR	0x04
#define	CPC700_IIC_HMADR	0x05
#define	CPC700_IIC_CNTL		0x06
#define	CPC700_IIC_MDCNTL	0x07
#define	CPC700_IIC_STS		0x08
#define	CPC700_IIC_EXTSTS	0x09
#define	CPC700_IIC_LSADR	0x0a
#define	CPC700_IIC_HSADR	0x0b
#define	CPC700_IIC_CLKDIV	0x0c
#define	CPC700_IIC_INTRMSK	0x0d
#define	CPC700_IIC_XFRCNT	0x0e
#define	CPC700_IIC_XTCNTLSS	0x0f

/*
 *  I/O ports.
 */
#define CPC700_UART0RBR		0xff600300	/* UART 0 Receiver Buffer Register, DLAB=0 */
#define CPC700_UART0THR		0xff600300	/* UART 0 Transmitter Holding Register, DLAB=0 */
#define CPC700_UART0DLL		0xff600300	/* UART 0 Divisor Latch (LSB), DLAB=1 */
#define CPC700_UART0IER		0xff600301	/* UART 0 Interrupt Enable Register, DLAB=0  */
#define CPC700_UART0DLM		0xff600301	/* UART 0 Divisor Latch (MSB), DLAB=1 */
#define CPC700_UART0IIR		0xff600302	/* UART 0 Interrupt Identification Register */
#define CPC700_UART0FCR		0xff600302	/* UART 0 FIFO Control Register */
#define CPC700_UART0LCR		0xff600303	/* UART 0 Line Control Register */
#define CPC700_UART0MCR		0xff600304	/* UART 0 Modem Control Register */
#define CPC700_UART0LSR		0xff600305	/* UART 0 Line Status Register */
#define CPC700_UART0MSSR	0xff600306	/* UART 0 Modem Status Register */
#define CPC700_UART0SCR		0xff600307	/* UART 0 Scratch Register */
#define CPC700_UART1RBR         0xff600400      /* UART 1 Receiver Buffer Register, DLAB=0 */
#define CPC700_UART1THR         0xff600400      /* UART 1 Transmitter Holding Register, DLAB=0 */
#define CPC700_UART1DLL         0xff600400      /* UART 1 Divisor Latch (LSB), DLAB=1 */
#define CPC700_UART1IER         0xff600401      /* UART 1 Interrupt Enable Register, DLAB=0  */
#define CPC700_UART1DLM         0xff600401      /* UART 1 Divisor Latch (MSB), DLAB=1 */
#define CPC700_UART1IIR         0xff600402      /* UART 1 Interrupt Identification Register */ 
#define CPC700_UART1FCR         0xff600402      /* UART 1 FIFO Control Register */
#define CPC700_UART1LCR         0xff600403      /* UART 1 Line Control Register */
#define CPC700_UART1MCR         0xff600404      /* UART 1 Modem Control Register */
#define CPC700_UART1LSR         0xff600405      /* UART 1 Line Status Register */
#define CPC700_UART1MSSR        0xff600406      /* UART 1 Modem Status Register */
#define CPC700_UART1SCR         0xff600407      /* UART 1 Scratch Register */

/*
 * Bus Support
 */
#define CPC700_PESRRD		0xff500850	/* PLB Error Status Register (read/clear) */
#define CPC700_PERRWR		0xff500854	/* PLB Error Status Register (set) */
#define CPC700_PACR		0xff50085c	/* PLB Arbiter Control Register */
#define CPC700_GESRRD		0xff500810	/* OPB Bridge Error Status Register (read/clear) */
#define CPC700_GESRWR		0xff500814	/* OPB Bridge Error Status Register (set) */
#define CPC700_GEAR		0xff500818	/* OPB Bridge Error Address Register */

/*
 * Universal Interrupt Controller
 */
#define CPC700_UICSR		0xff500880	/* UIC Status Register (read/clear) */
#define CPC700_UICSRS		0xff500884	/* UIC Status Register (set) */
#define CPC700_UICER		0xff500888	/* UIC Enable Register */
#define CPC700_UICCR		0xff50088c	/* UIC Critical Register */
#define CPC700_UICPR		0xff500890	/* UIC Polarity Register */
#define CPC700_UICTR		0xff500894	/* UIC Trigger Register */
#define CPC700_UICMSR		0xff500898	/* UIC Masked Status Register */
#define CPC700_UICVR		0xff50089c	/* UIC Vector Register */
#define CPC700_UICVCR		0xff5008a0	/* UIC Vector Configuration Register */

/*
 * General Purpose Timers
 */
#define CPC700_GPTTBC		0xff650000	/* GPT Time Base Counter */
#define CPC700_GPTCE		0xff650004	/* Capture Timers Enable */
#define CPC700_GPTEC		0xff650008	/* Capture Events Edge Detection Control */
#define CPC700_GPTSC		0xff65000c	/* Capture Events Synchronization Control */
#define CPC700_GPTIE		0xff650018	/* Timers Interrupt Enable */
#define CPC700_GPTIS		0xff65001c	/* Timers Interrupt Status */
#define CPC700_GPTISR		0xff650020	/* Timers Interrupt Status (clear upon read) */
#define CPC700_GPTIM		0xff650024	/* Timers Interrupt Mask */
#define CPC700_GPTCAPT0		0xff650040	/* Capture Timer 0 */
#define CPC700_GPTCAPT1         0xff650044      /* Capture Timer 1 */
#define CPC700_GPTCAPT2         0xff650048      /* Capture Timer 2 */
#define CPC700_GPTCAPT3         0xff65004c      /* Capture Timer 3 */
#define CPC700_GPTCAPT4         0xff650050      /* Capture Timer 4 */
#define CPC700_GPTCOMP0		0xff650080	/* Compare Timer 0 */
#define CPC700_GPTCOMP1         0xff650084      /* Compare Timer 1 */
#define CPC700_GPTCOMP2         0xff650088      /* Compare Timer 2 */
#define CPC700_GPTCOMP3         0xff65008c      /* Compare Timer 3 */
#define CPC700_GPTCOMP4         0xff650090      /* Compare Timer 4 */
#define CPC700_GPTMASK0		0xff65000c0	/* TBC Mask (Compare Timer 0) */
#define CPC700_GPTMASK1         0xff65000c4     /* TBC Mask (Compare Timer 1) */
#define CPC700_GPTMASK2         0xff65000c8     /* TBC Mask (Compare Timer 2) */
#define CPC700_GPTMASK3         0xff65000cc     /* TBC Mask (Compare Timer 3) */
#define CPC700_GPTMASK4         0xff65000d0     /* TBC Mask (Compare Timer 4) */

/*
 * CPR Registers
 */
#define CPC700_CRPRMCTRL	0xff500900	/* Peripheral Power Management Control */
#define CPC700_CPRRESET		0xff500904	/* Peripheral Reset Control */
#define CPC700_CPRCAPTEVNT	0xff500908	/* GPT Capture Event Generation */
#define CPC700_CPRPLLACCESS	0xff50090c	/* PLL Configureation Access Register (unlocks CPRPLLTUNE) */
#define CPC700_CPRPLLTUNE	0xff500910	/* PLL Configuration Register (resets system upon write) */
#define CPC700_CPRSTRAPREAD	0xff500914	/* Strapping Pin Status Read Register */


/*
 * CPC700 Memory Map
 */
#define	CPC700_PCI_MEM_BASE	0x80000000
#define	CPC700_PCI_MEM_SIZE	0x78000000
#define	CPC700_PCI_IO_BASE	0xf8000000
#define	CPC700_PCI_IO_SIZE	0x00010000
#define CPC700_PCI_IO_REMAP	0x00000000
#define	CPC700_PCI_IO2_BASE	0xf8800000
#define	CPC700_PCI_IO2_SIZE	0x03800000
#define CPC700_PCI_IO2_REMAP	0x00800000
#define CPC700_IO_BASE		0xfec00000
#define CPC700_IO_SIZE		0x00c00000

#ifdef __ASSEMBLER__
/*
 *  Macro used to setup CPC700 memory control.
 */
#define IORDER          eieio; sync

#define CPC700_MEM_SETUP(reg, val, mask)	\
	li 3, reg ; stw	3, 0(1) ; IORDER ;	\
	lwz	4, 0(2)	;			\
	lis 3, HI(mask); ori 3, 3, LO(mask) ;	\
	and 4, 4, 3 ;				\
	lis 3, HI(val) ; ori 3, 3, LO(val) ;	\
	or 4, 4, 3 ; stw 4, 0(2) ; IORDER

#define CPC700_MEM_RSETUP(reg, rval)    	\
	li 3, reg ; stw 3, 0(1) ; IORDER ;	\
	stw rval, 0(2) ; IORDER

#endif /* __ASSEMBLER__ */
#endif /* __CPC700_H__ */
