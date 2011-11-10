/*
 * See file CREDITS for list of people who contributed to this
 * project.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 * can work on pmon.loongson-support@ict.ac.cn.
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/mbuf.h>
#include <sys/malloc.h>
#include <sys/kernel.h>
#include <sys/socket.h>
#include <sys/syslog.h>

#include <sys/systm.h>
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

#if NBPFILTER > 0
#include <net/bpf.h>
#include <net/bpfdesc.h>
#endif

#if defined(__NetBSD__) || defined(__OpenBSD__)

#include <sys/ioctl.h>
#include <sys/errno.h>
#include <sys/device.h>

#if defined(__NetBSD__)
#include <net/if_ether.h>
#include <netinet/if_inarp.h>
#endif

#if defined(__OpenBSD__)
#include <netinet/if_ether.h>
#endif

#include <vm/vm.h>

#include <machine/cpu.h>
#include <machine/bus.h>
#include <machine/intr.h>

#include <dev/mii/miivar.h>

#include <dev/pci/pcivar.h>
#include <dev/pci/pcireg.h>
#include <dev/pci/pcidevs.h>


#else /* __FreeBSD__ */

#include <sys/sockio.h>

#include <netinet/if_ether.h>

#include <vm/vm.h>		/* for vtophys */
#include <vm/vm_param.h>	/* for vtophys */
#include <vm/pmap.h>		/* for vtophys */
#include <machine/clock.h>	/* for DELAY */

#include <pci/pcivar.h>

#endif /* __NetBSD__ || __OpenBSD__ */

#define offsetof(TYPE, MEMBER) ((size_t) &((TYPE *)0)->MEMBER)

/*
 * NOTE!  On the Alpha, we have an alignment constraint.  The
 * card DMAs the packet immediately following the RFA.  However,
 * the first thing in the packet is a 14-byte Ethernet header.
 * This means that the packet is misaligned.  To compensate,
 * we actually offset the RFA 2 bytes into the cluster.  This
 * aligns the packet after the Ethernet header at a 32-bit
 * boundary.  HOWEVER!  This means that the RFA is misaligned!
 */

#ifdef BADPCIBRIDGE
#define BADPCIBRIDGE
#define	RFA_ALIGNMENT_FUDGE	4
#else
#define	RFA_ALIGNMENT_FUDGE	2
#endif

#include <target/types.h>
#include <target/fcr-soc-dmfe.h>
#undef DEBUG_SROM
#undef DEBUG_SROM2

#undef UPDATE_SROM

/* PCI Registers.
 */
#define PCI_CFDA_PSM		0x43

#define CFRV_RN		0x000000f0	/* Revision Number */

#define WAKEUP		0x00		/* Power Saving Wakeup */
#define SLEEP		0x80		/* Power Saving Sleep Mode */

#define DC2114x_BRK	0x0020		/* CFRV break between DC21142 & DC21143 */

/* Ethernet chip registers.
 */
#define DE4X5_BMR	0x000		/* Bus Mode Register, CSR0 */
#define DE4X5_TPD	0x008		/* Transmit Poll Demand Reg, CSR1 */
#define CSR2		0x010		
#define DE4X5_RRBA	0x018		/* RX Ring Base Address Reg, CSR3*/
#define DE4X5_TRBA	0x020		/* TX Ring Base Address Reg, CSR4*/
#define DE4X5_STS	0x028		/* Status Register, CSR5 */
#define DE4X5_OMR	0x030		/* Operation Mode Register, CSR6 */
#define DE4X5_SICR	0x068		/* SIA Connectivity Register, */
#define DE4X5_APROM	0x048		/* Ethernet Address PROM */

/* Register bits.
 */
#define BMR_SWR		0x00000001	/* Software Reset */
#define STS_TS		0x00700000	/* Transmit Process State */
#define STS_RS		0x000e0000	/* Receive Process State */
#define OMR_ST		0x00002000	/* Start/Stop Transmission Command */
#define OMR_SR		0x00000002	/* Start/Stop Receive */
#define OMR_PS		0x00040000	/* Port Select */
#define OMR_SDP		0x02000000	/* SD Polarity - MUST BE ASSERTED */
#define OMR_PM		0x00000080	/* Pass All Multicast */
#define OMR_FD		0x00000200	/* full duplex */
#define OMR_PR		0x00000040	/* promise mode */

/* Descriptor bits.
 */
#define R_OWN		0x80000000	/* Own Bit */
#define RD_RER		0x02000000	/* Receive End Of Ring */
#define RD_LS		0x00000100	/* Last Descriptor */
#define RD_ES		0x00008000	/* Error Summary */
#define TD_TER		0x02000000	/* Transmit End Of Ring */
#define TD_TCH		0x01000000	/* Second address chained */
#define T_OWN		0x80000000	/* Own Bit */
#define TD_LS		0x40000000	/* Last Segment */
#define TD_FS		0x20000000	/* First Segment */
#define TD_ES		0x00008000	/* Error Summary */
#define TD_SET		0x08000000	/* Setup Packet */

/* */


/* CR9 definition: SROM/MII */
#define DCR9           0x48
#define CR9_SROM_READ   0x4800
#define CR9_SRCS        0x1
#define CR9_SRCLK       0x2
#define CR9_CRDOUT      0x8
#define SROM_DATA_0     0x0
#define SROM_DATA_1     0x4
#define PHY_DATA_1      0x20000
#define PHY_DATA_0      0x00000
#define MDCLKH          0x10000

#define PHY_POWER_DOWN  0x800

#define SROM_V41_CODE   0x14


