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

#include <dev/pci/if_fxpreg.h>
#include <dev/pci/if_fxpvar.h>
typedef struct FILE {
	int fd;
	int valid;
	int ungetcflag;
	int ungetchar;
} FILE;
extern FILE _iob[];
#define serialout (&_iob[1])

#ifdef __alpha__		/* XXX */
/* XXX XXX NEED REAL DMA MAPPING SUPPORT XXX XXX */
#undef vtophys
#define	vtophys(va)	alpha_XXX_dmamap((vm_offset_t)(va))
#endif /* __alpha__ */

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

#include <linux/types.h>
//#include <linux/pci.h>
#include "e1000_pmon.c"

#include "e1000_main.c"
#include "e1000_hw.c"
#include "e1000_param.c"

static long long e1000_read_mac(struct net_device *nic);

static int pci_read_config_dword(struct pci_dev *linuxpd, int reg, u32 *val)
{
	if ((reg & 3) || reg < 0 || reg >= 0x100) {
        	printf ("pci_read_config_dword: bad reg %x\n", reg);
        	return -1;
    	}
	*val=_pci_conf_read(linuxpd->pa.pa_tag, reg);       
	return 0;
}

static int pci_write_config_dword(struct pci_dev *linuxpd, int reg, u32 val)
{
    if ((reg & 3) || reg < 0 || reg >= 0x100) {
	    printf ("pci_write_config_dword: bad reg %x\n", reg);
	return -1;
    }
   _pci_conf_write(linuxpd->pa.pa_tag,reg,val); 
   return 0;
}

static int pci_read_config_word(struct pci_dev *linuxpd, int reg, u16 *val)
{
	if ((reg & 1) || reg < 0 || reg >= 0x100) {
        	printf ("pci_read_config_word: bad reg %x\n", reg);
        	return -1;
    	}
	*val=_pci_conf_readn(linuxpd->pa.pa_tag,reg,2);       
	return 0;
}

static int pci_write_config_word(struct pci_dev *linuxpd, int reg, u16 val)
{
    if ((reg & 1) || reg < 0 || reg >= 0x100) {
	    printf ("pci_write_config_word: bad reg %x\n", reg);
	return -1;
    }
   _pci_conf_writen(linuxpd->pa.pa_tag,reg,val,2); 
   return 0;
}

static int pci_read_config_byte(struct pci_dev *linuxpd, int reg, u8 *val)
{
    if (reg < 0 || reg >= 0x100) {
	    printf ("pci_write_config_word: bad reg %x\n", reg);
	return -1;
    }
	*val=_pci_conf_readn(linuxpd->pa.pa_tag,reg,1);       
	return 0;
}

static int pci_write_config_byte(struct pci_dev *linuxpd, int reg, u8 val)
{
    if (reg < 0 || reg >= 0x100) {
	    printf ("pci_write_config_word: bad reg %x\n", reg);
	return -1;
    }
   _pci_conf_writen(linuxpd->pa.pa_tag,reg,val,1); 
   return 0;
}

void netdev_init(struct net_device *netdev,struct pci_attach_args *pa)
{
    unsigned short  vendor;
    unsigned short  device,class;
    unsigned short  subsystem_vendor;
    unsigned short  subsystem_device;
    unsigned int i;
	static int irq=0;

        vendor = pa->pa_id & 0xffff;
        device = (pa->pa_id >> 16) & 0xffff;
        class=(pa->pa_class >>8);
        i=pci_conf_read(0,pa->pa_tag,PCI_SUBSYSTEM_VENDOR_ID);
        subsystem_vendor=i&0xffff;
        subsystem_device=i>>16;
	netdev->pcidev.vendor=vendor;
	netdev->pcidev.device=device;
	netdev->pcidev.subsystem_vendor=subsystem_vendor;
    netdev->pcidev.subsystem_device=subsystem_device;
	netdev->pcidev.pa=*pa;
	netdev->priv=kmalloc(sizeof(struct e1000_adapter),GFP_KERNEL);//&netdev->em;
	memset(netdev->priv,0,sizeof(struct e1000_adapter));
	netdev->addr_len=6;
	netdev->irq = netdev->pcidev.irq=irq++;
}

