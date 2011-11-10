/* $Id: if_gt.c,v 1.1.1.1 2006/09/14 01:59:08 root Exp $ */

/*
 * Copyright (c) 2001 Allegro Networks (www.allegronetworks.com)
 * Copyright (c) 2002 Opsycon AB.
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
 *      This product includes software developed by Opsycon AB.
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

#include "bpfilter.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/mbuf.h>
#include <sys/malloc.h>
#include <sys/kernel.h>
#include <sys/socket.h>
#include <sys/syslog.h>

#include <net/if.h>
#include <net/if_dl.h>
#include <net/if_media.h>
#include <net/if_types.h>

#ifdef INET
#include <netinet/in.h>
#include <netinet/in_systm.h>
#include <netinet/in_var.h>
#include <netinet/ip.h>
#endif

#ifdef IPX
#include <netipx/ipx.h>
#include <netipx/ipx_if.h>
#endif

#ifdef NS
#include <netns/ns.h>
#include <netns/ns_if.h>
#endif

#if 0
#if NBPFILTER > 0
#include <net/bpf.h>
#include <net/bpfdesc.h>
#endif
#endif

#include <sys/ioctl.h>
#include <sys/errno.h>
#include <sys/device.h>
#include <autoconf.h>

#include <netinet/if_ether.h>
#include <vm/vm.h>

#include <machine/cpu.h>
#include <machine/bus.h>
#include <machine/intr.h>

#include <dev/mii/miivar.h>

#include <dev/pci/pcivar.h>
#include <dev/pci/pcireg.h>
#include <dev/pci/pcidevs.h>

#include <dev/ic/if_gt.h>

extern int _pciverbose;

/* Prototypes */
static int gt_match 		(struct device *, void *, void *);
void gt_attach 			(struct device *, struct device *, void *);
static void abort 		(struct gt_softc *, u_int32_t);
static void reset_tx 		(struct gt_softc *);
static void reset_rx 		(struct gt_softc *);
static int gt_ioctl 		(struct ifnet *, u_long, caddr_t);
static void gt_init 		(void *);
static void gt_start 		(struct ifnet *);
static void gt_stop 		(struct gt_softc *, int);
static void gt_watchdog 	(struct ifnet *);
static int gt_add_rfabuf 	(struct gt_softc *, struct mbuf **);
static int gt_intr 		(void *);
static int gt_rx		(struct gt_softc *, u_int32_t);
static void read_mib_counters 	(struct gt_softc *);
int gt_miibus_readreg(void *, int, int);
int gt_miibus_writereg(void *, int, int, int);

void tgt_netstats 		(int);

int initAddressTable 	(int, int, int, int);
int addAddressTableEntry (int, uint, uint, uint, uint);

/* Compensate for lack of a generic ether_ioctl() */
static int      gt_ether_ioctl (struct ifnet *, u_int32_t, caddr_t);
#define ether_ioctl     gt_ether_ioctl

struct cfattach gt_ca = {                              
    sizeof(struct gt_softc), gt_match, gt_attach 
};                                                      
                                                        
struct cfdriver gt_cd = {                              
    NULL, "gt", DV_IFNET                           
};

/* Define ethernet MAC address */
extern char hwethadr[]; 

#define RFA_ALIGNMENT_FUDGE	2

/*
 * Check for Galileo gt642[46]0
 */
static int
gt_match(parent, match, aux)
    struct device *parent;
    void *match, *aux;
{
	return(1);		/* I suppose soooo... */
}

