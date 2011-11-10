/* $Id: mpc107reg.h,v 1.1.1.1 2006/09/14 01:59:08 root Exp $ */

/*
 * Copyright (c) 1997 Per Fogelstrom
 * Copyright (c) 2001 Patrik Lindergren
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
 *	This product includes software developed under OpenBSD for RTMX Inc
 *	by Per Fogelstrom, Opsycon AB.
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
 *
 * mpc107reg.h: PowerPC to PCI bridge controller
 */

#ifndef __MPC107REG_H__
#define __MPC107REG_H__

#ifdef MPC107_MEMORYMAP_PREP

#define	MPC107_PCI_MEM_BASE	0xc0000000
#define	MPC107_PCI_MEM_SIZE	(0xff000000-MPC107_PCI_MEM_BASE)
#define	MPC107_PCI_IO_BASE	0x80000000
#define	MPC107_PCI_ISA_IO_BASE	0x80000000
#define	MPC107_PCI_IO_SIZE	(0xbf800000-MPC107_PCI_IO_BASE)
/* offsets from base pointer */
#define MPC107_CONF_ADDR	(MPC107_PCI_IO_BASE + 0x0cf8)
#define MPC107_CONF_DATA	(MPC107_PCI_IO_BASE + 0x0cfc)

/* PCI i/o regions in PCI space */
#define PCI_IO_SPACE_PCI_BASE	0x00000000

/* PCI mem regions in PCI space */
#define PCI_MEM_SPACE_PCI_BASE	0x00000000	/* PCI Mem accessed from PCI */
#define PCI_LOCAL_MEM_PCI_BASE	0x80000000	/* CPU Mem accessed from PCI */
#define PCI_LOCAL_MEM_ISA_BASE	0x00000000	/* ISA Mem accessed from PCI */

#else /* Memory MAP CHRP */

#define	MPC107_PCI_MEM_BASE	0x80000000
#define	MPC107_PCI_MEM_SIZE	(0xFDF00000-MPC107_PCI_MEM_BASE)
#define	MPC107_PCI_IO_BASE	0xFE800000
#define	MPC107_PCI_IO_SIZE	(0xFEBFFFFF-MPC107_PCI_IO_BASE)
#define	MPC107_PCI_ISA_IO_BASE	0xFE000000
#define MPC107_PCI_ISA_IO_SIZE	(0xFE800000-MPC107_PCI_ISA_IO_BASE)
/* offsets from base pointer */
#define MPC107_CONF_ADDR	0xFEC00000
#define MPC107_CONF_DATA	0xFEE00000
#define MPC107_INT_ACK		0xFEF00000

/* PCI i/o regions in PCI space */
#define PCI_IO_SPACE_PCI_BASE	0x00800000

/* PCI mem regions in PCI space */
#define PCI_MEM_SPACE_PCI_BASE	0x80000000	/* PCI Mem accessed from PCI */
#define PCI_LOCAL_MEM_PCI_BASE	0x00000000	/* CPU Mem accessed from PCI */
#define PCI_LOCAL_MEM_ISA_BASE	0x00000000	/* ISA Mem accessed from PCI */

#endif /* MPC107_MEMORYMAP_PREP */


/*
 * PCI std regs.
 */
#define	MPC107_PCI_CMD		0x04	/* PCI command */
#define	MPC107_PCI_STAT		0x06	/* PCI status */

/*
 * Memory config regs.
 */
#define MPC107_CFG_LMBAR	0x10	/* Local Memory Base Address Register */
#define	MPC107_CFG_PCSRBAR	0x14	/* PCI BAR for REGS  */
#define	MPC107_CFG_PACR		0x46	/* PCI Arbiter Control Register */
#define MPC107_CFG_PMCR1	0x70	/* Power Management Configuration Register 1 */
#define MPC107_CFG_PMCR2	0x72	/* Power Management Configuration Register 2 */
#define	MPC107_CFG_ODCR		0x73	/* Output/Clock Driver Control Register */
#define	MPC107_CFG_CDCR		0x74	/* Output/Clock Driver Control Register */
#define	MPC107_CFG_MDCR		0x76	/* Misc I/O Control Register */
#define	MPC107_CFG_EUMBBAR	0x78	/* EUMBBAR CFG offset for base addr */
#define	MPC107_CFG_MSTA_03	0x80	/* Memory Starting Address Bank 0-3 */
#define	MPC107_CFG_MSTA_47	0x84	/* Memory Starting Address Bank 4-7 */
#define	MPC107_CFG_EMSTA_03	0x88	/* Extended Memory Starting Address Bank 0-3 */
#define	MPC107_CFG_EMSTA_47	0x8c	/* Extended Memory Starting Address Bank 4-7 */
#define	MPC107_CFG_MEND_03	0x90	/* Memory Ending Address Bank 0-3 */
#define	MPC107_CFG_MEND_47	0x94	/* Memory Ending Address Bank 4-7 */
#define	MPC107_CFG_EMEND_03	0x98	/* Extended Memory Ending Address Bank 0-3 */
#define	MPC107_CFG_EMEND_47	0x9c	/* Extended Memory Ending Address Bank 4-7 */
#define	MPC107_CFG_MBEN		0xa0	/* Memory bank enable register */
#define	MPC107_CFG_MPMR		0xa3	/* Memory Page Mode Register */
#define	MPC107_CFG_PICR1	0xa8	/* Processor Interface Configuration Register 1 */
#define	MPC107_CFG_PICR2	0xac	/* Processor Interface Configuration Register 2 */
#define MPC107_CFG_ECC_ERCNT	0xB8	/* ECC Single-Bit Error Count Register */
#define MPC107_CFG_ECC_ERRTRG	0xB9	/* ECC Single-Bit Error Trigger Register */
#define	MPC107_CFG_ERR_EN1	0xC0	/* Error Enabling Register 1 */
#define	MPC107_CFG_ERR_DR1	0xC1	/* Error Detection Register 1 */
#define	MPC107_CFG_ERR_EN2	0xC4	/* Error Enabling Register 2 */
#define	MPC107_CFG_ERR_DR2	0xC5	/* Error Detection Register 2 */
#define MPC107_CFG_ERR_PCIBUS	0xC7	/* PCI Bus Error Status Register */
#define MPC107_CFG_ERR_ADDR	0xC8	/* Processor/PCI Error Address Register */
#define MPC107_CFG_AMBOR	0xE0	/* Address Map B Options Register */
#define	MPC107_CFG_MCFG1	0xF0	/* Memory Configuration Register 1 */
#define	MPC107_CFG_MCFG2	0xF4	/* Memory Configuration Register 2 */
#define	MPC107_CFG_MCFG3	0xF8	/* Memory Configuration Register 3 */
#define	MPC107_CFG_MCFG4	0xFC	/* Memory Configuration Register 4 */

