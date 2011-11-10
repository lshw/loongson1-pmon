/* $Id: if_gt.h,v 1.1.1.1 2006/09/14 01:59:08 root Exp $ */

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
#ifndef _GTETH_H
#define _GTETH_H

#include <target/pmon_target.h>


#define ADDRESS_TABLE_ALIGNMENT             0x8
#define MAC_ENTRY_SIZE                      8
#define MAC_ADDRESS_STRING_SIZE             12
#define _8K_TABLE                           0

#define HASH_DEFUALT_MODE                   14
#define HASH_MODE                           13
#define HASH_SIZE                           12
#define PROMISCUOUS_MODE                    0

#define _8K_TABLE                           0
#define VALID								1
#define SKIP                                1<<1
#define SKIP_BIT                            1
#define HOP_NUMBER                          12
#define EIGHT_K                             0x8000
#define HALF_K                              0x8000/16

#define MAX_NUMBER_OF_ADDRESSES_TO_STORE    1000

#define NIBBLE_SWAPPING_16_BIT(X) (((X&0xf)<<4) | ((X&0xf0)>>4) | ((X&0xf00)<<4) | ((X&0xf000)>>4))
#define NIBBLE_SWAPPING_32_BIT(X) (((X&0xf)<<4) | ((X&0xf0)>>4) | ((X&0xf00)<<4) | ((X&0xf000)>>4) | ((X&0xf0000)<<4) | ((X&0xf00000)>>4) | ((X&0xf000000)<<4) | ((X&0xf0000000)>>4))
#define GT_NIBBLE(X) ((X&0x1)<<3)+((X&0x2)<<1)+((X&0x4)>>1)+((X&0x8)>>3)
#define FLIP_6_BITS(X) (((X&0x1)<<5)+((X&0x2)<<3)+((X&0x4)<<1)+((X&0x8)>>1)+((X&0x10)>>3)+((X&0x20)>>5))
#define FLIP_9_BITS(X) (((X&0x1)<<8)+((X&0x2)<<6)+((X&0x4)<<4)+((X&0x8)<<2)+((X&0x10)<<0)+((X&0x20)>>2)+((X&0x40)>>4)+((X&0x80)>>6)+((X&0x100)>>8))



typedef struct AddressTablestore
{
    unsigned int macL;
    unsigned int macH:16;
    unsigned int rd:1;
    unsigned int skip:1;
    unsigned int valid:1;


} ADDRESS_TABLE_STORE;


int hashTableFunction __P((u_int, u_int, int, int));
int initAddressTable __P((int, int, int, int));
int addAddressTableEntry __P((int, u_int, u_int, u_int, u_int));
boolean_t enableFiltering __P((int));
boolean_t disableFiltering __P((int port));
boolean_t scanAddressTable __P((int port));
boolean_t findFirstAddressTableEntry __P((int, u_int *,char *));
boolean_t addressTableDefragment __P((int, int, int, int));
boolean_t addressTableClear __P((int));


/* Keep the ring sizes a power of two for efficiency. */
#define TX_RING_SIZE    8	
#define TX_BUF_SZ	1536	/* Size of each temporary Tx buffer.*/
#define RX_RING_SIZE	8	
#define RX_BUF_SZ	1536	/* Size of each temporary Rx buffer.*/

#define RX_HASH_TABLE_SIZE 	16384
#define HASH_HOP_NUMBER 	12
#define ETH_IO_SIZE 		0x400

#define GTETH_READ(sc, off) \
		GT_READ((sc->port_offset + off))

#define GTETH_WRITE(sc, off, data) \
		GT_WRITE((sc->port_offset + off), data)

#define GTETH_SETBIT(sc, off, bits) {\
		u_int32_t val = GTETH_READ(sc, off) | (u_int32_t)(bits); \
		GTETH_WRITE(sc, off, val); }

#define GTETH_CLRBIT(sc, off, bits) {\
		u_int32_t val = GTETH_READ(sc, off) & (u_int32_t)(~(bits); \
		GTETH_WRITE(sc, off, val); }