/* The EEPROM commands include the alway-set leading bit. */
#define SROM_WRITE_CMD	5
#define SROM_READ_CMD	6
#define SROM_ERASE_CMD	7

#define SROM_HWADD	    0x0014	/* Hardware Address offset in SROM */
#define SROM_RD		0x00004000	/* Read from Boot ROM */
#define EE_DATA_WRITE	      0x04	/* EEPROM chip data in. */
#define EE_WRITE_0	    0x4801
#define EE_WRITE_1	    0x4805
#define EE_DATA_READ	      0x08	/* EEPROM chip data out. */
#define SROM_SR		0x00000800	/* Select Serial ROM when set */

#define DT_IN		0x00000004	/* Serial Data In */
#define DT_CLK		0x00000002	/* Serial ROM Clock */
#define DT_CS		0x00000001	/* Serial ROM Chip Select */

#define POLL_DEMAND	1


typedef int  s32;

struct eth_device {
//most of the fields come from struct fxp_softc
#if defined(__NetBSD__) || defined(__OpenBSD__)
	struct device sc_dev;		/* generic device structures */
	void *sc_ih;			/* interrupt handler cookie */
	bus_space_tag_t sc_st;		/* bus space tag */
	bus_space_handle_t sc_sh;	/* bus space handle */
	pci_chipset_tag_t sc_pc;	/* chipset handle needed by mips */
#else
	struct caddr_t csr;		/* control/status registers */
#endif /* __NetBSD__ || __OpenBSD__ */
#if defined(__OpenBSD__) || defined(__FreeBSD__)
	struct arpcom arpcom;		/* per-interface network data !!we use this*/
#endif
#if defined(__NetBSD__)
	struct ethercom sc_ethercom;	/* ethernet common part */
#endif
	struct mii_data sc_mii;		/* MII media information */
	unsigned char dev_addr[6];
    unsigned char       addr_len;   /* hardware address length  */
	unsigned long iobase;
	char *packet;
} ;
#define udelay delay



#define CONFIG_TULIP_FIX_DAVICOM

#ifdef CONFIG_TULIP_FIX_DAVICOM
#define RESET_DM9102(dev) {\
    unsigned long i;\
    i=INL(dev, 0x0);\
    udelay(1000);\
    OUTL(dev, i | BMR_SWR, DE4X5_BMR);\
    udelay(1000);\
}
#else
#define RESET_DE4X5(dev) {\
    int i;\
    i=INL(dev, DE4X5_BMR);\
    udelay(1000);\
    OUTL(dev, i | BMR_SWR, DE4X5_BMR);\
    udelay(1000);\
    OUTL(dev, i, DE4X5_BMR);\
    udelay(1000);\
    for (i=0;i<5;i++) {INL(dev, DE4X5_BMR); udelay(10000);}\
    udelay(1000);\
}
#endif

#define START_DE4X5(dev) {\
    s32 omr; \
    omr = INL(dev, DE4X5_OMR);\
    omr |= OMR_ST | OMR_SR | OMR_PR | OMR_FD;\
    OUTL(dev, omr, DE4X5_OMR);		/* Enable the TX and/or RX */\
}

#define STOP_DE4X5(dev) {\
    s32 omr; \
    omr = INL(dev, DE4X5_OMR);\
    omr &= ~(OMR_ST|OMR_SR);\
    OUTL(dev, omr, DE4X5_OMR);		/* Disable the TX and/or RX */ \
}

#define NUM_RX_DESC 8			/* Number of Rx descriptors */
#define NUM_TX_DESC 8			/* Number of TX descriptors */
#define RX_BUFF_SZ  1520 		//yanhua, should aligned to word
#define TX_BUFF_SZ  1520

#define TOUT_LOOP   100000

#define SETUP_FRAME_LEN 192
#define ETH_ALEN	6

#include <types.h>

struct de4x5_desc {
	volatile s32 status;
	u32 des1;
	u32 buf;
	u32 next;
};

static struct de4x5_desc _rx_ring[NUM_RX_DESC] __attribute__ ((aligned(32),section(".bss.align32"))); /* RX descriptor ring         */
static struct de4x5_desc _tx_ring[NUM_TX_DESC] __attribute__ ((aligned(32),section(".bss.align32"))); /* TX descriptor ring         */
static volatile struct de4x5_desc *rx_ring;
static volatile struct de4x5_desc *tx_ring;

char __NetRxPackets[NUM_RX_DESC][RX_BUFF_SZ] __attribute__((aligned(32),section(".bss.align32"))); //16
char (*NetRxPackets)[RX_BUFF_SZ];

char __NetTxPackets[NUM_TX_DESC][TX_BUFF_SZ] __attribute__((aligned(32),section(".bss.align32"))); //16
char (*NetTxPackets)[TX_BUFF_SZ];


static int rx_new;                             /* RX descriptor ring pointer */
static int tx_new;                             /* TX descriptor ring pointer */

static char rxRingSize;
static char txRingSize;


static int   dc21x4x_init(struct eth_device* dev);
static void  read_hw_addr(struct eth_device *dev);
 u16   loop_back(void);  

 u16 loop_back_reset(void);

#define virt_to_phys(addr) ((addr) & 0x1fffffff)

static int INL(struct eth_device* dev, u_long addr)
{
	return (*(volatile u_long *)(addr + dev->iobase));
}

static void OUTL(struct eth_device* dev, int command, u_long addr)
{
	*(volatile u_long *)(addr + dev->iobase) = (command);
	*(volatile u_long *)(addr + dev->iobase);
}

#define PHY_ADDR_1 1
#define PHY_ADDR_17 17