/* Attach the interface */
void 
gt_attach (parent, self, aux)
    struct device *parent, *self;
    void *aux;
{
    struct gt_softc *sc = (struct gt_softc *)self;
    struct confargs *cf = aux;
    struct ifnet *ifp;
    u_int32_t macH, macL;
    int i, isrmii;

    ifp = &sc->arpcom.ac_if;
    bcopy(sc->sc_dev.dv_xname, ifp->if_xname, IFNAMSIZ);
    bcopy(hwethadr, sc->arpcom.ac_enaddr, sizeof(sc->arpcom.ac_enaddr));
    sc->arpcom.ac_enaddr[5] += sc->sc_dev.dv_unit;
    printf(": address %s\n", ether_sprintf(sc->arpcom.ac_enaddr));

    sc->port_offset = sc->sc_dev.dv_unit * ETH_IO_SIZE;
    sc->phy_addr = cf->ca_baseaddr;

    ifp->if_softc = sc;
    ifp->if_flags = IFF_BROADCAST | IFF_SIMPLEX | IFF_MULTICAST;
    ifp->if_ioctl = gt_ioctl;
    ifp->if_start = gt_start; 
    ifp->if_watchdog = gt_watchdog;

   /* 
    *  Allocate Tx and Rx descriptor rings
    */
    sc->tx_ring = (TX_DESC *)(malloc(sizeof(TX_DESC) * TX_RING_SIZE,
                              M_DEVBUF, M_NOWAIT));
    sc->rx_ring = (RX_DESC *)(malloc(sizeof(RX_DESC) * RX_RING_SIZE,
                              M_DEVBUF, M_NOWAIT));

    sc->tx_m = (struct mbuf **)(malloc(sizeof(struct mbuf *) * TX_RING_SIZE,
                              M_DEVBUF, M_NOWAIT));
    sc->rx_m = (struct mbuf **)(malloc(sizeof(struct mbuf *) * RX_RING_SIZE,
                              M_DEVBUF, M_NOWAIT));
    bzero(sc->tx_m, sizeof(struct m_buf *) * TX_RING_SIZE);
    bzero(sc->rx_m, sizeof(struct m_buf *) * RX_RING_SIZE);

    /* 
     *  Allocate Tx data buffers
     */
    sc->tx_bp = malloc(TX_BUF_SZ*TX_RING_SIZE, M_DEVBUF, M_NOWAIT);

   /* 
    *  Initialize hash table
    */
    sc->hash_mode = 0;
    initAddressTable(sc->sc_dev.dv_unit,0,1,0);     
    macH = (sc->arpcom.ac_enaddr[0] << 8) | (sc->arpcom.ac_enaddr[1]);
    macL = (sc->arpcom.ac_enaddr[5] << 0) | (sc->arpcom.ac_enaddr[4] << 8) |
           (sc->arpcom.ac_enaddr[3] << 16)| (sc->arpcom.ac_enaddr[2] << 24);

    addAddressTableEntry(sc->sc_dev.dv_unit,macH,macL,1,0);

   /* 
    *  Reset port
    */
    gt_stop(sc,0);
    gt_miibus_writereg(sc, sc->phy_addr, 0, 0x8000);
    i = 100;
    while (i && gt_miibus_readreg(sc, sc->phy_addr, 0) & 0x8000) {
	delay(100);
	i--;
    }

   /* 
    *  Initialize Tx and Rx descriptors
    */
    reset_tx(sc);
    reset_rx(sc);

    GTETH_WRITE(sc, ETH0_INTERRUPT_MASK_REG, 0x0);

    /* 
     *  Setup eth0 Port Config Extend Reg, clear the MIB registers.
     *  When done set MIB counters clear mode to 'no effect' so each
     *  read doesn't zero the register.
     */
    isrmii = gt_miibus_readreg(sc, sc->phy_addr, 2) << 16;
    isrmii |= gt_miibus_readreg(sc, sc->phy_addr, 3) & 0xfff0;
    switch(isrmii) {
    case 0x20005c20:
    case 0x1378e0:	/* Intel LTX972A */
	isrmii = 0;
	break;
    default:
	isrmii = pcxrRMIIen;
	break;
    }

    GTETH_WRITE(sc, ETH0_PORT_CONFIG_EXT_REG,
				isrmii | pcxrFCTL | pcxrFCTLen | pcxrFLP);
    read_mib_counters(sc);
    GTETH_WRITE(sc, ETH0_PORT_CONFIG_EXT_REG,
				isrmii | pcxrFCTL | pcxrFCTLen | pcxrFLP |
		                pcxrPRIOrxOverride | pcxrMIBclrMode);

    /* 
     *  Big endian DMA, burst size 8 64bit words, frame boundary interrupts
     */
#if BYTE_ORDER == LITTLE_ENDIAN
    GTETH_WRITE(sc, ETH0_SDMA_CONFIG_REG,
				sdcrBLMR | sdcrBLMT |
    			        (0xf<<sdcrRCBit) | sdcrRIFB | (3<<sdcrBSZBit));
#else
    GTETH_WRITE(sc, ETH0_SDMA_CONFIG_REG,
    			        (0xf<<sdcrRCBit) | sdcrRIFB | (3<<sdcrBSZBit));
#endif

    /* 
     *  Start eth0 Rx DMA 
     */
    GTETH_WRITE(sc, ETH0_SDMA_COMMAND_REG, sdcmrERD);
    
    /* 
     *  Enable eth0 with 1/2K hash size
     */
    GTETH_WRITE(sc, ETH0_PORT_CONFIG_REG, pcrEN | pcrHS);

    /* 
     *  Attach the interface
     */
    if_attach(ifp);

    /*
     * Let the system queue as many packets as we have available 
     * Tx descriptors
     */
    ifp->if_snd.ifq_maxlen = TX_RING_SIZE - 1;
    ether_ifattach(ifp);
}

/*
 * Start packet transmission on the interface.
 */
static void
gt_start(ifp)
    struct ifnet *ifp;
{
    struct gt_softc *sc = ifp->if_softc;
    u_int16_t total_len;
    char *p;

   /*
    * Process all mbufs ready for transmit or until all available
    * transmit buffers are full.
    */
    CACHESYNC(&sc->tx_ring[sc->tx_next_in], sizeof(TX_DESC), SYNC_R);
    if(sc->tx_ring[sc->tx_next_in].cmdstat & txOwn) {
	return;		/* No buffers */
    }

    while (ifp->if_snd.ifq_head != NULL) {
        struct mbuf *m, *mb_head;
        u_int32_t nextTx;

        /*
         *  Grab a packet to transmit.
         */
        IF_DEQUEUE(&ifp->if_snd, mb_head);

        /*
         *  Go through each of the mbufs in the chain and copy the data
         *  collecting fragments to the transmit descriptors data buffer.
         */
        nextTx = sc->tx_next_in;
	total_len = 0;
	p = (char *)PA_TO_VA(sc->tx_ring[nextTx].buff_ptr);
        for (m = mb_head; m != NULL; m = m->m_next) {
            bcopy((char *)(mtod(m, vm_offset_t)), p, m->m_len);
            total_len += m->m_len;
	    p += m->m_len;
        }

        sc->tx_ring[nextTx].byte_cnt_res = total_len << 16;
	CACHESYNC((void *)(p - total_len), total_len, SYNC_W);

        sc->tx_next_in = (nextTx + 1) % TX_RING_SIZE;
	/* 
         *  Free the mbuf chain
         */
        m_freem(mb_head);

        sc->tx_ring[nextTx].cmdstat =
			txOwn | txFirst | txLast | txPad | txEI | txGenCRC;
	CACHESYNC(&sc->tx_ring[nextTx], sizeof(TX_DESC), SYNC_W);

        /* 
         *  Send the packet out the low priority queue
         */
        if(!(GTETH_READ(sc, ETH0_PORT_STATUS_REG) & psrTxInProg)) {
            GTETH_WRITE(sc, ETH0_SDMA_COMMAND_REG, sdcmrERD | sdcmrTXDL);
	}

#if 0
        /*
         *  Set a 5 second timer just in case we don't hear
  	 *  from the card again.
	 */
        ifp->if_timer = 300;
#endif
    } /* end while */

    sc->tx_queued++;
}

/*
 *  Stop the interface
 */
void
gt_stop(struct gt_softc *sc, int drain)
{

    /*
     *   Disable port
     */
    abort(sc, sdcmrAR | sdcmrAT);
    GTETH_WRITE(sc, ETH0_PORT_CONFIG_REG, 0);
}

static void
abort(struct gt_softc *sc, u_int32_t abort_bits)
{
    /* Return if neither Rx or Tx abort bits are set */
    if (!(abort_bits & (sdcmrAR | sdcmrAT)))
	return;

    /* Make sure only the Rx and Tx abort bits are set */
    abort_bits &= (sdcmrAR | sdcmrAT);

    /* Abort any Rx and Tx DMA immediately */
    GTETH_WRITE(sc, ETH0_SDMA_COMMAND_REG, abort_bits);
}

