/*	$Id: elan520.h,v 1.1.1.1 2006/09/14 01:59:08 root Exp $ */

/*
 * Copyright (c) 2002 Patrik Lindergren   (www.lindergren.com)
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
 *	This product includes software developed by Patrik Lindergren.
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

#ifndef __ELAN520_H__
#define __ELAN520_H__

#define ELAN520_MMCR			0xfffef000

/*
 * CPU
 */
#define ELAN520_MMCR_REVID		0x00	/* ELanSC520 Microcontroller Revision ID */
#define	ELAN520_MMCR_CPUCTL		0x02	/* Am5x86(c) CPU Control */

/*
 * SDRAM Controller
 */
#define ELAN520_MMCR_DRCCTL		0x10	/* SDRAM Control */
#define ELAN520_MMCR_DRCTMCTL		0x12	/* SDRAM Timing Control */
#define ELAN520_MMCR_DRCCFG		0x14	/* SDRAM Bank Configuration */
#define ELAN520_MMCR_DRCBENDADR		0x18	/* SDRAM Bank 0-3 Ending Address */
#define ELAN520_MMCR_ECCCTL		0x20	/* ECC Control */
#define ELAN520_MMCR_ECCSTA		0x21	/* ECC Status */
#define ELAN520_MMCR_ECCCKBPOS		0x22	/* ECC Check Bit Position */
#define ELAN520_MMCR_ECCCKTEST		0x23	/* ECC Check Code Test */
#define ELAN520_MMCR_ECCSBADD		0x24	/* ECC Single-Bit Error Address */
#define ELAN520_MMCR_ECCMBADD		0x28	/* ECC Multi-Bit Error Address */

/*
 * SDRAM Buffer
 */
#define ELAN520_MMCR_DBCTL		0x40	/* SDRAM Buffer Control */

/*
 * ROM/Flash Controller
 */
#define ELAN520_MMCR_BOOTCSCTL		0x50	/* *BOOTCS Control */
#define ELAN520_MMCR_ROMCS1CTL		0x54	/* *ROMCS1 Control */
#define ELAN520_MMCR_ROMCS2CTL		0x56	/* *ROMCS2 Control */

/*
 * PCI Bus Host Bridge
 */
#define ELAN520_MMCR_HBCTL		0x60	/* Host Bridge Control */
#define ELAN520_MMCR_HBTGTIRQCTL	0x62	/* Host Bridge Target Interrupt Control */
#define ELAN520_MMCR_HBTGTIRQSTA	0x64	/* Host Bridge Target Interrupt Status */
#define ELAN520_MMCR_HBMSTIRQCTL	0x66	/* Host Bridge Master Interrupt Control */
#define ELAN520_MMCR_HBMSTIRQSTA	0x68	/* Host Bridge Master Interrupt Status */
#define ELAN520_MMCR_MSTINTADD		0x6c	/* Host Bridge Master Interrupt Address */

/*
 * System Arbitration
 */
#define ELAN520_MMCR_SYSARBCTL		0x70	/* System Arbiter Control */
#define ELAN520_MMCR_PCIARBSTA		0x71	/* PCI Bus Arbiter Status */
#define ELAN520_MMCR_SYSARBMENB		0x72	/* System Arbiter Master Enable */
#define ELAN520_MMCR_ARBPCICTL		0x74	/* Arbiter Priority Control */

/*
 * System Address Mapping
 */
