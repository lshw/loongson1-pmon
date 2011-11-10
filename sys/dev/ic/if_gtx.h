/* $Id: if_gtx.h,v 1.1.1.1 2006/09/14 01:59:08 root Exp $ */

/*
 * Copyright (c) 2001 Allegro Networks (www.allegronetworks.com)
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
 *      This product includes software developed by Allegro Networks Inc.
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
#ifndef _IF_GTX_H
#define _IF_GTX_H

#include <target/pmon_target.h>

#if defined(POWERPC)
/* PPC uses the VA_TO_PA to get the physical address of the on-chip SRAM */
#define OCRAM_TO_PA(x)		VA_TO_PA(x)
#elif defined(MIPS)
/* MIPS maps the on-chip SRAM into the I/O space starting at 0xf0000000 */
/* Note that PA_TO_VA() works fine here, but mostly by accident... */
#define OCRAM_TO_PA(x)		((int)(x))
#else
#error	OCRAM_TO_PA needs to be defined for this architecure!
#endif


/* Keep the ring sizes a power of two for efficiency. */
#define TX_RING_SIZE    8	
#define RX_RING_SIZE	32
#define RX_BUF_SZ	1536	/* Size of each temporary Rx buffer.*/
#define TX_BUF_SZ	1536	/* Size of each temporary Tx buffer.*/


/* Serial Control Register */
#define	PSCR_PORTEN		(1 << 0)
#define	PSCR_FORCE_LINK_PASS	(1 << 1)
#define	PSCR_DIS_AN_DUPLEX	(1 << 2)
#define	PSCR_DIS_AN_FC		(1 << 3)
#define	PSCR_PAUSE_ADV		(1 << 4)
#define	PSCR_FORCE_FC_MODE(n)	(((n) & 3) << 5)
#define	PSCR_FORCE_BP_MODE(n)	(((n) & 3) << 7)
#define	PSCR_DIS_FORCE_LINK_FAIL	(1 << 10)
#define	PSCR_RETR_FOREVER	(1 << 11)
#define	PSCR_DIS_AN_SPEED	(1 << 13)
#define	PSCR_DTE_ADVERT		(1 << 14)
#define	PSCR_AN_BYPASS_EN	(1 << 15)
#define	PSCR_RESTART_AN		(1 << 16)
#define	PSCR_MRU(n)		(((n) & 7) << 17)
#define	PSCR_SET_FULLDX		(1 << 21)
#define	PSCR_SET_FC_EN		(1 << 22)
#define	PSCR_SET_GMII_SPEED	(1 << 23)
#define	PSCR_SET_MII_SPEED	(1 << 24)

#define	PSCR_DEFAULT_SETTING	\
    (PSCR_FORCE_LINK_PASS | PSCR_DIS_AN_FC | PSCR_PAUSE_ADV | 0x200 | \
     PSCR_DIS_FORCE_LINK_FAIL | PSCR_MRU(2) | PSCR_SET_FULLDX | PSCR_SET_FC_EN)

/* Bit definitions of the SMI Reg */
#define	SMI_PHYAD_0(n)	(((n) & 0x1f) << 0)
#define	SMI_PHYAD_1(n)	(((n) & 0x1f) << 5)
#define	SMI_PHYAD_2(n)	(((n) & 0x1f) << 10)

#define	SMI_DATA(n)	(((n) & 0xffff) << 0)
#define	SMI_PHYAD(n)	(((n) & 0x1f) << 16)
#define	SMI_REGAD(n)	(((n) & 0x1f) << 21)
#define	SMI_READ	(1 << 26)
#define	SMI_WRITE	(0 << 26)
#define	SMI_READVALID	(1 << 27)
#define	SMI_BUSY	(1 << 28)

/* Bit definitions of the Port Config Reg */
#define	PCR_UPM		1
#define	PCR_RXQ(n)	(((n) & 3) << 1)
#define	PCR_RXQARP(n)	(((n) & 7) << 4)
#define	PCR_RB		(1 << 7)
#define	PCR_RBIP	(1 << 8)
#define	PCR_RARP	(1 << 9)
#define	PCR_AMNOTXES	(1 << 12)
#define	PCR_TCP_CAPEN	(1 << 14)
#define	PCR_UDP_CAPEN	(1 << 15)
#define	PCR_TCPQ(n)	(((n) & 7) << 16)
#define	PCR_UDPQ(n)	(((n) & 7) << 19)
#define	PCR_BPDUQ(n)	(((n) & 7) << 22)

#define PCR_DEFAULT_SETTING \
    (PCR_RXQ(0) | PCR_RXQARP(0) | PCR_TCPQ(0) | PCR_UDPQ(0) | PCR_BPDUQ(0))