/* 
 * Watchdog timeout handler.  This routine is called when 
 * transmission has started on the interface and no
 * interrupt was received before the timeout.
 */
void
gt_watchdog(ifp)
    struct ifnet *ifp;
{
    struct gt_softc *sc = ifp->if_softc;

    log(LOG_ERR, "%s: device timeout\n", sc->sc_dev.dv_xname);
    ifp->if_oerrors++;
    //gt_init(sc);
}

static void
reset_tx(struct gt_softc *sc)
{
    int i;

    abort(sc, sdcmrAT);

    for (i=0; i < TX_RING_SIZE; i++) {
	sc->tx_ring[i].cmdstat  = 0; /* CPU owns */
	sc->tx_ring[i].byte_cnt_res = 0;
	sc->tx_ring[i].buff_ptr = (u_int32_t)VA_TO_PA(sc->tx_bp+i*TX_BUF_SZ);
	sc->tx_ring[i].next     = (u_int32_t)VA_TO_PA(sc->tx_ring + (i+1));
    }

    /* Wrap the ring. */
    sc->tx_ring[i-1].next = VA_TO_PA(sc->tx_ring);
    CACHESYNC(sc->tx_ring, TX_RING_SIZE * sizeof(TX_DESC), SYNC_W);

    /* setup only the lowest priority TxCDP reg */
    GTETH_WRITE(sc, ETH0_CURRENT_TX_DESC_PTR0, VA_TO_PA(sc->tx_ring));
    GTETH_WRITE(sc, ETH0_CURRENT_TX_DESC_PTR1, 0);

    /* Initialize Tx indeces and packet counter */
    sc->tx_next_in = 0;
    sc->tx_next_out = 0;
    sc->tx_count = 0;
}

static void
reset_rx(struct gt_softc *sc)
{
	struct mbuf *m;
	int i;

	abort(sc, sdcmrAR);
    
	/* Scan and release allocated mbufs */
	for (i=0; i<RX_RING_SIZE; i++) {
		if(sc->rx_m[i] != NULL) {
			m_free(sc->rx_m[i]);
		}
	}

	for (i=0; i < RX_RING_SIZE; i++) {
		RX_DESC *rx_desc;
		m = NULL;
		if(gt_add_rfabuf(sc, &m) < 0) {
			printf("%s: malloc failed\n", sc->sc_dev.dv_xname);
			break;
		}
		sc->rx_m[i] = m;
		rx_desc = &sc->rx_ring[i];
		rx_desc->next     = VA_TO_PA((sc->rx_ring + (i+1)));
		rx_desc->buff_ptr = VA_TO_PA(m->m_data);
		rx_desc->byte_sz_cnt = RX_BUF_SZ << 16;

		/*
		 *  Give ownership to device, set first and last,
		 * enable interrupt
		 */
		sc->rx_ring[i].cmdstat = (rxFirst | rxLast | rxOwn | rxEI);
    }

    /* Wrap the ring */
    sc->rx_ring[i-1].next = VA_TO_PA(sc->rx_ring);
    CACHESYNC(sc->rx_ring, RX_RING_SIZE * sizeof(RX_DESC), SYNC_W);

    /* Setup only the lowest priority RxFDP and RxCDP regs */
    for (i=0; i<4; i++) {
	if (i == 0) {
	    GTETH_WRITE(sc, ETH0_FIRST_RX_DESC_PTR0, VA_TO_PA(sc->rx_ring));
	    GTETH_WRITE(sc, ETH0_CURRENT_RX_DESC_PTR0, VA_TO_PA(sc->rx_ring));
	} else {
	    GTETH_WRITE(sc, ETH0_FIRST_RX_DESC_PTR0 + i*4, 0);
	    GTETH_WRITE(sc, ETH0_CURRENT_RX_DESC_PTR0 + i*4, 0);
	}
    }

    /* Initialize Rx index */
    sc->rx_next_out = 0;
}

int
gt_ioctl(ifp, command, data)
        struct ifnet *ifp;
        u_long command;
        caddr_t data;
{
        struct gt_softc *sc = ifp->if_softc;
        int s, error = 0;

        s = splimp();

        switch (command) {
        case SIOCPOLL:
                gt_intr(sc);
        	break;

        case SIOCSIFADDR:
                error = ether_ioctl(ifp, command, data);
                break;

        case SIOCSIFFLAGS:
                printf("case SIOCSIFFLAGS, marking interface up/down...\n");

                //sc->all_mcasts = (ifp->if_flags & IFF_ALLMULTI) ? 1 : 0;

                /*
                 * If interface is marked up and not running, then start it.
                 * If it is marked down and running, stop it.
                 * XXX If it's up then re-initialize it. This is so flags
                 * such as IFF_PROMISC are handled.
                 */
                if (ifp->if_flags & IFF_UP) {
                        gt_init(sc);
                } else {
                        if (ifp->if_flags & IFF_RUNNING)
                                gt_stop(sc, 1);
                }
                break;

#if USE_GT_MULTICAST
        case SIOCADDMULTI:
        case SIOCDELMULTI:
                //sc->all_mcasts = (ifp->if_flags & IFF_ALLMULTI) ? 1 : 0;
                error = (command == SIOCADDMULTI) ?
                    ether_addmulti(ifr, &sc->arpcom) :
                    ether_delmulti(ifr, &sc->arpcom);

                if (error == ENETRESET) {
                        /*
                         * Multicast list has changed; set the hardware
                         * filter accordingly.
                         */
                        /*if (!sc->all_mcasts)
                                fxp_mc_setup(sc);*/
                        /*
                         * fxp_mc_setup() can turn on all_mcasts if we run
                         * out of space, so check it again rather than else {}.
                         */
                        /*if (sc->all_mcasts)*/
                        if (0)
                                gt_init(sc);
                        error = 0;
                }
                break;
#endif

        case SIOCSIFMEDIA:
        case SIOCGIFMEDIA:
                /*error = ifmedia_ioctl(ifp, ifr, &sc->sc_mii.mii_media, command);*/
                break;

        default:
                error = EINVAL;
        }
        (void) splx(s);
        return (error);
}

void
gt_init(xsc)
    void *xsc;
{
    struct gt_softc *sc = xsc;
    struct ifnet *ifp = &sc->arpcom.ac_if;

    /*s = splimp();*/
    /*
     * Cancel any pending I/O
     */
    ifp->if_flags |= IFF_RUNNING;

    /* Stop and disable port, or reset to stable state */
    /*gt_stop(sc,0);*/
}