void phy_write( u8 phy_addr, u8 offset, u16 phy_data);
u16 phy_read(u8 phy_addr, u8 offset);
 u16 loop_back_17(void)
{
    u16 data_phy=0;
    u16 data_reset = 0;
    u16 data_loopback=0;
    u8 offset=0;

    printf("PHY RESET\n");
    data_phy = phy_read(PHY_ADDR_1, offset);
    printf("loop_back-read(before write)=0x%x \n",data_phy);
    data_phy &= ~0x8000;
    printf("loop_back-read(&~0x8000)=0x%x \n",data_phy);
    data_phy |= 0x8000;
    printf("loop_back-read(|0x8000)=0x%x \n",data_phy);
    data_reset = data_phy;
    phy_write(PHY_ADDR_1, offset, data_reset);
    data_phy = phy_read(PHY_ADDR_1, offset);
    printf("loop_back-read(after write)=0x%x \n",data_phy);

    printf("PHY SET LOOPBACK\n");
    data_phy = phy_read(PHY_ADDR_1, offset);
    printf("loop_back-read(before write)=0x%x \n",data_phy);
    data_phy &= ~0x4000;
    printf("loop_back-read(&~0x4000)=0x%x \n",data_phy);
    data_phy |= 0x4000;
    printf("loop_back-read(|0x4000)=0x%x \n",data_phy);
    data_loopback = data_phy;
    phy_write(PHY_ADDR_1, offset, data_loopback);
    data_phy = phy_read(PHY_ADDR_1, offset);
    printf("loop_back-read(after write)=0x%x \n",data_phy);


    printf("PHY SET LOOPBACK reg-17\n");
    offset=17;
    data_phy = phy_read(PHY_ADDR_1, offset);
    printf("loop_back-read(before write)=0x%x \n",data_phy);
    data_phy &= ~0x0300;
    printf("loop_back-read(&~0x0300)=0x%x \n",data_phy);
    data_phy |= 0x0100;
    printf("loop_back-read(|0x0100)=0x%x \n",data_phy);
    data_loopback = data_phy;
    phy_write(PHY_ADDR_1, offset, data_loopback);
    data_phy = phy_read(PHY_ADDR_1, offset);
    printf("loop_back-read(after write)=0x%x \n",data_phy);

    printf("###############################################\n");
    for(offset=0;offset<=30;offset++)
    {
        data_phy = phy_read(PHY_ADDR_1, offset);
        printf("loop_back-read(%d)=0x%x \n",offset,data_phy);
    }
    return data_loopback;

}


 u16 loop_back_print(void)
{
    u16 data_phy=0;
    u16 data_reset = 0;
    u16 data_loopback=0;
    u8 offset=0;
/*
    printf("PHY RESET\n");
    data_phy = phy_read(PHY_ADDR_1, offset);
    printf("loop_back-read(before write)=0x%x \n",data_phy);
    data_phy &= ~0x8000;
    printf("loop_back-read(&~0x8000)=0x%x \n",data_phy);
    data_phy |= 0x8000;
    printf("loop_back-read(|0x8000)=0x%x \n",data_phy);
    data_reset = data_phy;
    phy_write(PHY_ADDR_1, offset, data_reset);
    data_phy = phy_read(PHY_ADDR_1, offset);
    printf("loop_back-read(after write)=0x%x \n",data_phy);

    printf("PHY SET LOOPBACK\n");
    data_phy = phy_read(PHY_ADDR_1, offset);
    printf("loop_back-read(before write)=0x%x \n",data_phy);
    data_phy &= ~0x4000;
    printf("loop_back-read(&~0x4000)=0x%x \n",data_phy);
//    data_phy |= 0x4000;
//    printf("loop_back-read(|0x4000)=0x%x \n",data_phy);
    data_loopback = data_phy;
    phy_write(PHY_ADDR_1, offset, data_loopback);
    data_phy = phy_read(PHY_ADDR_1, offset);
    printf("loop_back-read(after write)=0x%x \n",data_phy); */
    printf("###############################################\n");
    for(offset=0;offset<=30;offset++)    
    {
        data_phy = phy_read(PHY_ADDR_1, offset);
        printf("loop_back-read(%d)=0x%x \n",offset,data_phy);    
    }
    return data_loopback;

}

 u16 loop_back_unset(void)
{
    u16 data_phy=0;
    u16 data_reset = 0;
    u16 data_loopback=0;
    u8 offset=0;

    printf("PHY RESET\n");
    data_phy = phy_read(PHY_ADDR_1, offset);
    printf("loop_back-read(before write)=0x%x \n",data_phy);
    data_phy &= ~0x8000;
    printf("loop_back-read(&~0x8000)=0x%x \n",data_phy);
    data_phy |= 0x8000;
    printf("loop_back-read(|0x8000)=0x%x \n",data_phy);
    data_reset = data_phy;
    phy_write(PHY_ADDR_1, offset, data_reset);
    data_phy = phy_read(PHY_ADDR_1, offset);
    printf("loop_back-read(after write)=0x%x \n",data_phy);

    printf("PHY UN-SET LOOPBACK\n");
    data_phy = phy_read(PHY_ADDR_1, offset);
    printf("loop_back-read(before write)=0x%x \n",data_phy);
    data_phy &= ~0x4000;
    printf("loop_back-read(&~0x4000)=0x%x \n",data_phy);
//    data_phy |= 0x4000;
//    printf("loop_back-read(|0x4000)=0x%x \n",data_phy);
    data_loopback = data_phy;
    phy_write(PHY_ADDR_1, offset, data_loopback);
    data_phy = phy_read(PHY_ADDR_1, offset);
    printf("loop_back-read(after write)=0x%x \n",data_phy);
    printf("###############################################\n");
    for(offset=0;offset<=30;offset++)
    {
        data_phy = phy_read(PHY_ADDR_1, offset);
        printf("loop_back-read(%d)=0x%x \n",offset,data_phy);
    }
    return data_loopback;

}

 u16 loop_back_reset(void)
{
    u16 data_phy=0;
    u16 data_reset = 0;
    u16 data_loopback=0;
    u8 offset=0;

    printf("PHY RESET\n");
    data_phy = phy_read(PHY_ADDR_1, offset);
    printf("loop_back-read(before write)=0x%x \n",data_phy);
    data_phy &= ~0x8000;
    printf("loop_back-read(&~0x8000)=0x%x \n",data_phy);
    data_phy |= 0x8000;
    printf("loop_back-read(|0x8000)=0x%x \n",data_phy);
    data_reset = data_phy;
    phy_write(PHY_ADDR_1, offset, data_reset);
    data_phy = phy_read(PHY_ADDR_1, offset);
    printf("loop_back-read(after write)=0x%x \n",data_phy);

    printf("PHY SET LOOPBACK\n");
    data_phy = phy_read(PHY_ADDR_1, offset);
    printf("loop_back-read(before write)=0x%x \n",data_phy);
    data_phy &= ~0x4000;
    printf("loop_back-read(&~0x4000)=0x%x \n",data_phy);
    data_phy |= 0x4000;
    printf("loop_back-read(|0x4000)=0x%x \n",data_phy);
    data_loopback = data_phy;
    phy_write(PHY_ADDR_1, offset, data_loopback);
    data_phy = phy_read(PHY_ADDR_1, offset);
    printf("loop_back-read(after write)=0x%x \n",data_phy);
    printf("###############################################\n");
    for(offset=0;offset<=30;offset++)
    {
        data_phy = phy_read(PHY_ADDR_1, offset);
        printf("loop_back-read(%d)=0x%x \n",offset,data_phy);
    }
    return data_loopback;

}
static void phy_write_1bit(unsigned long ioaddr, u32 phy_data);