/* Bit definitions of the Port Config Extend Reg */
#define	PCXR_SPAN	(1 << 1)
#define	PCXR_PAREN	(1 << 2)

#define	PCXR_DEFAULT_SETTING	0

/* Bit definitions of the Port Command Reg */

/* Bit definitions of the Port Status Reg */

/* Bit definitions of the SDMA Config Reg */
#define	SDCR_RIFB	(1 << 0)
#define	SDCR_RXBSZ(n)	(((n) & 7) << 1)
#define	SDCR_BLMR	(1 << 4)
#define	SDCR_BLMT	(1 << 5)
#define	SDCR_SWAPMODE	(1 << 6)
#define	SDCR_IGP_INT_RX(n) (((n) & 0x3fff) << 8)
#define	SDCR_TXBSZ(n)	(((n) & 7) << 22)

#define	SDCR_BLMRT	(SDCR_BLMR | SDCR_BLMT)
#define	SDCR_DEFAULT_SETTING \
    (SDCR_RXBSZ(4) | SDCR_IGP_INT_RX(0) | SDCR_TXBSZ(4))

/* Bit definitions of the SDMA Command Reg */

/* Bit definitions of the Interrupt Cause Reg */
#define	ICR_RXBUFFER	(1 << 0)
#define	ICR_EXTEND	(1 << 1)
#define	ICR_RXBUFFERQ(n) ((1 << ((n) & 7)) << 2)
#define	ICR_RXERROR	(1 << 10)
#define	ICR_RXERRORQ(n)	((1 << ((n) & 7)) << 11)
#define	ICR_RTXEND	(0xff << 19)
#define	ICR_TXEND(n)	((1 << ((n) & 7)) << 19)
#define	ICR_ETHERINTSUM	(1 << 31)

#define	XICR_TXBUFF(n)	((1 << ((n) & 7)) << 0)
#define	XICR_TXERROR(n)	((1 << ((n) & 7)) << 8)
#define	XICR_PHYSTC	(1 << 16)
#define	XICR_RXOVR	(1 << 18)
#define	XICR_TXUDR	(1 << 19)
#define	XICR_LINKCAHNGE	(1 << 20)
#define	XICR_PARTITION	(1 << 21)
#define	XICR_AUTONEGDONE (1 << 22)
#define	XICR_INTERNAL_ADDR_ERROR (1 << 23)
#define	XICR_ETHER_INT_SUM (1 << 31)


/*
 * The Rx and Tx descriptor lists. (funny swapping rules...)
 */
#if BYTE_ORDER == LITTLE_ENDIAN
typedef struct {
    u_int32_t cmdstat;
    u_int16_t l4_chk;
    u_int16_t byte_cnt;
    u_int32_t buff_ptr;
    u_int32_t next;
    char *vbuff_ptr;
    u_int32_t pad[3];
} TX_DESC;

typedef struct {
    u_int32_t cmdstat;
    u_int16_t buf_size;
    u_int16_t byte_cnt;
    u_int32_t buff_ptr;
    u_int32_t next;
    struct mbuf *rx_mbuf;
    char *vbuff_ptr;
    u_int32_t pad[2];
} RX_DESC;
#else
typedef struct {
    u_int16_t byte_cnt;
    u_int16_t l4_chk;
    u_int32_t cmdstat;
    u_int32_t next;
    u_int32_t buff_ptr;
    char *vbuff_ptr;
    u_int32_t pad[3];
} TX_DESC;

typedef struct {
    u_int16_t byte_cnt;
    u_int16_t buf_size;
    u_int32_t cmdstat;
    u_int32_t next;
    u_int32_t buff_ptr;
    struct mbuf *rx_mbuf;
    char *vbuff_ptr;
    u_int32_t pad[2];
} RX_DESC;
#endif


/* Values for the Tx command-status descriptor entry. */
#define	TX_O		(1<<31)
#define	TX_AM		(1<<30)
#define	TX_EI		(1<<23)
#define	TX_GC		(1<<22)
#define	TX_F		(1<<21)
#define	TX_L		(1<<20)
#define	TX_P		(1<<19)
#define	TX_GIPCHK	(1<<18)
#define	TX_GL4CHK	(1<<17)
#define	TX_L4TYPE	(1<<16)
#define	TX_VLAN		(1<<15)
#define	TX_IPV4HDLEN	(0x0f<<11)
#define	TX_L4CHK_MODE	(1<<10)
#define	TX_LLC_SNAP	(1<<9)
#define	TX_EC		(3<<1)
#define	TX_ES		(1<<0)