static int
gt_ether_ioctl(ifp, cmd, data)
        struct ifnet *ifp;
        u_int32_t cmd;
        caddr_t data;
{
        struct ifaddr *ifa = (struct ifaddr *) data;
        struct gt_softc *sc = ifp->if_softc;

        switch (cmd) {
#ifdef PMON
        case SIOCPOLL:
                gt_intr(sc);
                break;
#endif
        case SIOCSIFADDR:
                ifp->if_flags |= IFF_UP;

                switch (ifa->ifa_addr->sa_family) {
#ifdef INET
                case AF_INET:
                        gt_init(sc);
                        arp_ifinit(&sc->arpcom, ifa);
                        break;
#endif
#ifdef NS
                case AF_NS:
                    {
                         struct ns_addr *ina = &IA_SNS(ifa)->sns_addr;

                         if (ns_nullhost(*ina))
                                ina->x_host = *(union ns_host *)
                                    LLADDR(ifp->if_sadl);
                         else
                                bcopy(ina->x_host.c_host, LLADDR(ifp->if_sadl),
                                    ifp->if_addrlen);
                         /* Set new address. */
                         gt_init(sc);
                         break;
                    }
#endif
                default:
                        gt_init(sc);
                        break;
                }
                break;

        default:
                return (EINVAL);
        }

        return (0);
}

static int
gt_intr(arg)
    void *arg;
{
    struct gt_softc *sc = arg;
    struct ifnet *ifp = &sc->arpcom.ac_if;
    uint icr, psr;

    /*
     *  Read Interrupt Cause Register and ACK interrupts 
     */
    icr = GTETH_READ(sc, ETH0_INTERRUPT_CAUSE_REG);
    GTETH_WRITE(sc, ETH0_INTERRUPT_CAUSE_REG, 0);

    /*
     *  Process incoming packets if Rx descriptor returned to CPU ownership
     */
    gt_rx(sc, icr);

    /*
     *  Transmit more packets if queue isn't empty
     */
    if (ifp->if_snd.ifq_head != NULL)
        gt_start(ifp);

   /*  
    *  Check for Tx errors
    */
    if (icr & icrTxErrorLow)
        printf("%s: Tx resource error (low priority)\n", sc->sc_dev.dv_xname);
    if (icr & icrTxUdr)
        printf("%s: Tx underrun error\n", sc->sc_dev.dv_xname);

   /*  
    *  Check for Rx errors
    */
    if (icr & icrRxError) {
        //printf("%s: Rx resource error. Resetting Rx\n", sc->sc_dev.dv_xname);
        reset_rx(sc);
	/* Restart receive engine */
        GTETH_WRITE(sc, ETH0_SDMA_COMMAND_REG, sdcmrERD);
    }
    if (icr & icrRxOVR)
        printf("%s: Rx overrun\n", sc->sc_dev.dv_xname);

    /* 
     *  Check port status errors
     */
    psr = GTETH_READ(sc, ETH0_PORT_STATUS_REG);
    if (psr & psrPause)
        printf("%s: Pause!\n", sc->sc_dev.dv_xname);

    return(0);
}

static int
gt_rx(struct gt_softc *sc, u_int32_t status)
{
    struct ifnet *ifp = &sc->arpcom.ac_if;
    struct mbuf *m;
    int nextRx, cdp;
    RX_DESC *rd;
    u_int32_t cmdstat;
    u_int16_t total_len;

    /*
     *  Determine index to current descriptor
     */
    cdp = (GTETH_READ(sc, ETH0_CURRENT_RX_DESC_PTR0)
           - (u_int32_t)sc->rx_ring) / sizeof(RX_DESC);

    /* 
     *  Process to current descriptor
     */
    for (nextRx = sc->rx_next_out; nextRx != cdp;
         nextRx = (nextRx + 1) % RX_RING_SIZE) {

        rd = &sc->rx_ring[nextRx];
	CACHESYNC(rd, sizeof(RX_DESC), SYNC_R);
        cmdstat = (u_int32_t)rd->cmdstat;

	/* 
         *  Bail if gt owns descriptor.  This is the workaround for
         *  not relying on the icr register.
         */
        if (cmdstat & (u_int32_t)rxOwn) {
            break;
	}

        /* 
         *  Must be first and last (ie only) buffer of packet
         */
        if (!(cmdstat & (u_int32_t)rxFirst) || !(cmdstat & (u_int32_t)rxLast)) {
            printf("%s: descriptor not first and last!\n", sc->sc_dev.dv_xname);
            goto next;
        }

        /* 
         *  Drop this packet if there were any errors
         */
        if ((cmdstat & (u_int32_t)rxErrorSummary) || (status & icrRxError)) {
#ifdef DEBUG_GT
            printf("%s: dropped packet %p:%p\n",
		sc->sc_dev.dv_xname, cmdstat, status);
#endif
            goto next;
        }

	if((total_len = (rd->byte_sz_cnt & 0xffff)) > MCLBYTES) {
	    printf("%s: bad packet length %d\n", sc->sc_dev.dv_xname, total_len);
	    goto next;
	}

        /* 
         *  This is where the packets start to get processed to send
         *  to upper layers of the protocol stack.
         */
	m = sc->rx_m[nextRx];

        /* 
         *  Add a new buffer to the receive descriptor. The old
	 *  buffer is recycled if it fails to get a new buffer
	 *  and true is returned by gt_add_rfabuf.
         */
	if (!gt_add_rfabuf(sc, &sc->rx_m[nextRx])) {
	    struct ether_header *eh;

		rd->buff_ptr = VA_TO_PA(sc->rx_m[nextRx]->m_data);

	    total_len = rd->byte_sz_cnt & 0xffff;
	    if (total_len < sizeof(struct ether_header)) {
                printf("%s: buffer too small, freeing mbuf\n", sc->sc_dev.dv_xname);
	        m_freem(m);
		goto next;
	    }
	    m->m_pkthdr.rcvif = ifp;
	    m->m_pkthdr.len = m->m_len = total_len - sizeof(struct ether_header);

            /* 
             *  Realign data in mbuf. Discovery require receive buffer
	     *  to be 64 bit aligned so we can't do this before receive.
             */
            bcopy(m->m_data, m->m_data + RFA_ALIGNMENT_FUDGE, total_len);
	    m->m_data += RFA_ALIGNMENT_FUDGE;

            /*
             *  Send packet to upper layer
             */
            eh = mtod(m, struct ether_header *);
	    m->m_data += sizeof(struct ether_header);
	    ether_input(ifp, eh, m);
	}
	else {
	    printf("%s: gt recycling rxbuf!\n", sc->sc_dev.dv_xname);
	}

next:
	/* 
         *  Release ownership to device, set first and last, enable interrupt
         */
	rd->cmdstat = (u_int32_t)(rxFirst | rxLast | rxOwn | rxEI);
	CACHESYNC(rd, sizeof(RX_DESC), SYNC_W);
    }
    sc->rx_next_out = nextRx;
    return 0;
}