void phy_write_1( u32 phy_addr, u32 offset, u32 phy_data)
{
    u16 i;
    unsigned long ioaddr;

//    if (chip_id == PCI_DM9132_ID) {
 //       ioaddr = iobase + 0x80 + offset * 4;
   //     outw(phy_data, ioaddr);
  //  } else {
        /* DM9102/DM9102A Chip */
        ioaddr = MAC_REG_BASE + DCR9;

        /* Send 33 synchronization clock to Phy controller */
        for (i = 0; i < 35; i++)
            phy_write_1bit(ioaddr, PHY_DATA_1);

        /* Send start command(01) to Phy */
        phy_write_1bit(ioaddr, PHY_DATA_0);
        phy_write_1bit(ioaddr, PHY_DATA_1);

        /* Send write command(01) to Phy */
        phy_write_1bit(ioaddr, PHY_DATA_0);
        phy_write_1bit(ioaddr, PHY_DATA_1);

        /* Send Phy addres */
        for (i = 0x10; i > 0; i = i >> 1)
            phy_write_1bit(ioaddr, phy_addr & i ? PHY_DATA_1 : PHY_DATA_0);

        /* Send register addres */
        for (i = 0x10; i > 0; i = i >> 1)
            phy_write_1bit(ioaddr, offset & i ? PHY_DATA_1 : PHY_DATA_0);

        /* written trasnition */
        phy_write_1bit(ioaddr, PHY_DATA_1);
        phy_write_1bit(ioaddr, PHY_DATA_0);

        /* Write a word data to PHY controller */
        for ( i = 0x8000; i > 0; i >>= 1)
            phy_write_1bit(ioaddr, phy_data & i ? PHY_DATA_1 : PHY_DATA_0);
    //    }
}

static u16 phy_read_1bit(unsigned long ioaddr);
 u32 phy_read_1(u32 phy_addr, u32 offset)
{
    int i;
    u32 phy_data;
    unsigned long ioaddr;

        /* DM9102/DM9102A Chip */
        ioaddr = MAC_REG_BASE + DCR9;

        /* Send 33 synchronization clock to Phy controller */
        for (i = 0; i < 35; i++)
            phy_write_1bit(ioaddr, PHY_DATA_1);

        /* Send start command(01) to Phy */
        phy_write_1bit(ioaddr, PHY_DATA_0);
        phy_write_1bit(ioaddr, PHY_DATA_1);

        /* Send read command(10) to Phy */
        phy_write_1bit(ioaddr, PHY_DATA_1);
        phy_write_1bit(ioaddr, PHY_DATA_0);

        /* Send Phy addres */
        for (i = 0x10; i > 0; i = i >> 1)
            phy_write_1bit(ioaddr, phy_addr & i ? PHY_DATA_1 : PHY_DATA_0);

        /* Send register addres */
        for (i = 0x10; i > 0; i = i >> 1)
            phy_write_1bit(ioaddr, offset & i ? PHY_DATA_1 : PHY_DATA_0);
       /* Skip transition state */
        phy_read_1bit(ioaddr);

        /* read 16bit data */
        for (phy_data = 0, i = 0; i < 16; i++) {
            phy_data <<= 1;
            phy_data |= phy_read_1bit(ioaddr);
        }

    return phy_data;
}

////////////////////////////////////////////////////////////