#define ELAN520_MMCR_ADDDECCTL		0x80	/* Address Decode Control */
#define ELAN520_MMCR_WPVSTA		0x82	/* Write-Protect Violation Status */
#define ELAN520_MMCR_PAR0		0x88	/* Programmable Address Region 0 */
#define ELAN520_MMCR_PAR1		0x8c	/* Programmable Address Region 1 */
#define ELAN520_MMCR_PAR2		0x90	/* Programmable Address Region 2 */
#define ELAN520_MMCR_PAR3		0x94	/* Programmable Address Region 3 */
#define ELAN520_MMCR_PAR4		0x98	/* Programmable Address Region 4 */
#define ELAN520_MMCR_PAR5		0x9c	/* Programmable Address Region 5 */
#define ELAN520_MMCR_PAR6		0xa0	/* Programmable Address Region 6 */
#define ELAN520_MMCR_PAR7		0xa4	/* Programmable Address Region 7 */
#define ELAN520_MMCR_PAR8		0xa8	/* Programmable Address Region 8 */
#define ELAN520_MMCR_PAR9		0xac	/* Programmable Address Region 9 */
#define ELAN520_MMCR_PAR10		0xb0	/* Programmable Address Region 10 */
#define ELAN520_MMCR_PAR11		0xb4	/* Programmable Address Region 11 */
#define ELAN520_MMCR_PAR12		0xb8	/* Programmable Address Region 12 */
#define ELAN520_MMCR_PAR13		0xbc	/* Programmable Address Region 13 */
#define ELAN520_MMCR_PAR14		0xc0	/* Programmable Address Region 14 */
#define ELAN520_MMCR_PAR15		0xc4	/* Programmable Address Region 15 */

/*
 * GP Bus Controller
 */
#define ELAN520_MMCR_GPECHO		0xc00	/* GP Echo Mode */
#define ELAN520_MMCR_GPCSDW		0xc01	/* GP Chip Select Data Width */
#define ELAN520_MMCR_GPCSQUAL		0xc02	/* GP Chip Select Qualification */
#define ELAN520_MMCR_GPCSRT		0xc08	/* GP Chip Select Recovery Time */
#define ELAN520_MMCR_GPCSPW		0xc09	/* GP Chip Select Pulse Width */
#define ELAN520_MMCR_GPCSOFF		0xc0a	/* GP Chip Select Offset */
#define ELAN520_MMCR_GPRDW		0xc0b	/* GP Read Pulse Width */
#define ELAN520_MMCR_GPRDOFF		0xc0c	/* GP Read Offset */
#define ELAN520_MMCR_GPWRW		0xc0d	/* GP Write Pulse Width */
#define ELAN520_MMCR_GPWROFF		0xc0e	/* GP Write Offset */
#define ELAN520_MMCR_GPALEW		0xc0f	/* GPALE Pulse Width */
#define ELAN520_MMCR_GPALEOFF		0xc10	/* GPALE Offset */

/*
 * Programmable Input/Output
 */
#define ELAN520_MMCR_PIOPFS15_0		0xc20	/* PIO15-PIO0 Pin Function Select */
#define ELAN520_MMCR_PIOPFS31_16	0xc22	/* PIO31-PIO16 Pin Function Select */
#define ELAN520_MMCR_CSPFS		0xc24	/* Chip Select Pin Function Select */
#define ELAN520_MMCR_CLKSEL		0xc26	/* Clock Select */
#define ELAN520_MMCR_DSCTL		0xc28	/* Drive Strength Control */
#define ELAN520_MMCR_PIODIR15_0		0xc2a	/* PIO15-PIO0 Direction */
#define ELAN520_MMCR_PIODIR31_16	0xc2c	/* PIO31-PIO16 Direction */
#define ELAN520_MMCR_PIODATA15_0	0xc30	/* PIO15-PIO0 Data */
#define ELAN520_MMCR_PIODATA31_16	0xc32	/* PIO31-PIO16 Data */
#define ELAN520_MMCR_PIOSET15_0		0xc34	/* PIO15-PIO0 Set */
#define ELAN520_MMCR_PIOSET31_16	0xc36	/* PIO31-PIO16 Set */
#define ELAN520_MMCR_PIOCLEAR15_0	0xc38	/* PIO15-PIO0 Clear */
#define ELAN520_MMCR_PIOCLEAR31_16	0xc3a	/* PIO31-PIO16 Clear */

/*
 * Software Timer
 */
#define ELAN520_MMCR_SWTMRMILLI		0xc60	/* Software Timer Millisecond Count */
#define ELAN520_MMCR_SWTMRMICRO		0xc62	/* Software Timer Microsecond Count */
#define ELAN520_MMCR_SWTMRCFG		0xc64	/* Software Timer Configuration */

/*
 * General-Purpose Timers
 */