/*
 * Get a receive buffer and return it's data address.
 * Return 0 if recycled. The buffer is prepares such that
 * the pointer to "m" is stored in front of the data.
 */
int
gt_add_rfabuf(sc, oldm)
	struct gt_softc *sc;
	struct mbuf **oldm;
{
	struct mbuf *m, *pm;

	if(oldm != NULL) {
		pm = *oldm;
	}
	else {
		pm = NULL;
	}

	MGETHDR(m, M_DONTWAIT, MT_DATA);
	if (m != NULL) {
		MCLGET(m, M_DONTWAIT);
		if ((m->m_flags & M_EXT) == 0) {
			m_freem(m);
			if (pm == NULL) {
                                printf("gt_add_rfabuf: error 1\n");
				return -1;
                        }
			/* Recycle */
			m = pm;
			m->m_data = m->m_ext.ext_buf;
		}
	}
	else { /* Recycle */
		if (pm == NULL) {
                        printf("gt_add_rfabuf: error 2\n");
			return 1;
                }
		m = pm;
		m->m_data = m->m_ext.ext_buf;
	}

	/*
	 * Move the data pointer up so that the incoming data packet
	 * will be 64-bit aligned as requiered by the Discovery.
	 * The ether header is not a multiple of 4 bytes but the upper layer
	 * assumes data to be aligned so we will have to adjust this later.
	 */
	m->m_data = (void *)ALIGN(m->m_data);
	CACHESYNC((void *)m->m_data, RX_BUF_SZ, SYNC_R);

	*oldm = m;

	return (m == pm);
}

void 
read_mib_counters (sc)
    struct gt_softc *sc;
{
    u_int32_t *mib_reg = (u_int32_t *)&sc->mib;
    int i;

    for (i=0; i<sizeof(mib_counters_t)/sizeof(u_int32_t); i++) {
        mib_reg[i] = GTETH_READ(sc, ETH0_MIB_COUNTER_BASE + i*sizeof(u_int32_t));
    }
}

int
gt_miibus_readreg(arg, phy, reg)
	void *arg;
	int phy;
	int reg;
{
	struct gt_softc *sc = (struct gt_softc *)arg;
	unsigned int data = 0;
	u_int32_t phyreg;

	phyreg = GT_READ(ETH_PHY_ADDR_REG);

	if ((sc->sc_dev.dv_unit == 0 && (phyreg & 0x1f) != phy) ||
	    (sc->sc_dev.dv_unit == 1 && (phyreg & 0x3e0) != (phy << 5)) ||
	    (sc->sc_dev.dv_unit == 2 && (phyreg & 0x7c00) != (phy << 10)) ||
	    (sc->sc_dev.dv_unit > 2)) {
		return (0xffff);
	}

	phyreg = ((reg << SMIR_REGADBIT) & SMIR_REGADMASK) |
		 ((phy << SMIR_PHYADBIT) & SMIR_PHYADMASK) |
		 SMIR_READOP;
	GT_WRITE(ETH_SMI_REG, phyreg);
	do {
		phyreg = GT_READ(ETH_SMI_REG);
	} while (phyreg & SMIR_BUSY);

	data = GT_READ(ETH_SMI_REG);

	return (data & 0xffff);
}

int
gt_miibus_writereg(arg, phy, reg, data)
	void *arg;
	int phy;
	int reg;
	int data;
{
	struct gt_softc *sc = (struct gt_softc *)arg;
	u_int32_t phyreg;

	phyreg = GT_READ(ETH_PHY_ADDR_REG);

	if ((sc->sc_dev.dv_unit == 0 && (phyreg & 0x1f) != phy) ||
	    (sc->sc_dev.dv_unit == 1 && (phyreg & 0x3e0) != (phy << 5)) ||
	    (sc->sc_dev.dv_unit == 2 && (phyreg & 0x7c00) != (phy << 10)) ||
	    (sc->sc_dev.dv_unit > 2)) {
		return (0xffff);
	}

	phyreg = ((reg << SMIR_REGADBIT) & SMIR_REGADMASK) |
		 ((phy << SMIR_PHYADBIT) & SMIR_PHYADMASK) |
		 (data & SMIR_DATAMASK);
	GT_WRITE(ETH_SMI_REG, phyreg);
	do {
		phyreg = GT_READ(ETH_SMI_REG);
	} while (phyreg & SMIR_BUSY);

	return (0);
}

#define NUM_ETH_PORTS 	3
 
unsigned int addressTableHashMode[NUM_ETH_PORTS];     
unsigned int addressTableHashSize[NUM_ETH_PORTS];     
unsigned int addressTableBase[NUM_ETH_PORTS];
     