/*      
 *  Write a word to Phy register
 */         
            
 void phy_write( u8 phy_addr, u8 offset, u16 phy_data)
{           
    u16 i;  
    unsigned long ioaddr;
            
//    if (chip_id == PCI_DM9132_ID) {
 //       ioaddr = iobase + 0x80 + offset * 4;
   //     outw(phy_data, ioaddr);
  //  } else {
        /* DM9102/DM9102A Chip */
        ioaddr = MAC_REG_BASE + DCR9;
    
        /* Send 33 synchronization clock to Phy controller */
        for (i = 0; i < 35; i++)
            phy_write_1bit(ioaddr, PHY_DATA_1);
                
        /* Send start command(01) to Phy */
        phy_write_1bit(ioaddr, PHY_DATA_0);
        phy_write_1bit(ioaddr, PHY_DATA_1);
    
        /* Send write command(01) to Phy */
        phy_write_1bit(ioaddr, PHY_DATA_0);
        phy_write_1bit(ioaddr, PHY_DATA_1);

        /* Send Phy addres */
        for (i = 0x10; i > 0; i = i >> 1)
            phy_write_1bit(ioaddr, phy_addr & i ? PHY_DATA_1 : PHY_DATA_0);

        /* Send register addres */
        for (i = 0x10; i > 0; i = i >> 1)
            phy_write_1bit(ioaddr, offset & i ? PHY_DATA_1 : PHY_DATA_0);

        /* written trasnition */
        phy_write_1bit(ioaddr, PHY_DATA_1);
        phy_write_1bit(ioaddr, PHY_DATA_0);

        /* Write a word data to PHY controller */
        for ( i = 0x8000; i > 0; i >>= 1)
            phy_write_1bit(ioaddr, phy_data & i ? PHY_DATA_1 : PHY_DATA_0);
    //    }
}

/*
 *  Read a word data from phy register
 */

u16 phy_read(u8 phy_addr, u8 offset)
{
    int i;
    u16 phy_data;
    unsigned long ioaddr;

        /* DM9102/DM9102A Chip */
        ioaddr = MAC_REG_BASE + DCR9;

        /* Send 33 synchronization clock to Phy controller */
        for (i = 0; i < 35; i++)
            phy_write_1bit(ioaddr, PHY_DATA_1);

        /* Send start command(01) to Phy */
        phy_write_1bit(ioaddr, PHY_DATA_0);
        phy_write_1bit(ioaddr, PHY_DATA_1);

        /* Send read command(10) to Phy */
        phy_write_1bit(ioaddr, PHY_DATA_1);
        phy_write_1bit(ioaddr, PHY_DATA_0);

        /* Send Phy addres */
        for (i = 0x10; i > 0; i = i >> 1)
            phy_write_1bit(ioaddr, phy_addr & i ? PHY_DATA_1 : PHY_DATA_0);

        /* Send register addres */
        for (i = 0x10; i > 0; i = i >> 1)
            phy_write_1bit(ioaddr, offset & i ? PHY_DATA_1 : PHY_DATA_0);
       /* Skip transition state */
        phy_read_1bit(ioaddr);

        /* read 16bit data */
        for (phy_data = 0, i = 0; i < 16; i++) {
            phy_data <<= 1;
            phy_data |= phy_read_1bit(ioaddr);
        }

    return phy_data;
}


/*
 *  Write one bit data to Phy Controller
 */

static void phy_write_1bit(unsigned long ioaddr, u32 phy_data)
{
    phy_data |=1<<18;
    dmfe_outl(phy_data, ioaddr);         /* MII Clock Low */
    udelay(1);
    dmfe_outl(phy_data | MDCLKH, ioaddr);    /* MII Clock High */
    udelay(1);
    dmfe_outl(phy_data, ioaddr);         /* MII Clock Low */
    udelay(1);
}


/*
 *  Read one bit phy data from PHY controller
 */

static u16 phy_read_1bit(unsigned long ioaddr)
{
    u16 phy_data;

    //dmfe_outl(0x50000, ioaddr);
    dmfe_outl(0x10000, ioaddr);
    udelay(1);
    phy_data = ( dmfe_inl(ioaddr) >> 19 ) & 0x1;
    //dmfe_outl(0x40000, ioaddr);
    dmfe_outl(0x00000, ioaddr);
    udelay(1);

    return phy_data;
}

///////////////////////////////////////////////////////////


static int waitsend=0;
int dc21x4x_initialize(struct eth_device* dev)
{
	struct ifnet *ifp = &dev->arpcom.ac_if;
	dev->iobase = MAC_REG_BASE;

	/* Ensure we're not sleeping. */

	read_hw_addr(dev);

	dc21x4x_init(dev);
	ifp->if_flags |=IFF_RUNNING;	
	waitsend=0;

	return 0;
}
#define next_tx(x) (((x+1)==NUM_TX_DESC)?0:(x+1))
#define next_rx(x) (((x+1)==NUM_RX_DESC)?0:(x+1))

static void send_setup_frame(struct eth_device* dev);
/*
 * init function
 */