static int e1000_ether_ioctl(struct ifnet *ifp,FXP_IOCTLCMD_TYPE cmd,caddr_t data);

static struct pci_device_id *e1000_pci_id=0;
/*
 * Check if a device is an 82557.
 */
static void e1000_start(struct ifnet *ifp);
static int
em_match(parent, match, aux)
	struct device *parent;
#if defined(__BROKEN_INDIRECT_CONFIG) || defined(__OpenBSD__)
	void *match;
#else
	struct cfdata *match;
#endif
	void *aux;
{
	struct pci_attach_args *pa = aux;
if(getenv("noem"))return 0;
e1000_pci_id=pci_match_device(e1000_pci_tbl,pa);
return e1000_pci_id?1:0;
}

static void
e1000_shutdown(sc)
        void *sc;
{
struct e1000_adapter *adapter=((struct net_device *)sc)->priv;
        e1000_suspend(adapter->pdev, 3);
}


extern char activeif_name[];
static int em_intr(void *data)
{
struct net_device *netdev = data;
int irq=netdev->irq;
struct ifnet *ifp = &netdev->arpcom.ac_if;
	if(ifp->if_flags & IFF_RUNNING)
	{
		if(irqstate&(1<<irq))
		{
			e1000_intr(irq,data,0);
			run_task_queue(&tq_e1000);
		   if (ifp->if_snd.ifq_head != NULL)
		   e1000_start(ifp);
		}
	return 1;
	}
	return 0;
}

struct net_device *mynic_em;