/*****************************************************************************
* 
* boolean_t initAddressTable(int port,int hashMode,int hashSize,int hashDefaultMode)
* 
* This function will initialize the address table
* and will enableFiltering.
* Inputs
* hashMode - hash mode 0 or hash mode 1.
* hashSize - the size of the hash table (0=8K ,1=1/2K)
* hashDefaultMode - 0 = discard addresses not found in the address table ,
* 1 = pass addresses not found in the address table.
* port - ETHERNET port number.
* Outputs                                                      
* address table is allocated and initialized.
* TRUE if success.
* FALSE if fail to make the assignment.
*/
int initAddressTable(int port,int hashMode,int hashSize,int hashDefaultMode)
{
u_int32_t portControlReg;
unsigned int hashLengh[NUM_ETH_PORTS];

    hashLengh[0] = EIGHT_K; 
    hashLengh[1] = HALF_K;
    addressTableHashMode[port] = hashMode;
    addressTableHashSize[port] = hashSize;

    /* allocate memory for the address table */
    addressTableBase[port] = (unsigned int)malloc(hashLengh[hashSize]*MAC_ENTRY_SIZE,
                                                  M_DEVBUF, M_NOWAIT);
    /*
     *  Set-up ptr to hash table
     */
    GT_WRITE(ETH0_HASH_TABLE_PTR_REG+port*ETHERNET_PORTS_DIFFERENCE_OFFSETS,addressTableBase[port]);

    /* fill all the address table with 0's */
    memset((void*)addressTableBase[port],0,hashLengh[hashSize]*MAC_ENTRY_SIZE);

    /* set hash size hash mode and HDM in the PCR */
    portControlReg = 0x00000081;
    portControlReg = portControlReg & ~(1 << HASH_DEFUALT_MODE);
    portControlReg = portControlReg & ~(1 << HASH_MODE);
    portControlReg = portControlReg & ~(1 << HASH_SIZE);

    portControlReg = portControlReg |
         (hashDefaultMode<<HASH_DEFUALT_MODE) |
         (hashMode << HASH_MODE) |
         (hashSize << HASH_SIZE);

    GT_WRITE(ETH0_PORT_CONFIG_REG+port*ETHERNET_PORTS_DIFFERENCE_OFFSETS, portControlReg);

    /* enableFiltering */
    enableFiltering(port);

    return TRUE;
}

  
/*****************************************************************************
* 
* This function will add an entry to the address table and 
* depends on the hash mode and hash size that was initialized.
* Inputs
*   port - ETHERNET port number.
*   macH - the 2 most significant bytes of the MAC address.
*   macL - the 4 least significant bytes of the MAC address.
*   skip - if 1 , skip this address.
*   rd - the RD field in the address table.
* Outputs
*   address table entry is added.
*   TRUE if success.
*   FALSE if fail to make the assignment.
*/
int
addAddressTableEntry(port, macH, macL, rd, skip)
    int port;
    unsigned int macH;
    unsigned int macL;
    unsigned int rd;
    unsigned int skip;
{
    unsigned int addressHighValue;
    unsigned int addressLowValue;
    void* entryNumber;
    int hashBase = addressTableBase[port]; 
    int i;
    unsigned int addressLowRead;
    unsigned int addressHighRead;

    entryNumber = (void*)(hashBase + 
                          MAC_ENTRY_SIZE*hashTableFunction(macH,macL,(int)addressTableHashSize[port],
                                                           (int)addressTableHashMode[port]));
    addressLowValue = VALID | (skip<<SKIP_BIT) | (rd<<2) | (((macH>>8)&0xf)<<3) | (((macH>>12)&0xf)<<7) |
		      (((macH>>0)&0xf)<<11) | (((macH>>4)&0xf)<<15) | 
		      (((macL>>24)&0xf)<<19) | (((macL>>28)&0xf)<<23) | (((macL>>16)&0xf)<<27) |
		      ((((macL>>20)&0x1)<<31));

    addressHighValue = ((macL>>21)&0x7) | (((macL>>8)&0xf)<<3) | (((macL>>12)&0xf)<<7) | 
	               (((macL>>0)&0xf)<<11) | (((macL>>4)&0xf)<<15);

    /* find a free place */
    for(i = 0 ; i < HOP_NUMBER ; i++)
    {
        addressLowRead = *(unsigned int*)(entryNumber+i*MAC_ENTRY_SIZE+4);
        if((!(addressLowRead & VALID))/* || (addressLowRead & SKIP)*/)
        {
            entryNumber = entryNumber+i*MAC_ENTRY_SIZE;
            break;
        }
        else /* if the address is the same locate it at the same position */
        {
            addressHighRead = *(unsigned int*)(entryNumber+i*MAC_ENTRY_SIZE);
            if(((addressLowRead>>3)&0x1fffffff)==((addressLowValue>>3)&0x1fffffff)
                && ((addressHighRead/*&0x7ffff*/)==(addressHighValue/*&0x7ffff*/)))
            {
                printf("addressHighRead = 0x%x addressHighValue = 0x%x\n",addressHighRead,addressHighValue);
                entryNumber = entryNumber+i*MAC_ENTRY_SIZE;
                break;
            }
        }
            
    }

    if(i == HOP_NUMBER)
    {
        printf("The address table section is full\n");
        return 1;
    }

    /* write the address to the address table */
    *(unsigned int*)(entryNumber) = addressHighValue;
    *(unsigned int*)(entryNumber+4) = addressLowValue;

    return 0;
}


