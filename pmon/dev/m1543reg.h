/*	$Id: m1543reg.h,v 1.1.1.1 2006/09/14 01:59:08 root Exp $ */

/*
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
/* M1543C Registers */
#define PCI_DEV_NO_M1543C       0x0012

#define M1543C_VEND_ID_REG      0x00
#define M1543C_DEV_ID_REG       0x02
#define M1543C_PCI_CMD          0x04
#define M1543C_PCI_STAT         0x06
#define M1543C_DEV_REV          0x08

#define M1543C_PIC              0x40
#define M1543C_IORC             0x41
#define M1543C_ISAC1            0x42
#define M1543C_ISAC2            0x43

#define M1543C_BCSC             0x47
#define M1543C_PIRT1            0x48
#define M1543C_PIRT2            0x49
#define M1543C_PIRT3            0x4A
#define M1543C_PIRT4            0x4B
#define M1543C_PILET            0x4C

#define M1543C_PCSAD            0x55

#define M1543C_GPIS             0x59    /* General Purpose Input (GPI) Multiplexed Pin Select */
#define M1543C_GPOS             0x5A    /* General Purpose Output (GPO) Multiplexed Pin Select */
#define M1543C_DMDC             0x5C    /* Docking Mode Decode Control */
#define M1543C_SMCCI            0x5E    /* Suspend Mode Clock Control I */
#define M1543C_SMCCII           0x5F    /* Suspend Mode Clock Control II */

/* M1543C PMU Registers */
#define PCI_DEV_NO_M1543C_PMU   0x1C

#define M1543C_PMU_COESII       0x8C    /* Control of External Switch II (boot select jumper) */
#define M1543C_PMU_DOGPOI       0xC0    /* Data Output to GPO Pins */
#define M1543C_PMU_ODGPOII      0xC3    /* Data Output for GPO[23:22] */
#define M1543C_PMU_SMIRB        0xC6    /* Select Multifunctions in Resume Block */


/* M1543C Super I/O Registers */

#define M1543C_SIO_CNF_PORT     0x3F0
#define M1543C_SIO_IDX_PORT     0x3F0
#define M1543C_SIO_DATA_PORT    0x3F1

#define M1543C_SIO_RESET        0x02     /* Reset configuration register  */
#define M1543C_SIO_LUN_IDX      0x07     /* Logical device select register  */
#define M1543C_SIO_DEV_VER      0x1F     /* ALI defined device revision */
#define M1543C_SIO_DEV_ID2      0x20     /* ALI define device identification */
#define M1543C_SIO_DEV_ID1      0x21     /* ALI define device identification */
#define M1543C_SIO_DPDR         0x22     /* Direct powerdown register */
#define M1543C_SIO_APDR         0x23     /* Auto powerdown register */

#define M1543C_SIO_ENR          0x30     /* Device enable register */
#define M1543C_SIO_HIAR         0x60     /* I/O high address register */
#define M1543C_SIO_LOAR         0x61     /* I/O low address register */
#define M1543C_SIO_IRQR         0x70     /* Device irq channel register */
#define M1543C_SIO_DMAR         0x74     /* Device dma channel register */
#define M1543C_SIO_CNFR         0xf0     /* Device configuration register 1 */
#define M1543C_SIO_CNF2R        0xf1     /* Device configuration register 2 */

#define M1543C_SIO_LPT_CNFR     0x   /* Device configuration register */

#define M1543C_SIO_LUN_ENABLE   0x01     /* Logical device enable */
#define M1543C_SIO_LUN_DISABLE  0x00     /* Logical device disable */


/* M1543C Super I/O logical device numbers */

#define M1543C_SIO_LUN_FDC      0x00    /* floppy disk */
#define M1543C_SIO_LUN_LPT      0x03    /* LPT port */
#define M1543C_SIO_LUN_COM1     0x04    /* UART1 */
#define M1543C_SIO_LUN_COM2     0x05    /* UART3 */
#define M1543C_SIO_LUN_KBC      0x07    /* keyboard */
#define M1543C_SIO_LUN_UART3    0x0b    /* UART 3 */
#define M1543C_SIO_LUN_HOTK     0x0c    /* HOTK 3 */


/* M1543C Super I/O logical device */

#define M1543C_FDC_IO_ADRS      (PCI_ISA_SPACE_IO_BASE + 0x03f0)  /* floppy */
#define M1543C_LPT_IO_ADRS      (PCI_ISA_SPACE_IO_BASE + 0x03bc)  /* lpt */
#define M1543C_COM1_IO_ADRS     (PCI_ISA_SPACE_IO_BASE + 0x03f8)  /* com1 */
#define M1543C_COM2_IO_ADRS     (PCI_ISA_SPACE_IO_BASE + 0x02f8)  /* com2 */

/* DMA DEFINES */

#define M1543C_FDC_DMA          0x02
#define M1543C_LPT_DMA          0x03

#define M1543C_SIO_IRQR         0x70     /* Device irq channel register */
#define M1543C_FDC_IRQ          0x06
#define M1543C_LPT_IRQ          0x07
#define M1543C_COM1_IRQ         0x04
#define M1543C_COM2_IRQ         0x03
#define M1543C_KBC_IRQ          0x01
