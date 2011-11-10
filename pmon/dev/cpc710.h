/*	$Id: cpc710.h,v 1.1.1.1 2006/09/14 01:59:08 root Exp $ */

/*
 * Copyright (c) 2002 Opsycon AB  (www.opsycon.se)
 * Copyright (c) 2000 Rtmx, Inc   (www.rtmx.com)
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
 *      This product includes software developed for Rtmx, Inc by
 *      Opsycon Open System Consulting AB, Sweden.
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

#ifndef __CPC710_H__
#define __CPC710_H__
/*
 *  CPC710 addresses.
 */
#define PCI32_BAR_VALUE		0xff500000
#define PCI64_BAR_VALUE		0xff400000
#define	CPC710_PCI32_CFGADDR	(PCI32_BAR_VALUE + PCICFG_CFGA)
#define	CPC710_PCI32_CFGDATA	(PCI32_BAR_VALUE + PCICFG_CFGD)
#define	CPC710_PCI64_CFGADDR	(PCI64_BAR_VALUE + PCICFG_CFGA)
#define	CPC710_PCI64_CFGDATA	(PCI64_BAR_VALUE + PCICFG_CFGD)

#define	CPC710_PCI_CMD		0x04		/* Really pci config space */
#define	CPC710_PCI_STAT		0x06
#define	CPC710_PCI_REV		0x08
#define	CPC710_PCI_BUS		0x40
#define	CPC710_PCI_INT		0x68

/**********************************/
/* SPECIFIC PCI HOST BRIDGE SPACE */
/**********************************/

#define PCICFG_PSEA    0x000f6110      /* PCI Slave Error Address */
#define PCICFG_PCIDG   0x000f6120      /* PCI DIAGNOSTIC Register */
#define PCICFG_INTA    0x000f7700      /* Interrupt Acknowledge Cycle */
#define PCICFG_PIBAR   0x000f7800      /* PCI Base Address for IO */
#define PCICFG_PMBAR   0x000f7810      /* PCI Base Address for Memory */
#define PCICFG_CRR     0x000f7ef0      /* Component Reset Register */
#define PCICFG_PR      0x000f7f20      /* Personalization Register */
#define PCICFG_ACR     0x000f7f30      /* Arbiter Control Register */
#define PCICFG_PMSIZE  0x000f7f40      /* PCI Memory Address Space Size */
#define PCICFG_IOSIZE  0x000f7f60      /* PCI I/O Address Space Size */
#define PCICFG_SMBAR   0x000f7f80      /* System Base Address for PCI memory */
#define PCICFG_SIBAR   0x000f7fc0      /* System Base Address fo PCI I/O */
#define PCICFG_CTLRW   0x000f7fd0      /* Configuration Register R/W */
#define PCICFG_CTLRO   0x000f7fe0      /* Configuration Regiter R/O */
#define PCICFG_CFGA    0x000f8000      /* CONFIG_ADDR */
#define PCICFG_CFGD    0x000f8010      /* CONFIG_DATA */
#define PCICFG_PSSIZE  0x000f8100      /* PCI to Sytem CPU Address Space Size */
#define PCICFG_PPSIZE  0x000f8110      /* PCI to PCI Address Space Size */
#define PCICFG_BARPS   0x000f8120      /* CPU Base Address Register */
#define PCICFG_BARPP   0x000f8130      /* PCI Base Address Register */
#define PCICFG_PSBAR   0x000f8140      /* PCI Base Address Reg for CPU Access */
#define PCICFG_PPBAR   0x000f8150      /* PCI Base Address Reg for PCI Access */
#define PCICFG_BPMDLK  0x000f8200      /* Bottom of Peripheral Memory Space */
#define PCICFG_TPMDLK  0x000f8210      /* Top of Peripheral Memory Space */
#define PCICFG_BIODLK  0x000f8220      /* Bottom of Peripheral I/O Space */
#define PCICFG_TIODLK  0x000f8230      /* Top of Perioheral I/O Space */
#define PCICFG_DLKCTRL 0x000f8240	/* Deadlock control */
#define PCICFG_DLKDEV  0x000f8250	/* Deadlock device */
#define PCICFG_IT_ADD_RESET    0x000f8300      /* PCI64 Reset interrupt */
#define PCICFG_INT_SET 0x000f8310      /* Set of INTA,INTB, INTC,INTD on PCI64*/
#define PCICFG_CSTAT   0x000f9800      /* Channel Status Register */
#define PCICFG_PLSSR   0x000f9810      /* Processor Load/Store Register */ 