/*****************************************************************************
* 
* int hashTableFunction(unsigned int macH,unsigned int macL,int HashSize,int hash_mode)
* 
* This function will calculate the hash function of the address.
* depends on the hash mode and hash size.
* Inputs
* macH - the 2 most significant bytes of the MAC address.
* macL - the 4 least significant bytes of the MAC address.
* hashMode - hash mode 0 or hash mode 1.
* hashSize - the size of the hash table (0=8K ,1=1/2K).
* Outputs
* return the caculated entry.
* TRUE if success.
* FALSE if fail to make the assignment.
*/
int hashTableFunction(unsigned int macH,unsigned int macL,int HashSize,int hash_mode)
{
unsigned int hashResult;
unsigned int ethernetAddH;
unsigned int ethernetAddL;
unsigned int ethernetAdd0;
unsigned int ethernetAdd1;
unsigned int ethernetAdd2;
unsigned int ethernetAdd3;
unsigned int ethernetAddHSwapped = 0;
unsigned int ethernetAddLSwapped = 0;

	ethernetAddH = NIBBLE_SWAPPING_16_BIT(macH);
	ethernetAddL = NIBBLE_SWAPPING_32_BIT(macL);

	ethernetAddHSwapped = GT_NIBBLE(ethernetAddH&0xf)+((GT_NIBBLE((ethernetAddH>>4)&0xf))<<4)+
		((GT_NIBBLE((ethernetAddH>>8)&0xf))<<8)+((GT_NIBBLE((ethernetAddH>>12)&0xf))<<12);


	ethernetAddLSwapped = GT_NIBBLE(ethernetAddL&0xf)+((GT_NIBBLE((ethernetAddL>>4)&0xf))<<4)+
		((GT_NIBBLE((ethernetAddL>>8)&0xf))<<8)+((GT_NIBBLE((ethernetAddL>>12)&0xf))<<12)+
		((GT_NIBBLE((ethernetAddL>>16)&0xf))<<16)+((GT_NIBBLE((ethernetAddL>>20)&0xf))<<20)+
		((GT_NIBBLE((ethernetAddL>>24)&0xf))<<24)+((GT_NIBBLE((ethernetAddL>>28)&0xf))<<28);



	ethernetAddH = ethernetAddHSwapped;
	ethernetAddL = ethernetAddLSwapped;

	if(hash_mode == 0)
	{
		ethernetAdd0 = (ethernetAddL>>2) & 0x3f;
		ethernetAdd1 = (ethernetAddL & 0x3) | ((ethernetAddL>>8) & 0x7f)<<2;
		ethernetAdd2 = (ethernetAddL>>15) & 0x1ff;
		ethernetAdd3 = ((ethernetAddL>>24) & 0xff) | ((ethernetAddH & 0x1)<<8);

	}
	else
	{
		ethernetAdd0 = FLIP_6_BITS((ethernetAddL) & 0x3f);
		ethernetAdd1 = FLIP_9_BITS(((ethernetAddL>>6) & 0x1ff));
		ethernetAdd2 = FLIP_9_BITS((ethernetAddL>>15) & 0x1ff);
		ethernetAdd3 = FLIP_9_BITS((((ethernetAddL>>24) & 0xff) | ((ethernetAddH & 0x1)<<8)));

	}

	hashResult = (ethernetAdd0<<9) | (ethernetAdd1^ethernetAdd2^ethernetAdd3);

	if(HashSize == _8K_TABLE)
	{
		hashResult = hashResult & 0xffff;

	}
	else
	{
		hashResult = hashResult & 0x7ff;
	}

	return hashResult;
}




/*****************************************************************************
* 
* boolean_t enableFiltering(int port)
* 
* This function will set the Promiscuous mode to normal mode.
* in ordet to enable Filtering.
* Inputs
* port - ETHERNET port number.
* Outputs
* address filtering will be enabled.
* TRUE if success.
* FALSE if fail to make the assignment.
*/
boolean_t enableFiltering(int port)
{
u_int32_t portControlReg;

    portControlReg = GT_READ(ETH0_PORT_CONFIG_REG+port*ETHERNET_PORTS_DIFFERENCE_OFFSETS);

    portControlReg = portControlReg & ~(1<<PROMISCUOUS_MODE);

    GT_WRITE(ETH0_PORT_CONFIG_REG+port*ETHERNET_PORTS_DIFFERENCE_OFFSETS, portControlReg);

    return TRUE;
}



/*****************************************************************************
* 
* bool disableFiltering(int port)
* 
* This function will set the Promiscuous mode to Promiscuous mode.
* in order to disable Filtering.
* Inputs
* port - ETHERNET port number.
* Outputs
* address filtering will be enabled.
* TRUE if success.
* FALSE if fail to make the assignment.
*/
boolean_t disableFiltering(int port)
{
u_int32_t portControlReg;

    portControlReg = GT_READ(ETH0_PORT_CONFIG_REG+port*ETHERNET_PORTS_DIFFERENCE_OFFSETS);

    portControlReg = portControlReg | (1<<PROMISCUOUS_MODE);

    GT_WRITE(ETH0_PORT_CONFIG_REG+port*ETHERNET_PORTS_DIFFERENCE_OFFSETS, portControlReg);

    return TRUE;
}



/*****************************************************************************
* 
* boolean_t scanAddressTable(int port)
* 
* This function will scan and print all the address table valid entries.
* Inputs
* port - ETHERNET port number.
* TRUE if success.
* FALSE if fail to make the assignment.
*/
boolean_t scanAddressTable(int port)
{
unsigned int entry=0;
char dummyBuffer[100];

    while(findFirstAddressTableEntry(port,&entry,dummyBuffer) != FALSE);


    return TRUE;
}



/*****************************************************************************
* 
* boolean_t findFirstAddressTableEntry(int port,unsigned int* startEntry)
* 
* This function will scan and print the first address table valid entry.
* Inputs
* port - ETHERNET port number.
* TRUE if success.
* FALSE if fail to make the assignment.
*/
boolean_t findFirstAddressTableEntry(int port,unsigned int* startEntry,char* resultBuffer)
{
unsigned int currentEntry;
unsigned int hashLengh[NUM_ETH_PORTS];
unsigned int addressLowRead;
unsigned int addressHighRead;
unsigned int addressLowValue;
unsigned int addressHighValue;
unsigned int endTable;
int hashSize;
unsigned int firstEntry;

    hashLengh[0] = EIGHT_K; 
    hashLengh[1] = HALF_K;

    hashSize = addressTableHashSize[port];
    endTable = (addressTableBase[port]+hashLengh[hashSize]*MAC_ENTRY_SIZE);
    firstEntry = *startEntry;

    currentEntry = addressTableBase[port]+((firstEntry)*MAC_ENTRY_SIZE);
    while(currentEntry < endTable)
    {
        addressLowRead = *(unsigned int*)(currentEntry+4);
        if(addressLowRead & VALID)
        {
            addressHighRead = *(unsigned int*)(currentEntry);
            addressLowValue = (((addressHighRead>>11)&0xf)<<0) | (((addressHighRead>>15)&0xf)<<4) |
                (((addressHighRead>>3)&0xf)<<8) | (((addressHighRead>>7)&0xf)<<12) |
                (((addressLowRead>>27)&0xf)<<16) | (((addressHighRead>>0)&0x7)<<21) |
                (((addressLowRead>>31)&0x1)<<20) | (((addressLowRead>>19)&0xf)<<24) |
                (((addressLowRead>>23)&0xf)<<28);
            addressHighValue = (((addressLowRead>>11)&0xf)<<0) | (((addressLowRead>>15)&0xf)<<4) |
                (((addressLowRead>>3)&0xf)<<8) | (((addressLowRead>>7)&0xf)<<12);
            printf("valid address %04x%08x was found in entry # 0x%x SKIP=%d RD=%d\n",
                   addressHighValue,addressLowValue,firstEntry,
                   (addressLowRead>>1)&0x1,(addressLowRead>>2)&0x1);

            sprintf(resultBuffer,"%x!!!%02x!!!%02x!!!%02x!!!%02x!!!%02x!!!%02x!!!%x!!!%x!!!%x",firstEntry*8,(addressHighValue>>8)&0xff,
                    (addressHighValue>>0)&0xff,(addressLowValue>>24)&0xff,(addressLowValue>>16)&0xff,
                    (addressLowValue>>8)&0xff,(addressLowValue>>0)&0xff,
                    ((addressLowRead>>2)&0x1),((addressLowRead>>1)&0x1),VALID);
                    
            firstEntry++;
            *startEntry = firstEntry;
            return TRUE;
        }
        currentEntry += MAC_ENTRY_SIZE;
        firstEntry ++;

    }


    return FALSE;
}