static int dc21x4x_init(struct eth_device* dev)
{
	int		i;

	/* Ensure we're not sleeping. */

#ifdef CONFIG_TULIP_FIX_DAVICOM
	RESET_DM9102(dev);
#else
	RESET_DE4X5(dev);
#endif

	if ((INL(dev, DE4X5_STS) & (STS_TS | STS_RS)) != 0) {
		printf("Error: Cannot reset ethernet controller.\n");
		return 0;
	}

#ifdef CONFIG_TULIP_SELECT_MEDIA
	dc21x4x_select_media(dev);
#else
	//OUTL(dev, OMR_SDP | OMR_PS | OMR_PM, DE4X5_OMR);
	OUTL(dev, OMR_SDP | OMR_PS , DE4X5_OMR);
#endif

	/*
	 * initialise rx descriptors
	 * use it as chain structure
	 */
	rxRingSize = NUM_RX_DESC;
	txRingSize = NUM_TX_DESC;
	pci_sync_cache(dev, (vm_offset_t)_rx_ring, sizeof(_rx_ring), SYNC_R);
	pci_sync_cache(dev, (vm_offset_t)_tx_ring, sizeof(_tx_ring), SYNC_R);
	pci_sync_cache(dev, (vm_offset_t)__NetRxPackets, sizeof(__NetRxPackets), SYNC_R);
	pci_sync_cache(dev, (vm_offset_t)__NetTxPackets, sizeof(__NetTxPackets), SYNC_R);
	
	rx_ring = (struct de4x5_desc *)(0xa0000000 | (unsigned long)_rx_ring);
	tx_ring = (struct de4x5_desc *)(0xa0000000 | (unsigned long)_tx_ring);

	NetRxPackets = (char (*)[RX_BUFF_SZ])(0xa0000000|(unsigned long)__NetRxPackets);
	NetTxPackets = (char (*)[TX_BUFF_SZ])(0xa0000000|(unsigned long)__NetTxPackets);
	
	for (i = 0; i < NUM_RX_DESC; i++) {
		rx_ring[i].status = (R_OWN); //Initially MAC owns it.
		rx_ring[i].des1 = RX_BUFF_SZ;
		rx_ring[i].buf = virt_to_phys((u32) NetRxPackets[i]);//XXX
#ifdef CONFIG_TULIP_FIX_DAVICOM
		rx_ring[i].next = virt_to_phys((u32) &rx_ring[next_rx(i)]);
#else
		rx_ring[i].next = 0;
#endif
	}

	/*
	 * initialize tx descriptors
	 * use it as chain structure
	 */
	for (i=0; i < NUM_TX_DESC; i++) {
		tx_ring[i].status = 0;
		tx_ring[i].des1 = 0xe1000000;//TD_TCH;
		tx_ring[i].buf = virt_to_phys((u32) NetTxPackets[i]);

#ifdef CONFIG_TULIP_FIX_DAVICOM
		tx_ring[i].next = (virt_to_phys((u32) &tx_ring[next_tx(i)]));
#else
		tx_ring[i].next = 0;
#endif
	}



	/* Write the end of list marker to the descriptor lists. */
	rx_ring[rxRingSize - 1].des1 |= RD_RER; //Receive end of ring
	//tx_ring[txRingSize - 1].des1 |= TD_TER; //Transmit end of ring
	//tx_ring[txRingSize - 1].des1 |= TD_TCH; //Transmit end of ring

	/* Tell the adapter where the TX/RX rings are located. */
	printf("rx ring %x\n", virt_to_phys((u32)rx_ring));
	printf("tx ring %x\n", virt_to_phys((u32)tx_ring));
	OUTL(dev, virt_to_phys((u32) rx_ring), DE4X5_RRBA);
	OUTL(dev, virt_to_phys((u32) tx_ring), DE4X5_TRBA);

	START_DE4X5(dev);

	tx_new = 0;
	rx_new = 0;

#if 1
	{
		printf("DE4X5_BMR= %x\n",  INL(dev, DE4X5_BMR));
		printf("DE4X5_TPD= %x\n",  INL(dev, DE4X5_TPD));
		printf("DE4X5_RRBA= %x\n", INL(dev, DE4X5_RRBA));
		printf("DE4X5_TRBA= %x\n", INL(dev, DE4X5_TRBA));
		printf("DE4X5_STS= %x\n",  INL(dev, DE4X5_STS));
		printf("DE4X5_OMR= %x\n",  INL(dev, DE4X5_OMR));
	}
#endif	
	send_setup_frame(dev);
	printf("After setup\n");
	{
		printf("DE4X5_BMR= %x\n",  INL(dev, DE4X5_BMR));
		printf("DE4X5_TPD= %x\n",  INL(dev, DE4X5_TPD));
		printf("DE4X5_RRBA= %x\n", INL(dev, DE4X5_RRBA));
		printf("DE4X5_TRBA= %x\n", INL(dev, DE4X5_TRBA));
		printf("DE4X5_STS= %x\n",  INL(dev, DE4X5_STS));
		printf("DE4X5_OMR= %x\n",  INL(dev, DE4X5_OMR));
	}


	return 1;
}



static void dmfe_shutdown(void* dev)
{

	STOP_DE4X5(dev);
	OUTL(dev, 0, DE4X5_SICR);

}

static void read_hw_addr(struct eth_device *dev)
{
	static char maddr[ETH_ALEN]={0xba, 0x98, 0x76, 0x64, 0x32, 0x19};
	char *p = &dev->dev_addr[0];
	int i;
	{
		int i;
		int32_t v;
	char *s=getenv("ethaddr");
	if(s){
		for(i = 0; i < 6; i++) {
			gethex(&v, s, 2);
			maddr[i] = v;
			s += 3;         /* Don't get to fancy here :-) */
		} 
	 }
	} 

	for (i = 0; i < ETH_ALEN; i++) 
		*p++ = maddr[i];

	return;

}


void dump_tx()
{
	int i;
	for(i=0; i<NUM_TX_DESC; i++){
		printf("tx_ring %d:\n", i);
		printf("status %x\n", tx_ring[i].status);
		printf("tdes1 %x\n", tx_ring[i].des1);
		printf("buf %x\n", tx_ring[i].buf);
		printf("next %x\n",tx_ring[i].next);
	}

}