#define ELAN520_MMCR_GPTMRSTA		0xc70	/* GP Timers Status */
#define ELAN520_MMCR_GPTMR0CTL		0xc72	/* GP Timer 0 Mode/Control */
#define ELAN520_MMCR_GPTMR0CNT		0xc74	/* GP Timer 0 Count */
#define ELAN520_MMCR_GPTMR0MAXCMPA	0xc76	/* GP Timer 0 Maxcount Compare A */
#define ELAN520_MMCR_GPTMR0MAXCMPB	0xc78	/* GP Timer 0 Maxcount Compare B */
#define ELAN520_MMCR_GPTMR1CTL		0xc7a	/* GP Timer 1 Mode/Control */
#define ELAN520_MMCR_GPTMR1CNT		0xc7c	/* GP Timer 1 Count */
#define ELAN520_MMCR_GPTMR1MAXCMPA	0xc7e	/* GP Timer 1 Maxcount Compare A */
#define ELAN520_MMCR_GPTMR1MAXCMPB	0xc80	/* GP Timer 1 Maxcount Compare B */
#define ELAN520_MMCR_GPTMR2CTL		0xc82	/* GP Timer 2 Mode/Control */
#define ELAN520_MMCR_GPTMR2CNT		0xc84	/* GP Timer 2 Count */
#define ELAN520_MMCR_GPTMR2MAXCMPA	0xc8e	/* GP Timer 2 Maxcount Compare A */

/*
 * Watchdog Timer
 */
#define ELAN520_MMCR_WDTMRCTL		0xcb0	/* Watchdog Timer Control */
#define ELAN520_MMCR_WDTMRCNTL		0xcb2	/* Watchdog Timer Count Low */
#define ELAN520_MMCR_WDTMRCNTH		0xcb4	/* Watchdog Timer Count High */

/*
 * UART Serial Ports
 */
#define ELAN520_MMCR_UART1CTL		0xcc0	/* UART 1 General Control */
#define ELAN520_MMCR_UART1STA		0xcc1	/* UART 1 General Status */
#define ELAN520_MMCR_UART1FCRSHAD	0xcc2	/* UART 1 FIFO Control Shadow */
#define ELAN520_MMCR_UART2CTL		0xcc4	/* UART 2 General Control */
#define ELAN520_MMCR_UART2STA		0xcc5	/* UART 2 General Status */
#define ELAN520_MMCR_UART2FCRSHAD	0xcc6	/* UART 2 FIFO Control Shadow */

/*
 * Synchronous Serial Interface
 */
#define ELAN520_MMCR_SSICTL		0xcd0	/* SSI Control */
#define ELAN520_MMCR_SSIXMIT		0xcd1	/* SSI Transmit */
#define ELAN520_MMCR_SSICMD		0xcd2	/* SSI Command */
#define ELAN520_MMCR_SSISTA		0xcd3	/* SSI Status */
#define ELAN520_MMCR_SSIRCV		0xcd4	/* SSI Receive */

/*
 * Programmable Interrupt Controller
 */