static void
em_attach(parent, self, aux)
	struct device *parent, *self;
	void *aux;
{
	struct net_device  *sc = (struct net_device *)self;
	struct pci_attach_args *pa = aux;
	//pci_chipset_tag_t pc = pa->pa_pc;
	pci_intr_handle_t ih;
	const char *intrstr = NULL;
	struct ifnet *ifp;
#ifdef __OpenBSD__
	//bus_space_tag_t iot = pa->pa_iot;
	//bus_addr_t iobase;
	//bus_size_t iosize;
#endif

	mynic_em = sc;
	/*
	 * Allocate our interrupt.
	 */
	if (pci_intr_map(pc, pa->pa_intrtag, pa->pa_intrpin,
	    pa->pa_intrline, &ih)) {
		printf(": couldn't map interrupt\n");
		return;
	}
	
	intrstr = pci_intr_string(pc, ih);
#ifdef __OpenBSD__
	sc->sc_ih = pci_intr_establish(pc, ih, IPL_NET, em_intr, sc, self->dv_xname);
#else
	sc->sc_ih = pci_intr_establish(pc, ih, IPL_NET, em_intr, sc);
#endif
	
	if (sc->sc_ih == NULL) {
		printf(": couldn't establish interrupt");
		if (intrstr != NULL)
			printf(" at %s", intrstr);
		printf("\n");
		return;
	}
	
	netdev_init(sc,pa);
	/* Do generic parts of attach. */
	if (em_probe(sc,e1000_pci_id,&sc->pcidev)) {
		/* Failed! */
		cmd_wrprom_em0(0,0);  //zgj		
		return;
	}

#ifdef __OpenBSD__
	ifp = &sc->arpcom.ac_if;
	bcopy(sc->dev_addr, sc->arpcom.ac_enaddr, sc->addr_len);
#else
	ifp = &sc->sc_ethercom.ec_if;
#endif
	bcopy(sc->sc_dev.dv_xname, ifp->if_xname, IFNAMSIZ);
	
	ifp->if_softc = sc;
	ifp->if_flags = IFF_BROADCAST | IFF_SIMPLEX | IFF_MULTICAST;
	ifp->if_ioctl = e1000_ether_ioctl;
	ifp->if_start = e1000_start;
	ifp->if_watchdog = 0;

	printf(": %s, address %s\n", intrstr,
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
	shutdownhook_establish(e1000_shutdown, sc);

#ifndef PMON
	/*
	 * Add suspend hook, for similiar reasons..
	 */
	powerhook_establish(e1000_power, sc);
#endif
}


/*
 * Start packet transmission on the interface.
 */



static void e1000_start(struct ifnet *ifp)
{
	struct net_device *sc = ifp->if_softc;
	struct mbuf *mb_head;		
	struct sk_buff *skb;

	while(ifp->if_snd.ifq_head != NULL ){
		
		IF_DEQUEUE(&ifp->if_snd, mb_head);
		
		skb=dev_alloc_skb(mb_head->m_pkthdr.len);
		m_copydata(mb_head, 0, mb_head->m_pkthdr.len, skb->data);
		skb->len=mb_head->m_pkthdr.len;
		e1000_xmit_frame(skb,sc);

		m_freem(mb_head);
		wbflush();
	} 
}

static int
em_init(struct net_device *netdev)
{
    struct ifnet *ifp = &netdev->arpcom.ac_if;
	int stat=0;
    ifp->if_flags |= IFF_RUNNING;
	if(!netdev->opencount){ stat=e1000_open(netdev);netdev->opencount++;}
	e1000_tx_timeout_task(netdev);
	return stat;
}

static int
em_stop(struct net_device *netdev)
{
    struct ifnet *ifp = &netdev->arpcom.ac_if;
	ifp->if_timer = 0;
	ifp->if_flags &= ~(IFF_RUNNING | IFF_OACTIVE);
	if(netdev->opencount){e1000_close(netdev);netdev->opencount--;}
	return 0;
}

static int
e1000_ether_ioctl(ifp, cmd, data)
	struct ifnet *ifp;
	FXP_IOCTLCMD_TYPE cmd;
	caddr_t data;
{
	struct ifaddr *ifa = (struct ifaddr *) data;
	struct net_device *sc = ifp->if_softc;
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
			error = em_init(sc);
			if(error <0 )
				return(error);
			ifp->if_flags |= IFF_UP;

#ifdef __OpenBSD__
			arp_ifinit(&sc->arpcom, ifa);
#else
			arp_ifinit(ifp, ifa);
#endif
			
			break;
#endif

		default:
			error = em_init(sc);
			if(error <0 )
				return(error);
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
			error = em_init(sc);
			if(error <0 )
				return(error);
		} else {
			if (ifp->if_flags & IFF_RUNNING)
				em_stop(sc);
		}
		break;
       case SIOCETHTOOL:
       		{
       		long *p=data;
		mynic_em = sc;
       		cmd_setmac_em0(p[0],p[1]);
       		}
       		break;
	case SIOCGETHERADDR:
	{
		long long val;
		char *p=data;
		mynic_em = sc;
		val =e1000_read_mac(mynic_em);
		p[5] = val>>40&0xff; 
		p[4] = val>>32&0xff; 
		p[3] = val>>24&0xff; 
		p[2] = val>>16&0xff; 
		p[1] = val>>8&0xff; 
		p[0] = val&0xff; 

	}
	break;

       case SIOCRDEEPROM:
                {
                long *p=data;
		mynic_em = sc;
                cmd_reprom_em0(p[0],p[1]);
                }
                break;
       case SIOCWREEPROM:
                {
                long *p=data;
		mynic_em = sc;
                cmd_wrprom_em0(p[0],p[1]);
                }
                break;

	default:
		error = EINVAL;
	}

	splx(s);
	return (error);
}

struct cfattach em_ca = {
	sizeof(struct net_device), em_match, em_attach
};

