/* $Id: if_gtx.c,v 1.1.1.1 2006/09/14 01:59:08 root Exp $ */

/*
 * Copyright (c) 2001 Allegro Networks (www.allegronetworks.com)
 * Copyright (c) 2002 Opsycon AB (www.opsycon.se)
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

/*
 *  This driver is based on the if_gt.c code. It was ported over to
 *  the GT64360 Gigabit ethernet and made simple and functional for
 *  PMON2000. It is not optimized for performance etc but tailored
 *  to be small and efficient for the task it is designed for, booting.
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

#include <netinet/if_ether.h>
#include <vm/vm.h>

#include <machine/cpu.h>
#include <machine/bus.h>
#include <machine/intr.h>

#include <dev/mii/mii.h>

#include <dev/pci/pcivar.h>
#include <dev/pci/pcireg.h>
#include <dev/pci/pcidevs.h>

#include <dev/ic/if_gtx.h>

extern int _pciverbose;

/* Prototypes */
static int gtx_match 		__P((struct device *, void *, void *));
void gtx_attach 		__P((struct device *, struct device *, void *));
static void reset_tx 		__P((struct gtx_softc *));
static void reset_rx 		__P((struct gtx_softc *));
static int gtx_ioctl 		__P((struct ifnet *, u_long, caddr_t));
static void gtx_init 		__P((void *));
static void gtx_start 		__P((struct ifnet *));
static void gtx_reset 		__P((struct gtx_softc *, int));
static void gtx_watchdog 	__P((struct ifnet *));
static int gtx_add_rfabuf 	__P((struct gtx_softc *, struct mbuf **));
static int gtx_intr 		__P((void *));
static int gtx_rx		__P((struct gtx_softc *, u_int32_t, u_int32_t));
void gtx_read_mib_counters 	__P((struct gtx_softc *));
void tgt_netstats 		__P((int));

int initAddressTable 	__P((int, int, int, int));
int addAddressTableEntry __P((int, uint, uint, uint, uint));

/* Quick mechanism to allow allocations in the On-Chip SRAM */
static char *ocram = (char *)OC_SRAM_BASE;
#define ocramalloc(what, size) \
	what = (typeof(what))ocram; bzero(ocram, size * sizeof(* what)); \
	ocram += ALIGN(size * sizeof(* what)); 

/* Compensate for lack of a generic ether_ioctl() */
static int      gtx_ether_ioctl __P((struct ifnet *, u_int32_t, caddr_t));
#define ether_ioctl     gtx_ether_ioctl

/* MII access functions */
static void mii_write 		__P((int phyaddr, int reg, int value));

struct cfattach gtx_ca = {                              
    sizeof(struct gtx_softc), gtx_match, gtx_attach 
};                                                      
                                                        
struct cfdriver gtx_cd = {                              
    NULL, "gtx", DV_IFNET                           
};

/* Define ethernet MAC address */
extern char hwethadr[]; 
char phytab[] = {
#ifdef DISCO_II_PHY0
	DISCO_II_PHY0,
#else
	8,
#endif
#ifdef DISCO_II_PHY1
	DISCO_II_PHY1,
#else
	9,
#endif
#ifdef DISCO_II_PHY2
	DISCO_II_PHY2,
#else
	10
#endif
	};

#define RFA_ALIGNMENT_FUDGE	2	/* RX buffer magic offset */

/*
 * Check for Galileo gt643[46]0
 */
static int
gtx_match(parent, match, aux)
	struct device *parent;
	void *match, *aux;
{
	return(1);		/* I suppose soooo... */
}