#define ELAN520_MMCR__PICICR		0xd00	/* Interrupt Control */
#define ELAN520_MMCR_MPICMODE		0xd02	/* Master PIC Interrupt Mode */
#define ELAN520_MMCR_SL1PICMODE		0xd03	/* Slave 1 PIC Interrupt Mode */
#define ELAN520_MMCR_SL2PICMODE		0xd04	/* Slave 2 PIC Interrupt Mode */
#define ELAN520_MMCR_SWINT16_1		0xd08	/* Software Interrupt 16-1 Control */
#define ELAN520_MMCR_SWINT22_17		0xd0a	/* Software Interrupt 22-17/NMI Control */
#define ELAN520_MMCR_INTPINPOL		0xd10	/* Interrupt Pin Polarity */
#define ELAN520_MMCR_PCIHOSTMAP		0xd14	/* PCI Host Bridge Interrupt Mapping */
#define ELAN520_MMCR_ECCMAP		0xd18	/* ECC Interrupt Mapping */
#define ELAN520_MMCR_GPTMR0MAP		0xd1a	/* GP Timer 0 Interrupt Mapping */
#define ELAN520_MMCR_GPTMR1MAP		0xd1b	/* GP Timer 1 Interrupt Mapping */
#define ELAN520_MMCR_GPTMR2MAP		0xd1c	/* GP Timer 2 Interrupt Mapping */
#define ELAN520_MMCR_PIT0MAP		0xd20	/* PIT 0 Interrupt Mapping */
#define ELAN520_MMCR_PIT1MAP		0xd21	/* PIT 1 Interrupt Mapping */
#define ELAN520_MMCR_PIT2MAP		0xd22	/* PIT 2 Interrupt Mapping */
#define ELAN520_MMCR_UART1MAP		0xd28	/* UART 1 Interrupt Mapping */
#define ELAN520_MMCR_UART2MAP		0xd29	/* UART 2 Interrupt Mapping */
#define ELAN520_MMCR_PCIINTAMAP		0xd30	/* PCI Interrupt A Mapping */
#define ELAN520_MMCR_PCIINTBMAP		0xd31	/* PCI Interrupt B Mapping */
#define ELAN520_MMCR_PCIINTCMAP		0xd32	/* PCI Interrupt C Mapping */
#define ELAN520_MMCR_PCIINTDMAP		0xd33	/* PCI Interrupt D Mapping */
#define ELAN520_MMCR_DMABCINTMAP	0xd40	/* DMA Buffer Chaining Interrupt Mapping */
#define ELAN520_MMCR_SSIMAP		0xd41	/* SSI Interrupt Mapping */
#define ELAN520_MMCR_WDTMAP		0xd42	/* Watchdog Timer Interrupt Mapping */
#define ELAN520_MMCR_RTCMAP		0xd43	/* RTC Interrupt Mapping */
#define ELAN520_MMCR_WPVMAP		0xd44	/* Write-Protect Violation Interrupt Mapping */
#define ELAN520_MMCR_ICEMAP		0xd45	/* AMDebug(tm) Technology RX/TX Interrupt Mapping */
#define ELAN520_MMCR_FERRMAP		0xd46	/* Floating Point Error Interrupt Mapping */
#define ELAN520_MMCR_GP0IMAP		0xd50	/* GPIRQ0 Interrupt Mapping */
#define ELAN520_MMCR_GP1IMAP		0xd51	/* GPIRQ1 Interrupt Mapping */
#define ELAN520_MMCR_GP2IMAP		0xd52	/* GPIRQ2 Interrupt Mapping */
#define ELAN520_MMCR_GP3IMAP		0xd53	/* GPIRQ3 Interrupt Mapping */
#define ELAN520_MMCR_GP4IMAP		0xd54	/* GPIRQ4 Interrupt Mapping */
#define ELAN520_MMCR_GP5IMAP		0xd55	/* GPIRQ5 Interrupt Mapping */
#define ELAN520_MMCR_GP6IMAP		0xd56	/* GPIRQ6 Interrupt Mapping */
#define ELAN520_MMCR_GP7IMAP		0xd57	/* GPIRQ7 Interrupt Mapping */
#define ELAN520_MMCR_GP8IMAP		0xd58	/* GPIRQ8 Interrupt Mapping */
#define ELAN520_MMCR_GP9IMAP		0xd59	/* GPIRQ9 Interrupt Mapping */
#define ELAN520_MMCR_GP10IMAP		0xd5a	/* GPIRQ10 Interrupt Mapping */

/*
 * Reset Generation
 */
#define ELAN520_MMCR_SYSINFO		0xd70	/* System Board Information */
#define ELAN520_MMCR_RESCFG		0xd72	/* Reset Configuration */
#define ELAN520_MMCR_RESSTA		0xd74	/* Reset Status */

/*
 * GP DMA Controller
 */