#define MPC107_PICR1_FLASH_WR_EN	0x1000
/*
 * Clock drive strength settings
 */
#define	PCI50OHM		0x80
#define	PCI25OHM		0x00
#define	CPU40OHM		0x40
#define	PCI20OHM		0x00
#define	MEM8OHM			0x30
#define	MEM13OHM		0x20
#define	MEM20OHM		0x10
#define	MEM40OHM		0x00
#define	PCICLK8OHM		0x0c
#define	PCICLK13OHM		0x08
#define	PCICLK20OHM		0x04
#define	PCICLK40OHM		0x00
#define	MEMCLK8OHM		0x03
#define	MEMCLK13OHM		0x02
#define	MEMCLK20OHM		0x01
#define	MEMCLK40OHM		0x00

/*
 *  Embedded Utilites Memory Block.
 */
#define	MPC107_EUMBAR		0xfdf00000	/* Were block is placed */

/*
 *  I2C Registers.
 */
#define	MPC107_I2C_ADR		(MPC107_EUMBAR + 0x3000)
#define	MPC107_I2C_FDR		(MPC107_EUMBAR + 0x3004)
#define	MPC107_I2C_CCR		(MPC107_EUMBAR + 0x3008)
#define	MPC107_I2C_CSR		(MPC107_EUMBAR + 0x300c)
#define	MPC107_I2C_CDR		(MPC107_EUMBAR + 0x3010)

#define	I2C_CCR_MEN	0x80	/* Enable */
#define	I2C_CCR_MSTA	0x20	/* Start */
#define	I2C_CCR_MTX	0x10	/* TX mode set */
#define	I2C_CCR_TXAK	0x08	/* Transfer ack */
#define	I2C_CCR_RSTA	0x04	/* Restart */

#define	I2C_CSR_MCF	0x80	/* Transfer complete */
#define	I2C_CSR_MBB	0x20	/* Bus busy */
#define	I2C_CSR_MIF	0x02	/* Interrupt */
#define	I2C_CSR_RXAK	0x01	/* No receive ack */

#define	I2C_READ	1
#define	I2C_WRITE	0

#define MPC107_IRQ0	0
#define MPC107_IRQ1	1
#define MPC107_IRQ2	2
#define MPC107_IRQ3	3
#define MPC107_IRQ4	4
#define MPC107_IRQ5	5
#define MPC107_IRQ6	6
#define MPC107_IRQ7	7
#define MPC107_IRQ8	8
#define MPC107_IRQ9	9
#define MPC107_IRQ10	10
#define MPC107_IRQ11	11
#define MPC107_IRQ12	12
#define MPC107_IRQ13	13
#define MPC107_IRQ14	14
#define MPC107_IRQ15	15

/*
 *  Macros used to access MPC107 config space.
 *  Depend on register 1, 2 and 3 set up correctly.
 */

#define	MPC_CFG_AD(reg)						\
	ori 0, 22, ((reg) & ~3) ; stwbrx 0, 0, 20 ;		\
	IORDER

#define	MPC_CFG_WR(reg, data)					\
	ori 4, 22, ((reg) & ~3) ; stwbrx 4, 0, 20 ; sync;	\
	lis 4, HI(data) ; ori 4, 4, LO(data);			\
	stwbrx 4, 0, 21 ; IORDER
	
#define	MPC_CFG_RWR(reg)					\
	ori 0, 22, ((reg) & ~3) ; stwbrx 0, 0, 20 ; sync;	\
	stwbrx 4, 0, 21 ; IORDER
	
#define	MPC_CFG_RD(reg)						\
	ori 4, 22, ((reg) & ~3) ; stwbrx 4, 0, 20 ; sync;	\
	lwbrx 4, 0, 21 ; IORDER
	
#define	MPC_CFG_RD3(reg)					\
	ori 4, 22, ((reg) & ~3) ; stwbrx 4, 0, 20 ; sync;	\
	lwbrx 3, 0, 21 ; IORDER
	
#endif /* __MPC107REG_H__ */