/*****************************************************************************
* 
* boolean_t addressTableDefragment(int port,int hashMode,int hashSize)
* 
* This function will save all adresses in memory reinitialize the address table  
* and add all the addresses to the new table.
* Inputs
* hashMode - hash mode 0 or hash mode 1.
* hashSize - the size of the hash table.
* port - ETHERNET port number.
* Outputs
* address table is reinitialized with the old addresses all skip addresses 
* will be deleted.
* TRUE if success.
* FALSE if fail to make the assignment.
*/
boolean_t addressTableDefragment(int port,int hashMode,int hashSize,int hashDefaultMode)
{
ADDRESS_TABLE_STORE addresses[MAX_NUMBER_OF_ADDRESSES_TO_STORE];
u_int32_t numberOfAddresses = 0;
u_int32_t entry;
u_int32_t endTable;
u_int32_t addressLowRead;
u_int32_t addressHighRead;
u_int32_t addressLowValue;
u_int32_t addressHighValue;
u_int32_t hashLengh[NUM_ETH_PORTS];
u_int32_t portControlReg;
int i;

    hashLengh[0] = EIGHT_K; 
    hashLengh[1] = HALF_K;

    /* keep all addresses in memory */
    endTable = (addressTableBase[port]+hashLengh[addressTableHashSize[port]]*MAC_ENTRY_SIZE);
    for(entry = addressTableBase[port] ; entry < endTable ; entry+=MAC_ENTRY_SIZE)
    {
        addressLowRead = *(unsigned int*)(entry+4);
        if(addressLowRead & VALID)
        {
            addressHighRead = *(unsigned int*)(entry);
            addressLowValue = (((addressHighRead>>11)&0xf)<<0) | (((addressHighRead>>15)&0xf)<<4) |
                (((addressHighRead>>3)&0xf)<<8) | (((addressHighRead>>7)&0xf)<<12) |
                (((addressLowRead>>27)&0xf)<<16) | (((addressHighRead>>0)&0x7)<<21) |
                (((addressLowRead>>31)&0x1)<<20) | (((addressLowRead>>19)&0xf)<<24) |
                (((addressLowRead>>23)&0xf)<<28);
            addressHighValue = (((addressLowRead>>11)&0xf)<<0) | (((addressLowRead>>15)&0xf)<<4) |
                (((addressLowRead>>3)&0xf)<<8) | (((addressLowRead>>7)&0xf)<<12);
            addresses[numberOfAddresses].macH = addressHighValue;
            addresses[numberOfAddresses].macL = addressLowValue;
            addresses[numberOfAddresses].rd = (addressLowRead>>2)&0x1;
            addresses[numberOfAddresses].skip = (addressLowRead>>1)&0x1;
            addresses[numberOfAddresses].valid = (addressLowRead)&0x1;
            numberOfAddresses++;
            if(numberOfAddresses > MAX_NUMBER_OF_ADDRESSES_TO_STORE)
            {
                printf("can't store so many addresses\n");
                return 1;
            } /* if(numberOfAddresses > MAX_NUMBER_OF_ADDRESSES_TO_STORE) */

        } /* if(addressLowRead & VALID) */
    } /* for(entry = addressTableBase[port] ; entry < endTable ; entry+=MAC_ENTRY_SIZE) */

    /* clear the table */
    addressTableClear(port);

    /* if the table size is the same use the same table else allocate new one */
    /* if the new table is smaller - do not allocate a new one */
    if((hashSize == addressTableHashSize[port]) || (addressTableHashSize[port] == _8K_TABLE))
    {
        portControlReg = GT_READ(ETH0_PORT_CONFIG_REG+port*ETHERNET_PORTS_DIFFERENCE_OFFSETS);

        portControlReg = portControlReg & ~(1 << HASH_DEFUALT_MODE);
        portControlReg = portControlReg & ~(1 << HASH_MODE);
        portControlReg = portControlReg & ~(1 << HASH_SIZE);

        portControlReg = portControlReg |
            (hashDefaultMode<<HASH_DEFUALT_MODE) |
             (hashMode << HASH_MODE) |
             (hashSize << HASH_SIZE);

        GT_WRITE(ETH0_PORT_CONFIG_REG+port*ETHERNET_PORTS_DIFFERENCE_OFFSETS, portControlReg);
    }
    else
    {
        initAddressTable(port,hashMode,hashSize,hashDefaultMode);
    }

    /* add all the addresses to the table */
    for(i = 0 ; i < numberOfAddresses ; i++)
    {
        if((addresses[i].skip == 0) & (addresses[i].valid == 1))
        {
            /* add only the valid and unskipped addresses */
            addAddressTableEntry(port,addresses[i].macH,addresses[i].macL,addresses[i].rd,addresses[i].skip);
        }
    }



    return 0;
}



/*****************************************************************************
* 
* boolean_t addressTableClear(int port)
* 
* This function will clear the address table  
* Inputs
* port - ETHERNET port number.
* Outputs
* address table is clear.
* TRUE if success.
* FALSE if fail to make the assignment.
*/
boolean_t addressTableClear(int port)
{
unsigned int hashLengh[NUM_ETH_PORTS];

    hashLengh[0] = EIGHT_K; 
    hashLengh[1] = HALF_K;

    memset((void*)addressTableBase[port],0,hashLengh[addressTableHashSize[port]]*MAC_ENTRY_SIZE);

    return TRUE;
}