/* Attach the interface */
void 
gtx_attach (parent, self, aux)
	struct device *parent, *self;
	void *aux;
{
	struct gtx_softc *sc = (struct gtx_softc *)self;
	struct ifnet *ifp;
	unsigned int tmp, regtmp;

	ifp = &sc->arpcom.ac_if;
	bcopy(sc->sc_dev.dv_xname, ifp->if_xname, IFNAMSIZ);
	bcopy(hwethadr, sc->arpcom.ac_enaddr, sizeof(sc->arpcom.ac_enaddr));
	sc->arpcom.ac_enaddr[5] += sc->sc_dev.dv_unit;
	sc->sc_port = sc->sc_dev.dv_unit;
	printf(": address %s\n", ether_sprintf(sc->arpcom.ac_enaddr));

	/* Set PHY address for port */
	sc->phy_addr = phytab[sc->sc_port]; 
	regtmp = GT_READ(ETH_PHY_ADDR_REG);
	regtmp &= ~(0x1f << (5 * sc->sc_port));
	regtmp |= sc->phy_addr << (5 * sc->sc_port);
	GT_WRITE(ETH_PHY_ADDR_REG, regtmp);

	ifp->if_softc = sc;
	ifp->if_flags = IFF_BROADCAST | IFF_SIMPLEX | IFF_MULTICAST;
	ifp->if_ioctl = gtx_ioctl;
	ifp->if_start = gtx_start; 
	ifp->if_watchdog = gtx_watchdog;

	/* 
	 *  Allocate Tx and Rx descriptor rings. These are located in the
	 *  DiscoveryII on-chip SRAM.
	 */
	ocramalloc(sc->tx_ring, TX_RING_SIZE);
	ocramalloc(sc->rx_ring, RX_RING_SIZE);

	/* 
	 *  Allocate Tx data buffers
	 */
#if defined JAGUAR_ATX
	ocramalloc(sc->tx_buff, TX_RING_SIZE * TX_BUF_SZ);
	ocramalloc(sc->rx_buff, RX_RING_SIZE * RX_BUF_SZ);
#else
	sc->tx_buff = (u_int8_t *)(malloc(TX_BUF_SZ*TX_RING_SIZE, 
                              M_DEVBUF, M_NOWAIT));
#endif

	/* Reset and disable port */
	gtx_reset(sc, 0);

	/* Initialize Tx and Rx descriptors */
	reset_tx(sc);
	reset_rx(sc);

	/* 
	 *  Setup eth Port Config Extend Reg, clear the MIB registers.
	 *  When done set MIB counters clear mode to 'no effect' so each
	 *  read doesn't zero the register.
	 */
	GT_WRITE(ETH_PORT_CONFIG_EXTEND_REG(sc->sc_port), PCXR_DEFAULT_SETTING);
	GT_WRITE(ETH_PORT_CONFIG_REG(sc->sc_port), PCR_DEFAULT_SETTING);

	GT_WRITE(ETH_PORT_SERIAL_CONTROL_REG(sc->sc_port), PSCR_DEFAULT_SETTING);
	GT_WRITE(ETH_PORT_SERIAL_CONTROL_REG(sc->sc_port),
		PSCR_DEFAULT_SETTING | PSCR_PORTEN);

	/* Set port ethernet address and init unicast address table. */
	tmp = (sc->arpcom.ac_enaddr[4] << 8) | sc->arpcom.ac_enaddr[5];
	GT_WRITE(ETH_MAC_ADDR_LOW(sc->sc_port), tmp);
	tmp = (sc->arpcom.ac_enaddr[0] << 24) | (sc->arpcom.ac_enaddr[1] << 16);
	tmp |= (sc->arpcom.ac_enaddr[2] << 8) | sc->arpcom.ac_enaddr[3];
	GT_WRITE(ETH_MAC_ADDR_HIGH(sc->sc_port), tmp);

	tmp = sc->arpcom.ac_enaddr[5] & 0x0c;
	regtmp = GT_READ(ETH_DA_FILTER_UNICAST_TABLE_BASE(sc->sc_port) + tmp);
	regtmp |= 1 << ((sc->arpcom.ac_enaddr[5] & 0x03) * 8);
	tmp = sc->arpcom.ac_enaddr[5] & 0x0c;
	GT_WRITE(ETH_DA_FILTER_UNICAST_TABLE_BASE(sc->sc_port) + tmp, regtmp);

	/* "Endian" DMA, burst size 8 64bit words, frame boundary interrupts */
#if BYTE_ORDER == LITTLE_ENDIAN
	GT_WRITE(ETH_SDMA_CONFIG_REG(sc->sc_port),
		SDCR_DEFAULT_SETTING | SDCR_BLMRT);
#else
	GT_WRITE(ETH_SDMA_CONFIG_REG(sc->sc_port), SDCR_DEFAULT_SETTING);
#endif

	GT_WRITE(ETH_TX_QUEUE_0_TOKEN_BUCKET_COUNT(sc->sc_port), 0x3fffffff);
	GT_WRITE(ETH_TX_QUEUE_0_TOKEN_BUCKET_CONFIG(sc->sc_port), 0x3ffffcff);
	GT_WRITE(ETH_MAXIMUM_TRANSMIT_UNIT(sc->sc_port), 0);

	/* Start eth0 Rx DMA, only one queue */
	GT_WRITE(ETH_RECEIVE_QUEUE_COMMAND_REG(sc->sc_port), 0x01);

	/* reset the PHYs so that autoneg works out the initial state */
	mii_write(phytab[sc->sc_port], MII_BMCR, BMCR_RESET);

#ifdef OCELOT_C
	/* set up LED indication */
	mii_write(phytab[sc->sc_port], 0x18, 0x4108);
#endif

	/* Set up interrupt callback */
	tgt_poll_register(IPL_NET, gtx_intr, sc);

	/* Attach the interface */
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
gtx_start(ifp)
    struct ifnet *ifp;
{
    struct gtx_softc *sc = ifp->if_softc;
    u_int16_t total_len;
    char *p;

   /*
    * Process all mbufs ready for transmit or until all available
    * transmit buffers are full.
    */
    CACHESYNC(&sc->tx_ring[sc->tx_next_in], sizeof(TX_DESC), SYNC_R);
    if(sc->tx_ring[sc->tx_next_in].cmdstat & TX_O) {
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
	 *
	 *  XXX For speed this should use multiple TX descs instead.
         */
        nextTx = sc->tx_next_in;
	total_len = 0;
	p = sc->tx_ring[nextTx].vbuff_ptr;
        for (m = mb_head; m != NULL; m = m->m_next) {
            bcopy((char *)(mtod(m, vm_offset_t)), p, m->m_len);
            total_len += m->m_len;
	    p += m->m_len;
        }

        sc->tx_ring[nextTx].byte_cnt = total_len;
	CACHESYNC((void *)(p - total_len), total_len, SYNC_W);

        sc->tx_next_in = (nextTx + 1) % TX_RING_SIZE;
	/* 
         *  Free the mbuf chain
         */
        m_freem(mb_head);

        sc->tx_ring[nextTx].cmdstat =
			TX_O | TX_F | TX_L | TX_P | TX_EI | TX_GC;
	CACHESYNC(&sc->tx_ring[nextTx], sizeof(TX_DESC), SYNC_W);

	/* Send the packet out using queue 0 */
	GT_WRITE(ETH_TRANSMIT_QUEUE_COMMAND_REG(sc->sc_port), 1);

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
 *  Reset the interface, clear all tables.
 */
void
gtx_reset(struct gtx_softc *sc, int drain)
{
	u_int32_t status;
	u_int32_t das, dao;
	int index;

	status = GT_READ(ETH_TRANSMIT_QUEUE_COMMAND_REG(sc->sc_port)) << 8;
	if (status & 0xff00) {
		GT_WRITE(ETH_TRANSMIT_QUEUE_COMMAND_REG(sc->sc_port), status);
	}

	status = GT_READ(ETH_RECEIVE_QUEUE_COMMAND_REG(sc->sc_port)) << 8;
	if (status & 0xff00) {
		GT_WRITE(ETH_RECEIVE_QUEUE_COMMAND_REG(sc->sc_port), status);
	}

	while((GT_READ(ETH_TRANSMIT_QUEUE_COMMAND_REG(sc->sc_port)) & 0xff) &&
		(GT_READ(ETH_RECEIVE_QUEUE_COMMAND_REG(sc->sc_port)) & 0xff))
		;

	if (drain)
		return;

	status = GT_READ(ETH_PORT_SERIAL_CONTROL_REG(sc->sc_port));
	status &= ~PSCR_PORTEN;
	GT_WRITE(ETH_PORT_SERIAL_CONTROL_REG(sc->sc_port), status);

	/*
	 * Reset and clear the Mac tables. It is mandatory for
	 * code caling this code non-drain to call reinit.
	 */
	das = ETH_DA_FILTER_UNICAST_TABLE_BASE(sc->sc_port);
	GT_WRITE(das + 0x0, 0);
	GT_WRITE(das + 0x4, 0);
	GT_WRITE(das + 0x8, 0);
	GT_WRITE(das + 0xc, 0);
	das = ETH_DA_FILTER_SPECIAL_MULTICAST_TABLE_BASE(sc->sc_port);
	dao = ETH_DA_FILTER_OTHER_MULTICAST_TABLE_BASE(sc->sc_port);
	for (index = 0; index < 256; index += 4) {
		GT_WRITE(das + index, 0);
		GT_WRITE(dao + index, 0);
	}
}

/* 
 * Watchdog timeout handler.  This routine is called when 
 * transmission has started on the interface and no
 * interrupt was received before the timeout.
 */
void
gtx_watchdog(ifp)
	struct ifnet *ifp;
{
	struct gtx_softc *sc = ifp->if_softc;

	log(LOG_ERR, "%s: device timeout\n", sc->sc_dev.dv_xname);
	ifp->if_oerrors++;
    //gtx_init(sc);
}

static void
reset_tx(struct gtx_softc *sc)
{
	int i;

	for (i = 0; i < TX_RING_SIZE; i++) {
		TX_DESC *txp;

		txp = &sc->tx_ring[i];
		txp->cmdstat  = 0; /* CPU owns */
		txp->byte_cnt = 0;
		txp->next     = (u_int32_t)OCRAM_TO_PA(sc->tx_ring + (i+1));
#if defined JAGUAR_ATX
		txp->vbuff_ptr = (char *)(sc->tx_buff + (i * TX_BUF_SZ));
		txp->buff_ptr = (u_int32_t)OCRAM_TO_PA(txp->vbuff_ptr);
#else
		txp->vbuff_ptr = (char *)(sc->tx_buff + (i * TX_BUF_SZ));
		txp->buff_ptr = (u_int32_t)VA_TO_PA(txp->vbuff_ptr);
#endif
		CACHESYNC(txp, sizeof(TX_DESC), SYNC_W);
	}

	/* Wrap the ring. */
	sc->tx_ring[i-1].next = OCRAM_TO_PA(sc->tx_ring);
	CACHESYNC(&sc->tx_ring[i-1], sizeof(TX_DESC), SYNC_W);

	/* setup only the lowest priority TxCDP reg */
	GT_WRITE(ETH_TX_CURRENT_QUEUE_DESC_PTR_0(sc->sc_port),
		OCRAM_TO_PA(sc->tx_ring));
	GT_WRITE(ETH_TX_CURRENT_QUEUE_DESC_PTR_1(sc->sc_port), 0);

	/* Initialize Tx indeces and packet counter */
	sc->tx_next_in = 0;
	sc->tx_next_out = 0;
	sc->tx_count = 0;
}

static void
reset_rx(struct gtx_softc *sc)
{
	int i;

	for (i=0; i<RX_RING_SIZE; i++) {
		RX_DESC *rx_desc;
		struct mbuf *m;

		rx_desc = &sc->rx_ring[i];
		if (rx_desc->rx_mbuf == NULL) {
			m = NULL;
			if(gtx_add_rfabuf(sc, &m) < 0) {
				printf("%s:malloc fail\n", sc->sc_dev.dv_xname);
				break;
			}
			rx_desc->rx_mbuf = m;
		}
#if defined JAGUAR_ATX
		rx_desc->vbuff_ptr = (char *)sc->rx_buff + (i * RX_BUF_SZ);
		rx_desc->buff_ptr  = OCRAM_TO_PA(rx_desc->vbuff_ptr);
#else
		rx_desc->vbuff_ptr = NULL;
		rx_desc->buff_ptr = VA_TO_PA(m->m_data);
#endif
		rx_desc->buf_size = RX_BUF_SZ;
		rx_desc->byte_cnt = 0;
		rx_desc->next     = OCRAM_TO_PA((sc->rx_ring + (i+1)));

		/*
		 *  Give ownership to device, set first and last,
		 * enable interrupt
		 */
		sc->rx_ring[i].cmdstat = (RX_F | RX_L | RX_O | RX_EI);
		CACHESYNC(&sc->rx_ring[i], sizeof(RX_DESC), SYNC_W);
	}

	/* Wrap the ring */
	sc->rx_ring[i-1].next = OCRAM_TO_PA(sc->rx_ring);
	CACHESYNC(&sc->rx_ring[i-1], sizeof(RX_DESC), SYNC_W);

	/* Setup only the lowest priority regs */
	for (i=0; i<4; i++) {
		if (i == 0) {
			GT_WRITE(ETH_RX_CURRENT_QUEUE_DESC_PTR_0(sc->sc_port),
				OCRAM_TO_PA(sc->rx_ring));
		}
		else {
			GT_WRITE(ETH_RX_CURRENT_QUEUE_DESC_PTR_0(sc->sc_port)
				+ i*4, 0);
		}
	}

	sc->rx_next_out = 0;
}

int
gtx_ioctl(ifp, command, data)
        struct ifnet *ifp;
        u_long command;
        caddr_t data;
{
        struct gtx_softc *sc = ifp->if_softc;
        int s, error = 0;

        s = splimp();

        switch (command) {
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
                        gtx_init(sc);
                } else {
                        if (ifp->if_flags & IFF_RUNNING)
                                gtx_reset(sc, 1);
                }
                break;

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
gtx_init(xsc)
    void *xsc;
{
    struct gtx_softc *sc = xsc;
    struct ifnet *ifp = &sc->arpcom.ac_if;

    /*s = splimp();*/
    /*
     * Cancel any pending I/O
     */
    ifp->if_flags |= IFF_RUNNING;

    /* Stop and disable port, or reset to stable state */
    /*gtx_reset(sc,0);*/
}

static int
gtx_ether_ioctl(ifp, cmd, data)
        struct ifnet *ifp;
        u_int32_t cmd;
        caddr_t data;
{
        struct ifaddr *ifa = (struct ifaddr *) data;
        struct gtx_softc *sc = ifp->if_softc;

        switch (cmd) {
        case SIOCSIFADDR:
                ifp->if_flags |= IFF_UP;

                switch (ifa->ifa_addr->sa_family) {
#ifdef INET
                case AF_INET:
                        gtx_init(sc);
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
                         gtx_init(sc);
                         break;
                    }
#endif
                default:
                        gtx_init(sc);
                        break;
                }
                break;

        default:
                return (EINVAL);
        }

        return (0);
}

static int
gtx_intr(arg)
    void *arg;
{
	struct gtx_softc *sc = arg;
	struct ifnet *ifp = &sc->arpcom.ac_if;
	u_int32_t icr, xicr;

	/* Read Interrupt Cause Register and ACK interrupts */
	icr = GT_READ(ETH_INTERRUPT_CAUSE_REG(sc->sc_port));
	GT_WRITE(ETH_INTERRUPT_CAUSE_REG(sc->sc_port), 0);
	xicr = GT_READ(ETH_INTERRUPT_CAUSE_EXTEND_REG(sc->sc_port));
	GT_WRITE(ETH_INTERRUPT_CAUSE_EXTEND_REG(sc->sc_port), 0);

	/* Process incoming packets if Rx descriptor returned to CPU */
	gtx_rx(sc, icr, xicr);

	/* Transmit more packets if queue isn't empty */
	if (ifp->if_snd.ifq_head != NULL)
		gtx_start(ifp);

	/* Check for Tx errors */
	if (icr & ICR_TXEND(0)) {
		if (xicr & XICR_TXERROR(0))
			printf("%s: Tx error\n", sc->sc_dev.dv_xname);
	}

	/* Check for Rx errors */
	if (icr & ICR_RXERROR) {
		printf("%s: Rx. Resetting Rx\n", sc->sc_dev.dv_xname);
		reset_rx(sc);
		/* Restart receive engine */
		GT_WRITE(ETH_RECEIVE_QUEUE_COMMAND_REG(sc->sc_port), 0x01);
	}
	if (xicr & XICR_RXOVR)
		printf("%s: Rx overrun\n", sc->sc_dev.dv_xname);

#if 0
	/* Check port status errors */
	psr = GT_READ(ETH_PORT_STATUS_REG(sc->sc_port));
	if (psr & psrPause)
		printf("%s: Pause!\n", sc->sc_dev.dv_xname);
#endif

	return(0);
}

static int
gtx_rx(struct gtx_softc *sc, u_int32_t status, u_int32_t xstatus)
{
	struct ifnet *ifp = &sc->arpcom.ac_if;
	struct mbuf *m;
	int nextRx;
	RX_DESC *rd;
	u_int32_t cmdstat;
	u_int16_t total_len;

	/* 
	 *  Process to current descriptor
	 */
	for (nextRx = sc->rx_next_out;; nextRx = (nextRx + 1) % RX_RING_SIZE) {

		/* This is the only place where we touch rx descriptors */
		rd = &sc->rx_ring[nextRx];
		cmdstat = (u_int32_t)rd->cmdstat;

		/* 
		 *  Bail if gt owns descriptor.  This is the workaround for
		 *  not relying on the icr register.
		 */
		if (cmdstat & RX_O) {
			CACHESYNC(rd, sizeof(RX_DESC), SYNC_R);  /* Push back */
			break;
		}

		/* 
		 *  Must be first and last (ie only) buffer of packet
		 */
		if (!(cmdstat & RX_F) || !(cmdstat & RX_L)) {
			printf("%s: descriptor not first and last!\n",
			    sc->sc_dev.dv_xname);
			goto next;
		}

		/* 
		 *  Drop this packet if there were any errors
		 */
		if ((cmdstat & RX_ES) || (status & ICR_RXERROR)) {
			printf("%s: dropped packet %p:%p\n",
			    sc->sc_dev.dv_xname, cmdstat, status);
			goto next;
		}

		if((total_len = rd->byte_cnt) > MCLBYTES) {
			printf("%s: bad packet length %d\n",
			    sc->sc_dev.dv_xname, total_len);
			goto next;
		}

		/* 
		 *  This is where the packets start to get processed to send
		 *  to upper layers of the protocol stack.
		 */
		m = rd->rx_mbuf;

		/* 
		 *  Add a new buffer to the receive descriptor. The old
		 *  buffer is recycled if we fail to get a new buffer
		 *  and true is returned by gtx_add_rfabuf.
		 */
		if (!gtx_add_rfabuf(sc, &rd->rx_mbuf)) {
			struct ether_header *eh;

#ifdef JAGUAR_ATX
			bcopy(rd->vbuff_ptr, m->m_data, rd->byte_cnt + 2);
			CACHESYNC(rd->vbuff_ptr, RX_BUF_SZ, SYNC_R);
#else
			/*  Attach the new buffer to this descriptor */
			rd->buff_ptr = VA_TO_PA(rd->rx_mbuf->m_data);
#endif

			/* extract, save, and reset the length of this packet */
			total_len = rd->byte_cnt;
			rd->byte_cnt = 0;
			if (total_len < sizeof(struct ether_header)) {
				printf("%s: short packet received\n",
				    sc->sc_dev.dv_xname);
				m_freem(m);
				goto next;
			}

			/* set up packet header interface and length */
			m->m_pkthdr.rcvif = ifp;
			m->m_len = total_len - sizeof(struct ether_header);
			m->m_pkthdr.len = m->m_len;

			/* 
			 *  Realign m_data to point actual input data.
			 *  The DiscoveryII puts data into the buffer
			 *  starting at a 2 byte offset (thanks gyus,
			 *  no more copy!). The receive buffer pointer
			 *  must be 64 bit aligned though so we adjust
			 *  the pointer here before sending up to input.
			 */
			m->m_data += RFA_ALIGNMENT_FUDGE;
			eh = mtod(m, struct ether_header *);
			m->m_data += sizeof(struct ether_header);
			ether_input(ifp, eh, m);
		}
		else {
			printf("%s: recycling rxbuf!\n", sc->sc_dev.dv_xname);
		}

next:
		/*  Release ownership to device */
		rd->cmdstat = RX_F | RX_L | RX_O | RX_EI;
		CACHESYNC(rd, sizeof(RX_DESC), SYNC_W);
	}

	sc->rx_next_out = nextRx;
	return 0;
}

/*
 * Get a receive buffer and return it's data address.
 * Return 0 if recycled. 
 */
int
gtx_add_rfabuf(sc, oldm)
	struct gtx_softc *sc;
	struct mbuf **oldm;
{
	struct mbuf *m, *pm;

	pm = *oldm;

	MGETHDR(m, M_DONTWAIT, MT_DATA);
	if (m != NULL) {
		MCLGET(m, M_DONTWAIT);
		if ((m->m_flags & M_EXT) == 0) {
			m_freem(m);
			if (pm == NULL) {
                                printf("gtx_add_rfabuf: error 1\n");
				return -1;
                        }
			/* Recycle */
			m = pm;
			m->m_data = m->m_ext.ext_buf;
		}
	}
	else { /* Recycle */
		if (pm == NULL) {
                        printf("gtx_add_rfabuf: error 2\n");
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
gtx_read_mib_counters (sc)
    struct gtx_softc *sc;
{
#if 0
    u_int32_t *mib_reg = (u_int32_t *)&sc->mib;
    int i;

    for (i=0; i<sizeof(mib_counters_t)/sizeof(u_int32_t); i++) {
        mib_reg[i] = GTETH_READ(sc, ETH0_MIB_COUNTER_BASE + i*sizeof(u_int32_t));
    }
#endif
}

static void mii_write (phyaddr, reg, value)
    int phyaddr;
    int reg;
    int value;
{
    int data;

    /* wait for device to become non-busy */
    while (GT_READ(ETH_SMI_REG) & (0x1 << 28)) 
	;

    data = value & 0xffff;
    data |= (phyaddr & 0x1f) << 16;
    data |= (reg & 0x1f) << 21;
    GT_WRITE(ETH_SMI_REG, data);
}