/****************************/
/* SYSTEM SPACE ADDRESS MAP */
/****************************/

/* Standard System Registers */

#define CPC710_PIDR    0xff000008      /* Physical Identifier Register */
#define CPC710_CNFR    0xff00000c      /* Connectivity Configuration Register */
#define CPC710_RSTR    0xff000010      /* Connectivity Reset Register */
#define CPC710_SPOR    0xff0000e8      /* Software POR Register */

/* Specific System Registers */

#define CPC710_UCTL    0xff001000      /* CPC710 & System Control Register */
#define CPC710_MPSR    0xff001010      /* Multiprocessor Semaphore Regoster */
#define CPC710_SIOC    0xff001020      /* System I/O Control */
#define CPC710_ABCNTL  0xff001030      /* 60x Arbiter Control Register */
#define CPC710_SRST    0xff001040      /* CPU Soft Reset Register */
#define CPC710_ERRC    0xff001050      /* Error Control Register */
#define CPC710_SESR    0xff001060      /* System Error Status Register */
#define CPC710_SEAR    0xff001070      /* System Error Address Register */
#define CPC710_PGCHP   0xff001100      /* Chip Program Register */
#define CPC710_RGBAN1  0xff001110      /* Free Register 1 */ 
#define CPC710_RGBAN2  0xff001120      /* Free Register 2 */
#define CPC710_GPDIR   0xff001130      /* GPIO Direction Register */
#define CPC710_GPIN    0xff001140      /* GPIO Input Register */
#define CPC710_GPOUT   0xff001150      /* GPIO Output Register */
#define CPC710_ATAS    0xff001160      /* Address Transfer Attr for Snoop Reg */
#define CPC710_AVDG    0xff001170      /* CPC710 Diagnostic Register */
#define CPC710_MCCR    0xff001200      /* Memory Controller Register */
#define CPC710_MESR    0xff001220      /* Memory Error Status Regster */
#define CPC710_MEAR    0xff001230      /* Memory Error Address Register */
#define CPC710_MCER0   0xff001300      /* Memory Config Extent Register 0 */
#define CPC710_MCER1   0xff001310      /* Memory Config Extent Register 1 */
#define CPC710_MCER2   0xff001320      /* Memory Config Extent Register 2 */
#define CPC710_MCER3   0xff001330      /* Memory Config Extent Register 3 */
#define CPC710_MCER4   0xff001340      /* Memory Config Extent Register 4 */
#define CPC710_MCER5   0xff001350      /* Memory Config Extent Register 5 */
#define CPC710_MCER6   0xff001360      /* Memory Config Extent Register 6 */
#define CPC710_MCER7   0xff001370      /* Memory Config Extent Register 7 */
#define CPC710_SIOR0   0xff001400      /* SIO Reg 0 (DIMM PDs) */
#define CPC710_SIOR1   0xff001420      /* SIO Reg 1 (Planar,DIMM,CPU, etc) */

/* SYSTEM STANDARD CONFIG */

#define CPC710_DCR     0xff200000      /* Device Characteristic Register */
#define CPC710_DID     0xff200004      /* Device ID Register */
#define CPC710_BAR     0xff200018      /* Base Address Register */

/* DEVICE SPECIFIC CONFIG SPACE */

#define CPC710_PCIENB  0xff201000      /* PCI Bar Enable Register */

#endif /* __CPC710_H__ */