/* Bit definitions of the SMI Reg */
enum {
    SMIR_DATAMASK = 0xffff,
    SMIR_PHYADMASK = 0x1f<<16,
    SMIR_PHYADBIT = 16,
    SMIR_REGADMASK = 0x1f<<21,
    SMIR_REGADBIT = 21,
    SMIR_READOP = 1<<26,
    SMIR_READVALID = 1<<27,
    SMIR_BUSY = 1<<28
};

/* Bit definitions of the Port Config Reg */
enum pcr_bits {
    pcrPM = 1,
    pcrRBM = 2,
    pcrPBF = 4,
    pcrEN = 1<<7,
    pcrLPBKMask = 0x3<<8,
    pcrLPBKBit = 8,
    pcrFC = 1<<10,
    pcrHS = 1<<12,
    pcrHM = 1<<13,
    pcrHDM = 1<<14,
    pcrHD = 1<<15,
    pcrISLMask = 0x7<<28,
    pcrISLBit = 28,
    pcrACCS = 1<<31
};

/* Bit definitions of the Port Config Extend Reg */
enum pcxr_bits {
    pcxrIGMP = 1,
    pcxrSPAN = 2,
    pcxrPAR = 4,
    pcxrPRIOtxMask = 0x7<<3,
    pcxrPRIOtxBit = 3,
    pcxrPRIOrxMask = 0x3<<6,
    pcxrPRIOrxBit = 6,
    pcxrPRIOrxOverride = 1<<8,
    pcxrDPLXen = 1<<9,
    pcxrFCTLen = 1<<10,
    pcxrFLP = 1<<11,
    pcxrFCTL = 1<<12,
    pcxrMFLMask = 0x3<<14,
    pcxrMFLBit = 14,
    pcxrMIBclrMode = 1<<16,
    pcxrSpeed = 1<<18,
    pcxrSpeeden = 1<<19,
    pcxrRMIIen = 1<<20,
    pcxrDSCPen = 1<<21
};

/* Bit definitions of the Port Command Reg */
enum pcmr_bits {
    pcmrFJ = 1<<15
};


/* Bit definitions of the Port Status Reg */
enum psr_bits {
    psrSpeed = 1,
    psrDuplex = 2,
    psrFctl = 4,
    psrLink = 8,
    psrPause = 1<<4,
    psrTxLow = 1<<5,
    psrTxHigh = 1<<6,
    psrTxInProg = 1<<7
};

/* Bit definitions of the SDMA Config Reg */
enum sdcr_bits {
    sdcrRCMask = 0xf<<2,
    sdcrRCBit = 2,
    sdcrBLMR = 1<<6,
    sdcrBLMT = 1<<7,
    sdcrPOVR = 1<<8,
    sdcrRIFB = 1<<9,
    sdcrBSZMask = 0x3<<12,
    sdcrBSZBit = 12
};

/* Bit definitions of the SDMA Command Reg */
enum sdcmr_bits {
    sdcmrERD = 1<<7,
    sdcmrAR = 1<<15,
    sdcmrSTDH = 1<<16,
    sdcmrSTDL = 1<<17,
    sdcmrTXDH = 1<<23,
    sdcmrTXDL = 1<<24,
    sdcmrAT = 1<<31
};

/* Bit definitions of the Interrupt Cause Reg */
enum icr_bits {
    icrRxBuffer = 1,
    icrTxBufferHigh = 1<<2,
    icrTxBufferLow = 1<<3,
    icrTxEndHigh = 1<<6,
    icrTxEndLow = 1<<7,
    icrRxError = 1<<8,
    icrTxErrorHigh = 1<<10,
    icrTxErrorLow = 1<<11,
    icrRxOVR = 1<<12,
    icrTxUdr = 1<<13,
    icrRxBufferQ0 = 1<<16,
    icrRxBufferQ1 = 1<<17,
    icrRxBufferQ2 = 1<<18,
    icrRxBufferQ3 = 1<<19,
    icrRxErrorQ0 = 1<<20,
    icrRxErrorQ1 = 1<<21,
    icrRxErrorQ2 = 1<<22,
    icrRxErrorQ3 = 1<<23,
    icrMIIPhySTC = 1<<28,
    icrSMIdone = 1<<29,
    icrEtherIntSum = 1<<31
};