/* Values for the Rx command-status descriptor entry. */
#define	RX_O		(1<<31)
#define	RX_L4CHKOK	(1<<30)
#define	RX_EI		(1<<29)
#define	RX_U		(1<<28)
#define	RX_F		(1<<27)
#define	RX_L		(1<<26)
#define	RX_IPHEADOK	(1<<25)
#define	RX_L3IP		(1<<24)
#define	RX_LAYER2EV2	(1<<23)
#define	RX_LAYER4	(3<<21)
#define	RX_BPDU		(1<<20)
#define	RX_VLAN		(1<<19)
#define	RX_L4CHK	(0xffff<<3)
#define	RX_EC		(3<<1)
#define	RX_ES		1

/* Bit fields of a Hash Table Entry */
enum hash_table_entry {
    hteValid = 1,
    hteSkip = 2,
    hteRD = 4
};

// The GT643x0 MIB counters
typedef struct {
    u_int32_t bytesReceived;
    u_int32_t bytesSent;
    u_int32_t framesReceived;
    u_int32_t framesSent;
    u_int32_t totalBytesReceived;
    u_int32_t totalFramesReceived;
    u_int32_t broadcastFramesReceived;
    u_int32_t multicastFramesReceived;
    u_int32_t CRCErrors;
    u_int32_t oversizeFrames;
    u_int32_t fragments;
    u_int32_t jabber;
    u_int32_t collisions;
    u_int32_t lateCollisions;
    u_int32_t frames64_bytes;
    u_int32_t frames65_127_bytes;
    u_int32_t frames128_255_bytes;
    u_int32_t frames256_511_bytes;
    u_int32_t frames512_1023_bytes;
    u_int32_t frames1024_MaxSize;
    u_int32_t rxErrors;
    u_int32_t droppedFrames;
    u_int32_t multicastFramesSent;
    u_int32_t broadcastFramesSent;
    u_int32_t undersizeFrames;
} mib_counters_t;

/*
 *      Network device statistics. Akin to the 2.0 ether stats but
 *      with byte counters.
 */

struct net_device_stats
{
        unsigned long   rx_packets;             /* total packets received */
        unsigned long   tx_packets;             /* total packets transmitted */
        unsigned long   rx_bytes;               /* total bytes received */
        unsigned long   tx_bytes;               /* total bytes transmitted */
        unsigned long   rx_errors;              /* bad packets received */
        unsigned long   tx_errors;              /* packet transmit problems */
        unsigned long   rx_dropped;             /* no space in linux buffers */
        unsigned long   tx_dropped;             /* no space available in linux */
        unsigned long   multicast;              /* multicast packets received */
        unsigned long   collisions;

        /* detailed rx_errors: */
        unsigned long   rx_length_errors;
        unsigned long   rx_over_errors;         /* receiver ring buff overflow */
        unsigned long   rx_crc_errors;          /* recved pkt with crc error */
        unsigned long   rx_frame_errors;        /* recv'd frame alignment error */
        unsigned long   rx_fifo_errors;         /* recv'r fifo overrun */
        unsigned long   rx_missed_errors;       /* receiver missed packet */

        /* detailed tx_errors */
        unsigned long   tx_aborted_errors;
        unsigned long   tx_carrier_errors;
        unsigned long   tx_fifo_errors;
        unsigned long   tx_heartbeat_errors;
        unsigned long   tx_window_errors;

        /* for cslip etc */
        unsigned long   rx_compressed;
        unsigned long   tx_compressed;
};

struct gtx_softc {
	struct device	sc_dev;		/* Generic device structures */
	void		*sc_ih;		/* Interrupt handler cookie */
	bus_space_tag_t	sc_st;		/* Bus space tag */
	bus_space_handle_t sc_sh;	/* Bus space handle */
	pci_chipset_tag_t sc_pc;	/* Chipset handle needed by mips */
	struct arpcom	arpcom;		/* Per interface network data */
	RX_DESC		*rx_ring;
	TX_DESC		*tx_ring;
	u_int32_t	*rx_hash;
	int		sc_port;	/* Easy access port number */
	int hash_mode;

    /*
     *  Tx buffers with less than 8 bytes
     *  of payload must be 8-byte aligned
     */
    u_int8_t* tx_buff;
    u_int8_t* rx_buff;

    int rx_next_out;		/* The next free ring entry to receive */
    int tx_next_in;		/* The next free ring entry to send */
    int tx_next_out;		/* The last ring entry the ISR processed */
    int tx_count;       	/* Packets waiting to be sent in Tx ring */
    int tx_queued;
    int tx_full;        	/* Tx ring is full */
    
    mib_counters_t mib;
    struct net_device_stats stats;

    int chip_rev;
    int phy_addr; 		/* PHY address */
    unsigned char phys[2]; 	/* MII device addresses */
};
#endif /* !_IF_GTX_H_ */