struct cfdriver em_cd = {
	NULL, "em", DV_IFNET
};

static long long e1000_read_mac(struct net_device *nic)
{

        int i;
        long long mac_tmp = 0;
        struct e1000_adapter *adapter = (struct e1000_adapter *) (mynic_em->priv);

        e1000_read_mac_addr(&adapter->hw);
        memcpy(mynic_em->dev_addr, adapter->hw.mac_addr, mynic_em->addr_len);

        for (i = 0; i < 6; i++) {
                mac_tmp <<= 8;
                mac_tmp |=  adapter->hw.mac_addr[i];
}
        return mac_tmp;
}

#include <pmon.h>

        unsigned short val = 0;
int cmd_setmac_em0(int ac, char *av[])
{
        int i;
        unsigned short v;
        struct net_device *nic = mynic_em ;
        struct e1000_adapter *adapter = (struct e1000_adapter *) (mynic_em->priv);

        if(nic == NULL){
               printf("E1000 interface not initialized\n");
                return 0;
        }
        if(ac != 2){
        long long macaddr;
        u_int8_t *paddr;
        u_int8_t enaddr[6];
        macaddr=e1000_read_mac(nic);
        paddr=(uint8_t*)&macaddr;
        enaddr[0] = paddr[5- 0];
        enaddr[1] = paddr[5- 1];
        enaddr[2] = paddr[5- 2];
        enaddr[3] = paddr[5- 3];
        enaddr[4] = paddr[5- 4];
        enaddr[5] = paddr[5- 5];
                printf("MAC ADDRESS ");
                for(i=0; i<6; i++){
                        printf("%02x",enaddr[i]);
                        if(i==5)
                                printf("\n");
                        else
                                printf(":");
                }
                printf("Use \"setmac <mac> \" to set mac address\n");
                return 0;
        }
        for (i = 0; i < 3; i++) {
                val = 0;
                gethex(&v, av[1], 2);
                val = v ;
                av[1]+=3;
                gethex(&v, av[1], 2);
                val = val | (v << 8);
                av[1] += 3;
        e1000_write_eeprom(&adapter->hw,i,1,&val);
        }

        if(e1000_update_eeprom_checksum(&adapter->hw) == 0)
                printf("the checksum is right!\n");
        printf("The MAC address have been written done\n");
        return 0;
}
#if 1
static unsigned long next = 1;
           /* RAND_MAX assumed to be 32767 */
static int myrand(void) {
               next = next * 1103515245 + 12345;
               return((unsigned)(next/65536) % 32768);
           }

static void mysrand(unsigned int seed) {
               next = seed;
           }