/*
 * The Rx and Tx descriptor lists. (funny swapping rules...)
 */
#if BYTE_ORDER == LITTLE_ENDIAN
typedef struct {
    u_int32_t cmdstat;
    u_int32_t byte_cnt_res;
    u_int32_t buff_ptr;
    u_int32_t next;
} TX_DESC;

typedef struct {
    u_int32_t cmdstat;
    u_int32_t byte_sz_cnt;
    u_int32_t buff_ptr;
    u_int32_t next;
} RX_DESC;
#else
typedef struct {
    u_int32_t byte_cnt_res;
    u_int32_t cmdstat;
    u_int32_t next;
    u_int32_t buff_ptr;
} TX_DESC;

typedef struct {
    u_int32_t byte_sz_cnt;
    u_int32_t cmdstat;
    u_int32_t next;
    u_int32_t buff_ptr;
} RX_DESC;
#endif


/* Values for the Tx command-status descriptor entry. */
enum td_cmdstat {
    txOwn = 1<<31,
    txAutoMode = 1<<30,
    txEI = 1<<23,
    txGenCRC = 1<<22,
    txPad = 1<<18,
    txFirst = 1<<17,
    txLast = 1<<16,
    txErrorSummary = 1<<15,
    txReTxCntMask = 0x0f<<10,
    txReTxCntBit = 10,
    txCollision = 1<<9,
    txReTxLimit = 1<<8,
    txUnderrun = 1<<6,
    txLateCollision = 1<<5
};

#define TxReTxCntBit 10

/* Values for the Rx command-status descriptor entry. */
enum rd_cmdstat {
    rxOwn = 1<<31,
    rxAutoMode = 1<<30,
    rxEI = 1<<23,
    rxFirst = 1<<17,
    rxLast = 1<<16,
    rxErrorSummary = 1<<15,
    rxIGMP = 1<<14,
    rxHashExpired = 1<<13,
    rxMissedFrame = 1<<12,
    rxFrameType = 1<<11,
    rxShortFrame = 1<<8,
    rxMaxFrameLen = 1<<7,
    rxOverrun = 1<<6,
    rxCollision = 1<<4,
    rxCRCError = 1
};

/* Bit fields of a Hash Table Entry */
enum hash_table_entry {
    hteValid = 1,
    hteSkip = 2,
    hteRD = 4
};

// The GT64240 MIB counters
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

struct gt_softc {
    struct device sc_dev;         /* Generic device structures */
    void *sc_ih;                  /* Interrupt handler cookie */
    bus_space_tag_t sc_st;        /* Bus space tag */
    bus_space_handle_t sc_sh;     /* Bus space handle */
    pci_chipset_tag_t sc_pc;      /* Chipset handle needed by mips */
    struct arpcom arpcom;	  /* Per interface network data */
    RX_DESC* rx_ring;
    TX_DESC* tx_ring;
    u_int32_t* rx_hash;
    int hash_mode;

    /*
     *  Tx buffers with less than 8 bytes
     *  of payload must be 8-byte aligned
     */
    char* tx_bp;

    int rx_next_out;		/* The next free ring entry to receive */
    int tx_next_in;		/* The next free ring entry to send */
    int tx_next_out;		/* The last ring entry the ISR processed */
    int tx_count;       	/* Number of packets waiting to be sent in Tx ring */
    int tx_queued;
    int tx_full;        	/* Tx ring is full */
    
    struct mbuf **rx_m;		/* Receive buffer mbuf linkage */
    struct mbuf **tx_m;		/* Transmit buffer mbuf linkage */ 

    mib_counters_t mib;
    struct net_device_stats stats;

    int chip_rev;
    u_int32_t port_offset;
    int phy_addr; 		/* PHY address */
    unsigned char phys[2]; 	/* MII device addresses */
};
#endif