static int
dmfe_ether_ioctl(ifp, cmd, data)
	struct ifnet *ifp;
	u_long cmd;
	caddr_t data;
{
	struct ifaddr *ifa = (struct ifaddr *) data;
	struct eth_device *sc = ifp->if_softc;
	int error = 0;
	
	int s;
	s = splimp();
		
	switch (cmd) {
#ifdef PMON
	case SIOCPOLL:
		break;
#endif
	case SIOCSIFADDR:

		switch (ifa->ifa_addr->sa_family) {
#ifdef INET
		case AF_INET:
			dc21x4x_initialize(sc);
			ifp->if_flags |= IFF_UP;

#ifdef __OpenBSD__
			arp_ifinit(&sc->arpcom, ifa);
#else
			arp_ifinit(ifp, ifa);
#endif
			
			break;
#endif

		default:
			dc21x4x_initialize(sc);
			ifp->if_flags |= IFF_UP;
			break;
		}
		break;
	case SIOCSIFFLAGS:

		/*
		 * If interface is marked up and not running, then start it.
		 * If it is marked down and running, stop it.
		 * XXX If it's up then re-initialize it. This is so flags
		 * such as IFF_PROMISC are handled.
		 */
		if (ifp->if_flags & IFF_UP) {
			dc21x4x_initialize(sc);
		} else {
			if (ifp->if_flags & IFF_RUNNING)
			dmfe_shutdown(sc);
		}
		break;

	default:
		error = EINVAL;
	}

	splx(s);
	return (error);
}

static char	setup_frame[SETUP_FRAME_LEN];
static void send_setup_frame(struct eth_device* dev)
{
	int		i;
	char 	*pa = (char *)(0xa0000000 |(unsigned long )&setup_frame[0]);

	memset(pa, 0x00, SETUP_FRAME_LEN);

	for (i = 0; i < ETH_ALEN; i++) {
		*(pa + (i & 1)) = dev->dev_addr[i];
		if (i & 0x01) {
			pa += 4;
		}
	}

	for(i = 0; tx_ring[tx_new].status & (T_OWN); i++) {
		if (i >= TOUT_LOOP) {
			printf("tx error buffer not ready tx_ring[%d]\n", i, tx_ring[tx_new].status);
			goto Done;
		}
	}

	tx_ring[tx_new].buf = (virt_to_phys((u32) &setup_frame[0]));
	tx_ring[tx_new].des1 = (TD_TCH | TD_SET| SETUP_FRAME_LEN);
	tx_ring[tx_new].status = (T_OWN);

	OUTL(dev, POLL_DEMAND, DE4X5_TPD);

	for(i = 0; tx_ring[tx_new].status & (T_OWN); i++) {
		if (i >= TOUT_LOOP) {
			printf("tx buffer not ready\n");
			goto Done;
		}
	}

	if ((tx_ring[tx_new].status) != 0x7FFFFFFF) {
		printf("TX error status2 = 0x%08X\n", (tx_ring[tx_new].status));
	}
	tx_ring[tx_new].des1 = 0xe1000000;//TD_TCH;
	tx_ring[tx_new].buf = virt_to_phys((u32) NetTxPackets[tx_new]);
	tx_ring[tx_new].next = (virt_to_phys((u32) &tx_ring[next_tx(tx_new)]));
	tx_new = next_tx(tx_new);

Done:
	return;
}

static void dmfe_start(struct ifnet *ifp)
{
	struct eth_device *sc = ifp->if_softc;
	struct mbuf *mb_head;		
	int length;

	while(ifp->if_snd.ifq_head != NULL ){

		if((tx_ring[tx_new].status & (T_OWN)))break;
		//printf("tx_new=%x\n",tx_new);
		
		IF_DEQUEUE(&ifp->if_snd, mb_head);
		
		m_copydata(mb_head, 0, mb_head->m_pkthdr.len, NetTxPackets[tx_new]);
		if(mb_head->m_pkthdr.len<60)
		memset(NetTxPackets[tx_new]+mb_head->m_pkthdr.len, 0, 60-mb_head->m_pkthdr.len);
		length=max(60, mb_head->m_pkthdr.len);
		tx_ring[tx_new].des1   = (0xe1000000 | length); //frame in a single TD
		tx_ring[tx_new].buf = virt_to_phys((u32) NetTxPackets[tx_new]);
		tx_ring[tx_new].next = (virt_to_phys((u32) &tx_ring[next_tx(tx_new)]));
		tx_ring[tx_new].status = (T_OWN);

	/*
	 * command the mac to start transmit process
	 */
		OUTL(sc, POLL_DEMAND, DE4X5_TPD);
    	tx_new = next_tx(tx_new);
		m_freem(mb_head);
		wbflush();
	} 
}


static struct mbuf * getmbuf(struct eth_device *sc)
{
	struct mbuf *m;


	MGETHDR(m, M_DONTWAIT, MT_DATA);
	if(m == NULL){
		printf("getmbuf for reception failed\n");
		return  NULL;
	} else {
		MCLGET(m, M_DONTWAIT);
		if ((m->m_flags & M_EXT) == 0) {
			m_freem(m);
			return NULL;
		}
		if(m->m_data != m->m_ext.ext_buf){
			printf("m_data not equal to ext_buf!!!\n");
		}
	}
	
#if defined(__mips__)
	/*
	 * Sync the buffer so we can access it uncached.
	 */
	if (m->m_ext.ext_buf!=NULL) {
		pci_sync_cache(sc->sc_pc, (vm_offset_t)m->m_ext.ext_buf,
				MCLBYTES, SYNC_R);
	}
	m->m_data += RFA_ALIGNMENT_FUDGE;
#else
	m->m_data += RFA_ALIGNMENT_FUDGE;
#endif
	return m;
}