#endif
int cmd_wrprom_em0(int ac,char **av)
{
        int i=0;
	unsigned long clocks_num=0;
        unsigned short eeprom_data;
        unsigned char tmp[4];
	unsigned short rom_82541[EEPROM_CHECKSUM_REG+1]={
                                0x1b00, 0x0821, 0x23a7, 0x0210, 0xffff, 0x1000 ,0xffff, 0xffff,
                                0xc802, 0x3502, 0x640b, 0x1376, 0x8086, 0x107c, 0x8086, 0xb284,
                                0x20dd, 0x5555, 0x0000, 0x2f90, 0x3200, 0x0012, 0x1e20, 0x0012,
                                0x1e20, 0x0012, 0x1e20, 0x0012, 0x1e20, 0x0009, 0x0200, 0x0000,
                                0x000c, 0x93a6, 0x280b, 0x0000, 0x0400, 0xffff, 0xffff, 0xffff,
                                0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0x0602,
                                0x0100, 0x4000, 0x1216, 0x4007, 0xffff, 0xffff, 0xffff, 0xffff,
                                0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0x7dfa
                        };
unsigned short rom_82546[EEPROM_CHECKSUM_REG+1]=
{
0x1234, 0x5678, 0xab90, 0x0320, 0xffff, 0xffff, 0xffff, 0xffff,
0xffff, 0xffff, 0x4408, 0x000c, 0x8086, 0x1010, 0x8086, 0x3200,
0x000c, 0x1010, 0x0000, 0x2102, 0x10c8, 0xffff, 0xffff, 0xffff,
0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff,
0x000c, 0xff61, 0x5004, 0x2102, 0x00c8, 0xffff, 0xffff, 0xffff,
0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0x0602,
0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff,
0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff,
};
		unsigned short *rom=rom_82541;
        struct e1000_adapter *adapter = (struct e1000_adapter *)(mynic_em->priv);
		
		if(PCI_HDRTYPE_MULTIFN(pci_conf_read(0,mynic_em->pcidev.pa.pa_tag,PCI_BHLC_REG)))
		rom=rom_82546;
        printf("write the whole eeprom\n");

#if 1
		clocks_num =CPU_GetCOUNT();
		mysrand(clocks_num);
		for( i = 0; i < 4;i++ )
		{
			tmp[i]=myrand()%256;
			printf( " tmp[%d]=0x%2x\n", i,tmp[i]);
		}
		eeprom_data =tmp[1] |( tmp[0]<<8);
		rom[1] = eeprom_data ;
		printf("eeprom_data [1] = 0x%4x\n",eeprom_data);
                eeprom_data =tmp[3] |( tmp[2]<<8);
		rom[2] = eeprom_data;
		printf("eeprom_data [2] = 0x%4x\n",eeprom_data);
#endif

	if(ac>1)
	{
	 //offset:data,data
	 int i;
	 int offset;
	 int data;
	 for(i=1;i<ac;i++)
	 {
	 	char *p=av[i];
		char *nextp;
	 	int offset=strtoul(p,&nextp,0);
		while(nextp!=p)
		{
		p=++nextp;
		data=strtoul(p,&nextp,0);
		if(nextp==p)break;
		rom[offset++]=data;
		}
	 }
	}
        for(i=0; i< EEPROM_CHECKSUM_REG; i++)
        {
                eeprom_data = rom[i];
		printf("rom[%d] = 0x%x\n",i,rom[i]);
                e1000_write_eeprom(&adapter->hw, i, 1 , &eeprom_data) ;
        }
        if(e1000_update_eeprom_checksum(&adapter->hw) == 0)
                printf("the checksum is right!\n");
        printf("The whole eeprom have been written done\n");
	return 0;
}

int cmd_reprom_em0(int ac, char *av)
{
        int i;
        unsigned short eeprom_data;
        struct e1000_adapter *adapter = (struct e1000_adapter *) (mynic_em->priv);
        printf("dump eprom:\n");

        for(i=0; i <= EEPROM_CHECKSUM_REG;)
        {
                if(e1000_read_eeprom(&adapter->hw, i, 1 , &eeprom_data) < 0)
                {
                //        printf("EEPROM Read Error\n");
                //        return -E1000_ERR_EEPROM;
                }
                printf("%04x ", eeprom_data);
                ++i;
                if( i%8 == 0 )
                        printf("\n");
        }
        return 0;
}

static const Optdesc netdmp_opts[] =
{
    {"<interface>", "Interface name"},
    {"<netdmp>", "IP Address"},
    {0}
};

static const Cmd Cmds[] =
{
        {"em"},
        {"setmac_em", "", NULL,
                    "Set mac address into E1000 eeprom", cmd_setmac_em0, 1, 5, 0},
        {"readrom_em", "", NULL,
                        "dump E1000 eprom content", cmd_reprom_em0, 1, 2, 0},
        {"writerom_em", "", NULL,
                        "write E1000 eprom content", cmd_wrprom_em0, 1, 2, 0},
        {0, 0}
};


static void init_cmd __P((void)) __attribute__ ((constructor));

static void
init_cmd()
{
        cmdlist_expand(Cmds, 1);
}