#define ELAN520_MMCR_GPDMACTL		0xd80	/* GP-DMA Control */
#define ELAN520_MMCR_GPDMAMMIO		0xd81	/* GP-DMA Memory-Mapped I/O */
#define ELAN520_MMCR_GPDMAEXTCHMAPA	0xd82	/* GP-DMA Resource Channel Map A */
#define ELAN520_MMCR_GPDMAEXTCHMAPB	0xd84	/* GP-DMA Resource Channel Map B */
#define ELAN520_MMCR_GPDMAEXTPG0	0xd86	/* GP-DMA Channel 0 Extended Page */
#define ELAN520_MMCR_GPDMAEXTPG1	0xd87	/* GP-DMA Channel 1 Extended Page */
#define ELAN520_MMCR_GPDMAEXTPG2	0xd88	/* GP-DMA Channel 2 Extended Page */
#define ELAN520_MMCR_GPDMAEXTPG3	0xd89	/* GP-DMA Channel 3 Extended Page */
#define ELAN520_MMCR_GPDMAEXTPG4	0xd8a	/* GP-DMA Channel 4 Extended Page */
#define ELAN520_MMCR_GPDMAEXTPG5	0xd8b	/* GP-DMA Channel 5 Extended Page */
#define ELAN520_MMCR_GPDMAEXTPG6	0xd8c	/* GP-DMA Channel 6 Extended Page */
#define ELAN520_MMCR_GPDMAEXTPG7	0xd8d	/* GP-DMA Channel 7 Extended Page */
#define ELAN520_MMCR_GPDMAEXTTC3	0xd90	/* GP-DMA Channel 3 Extended Transfer Count */
#define ELAN520_MMCR_GPDMAEXTTC5	0xd91	/* GP-DMA Channel 5 Extended Transfer Count */
#define ELAN520_MMCR_GPDMAEXTTC6	0xd92	/* GP-DMA Channel 6 Extended Transfer Count */
#define ELAN520_MMCR_GPDMAEXTTC7	0xd93	/* GP-DMA Channel 7 Extended Transfer Count */
#define ELAN520_MMCR_GPDMABCCTL		0xd98	/* Buffer Chaining Control */
#define ELAN520_MMCR_GPDMABCSTA		0xd99	/* Buffer Chaining Status */
#define ELAN520_MMCR_GPDMABSINTENB	0xd9a	/* Buffer Chaining Interrupt Enable */
#define ELAN520_MMCR_GPDMABCVAL		0xd9b	/* Buffer Chaining Valid */
#define ELAN520_MMCR_GPDMANXTADDL3	0xda0	/* GP-DMA Channel 3 Next Address Low */
#define ELAN520_MMCR_GPDMANXTADDH3	0xda2	/* GP-DMA Channel 3 Next Address High */
#define ELAN520_MMCR_GPDMANXTADDL5	0xda4	/* GP-DMA Channel 5 Next Address Low */
#define ELAN520_MMCR_GPDMANXTADDH5	0xda6	/* GP-DMA Channel 5 Next Address High */
#define ELAN520_MMCR_GPDMANXTADDL6	0xda8	/* GP-DMA Channel 6 Next Address Low */
#define ELAN520_MMCR_GPDMANXTADDH6	0xdaa	/* GP-DMA Channel 6 Next Address High */
#define ELAN520_MMCR_GPDMANXTADDL7	0xdac	/* GP-DMA Channel 7 Next Address Low */
#define ELAN520_MMCR_GPDMANXTADDH7	0xdae	/* GP-DMA Channel 7 Next Address High */
#define ELAN520_MMCR_GPDMANXTTCL3	0xdb0	/* GP-DMA Channel 3 Next Transfer Count Low */
#define ELAN520_MMCR_GPDMANXTTCH3	0xdb2	/* GP-DMA Channel 3 Next Transfer Count High */
#define ELAN520_MMCR_GPDMANXTTCL5	0xdb4	/* GP-DMA Channel 5 Next Transfer Count Low */
#define ELAN520_MMCR_GPDMANXTTCH5	0xdb6	/* GP-DMA Channel 5 Next Transfer Count High */
#define ELAN520_MMCR_GPDMANXTTCL6	0xdb8	/* GP-DMA Channel 6 Next Transfer Count Low */
#define ELAN520_MMCR_GPDMANXTTCH6	0xdba	/* GP-DMA Channel 6 Next Transfer Count High */
#define ELAN520_MMCR_GPDMANXTTCL7	0xdbc	/* GP-DMA Channel 7 Next Transfer Count Low */
#define ELAN520_MMCR_GPDMANXTTCH7	0xdbe	/* GP-DMA Channel 7 Next Transfer Count High */


/*
 * PCI Configuration Access Registers
 */
#define ELAN520_PCICFGADDR		0x0cf8
#define ELAN520_PCICFGDATA		0x0cfc
   
#endif /* __ELAN520_H__ */