int dmfe_poll(struct eth_device *sc);
int dmfe_intr(void *arg)
{
	struct eth_device *sc = (struct eth_device *)arg;
	struct ifnet *ifp = &sc->arpcom.ac_if;

		if (ifp->if_flags & IFF_RUNNING) 
		{
			dmfe_poll(sc);
	/*
	 * wait transmit to complete
	 */
		if ((tx_ring[tx_new].status) & TD_ES) { 
			tx_ring[tx_new].status = 0x0;
		}

		if(ifp->if_snd.ifq_head != NULL )dmfe_start(ifp);
		}
		return 1;
}


int dmfe_poll(struct eth_device *sc)
{
	
	struct ifnet *ifp = &sc->arpcom.ac_if;
	struct mbuf *m;
	struct ether_header *eh;
	s32		status;
	int	 length;
	int 	received=0;
	
	for ( ; !received; ) {
		length    = 0;
			
		status = (s32)(rx_ring[rx_new].status);
		
		if (status & R_OWN) {
			break;
		}

		rx_ring[rx_new].next = virt_to_phys((u32) &rx_ring[next_rx(rx_new)]);

		if (status & RD_LS) {
			/* Valid frame status.
			 */
			if (status & RD_ES) {

				/* There was an error.
				 */
				printf("RX error status = 0x%08X\n", status);
			} else {
				/* A valid frame received.
				 */
				length = (rx_ring[rx_new].status >> 16) & 0x3fff;

				/* Pass the packet up to the protocol
				 * layers.
				 */
				//printf("received a packet status %x\n", status);
				{
	m =getmbuf(sc);
	if (m == NULL){
		printf("getmbuf failed in  rtl8139_poll\n");
		return 0; // no successful
	}
	
		bcopy(NetRxPackets[rx_new], mtod(m, caddr_t), length - 4);

	//hand up  the received package to upper protocol for further dealt
	m->m_pkthdr.rcvif = ifp;
	m->m_pkthdr.len = m->m_len = (length -4)  -sizeof(struct ether_header);

	eh=mtod(m, struct ether_header *); 	

	m->m_data += sizeof(struct ether_header);
	//printf("%s, etype %x:\n", __FUNCTION__, eh->ether_type);
	ether_input(ifp, eh, m);
				}
				received =1;
			}

			/* Change buffer ownership for this frame, back
			 * to the adapter.
			 */
			rx_ring[rx_new].status = (R_OWN);
		}

		/* Update entry information.
		 */
		rx_new = next_rx(rx_new);
	}

	return length;
}

static int
dmfe_match(parent, match, aux)
	struct device *parent;
#if defined(__BROKEN_INDIRECT_CONFIG) || defined(__OpenBSD__)
	void *match;
#else
	struct cfdata *match;
#endif
	void *aux;
{
	return 1;
}

static void
dmfe_attach(parent, self, aux)
	struct device *parent, *self;
	void *aux;
{
	struct eth_device *sc = (struct eth_device *)self;
	struct ifnet *ifp;


	sc->addr_len=6;
	read_hw_addr(sc);
	sc->iobase = MAC_REG_BASE;

#ifdef __OpenBSD__
	ifp = &sc->arpcom.ac_if;
	bcopy(sc->dev_addr, sc->arpcom.ac_enaddr, sc->addr_len);
#else
	ifp = &sc->sc_ethercom.ec_if;
#endif
	bcopy(sc->sc_dev.dv_xname, ifp->if_xname, IFNAMSIZ);
	
	ifp->if_softc = sc;
	ifp->if_flags = IFF_BROADCAST | IFF_SIMPLEX | IFF_MULTICAST;
	ifp->if_ioctl = dmfe_ether_ioctl;
	ifp->if_start = dmfe_start;
	ifp->if_watchdog = 0;

	printf(": address %s\n",
	    ether_sprintf(sc->arpcom.ac_enaddr));

	/*
	 * Attach the interface.
	 */
	if_attach(ifp);
	/*
	 * Let the system queue as many packets as we have available
	 * TX descriptors.
	 */
	ifp->if_snd.ifq_maxlen = 4;
#ifdef __NetBSD__
	ether_ifattach(ifp, sc->dev_addr);
#else
	ether_ifattach(ifp);
#endif
#if NBPFILTER > 0
#ifdef __OpenBSD__
	bpfattach(&sc->arpcom.ac_if.if_bpf, ifp, DLT_EN10MB,
	    sizeof(struct ether_header));
#else
	bpfattach(&sc->sc_ethercom.ec_if.if_bpf, ifp, DLT_EN10MB,
	    sizeof(struct ether_header));
#endif
#endif

	/*
	 * Add shutdown hook so that DMA is disabled prior to reboot. Not
	 * doing do could allow DMA to corrupt kernel memory during the
	 * reboot before the driver initializes.
	 */
	shutdownhook_establish(dmfe_shutdown, sc);

#ifndef PMON
	/*
	 * Add suspend hook, for similiar reasons..
	 */
	powerhook_establish(dmfe_power, sc);
#endif
	tgt_poll_register(IPL_NET, dmfe_intr, sc);
}

struct cfattach dmfe_ca = {
	sizeof(struct eth_device), dmfe_match, dmfe_attach
};

struct cfdriver dmfe_cd = {
	NULL, "dmfe", DV_IFNET
};
