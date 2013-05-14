/*
 * URB OHCI HCD (Host Controller Driver) for USB on the AU1x00.
 *
 * (C) Copyright 2003
 * Gary Jennejohn, DENX Software Engineering <gj@denx.de>
 *
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
 *
 * Note: Part of this code has been derived from linux
 *
 * (C) Copyright 2006
 *  Imported to godson pmon by yanhua@ict.ac.cn
 */
/*
 * IMPORTANT NOTES
 * 1 - you MUST define LITTLEENDIAN in the configuration file for the
 *     board or this driver will NOT work!
 * 2 - this driver is intended for use with USB Mass Storage Devices
 *     (BBB) ONLY. There is NO support for Interrupt or Isochronous pipes!
 */


/************************************************************************

 Copyright (C)
 File name:     usb-ohci.c
 Author:  ***      Version:  ***      Date: ***
 Description:   This C file is the main implementation of usb ohci controller 
                according to the USB-OHCI Interface spec 1.0a.
                If you want to understand this file well, please see the 
                USB spec 1.1 and USB-OHCI interface spec 1.0a carefully, you 
                can acquire them from www.usb.org.
 Others:        The version of PMON which this C file belongs to is used on 
                Loongson based Platform to do the necessary initialization 
                and load the Linux kernel.
 Function List:
 
 Revision History:
 
 --------------------------------------------------------------------------
  Date          Author          Activity ID     Activity Headline
  2008-03-10    QianYuli        PMON00000001    Add comment to each function
  2008-02-28    QianYuli        PMON00000001    Add multi-usb-devices supports
*************************************************************************/

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/mbuf.h>
#include <sys/malloc.h>
#include <sys/kernel.h>

#include <vm/vm.h>		/* for vtophys */

#include <machine/cpu.h>
#include <machine/bus.h>
#include <machine/intr.h>

#include <dev/pci/pcivar.h>
#include <dev/pci/pcireg.h>
#include <dev/pci/pcidevs.h>
#include <sys/device.h>
#include <autoconf.h>

#include "usb.h"
#include "usb-ohci.h"

#define OHCI_USE_NPS		  /* force NoPowerSwitching mode */
#define OHCI_VERBOSE_DEBUG	0 /* not always helpful */

#define USBH_ENABLE_BE (1<<0)
#define USBH_ENABLE_C  (1<<1)
#define USBH_ENABLE_E  (1<<2)
#define USBH_ENABLE_CE (1<<3)
#define USBH_ENABLE_RD (1<<4)


#ifdef LITTLEENDIAN
#define USBH_ENABLE_INIT (USBH_ENABLE_CE | USBH_ENABLE_E | USBH_ENABLE_C)
#else
#define USBH_ENABLE_INIT (USBH_ENABLE_CE | USBH_ENABLE_E | USBH_ENABLE_C | USBH_ENABLE_BE)
#endif


/* For initializing controller (mask in an HCFS mode too) */
#define OHCI_CONTROL_INIT \
	(OHCI_CTRL_CBSR & 0x3) | OHCI_CTRL_IE | OHCI_CTRL_PLE

#define min_t(type,x,y) ({ type __x = (x); type __y = (y); __x < __y ? __x: __y; })

#undef DEBUG
#ifdef DEBUG
#define dbg(format, arg...) printf("DEBUG: " format "\n", ## arg)
#else
#define dbg(format, arg...) do {} while(0)
#endif /* DEBUG */
#define err(format, arg...) printf("ERROR: " format "\n", ## arg)
#define SHOW_INFO
#ifdef SHOW_INFO
#define info(format, arg...) printf("INFO: " format "\n", ## arg)
#else
#define info(format, arg...) do {} while(0)
#endif

#define m16_swap(x) swap_16(x)
#define m32_swap(x) swap_32(x)

#undef readl
#undef writel

#define readl(addr) *(volatile u32*)(((u32)(addr)) | 0xa0000000)
#define writel(val, addr) *(volatile u32*)(((u32)(addr)) | 0xa0000000) = (val)

#define MAX_OHCI_C 4   /*In most case it is enough */
ohci_t *usb_ohci_dev[MAX_OHCI_C];
/* RHSC flag */
int got_rhsc;
/* device which was disconnected */
struct usb_device *devgone;

//QYL-2008-03-07
#define OHCI_MAX_USBDEVICES 8
#define OHCI_MAX_ENDPOINTS_PER_DEVICE 32
urb_priv_t ohci_urb[OHCI_MAX_USBDEVICES][OHCI_MAX_ENDPOINTS_PER_DEVICE];
extern struct usb_device usb_dev[];
extern int dev_index;

/*
 * Hook to initialize hostcontroller
 */
static int ohci_match(struct device *parent, void *match, void *aux);
static void ohci_attach(struct device *parent, struct device *self, void *aux);
static int ohci_submit_bulk_msg(struct usb_device *dev, unsigned long pipe, void *buffer, int transfer_len);
static int ohci_submit_control_msg(struct usb_device *dev, unsigned long pipe, void *buffer, int transfer_len, struct devrequest *setup);
static int ohci_submit_int_msg(struct usb_device *dev, unsigned long pipe, void *buffer,
		int transfer_len, int interval);
static void ohci_device_notify(struct usb_device *dev, int port);

int usb_lowlevel_init(ohci_t *ohci);

int ohci_debug = 0;

struct usb_ops ohci_usb_op = {
	.submit_bulk_msg	= 	ohci_submit_bulk_msg,
	.submit_control_msg	= 	ohci_submit_control_msg,
	.submit_int_msg		= 	ohci_submit_int_msg,
};

struct cfattach ohci_ca = {
	.ca_devsize = sizeof(struct ohci),
	.ca_match 	= ohci_match,
	.ca_attach 	= ohci_attach,
};

struct cfdriver ohci_cd = {
	.cd_devs = NULL,
	.cd_name = "ohci",
	.cd_class = DV_DULL,
};

/* forward declaration */
static int hc_interrupt (void *);

//QYL-2008-03-07
static int hc_check_ohci_controller(void *);
void arouse_usb_int_pipe(ohci_t *);
u_int32_t check_device_sequence(ohci_t *pohci);

#ifdef CONFIG_SM502_USB_HCD
/* panel clock */
#define SM501_CLOCK_P2XCLK      (24)
/* crt clock */   
#define SM501_CLOCK_V2XCLK      (16)
/* main clock */
#define SM501_CLOCK_MCLK        (8)
/* SDRAM controller clock */
#define SM501_CLOCK_M1XCLK      (0)

#define  SM502_BUF_SIZE 0x2000000
#define  SM502_USB_MEM_SIZE 0x90000
#define  SM502_USB_BUF_SIZE 0x4000
#define  SM502_HCCA_SIZE 0x100

unsigned long sm502_reg_base;

/* clock value structure. */
struct sm501_clock {
	unsigned long mclk;
	int divider;
	int shift;

	long multipleM;
	int dividerN;
	short divby2;
};

/* Perform a rounded division. */
static long sm501fb_round_div(long num, long denom)
{
        /* n / d + 1 / 2 = (2n + d) / 2d */
        return (2 * num + denom) / (2 * denom);
}

/* sm501_select_clock
 *
 * selects nearest discrete clock frequency the SM501 can achive
 *   the maximum divisor is 3 or 5
 */
static unsigned long sm501_select_clock(unsigned long freq,
					struct sm501_clock *clock,
					int max_div)
{
	unsigned long mclk;
	int divider;
	int shift;
	long diff;
	long best_diff = 999999999;

	/* Try 288MHz and 336MHz clocks. */
	for (mclk = 288000000; mclk <= 336000000; mclk += 48000000) {
		/* try dividers 1 and 3 for CRT and for panel,
		   try divider 5 for panel only.*/

		for (divider = 1; divider <= max_div; divider += 2) {
			/* try all 8 shift values.*/
			for (shift = 0; shift < 8; shift++) {
				/* Calculate difference to requested clock */
				diff = sm501fb_round_div(mclk, divider << shift) - freq;
				if (diff < 0)
					diff = -diff;

				/* If it is less than the current, use it */
				if (diff < best_diff) {
					best_diff = diff;

					clock->mclk = mclk;
					clock->divider = divider;
					clock->shift = shift;
				}
			}
		}
	}

	/* Return best clock. */
	return clock->mclk / (clock->divider << clock->shift);
}


unsigned long sm501_set_clock(int clksrc, unsigned long req_freq)
{
	unsigned long mode = readl(sm502_reg_base + 0x0054);
	unsigned long clock = readl(sm502_reg_base + 0x003c);
	unsigned char reg = 0;
	unsigned long sm501_freq,pllclock;

	struct sm501_clock to;

	switch (clksrc) {
	case SM501_CLOCK_P2XCLK:
		break;
	case SM501_CLOCK_V2XCLK: 
		break;
	case SM501_CLOCK_MCLK:
	case SM501_CLOCK_M1XCLK:

		sm501_freq = sm501_select_clock( req_freq, &to, 3);
		reg = to.shift & 0x07;
		if(to.divider == 3)
			reg |= 0x08;
		if(to.mclk != 288000000)
			reg |= 0x10;
		break;
	default :
		return 0;
	}

	mode = readl(sm502_reg_base + 0x0054);
	clock = readl(sm502_reg_base + 0x003c);

	clock = clock & ~(0xFF << clksrc);
	clock |= reg<<clksrc;

	mode = mode & 0x03;

	switch(mode){
	case 0:
		writel(clock, sm502_reg_base + 0x0044);
		mode = 0;
		break;
	case 1:
		writel(clock, sm502_reg_base + 0x004c);
		mode = 1;
		break;
	default :
		return -1;

	}

	readl(sm502_reg_base + 0x00);
	delay(16000);

	return sm501_freq;

}

void sm502_mem_init(struct ohci *ohci, bus_addr_t base, bus_addr_t size)
{
	struct pci_attach_args *pa = &ohci->pa;
	bus_space_tag_t memt= pa->pa_memt;
	u32 value;

	value = readl(sm502_reg_base + 0x0010);
	writel(((value&~0x07000000)|(5 <<24))|(3<<13)|(2<<22)|(2<<11), sm502_reg_base+0x0010);

	value = readl(sm502_reg_base + 0x0010);
	printf("sm502 mem %x\n", (value >> 24) & 0x7);

	bus_space_map(memt, base, size, 0, (unsigned long *)&ohci->lmem);
	ohci->free = SM502_BUF_SIZE - SM502_USB_MEM_SIZE;
	printf("sm502-usb mem %08x->%08x\n", base, ohci->lmem);
	ohci->lmem_paddr = base + SM502_BUF_SIZE - SM502_USB_MEM_SIZE;
}

void *sm502_mem_alloc(struct ohci *ohci, size_t size)
{
	void *p;
	
	if(ohci->lmem == NULL) {
		printf("sm502 mem not initialized");
		return NULL;
	}

	
	if(size + ohci->free - (SM502_BUF_SIZE - SM502_USB_MEM_SIZE)>= SM502_USB_MEM_SIZE){
		printf("sm502-usb: out of memory %x/%x\n", size, ohci->free);
		return NULL;
	}
	p = (void *)(((unsigned long)ohci->lmem) + ohci->free);
	ohci->free += size;

	return p;
}

void sm502_bufs_init(struct ohci *ohci)
{
	int i;

	ohci->free += 0x1000 - 1;
	ohci->free &= ~0x0fff;

	for(i=0; i<MAX_SM502_BUFS; i++)
		ohci->sm502_bufs[i] = sm502_mem_alloc(ohci, SM502_USB_BUF_SIZE);

	memset(ohci->sm502_buf_use, 0, sizeof(ohci->sm502_buf_use));
}

void *sm502_alloc_buf(struct ohci *ohci)
{
	int i;

	for(i=0; i<MAX_SM502_BUFS; i++){
		if(ohci->sm502_buf_use[i] == 0)
			break;
	}

	if(i== MAX_SM502_BUFS)
		return NULL;
	else {
		ohci->sm502_buf_use[i]=1;
		return ohci->sm502_bufs[i];
	}
}

void sm502_free_buf(struct ohci *ohci, void *addr)
{
	int i;

	for(i=0; i<MAX_SM502_BUFS; i++)
		if(ohci->sm502_bufs[i] == addr)
			ohci->sm502_buf_use[i] = 0;
}

bus_addr_t sm502_vtophys(struct ohci *ohci, void *addr)
{
	return (unsigned long)addr - (unsigned long)ohci->lmem;
}

void * sm502_phystov(struct ohci *ohci, bus_addr_t phys)
{
	return (void*)((unsigned long)ohci->lmem + phys);
}

#endif
/*===========================================================================
*
*FUNTION: ohci_match
*
*DESCRIPTION: This function is used to check whether the interface attched to 
*             the PCI bus is a USB OHCI controller.
*
*PARAMETERS:
*          [IN] parent: not used here.
*          [IN] self: not used here.
*          [IN] aux:  pointer used to be cast to struct pci_attach_args*, so as
*                     to be used to do the PCI bus access. 
*
*RETURN VALUE: 1 : Successfully found USB OHCI controller.
*              0 : haven't found USB OHCI controller.
*                   
*Note:     This function is not called by system so clearly or directly, but it
*          is sure that this function is called at the right time by system,and 
*          the proper parameters will be passed in, so don't worry~~
*===========================================================================*/
static int ohci_match(struct device *parent, void *match, void *aux)
{
		
	struct pci_attach_args *pa = aux;
#ifdef CONFIG_SM502_USB_HCD
	char *no502;
#endif

	if(PCI_CLASS(pa->pa_class) == PCI_CLASS_SERIALBUS && 
		  PCI_SUBCLASS(pa->pa_class) == PCI_SUBCLASS_SERIALBUS_USB){ 
		if(((pa->pa_class >>8) & 0xff) == 0x10){
			//printf("Found usb ohci controller\n");
			return 1; 
		}
	}
#ifdef CONFIG_SM502_USB_HCD
	if((no502 = getenv("no502"))&&strtoul(no502, NULL, 0))
		return 0;
	if(PCI_VENDOR(pa->pa_id) == 0x126f && 
		PCI_PRODUCT(pa->pa_id) == 0x501)
			return 1;
#endif

	return 0;
}

extern struct hostcontroller host_controller;
extern struct usb_device * usb_alloc_new_device(void *hc_private);


/*===========================================================================
*
*FUNTION: ohci_attach
*
*DESCRIPTION: This function is used to do the attachment of the USB OHCI controller,
*             for USB OHCI controller is  accessed through PCI bus, so here it 
*             needs do some initializations such as mapping between CPU address 
*             and PCI address\A1\A2 configuring the OHCI registers\A1\A2enumerating the 
*             USB devices attached to the USB HUB ports.In fact,this function 
*             does the most of initial works by calling other related functions.
*
*PARAMETERS:
*          [IN] parent: not used here.
*          [IN] self: pointer used to be cast to struct ohci*,so as to do the
*                     the things such as accessing OHCI registers.
*          [IN] aux:  pointer used to be cast to struct pci_attach_args*, so as
*                     to be used to do the PCI configuration for USB OHCI controller 
*
*RETURN VALUE: None
*                   
*Note:     This function is not called by system so clearly or directly, but it
*          is sure that this function is called at the right time by system,and 
*          the proper parameters will be passed in, so don't worry~~
*===========================================================================*/
static void ohci_attach(struct device *parent, struct device *self, void *aux)
{
	struct ohci *ohci = (struct ohci*)self;
	struct pci_attach_args *pa = (struct pci_attach_args *)aux;
	static int ohci_dev_index = 0;
	int val;
	
#define  OHCI_PCI_MMBA 0x10
	pci_chipset_tag_t pc = pa->pa_pc;
	bus_space_tag_t memt = pa->pa_memt;
	bus_addr_t membase; 
	bus_addr_t memsize;
	int cachable;
#ifdef CONFIG_SM502_USB_HCD
	bus_addr_t membase2;
	bus_addr_t memsize2;
#endif

	/* Or we just return false in the match function */
	if(ohci_dev_index >= MAX_OHCI_C) {
		printf("Exceed max controller limits\n");
		return;
	}
#ifdef USB_OHCI_NO_ROM 
	if(PCI_VENDOR(pa->pa_id) == 0x1033){
		val = pci_conf_read(ohci->sc_pc, pa->pa_tag, 0xe0);
		pci_conf_write(ohci->sc_pc, pa->pa_tag, 0xe0, (val & ~0x3) | 0x3);
		//pci_conf_write(ohci->sc_pc, pa->pa_tag, 0xe0, (val & ~0x7) | 0x4);
		//pci_conf_write(ohci->sc_pc, pa->pa_tag, 0xe4, (1<<5));
	}
#endif
	ohci->pa = *pa;
	usb_ohci_dev[ohci_dev_index++] = ohci;
#ifdef CONFIG_SM502_USB_HCD
	if(PCI_VENDOR(pa->pa_id) == 0x126f) {
		pci_mem_find(pc, pa->pa_tag, 0x14, &membase, &memsize, &cachable);
		sm502_reg_base = membase | 0xb0000000;
		printf("sm502-usb membase %x\n", membase);
		membase = membase + 0x40000;
		ohci->flags = 0x80;
		goto SM502_HC;
	}
#endif

#ifdef LMDEXXON
#define USB_MASK_WEBCAM 0x01
#define USB_MASK_SD		0x02
	if(pa->pa_device == 15 && pa->pa_function == 0) {
		ohci->hc.notify = ohci_device_notify;
		//ohci->hc.port_mask = 0x02; /*ht keyboard*/ 
	}

	if(pa->pa_device == 17) {
		unsigned char *envstr;

		/*not probe webcam and sd-reader*/
		ohci->hc.port_mask = USB_MASK_WEBCAM | USB_MASK_SD;
		if((envstr = getenv("webcam"))&&!strcmp("yes", envstr))
			ohci->hc.port_mask &= ~USB_MASK_WEBCAM;
		if((envstr = getenv("sdc"))&&!strcmp("yes",envstr))
			ohci->hc.port_mask &= ~USB_MASK_SD;
	}
#endif

#ifdef LOONGSON2F_7INCH

#define PCI_DEVICE_CS5536_ID 14
#define PCI_DEVICE_UPD710102_ID 9
#define USB_MASK_PORT0 (1 << 0)
#define USB_MASK_PORT3 (1 << 3)
    if(pa->pa_device == PCI_DEVICE_UPD710102_ID){
        /*not probe webcam */
		char * envstr;
        ohci->hc.port_mask = USB_MASK_PORT0;
        if((envstr = getenv("webcam"))&&!strcmp("yes", envstr)){
            ohci->hc.port_mask &= ~USB_MASK_PORT0;
        }
    }
    if(pa->pa_device == PCI_DEVICE_CS5536_ID){
        /*no probe of sd-reader as default */
		char *envstr;
        ohci->hc.port_mask = USB_MASK_PORT0;
        if((envstr = getenv("sdc"))&&!strcmp("yes", envstr)){
            ohci->hc.port_mask &= ~USB_MASK_PORT0;
        }
		ohci->hc.port_mask |= USB_MASK_PORT3; /*wifi*/
    }
#endif

	if(pci_mem_find(pc, pa->pa_tag, OHCI_PCI_MMBA, &membase, &memsize, &cachable)){
		printf("Can not find mem space for ohci\n");
		return;
	}
#ifdef CONFIG_SM502_USB_HCD
SM502_HC:
	if((ohci->flags & 0x80) && pci_mem_find(pa->pa_pc, pa->pa_tag, 0x10, &membase2, &memsize2, &cachable))
	{
		printf("can not locate sm501 usb mem \n");
		return;
	}
#endif
	if(bus_space_map(memt, membase, memsize, 0, &ohci->sc_sh)){
		printf("Cant map mem space\n");
		return;
	}
#ifdef CONFIG_SM502_USB_HCD
	/*Can not be failed*/
	if(ohci->flags & 0x80)
		sm502_mem_init(ohci, membase2, memsize2);
#endif

	usb_lowlevel_init(ohci);
	ohci->hc.uop = &ohci_usb_op;
	/*
	 * Now build the device tree
	 */
	TAILQ_INSERT_TAIL(&host_controller, &ohci->hc, hc_list);

#if 0
	usb_scan_devices(ohci);
#else
	ohci->sc_ih = pci_intr_establish(pc, ih, IPL_BIO, hc_interrupt, ohci,
	   self->dv_xname);
#endif
	ohci->rdev = usb_alloc_new_device(ohci);

    /*do the enumeration of  the USB devices attached to the USB HUB(here root hub) 
    ports.*/
    usb_new_device(ohci->rdev);
    
}


static int lohci_match(struct device *parent, void *match, void *aux)
{
	return 1;
}

static void lohci_attach(struct device *parent, struct device *self, void *aux)
{
	struct ohci *ohci = (struct ohci*)self;
	static int ohci_dev_index = 0;
	struct confargs *cf = aux;

	printf("usb lohci init\n");
	/*end usb reset*/
#if defined(LS1ASOC)
	/* enable USB */
	*(volatile int *)0xbfd00420 &= ~0x200000;
	/*ls1a usb reset stop*/
	*(volatile int *)0xbff10204 |= 0x40000000;
#elif defined(LS1BSOC) /* LS1BSOC */
	/* enable USB */
	*(volatile int *)0xbfd00424 &= ~0x800;
	/*ls1b usb reset stop*/
	*(volatile int *)0xbfd00424 |= 0x80000000;
#elif defined(LS1CSOC)
	*(volatile int *)0xbfd00424 &= ~(1 << 31);
	delay(100);
	*(volatile int *)0xbfd00424 |= (1 << 31);
#endif

	/* Or we just return false in the match function */
	if(ohci_dev_index >= MAX_OHCI_C) {
		printf("Exceed max controller limits\n");
		return;
	}

	usb_ohci_dev[ohci_dev_index++] = ohci;

	ohci->sc_sh = (bus_space_handle_t)cf->ca_baseaddr;

	usb_lowlevel_init(ohci);
	ohci->hc.uop = &ohci_usb_op;
	/*
	 * Now build the device tree
	 */
	TAILQ_INSERT_TAIL(&host_controller, &ohci->hc, hc_list);

#if 0
	usb_scan_devices(ohci);
#else
	ohci->sc_ih = pci_intr_establish(pc, ih, IPL_BIO, hc_interrupt, ohci,
	   self->dv_xname);
#endif
	ohci->rdev = usb_alloc_new_device(ohci);

    /*do the enumeration of  the USB devices attached to the USB HUB(here root hub) 
    ports.*/
    usb_new_device(ohci->rdev);
}

struct cfattach lohci_ca = {
	.ca_devsize = sizeof(struct ohci),
	.ca_match 	= lohci_match,
	.ca_attach 	= lohci_attach,
};

struct cfdriver lohci_cd = {
	.cd_devs = NULL,
	.cd_name = "ohci",
	.cd_class = DV_DULL,
};

#define NOTUSBIRQ -0x100
/* AMD-756 (D2 rev) reports corrupt register contents in some cases.
 * The erratum (#4) description is incorrect.  AMD's workaround waits
 * till some bits (mostly reserved) are clear; ok for all revs.
 */


#define OHCI_QUIRK_AMD756 0xabcd
#define read_roothub(hc, register, mask) ({ \
	u32 temp = readl (&hc->regs->roothub.register); \
	if (hc->flags & OHCI_QUIRK_AMD756) \
		while (temp & mask) \
			temp = readl (&hc->regs->roothub.register); \
	temp; })

static u32 roothub_a (struct ohci *hc)
	{ return read_roothub (hc, a, 0xfc0fe000); }
static inline u32 roothub_b (struct ohci *hc)
	{ return readl (&hc->regs->roothub.b); }
static inline u32 roothub_status (struct ohci *hc)
	{ return readl (&hc->regs->roothub.status); }
static u32 roothub_portstatus (struct ohci *hc, int i)
	{ return read_roothub (hc, portstatus [i], 0xffe0fce0); }


static void
td_submit_job (struct usb_device * dev, unsigned long pipe, void * buffer,
	int transfer_len, struct devrequest * setup, urb_priv_t * urb, int interval);

/*===========================================================================
*
*FUNTION: urb_free_priv
*
*DESCRIPTION: This function is used to free HCD-private data associated with 
*             this URB.
*
*PARAMETERS:
*          [IN] urb: pointer to the struct urb_priv
*
*RETURN VALUE: None
*                   
*===========================================================================*/
static void urb_free_priv (urb_priv_t * urb)
{
	int		i;
	int		last;
	struct td	* td;

	last = urb->length - 1;
	if (last >= 0) {
		for (i = 0; i <= last; i++) {
			td = urb->td[i];
			if (td) {
				td->usb_dev = NULL;
				td->retbuf = NULL;
				urb->td[i] = NULL;
			}
		}
	}
}

/*-------------------------------------------------------------------------*
 * Interface functions (URB)
 *-------------------------------------------------------------------------*/

/*===========================================================================
*
*FUNTION: sochi_submit_job
*
*DESCRIPTION: This function is used to get a transfer request.
*
*PARAMETERS:
*          [IN] dev:  a pointer to the USB device the TDs belongs to.
*          [IN] pipe:  describe the property of a pipe,more details about it,
*                      please see the usb.h. 
*          [IN] buffer:an all-purpose pointer to the data that would be transfered
*                      through USB channel.
*          [IN] transfer_len: the length of data to be transfered.
*          [IN] setup: an pointer to the struct devrequest *,which is used when
*                       the transfer type is setup of control transfer.
*          [IN] interval: not used here.
*RETURN VALUE: -1 : indicates that something wrong happened during submiting job.
*               0 : function works ok.     
*===========================================================================*/
int sohci_submit_job(struct usb_device *dev, unsigned long pipe, void *buffer,
		int transfer_len, struct devrequest *setup, int interval)
{
	ohci_t *ohci;
	ed_t * ed;
	urb_priv_t *purb_priv;
	int i, size = 0;

    //QYL-2008-03-07
    u_int32_t dev_num,ed_num;

	ohci = dev->hc_private;

	/* when controller's hung, permit only roothub cleanup attempts
	 * such as powering down ports */
	if (ohci->disabled) {
		err("sohci_submit_job: EPIPE");
		return -1;
	}

	/* every endpoint has a ed, locate and fill it */
	if (!(ed = ep_add_ed (dev, pipe))) {
		err("sohci_submit_job: ENOMEM");
		return -1;
	}

	ed->usb_dev = dev;
	ed->int_interval = interval;

	/* for the private part of the URB we need the number of TDs (size) */
	switch (usb_pipetype (pipe)) {
		case PIPE_BULK: /* one TD for every 4096 Byte */
			size =  (transfer_len - 1) / 4096 + 1;
			break;
		case PIPE_CONTROL: /* 1 TD for setup, 1 for ACK and 1 for every 4096 B */
			size = (transfer_len == 0)? 2:
						(transfer_len - 1) / 4096 + 3;
			break;
		case PIPE_INTERRUPT:
			size =  (transfer_len - 1) / 4096 + 1;
			break;
		default:
			break;
	}

	if (size >= (N_URB_TD - 1)) {
		err("need %d TDs, only have %d", size, N_URB_TD);
		return -1;
	}

    for(dev_num = 0; dev_num < USB_MAX_DEVICE; dev_num++)
    {
        if (dev == &usb_dev[dev_num])
        {
            break;
        }
    }
    ed_num = usb_pipeendpoint(pipe);
    purb_priv = &ohci_urb[dev_num][ed_num];
    
	purb_priv->pipe = pipe;
	purb_priv->trans_buffer = buffer;
	purb_priv->setup_buffer = (unsigned char *)setup;

	/* fill the private part of the URB */
	purb_priv->length = size;
	purb_priv->ed = ed;
	purb_priv->actual_length = 0;
	purb_priv->trans_length = transfer_len;

#ifdef CONFIG_SM502_USB_HCD
	if(ohci->flags & 0x80){
		if(transfer_len && !usb_pipecontrol(pipe)) {
			purb_priv->dev_trans_buffer = sm502_alloc_buf(ohci);
			if(purb_priv->dev_trans_buffer == NULL){
				printf("sm502-usb: out of memory at %d\n", __LINE__);
				return -1;
			}
		} else
			purb_priv->dev_trans_buffer = NULL;
	}
#endif

	/* allocate the TDs */
	/* note that td[0] was allocated in ep_add_ed */
	for (i = 0; i < size; i++) {
		purb_priv->td[i] = td_alloc (dev);
		if (!purb_priv->td[i]) {
			purb_priv->length = i;
			urb_free_priv (purb_priv);
			err("sohci_submit_job: ENOMEM");
			return -1;
		}
	}

	if (ed->state == ED_NEW || (ed->state & ED_DEL)) {
		urb_free_priv (purb_priv);
		err("sohci_submit_job: EINVAL");
		return -1;
	}

	/* link the ed into a chain if is not already */
	if (ed->state != ED_OPER)
		ep_link (ohci, ed);

	/* fill the TDs and link it to the ed */
	td_submit_job(dev, pipe, buffer, transfer_len, setup, purb_priv, interval);

	return 0;
}

/*-------------------------------------------------------------------------*/

#ifdef DEBUG
/* tell us the current USB frame number */

static int sohci_get_current_frame_number (struct usb_device *usb_dev)
{
	ohci_t *ohci = usb_dev->hc_private;

	return m16_swap (ohci->hcca->frame_no);
}
#endif

/*===========================================================================
*
*FUNTION: usb_calc_bus_time
*
*DESCRIPTION: This function is used to caculate the usb bus time in bits.
*
*PARAMETERS:
*          [IN] speed: indicates the usb transfer speed(USB_SPEED_LOW&USB_SPEED_FULL).
*          [IN] is_input:  indicates that whether the transfer direction is input. 
*          [IN] isoc : indicates that whether it is a isochronous transfer.
*          [IN] bytecount: indicates number of bytes to be transfered.
*
*RETURN VALUE: -1 : indicates the bogus device speed
*              other case,return the bits time calculated.
*                   
*===========================================================================*/
long usb_calc_bus_time (int speed, int is_input, int isoc, int bytecount)
{
	unsigned long   tmp;

	switch (speed) {
		case USB_SPEED_LOW:     /* INTR only */
			if (is_input) {
				tmp = (67667L * (31L + 10L * BitTime (bytecount))) / 1000L;
				return (64060L + (2 * BW_HUB_LS_SETUP) + BW_HOST_DELAY + tmp);
			} else {
				tmp = (66700L * (31L + 10L * BitTime (bytecount))) / 1000L;
				return (64107L + (2 * BW_HUB_LS_SETUP) + BW_HOST_DELAY + tmp);
			}
		case USB_SPEED_FULL:    /* ISOC or INTR */
			if (isoc) {
				tmp = (8354L * (31L + 10L * BitTime (bytecount))) / 1000L;
				return (((is_input) ? 7268L : 6265L) + BW_HOST_DELAY + tmp);
			} else {
				tmp = (8354L * (31L + 10L * BitTime (bytecount))) / 1000L;
				return (9107L + BW_HOST_DELAY + tmp);
			}
		default:
			printf ("bogus device speed!\n");
			return -1;
	}
}
/*===========================================================================
*
*FUNTION: balance
*
*DESCRIPTION: This function is used to search for the least loaded schedule of
*             that period that has enough bandwidth left unreserved to balance
*             usb frame bandwidth.
*
*PARAMETERS:[IN] ohci: A pointer to the struct ohci which stores useful 
*                     information of the USB OHCI controller.
*           [IN] interval: The period between consecutive requests for data input
*                       to a Universal Serial Bus Endpoint.
*           [IN] load: The bit time the TD needs.
*
*RETURN VALUE: value indicates the branch index.
*                   
*===========================================================================*/
static int balance (ohci_t *ohci, int interval, int load)
{
	int i, branch = -1;

	/* iso periods can be huge; iso tds specify frame numbers */
	if (interval > NUM_INTS)
		interval = NUM_INTS;

	/* search for the least loaded schedule branch of that period
	 *      * that has enough bandwidth left unreserved.
	 *           */
	for (i = 0; i < interval ; i++) {
		if (branch < 0 || ohci->load [branch] > ohci->load [i]) {
			int j;

			/* usb 1.1 says 90% of one frame */
			for (j = i; j < NUM_INTS; j += interval) {
				if ((ohci->load [j] + load) > 900)
					break;
			}
			if (j < NUM_INTS)
				continue;
			branch = i;
		}
	}
	return branch;
}

/*===========================================================================
*
*FUNTION: periodic_link
*
*DESCRIPTION: This function is used to add EDs to  the periodic table .
*
*PARAMETERS:
*          [IN] ohci: a pointer to the struct ohci which stores useful information
*                     of the USB OHCI controller.
*          [IN] ed :  a pointer to the stuctr ed which contains informaiton 
*                     about ed. 
*
*RETURN VALUE: none.
*                   
*===========================================================================*/
static void periodic_link (ohci_t *ohci, ed_t *ed)
{
	unsigned    i;
		 
	if(ohci_debug)printf("link %x branch %d [%dus.], interval %d\n",
			ed, ed->int_branch, ed->int_load, ed->int_interval);

	for (i = ed->int_branch; i < NUM_INTS; i += ed->int_interval) {
		ed_t   **prev = &ohci->periodic [i];
		volatile unsigned int *prev_p = &ohci->hcca->int_table [i];
		ed_t   *here = *prev;

		/* sorting each branch by period (slow before fast)
		 * lets us share the faster parts of the tree.
		 * (plus maybe: put interrupt eds before iso)
		 */
		while (here && ed != here) {
			if (ed->int_interval > here->int_interval)
				break;
			prev = &here->ed_next;
			prev_p = &here->hwNextED;
		}
		if (ed != here) {
			ed->ed_next = here;
			if (here)
				ed->hwNextED = *prev_p;
			*prev = ed;
#ifdef CONFIG_SM502_USB_HCD
			if(ohci->flags & 0x80)
				*prev_p = sm502_vtophys(ohci, ed);
			else
#endif
			{
				*prev_p = vtophys(ed);
			}
		}
		ohci->load [i] += ed->int_load;
	}
}

/*===========================================================================
*
*FUNTION: periodic_unlink
*
*DESCRIPTION: This function is used to scan the periodic table to find and 
*             unlink this ED.
*
*PARAMETERS:
*          [IN] ohci: a pointer to the struct ohci which stores useful information
*                     of the USB OHCI controller.
*          [IN] edi:  a pointer to the stuctr ed which contains informaiton 
*                     about ED. 
*
*RETURN VALUE: none.
*                   
*===========================================================================*/
static void periodic_unlink (ohci_t *ohci, ed_t *ed)
{
	int i;

	for (i = ed->int_branch; i < NUM_INTS; i += ed->int_interval) {
		ed_t   *temp;
		ed_t   **prev = &ohci->periodic[i];
		volatile unsigned int *prev_p = &ohci->hcca->int_table [i];

		while (*prev && (temp = *prev) != ed) {
			prev_p = &temp->hwNextED;
			prev = &temp->ed_next;
		}
		if (*prev) {
			*prev_p = ed->hwNextED;
			*prev = ed->ed_next;
		}
		ohci->load [i] -= ed->int_load;
	}

	if(ohci_debug)printf("unlink %p branch %d [%dus.], interval %d\n",
		ed, ed->int_branch, ed->int_load, ed->int_interval);	
}

/*-------------------------------------------------------------------------*
 * ED handling functions
 *-------------------------------------------------------------------------*/
/*===========================================================================
*
*FUNTION: ep_link
*
*DESCRIPTION: This function is used to link an ed(endpoint descriptor) to one 
*             of the HC chains.
*
*PARAMETERS:
*          [IN] ohci: a pointer to the struct ohci which stores useful information
*                     of the USB OHCI controller.
*          [IN] edi:  a pointer to the stuctr ed which contains informaiton 
*                     about ED. 
*
*RETURN VALUE: always returns zero.
*                   
*===========================================================================*/
static int ep_link (ohci_t *ohci, ed_t *edi)
{
	int branch = -1;
	volatile ed_t *ed = edi;

	ed->state = ED_OPER;

	switch (ed->type) {
	case PIPE_CONTROL:
		ed->hwNextED = 0;
		if (ohci->ed_controltail == NULL) {
#ifdef CONFIG_SM502_USB_HCD
			if(ohci->flags & 0x80)
			{
				writel(sm502_vtophys(ohci, (void *)ed), &ohci->regs->ed_controlhead);
			}
			else
#endif
			{
				writel (vtophys(ed), &ohci->regs->ed_controlhead);
			}
		} else {
#ifdef CONFIG_SM502_USB_HCD
			if(ohci->flags & 0x80)
			{
				ohci->ed_controltail->hwNextED = m32_swap(sm502_vtophys(ohci, (void *)ed));
			}
			else
#endif
			{
				ohci->ed_controltail->hwNextED = m32_swap (vtophys(ed));
			}
		}
		ed->ed_prev = ohci->ed_controltail;
		if (!ohci->ed_controltail && !ohci->ed_rm_list[0] &&
			!ohci->ed_rm_list[1] && !ohci->sleeping) {
			ohci->hc_control |= OHCI_CTRL_CLE;
			writel (ohci->hc_control, &ohci->regs->control);
		}
		ohci->ed_controltail = edi;
		break;

	case PIPE_BULK:
		ed->hwNextED = 0;
		if (ohci->ed_bulktail == NULL) {
#ifdef CONFIG_SM502_USB_HCD
			if(ohci->flags & 0x80)
			{
				writel (sm502_vtophys(ohci, (void *)ed), &ohci->regs->ed_bulkhead);
			}
			else
#endif
			{
				writel ((long)vtophys(ed), &ohci->regs->ed_bulkhead);
			}
		} else {
#ifdef CONFIG_SM502_USB_HCD
			if(ohci->flags & 0x80)
			{
				ohci->ed_bulktail->hwNextED = sm502_vtophys(ohci, (void *)ed);
			}
			else
#endif
			{
				ohci->ed_bulktail->hwNextED = vtophys(ed);
			}
		}
		ed->ed_prev = ohci->ed_bulktail;
		if (!ohci->ed_bulktail && !ohci->ed_rm_list[0] &&
			!ohci->ed_rm_list[1] && !ohci->sleeping) {
			ohci->hc_control |= OHCI_CTRL_BLE;
			writel (ohci->hc_control, &ohci->regs->control);
		}
		ohci->ed_bulktail = edi;
		break;
	case PIPE_INTERRUPT:
		ed->hwNextED = 0;
		branch = balance(ohci, ed->int_interval, ed->int_load);
		ed->int_branch = branch;
		periodic_link (ohci, (ed_t *)ed);
		break;
	}
	return 0;
}

/*===========================================================================
*
*FUNTION: ep_unlink
*
*DESCRIPTION: This function is used to unlink an ed from one of the HC chains.
*             Just the link to the ed is unlinked.The link from the ed still 
*             points to the another opeartional ed or NULL,so the HC can eventually
*             finish the processing of the unlinked ed.
*
*PARAMETERS:
*          [IN] ohci: a pointer to the struct ohci which stores useful information
*                     of the USB OHCI controller.
*          [IN] edi:  a pointer to the stuctr ed which contains informaiton 
*                     about ED. 
*
*RETURN VALUE: always returns zero.
*                   
*===========================================================================*/
static int ep_unlink (ohci_t *ohci, ed_t *ed)
{
	ed->hwINFO |= m32_swap (OHCI_ED_SKIP);

	switch (ed->type) {
	case PIPE_CONTROL:
		if (ed->ed_prev == NULL) {
			if (!ed->hwNextED) {
				ohci->hc_control &= ~OHCI_CTRL_CLE;
				writel (ohci->hc_control, &ohci->regs->control);
			}
			writel (m32_swap (*((u32 *)&ed->hwNextED)), &ohci->regs->ed_controlhead);
		} else {
			ed->ed_prev->hwNextED = ed->hwNextED;
		}
		if (ohci->ed_controltail == ed) {
			ohci->ed_controltail = ed->ed_prev;
		} else {
#ifdef CONFIG_SM502_USB_HCD
			if(ohci->flags & 0x80) 
			{
				((ed_t *)sm502_phystov(ohci, *((u32 *)&ed->hwNextED)))->ed_prev = ed->ed_prev;
			}
			else
#endif
			{
				((ed_t *)CACHED_TO_UNCACHED(*((u32 *)&ed->hwNextED)))->ed_prev = ed->ed_prev;
			}
		}
		break;

	case PIPE_BULK:
		if (ed->ed_prev == NULL) {
			if (!ed->hwNextED) {
				ohci->hc_control &= ~OHCI_CTRL_BLE;
				writel (ohci->hc_control, &ohci->regs->control);
			}
			writel (m32_swap (*((u32 *)&ed->hwNextED)), &ohci->regs->ed_bulkhead);
		} else {
			ed->ed_prev->hwNextED = ed->hwNextED;
		}
		if (ohci->ed_bulktail == ed) {
			ohci->ed_bulktail = ed->ed_prev;
		} else {
#ifdef CONFIG_SM502_USB_HCD
			if(ohci->flags & 0x80)
			{
				((ed_t *)sm502_phystov(ohci, ed->hwNextED))->ed_prev = ed->ed_prev;
			}
			else
#endif
			{
				((ed_t *)CACHED_TO_UNCACHED(ed->hwNextED))->ed_prev = ed->ed_prev;
			}
		}
		break;
	case PIPE_INTERRUPT:
		periodic_unlink (ohci, ed);
		break;
	}
	ed->state = ED_UNLINK;
	return 0;
}

/*===========================================================================
*
*FUNTION: ep_add_ed
*
*DESCRIPTION: This function is used to add/reinit an endpoint.This should be 
*             done once at the usb_set_configuration command,but the USB stack 
*             is a little bit stateless, so we do it at every transaction.If 
*             the state of the ed is ED_NEW,then a dummy td is added and the 
*             state is changed to ED_UNLINK,in all other cases the state is 
*             left unchanged.The ed info fields are setted anyway even though
*             most of them should not be changed.
*
*PARAMETERS:
*          [IN] usb_dev: a pointer to the struct usb_device which contains useful 
*                        information of the relevant usb device.
*          [IN] pipe:  describe the property of a pipe,more details about it,
*                      please see the usb.h. 
*
*RETURN VALUE: a pointer to struct ed.
*                   
*===========================================================================*/
static ed_t * ep_add_ed (struct usb_device *usb_dev, unsigned long pipe)
{
	td_t *td;
	ed_t *ed_ret;
	volatile ed_t *ed;
	
	ohci_t *ohci = usb_dev->hc_private;
	struct ohci_device *ohci_dev = ohci->ohci_dev;

    //QYL-2008-03-07
    u_int32_t cpued_num = 0;

    cpued_num = ((usb_pipedevice(pipe)&0x3)<<3)|((usb_pipeendpoint(pipe)&0x3)<<1)|(usb_pipein(pipe));
    ed = ed_ret = &ohci_dev->cpu_ed[cpued_num];
    
	if ((ed->state & ED_DEL) || (ed->state & ED_URB_DEL)) {
		err("ep_add_ed: pending delete %x/%d\n", ed->state, 
				(usb_pipeendpoint(pipe) << 1) | (usb_pipecontrol (pipe)? 0: usb_pipeout (pipe)));
		/* pending delete request */
		return NULL;
	}

	if (ed->state == ED_NEW) {
		ed->hwINFO = m32_swap (OHCI_ED_SKIP); /* skip ed */
		/* dummy td; end of td list for ed */
		td = td_alloc (usb_dev);
#ifdef CONFIG_SM502_USB_HCD
		if(ohci->flags & 0x80){
			ed->hwTailP = sm502_vtophys(ohci, td);
		} 
		else 
#endif
		{
			ed->hwTailP = vtophys(td);
		}

		ed->hwHeadP = ed->hwTailP;
		ed->state = ED_UNLINK;
		ed->type = usb_pipetype (pipe);
		ohci_dev->ed_cnt++;
	}

	ed->hwINFO = m32_swap (usb_pipedevice (pipe)
			| usb_pipeendpoint (pipe) << 7
			| (usb_pipeisoc (pipe)? 0x8000: 0)
			| (usb_pipecontrol (pipe)? 0: (usb_pipeout (pipe)? 0x800: 0x1000))
			| usb_pipeslow (pipe) << 13
			| usb_maxpacket (usb_dev, pipe) << 16);
	ed->oINFO = ed->hwINFO;

	if(usb_pipetype(pipe) == PIPE_INTERRUPT) 
		ed->int_load = usb_calc_bus_time (
				USB_SPEED_LOW, !usb_pipeout(pipe), 0, 64) / 1000; /*FIXME*/
	return ed_ret;
}

/*-------------------------------------------------------------------------*
 * TD handling functions
 *-------------------------------------------------------------------------*/
/*===========================================================================
*
*FUNTION: td_fill
*
*DESCRIPTION: This function is used to enqueue next TD for this URB(see OHCI
*             sepc 5.2.8.2).
*
*PARAMETERS:
*          [IN] ohci: a pointer to the struct ohci which stores useful information
*                     of the USB OHCI controller.
*          [IN] info: discribe the hardware information of td(see OHCI spec 4.3.1).
*          [IN] data: an all-purpose pointer to the data that would be transfered
*                     through USB channel.
*          [IN] dev:  a pointer to the USB device this TD belongs to.
*          [IN] index: the order of this TD.
*          [IN] urb_priv: a pointer to the struct urb_priv.
*          [OUT] retbuf: an all-purpose pointer to the returned data if exist.
*
*RETURN VALUE: none.
*                   
*===========================================================================*/
static void td_fill (ohci_t *ohci, unsigned int info,
	void *data, int len,
	struct usb_device *dev, int index, urb_priv_t *urb_priv, void*retbuf)
{
	volatile td_t  *td, *td_pt;
#ifdef OHCI_FILL_TRACE
	int i;
#endif

	if (index > urb_priv->length) {
		err("index > length \n");
		return;
	}

	if (index != urb_priv->length - 1) 
		info |= (7 << 21);

	/* use this td as the next dummy */
#ifdef CONFIG_SM502_USB_HCD
	if(ohci->flags & 0x80)
	{
		/* Nothing to do for sm502 */
		//td_pt = (td_t *)CACHED_TO_UNCACHED(urb_priv->td[index]);
		td_pt = (td_t *)(urb_priv->td[index]);
	}
	else
#endif
	{
		td_pt = (td_t *)CACHED_TO_UNCACHED(urb_priv->td[index]);
	}
	td_pt->hwNextTD = 0;

	/* fill the old dummy TD */
#ifdef CONFIG_SM502_USB_HCD
	if(ohci->flags & 0x80)
	{
		unsigned long tdP;

		tdP = urb_priv->ed->hwTailP & ~0xf;
		td = urb_priv->td[index] = (td_t *)sm502_phystov(ohci, tdP);
	}
	else
#endif
	{
		td = urb_priv->td[index]= (td_t *)(CACHED_TO_UNCACHED(urb_priv->ed->hwTailP) & ~0xf);
	}


	td->ed = urb_priv->ed;
	td->next_dl_td = NULL;
	td->index = index;
	td->data = (u32)(data);
	td->transfer_len = len; //for debug purpose
	td->retbuf = retbuf;

	if (!len)
		data = 0;

	td->hwINFO = m32_swap (info);
	
	if(len==0)
		td->hwCBP = 0; //take special care
	else{
#ifdef CONFIG_SM502_USB_HCD
		if(ohci->flags & 0x80)
			td->hwCBP = sm502_vtophys(ohci, data);
		else
#endif
			td->hwCBP = vtophys(data);
	}

	if (data){
#ifdef CONFIG_SM502_USB_HCD
		if(ohci->flags & 0x80)
			td->hwBE = sm502_vtophys(ohci, data + len -1);
		else
#endif
			td->hwBE = vtophys(data+ len - 1);
	}
	else
		td->hwBE = 0;

#if 0
	printf("td_fill: td=%x\n",td);
	printf("hwINFO =%x, hwCBP=%x, hwNextTD=%x, hwBE=%x\n",
					td->hwINFO, td->hwCBP, td->hwNextTD, td->hwBE);
#endif

#ifdef CONFIG_SM502_USB_HCD
	if(ohci->flags & 0x80)
		td->hwNextTD = sm502_vtophys(ohci, (void *)td_pt);
	else 
#endif
	{
		td->hwNextTD = vtophys(m32_swap (td_pt));
	}
	td->hwPSW [0] = ((u32)data & 0x0FFF) | 0xE000;
	/* append to queue */
	wbflush();
	td->ed->hwTailP = td->hwNextTD;
}

/*===========================================================================
*
*FUNTION: td_submit_job
*
*DESCRIPTION: This function is used to prepare all TDs of a transfer.
*
*PARAMETERS:
*          [IN] dev:  a pointer to the USB device the TDs belongs to.
*          [IN] pipe:  describe the property of a pipe,more details about it,
*                      please see the usb.h. 
*          [IN] buffer:an all-purpose pointer to the data that would be transfered
*                      through USB channel.
*          [IN] transfer_len: the length of data to be transfered.
*          [IN] setup: an pointer to the struct devrequest *,which is used when
*                       the transfer type is setup of control transfer.
*          [IN] urb: a pointer to the struct urb_priv.
*          [IN] interval: not used here.
*
*RETURN VALUE: none.
*                   
*===========================================================================*/
static void td_submit_job (struct usb_device *dev, unsigned long pipe, void
				*buffer, int transfer_len, struct devrequest *setup, urb_priv_t
				*urb, int interval)
{
	ohci_t *ohci = dev->hc_private;
	int data_len = transfer_len;
	void *data;
	int cnt = 0;
	u32 info = 0; int periodic = 0;
	unsigned int toggle = 0;

	/* OHCI handles the DATA-toggles itself, we just use the USB-toggle bits for reseting */
#ifdef CONFIG_SM502_USB_HCD
	if(ohci->flags & 0x80)
	{
		/*Currently nothing to do*/			
	}
	else
#endif
	{
		pci_sync_cache(ohci->sc_pc, (vm_offset_t)buffer, transfer_len, SYNC_W);
	}

	if(usb_gettoggle(dev, usb_pipeendpoint(pipe), usb_pipeout(pipe))) {
		toggle = TD_T_TOGGLE;
	} else {
		toggle = TD_T_DATA0;
		usb_settoggle(dev, usb_pipeendpoint(pipe), usb_pipeout(pipe), 1);
	}
	urb->td_cnt = 0;
	if (data_len) 
	{
#ifdef CONFIG_SM502_USB_HCD
		if(ohci->flags & 0x80)
		{
			if(usb_pipeout(pipe))
				memcpy(urb->dev_trans_buffer, buffer, transfer_len);
			data =  urb->dev_trans_buffer;
		}
		else
#endif
		{
			data = buffer; //XXX
		}
	}
   	else
	{
		data = 0;
	}

	switch (usb_pipetype (pipe)) {
	case PIPE_INTERRUPT:
		info = usb_pipeout (urb->pipe)?
			TD_CC | TD_DP_OUT | toggle: TD_CC | TD_R | TD_DP_IN | toggle;
#ifdef CONFIG_SM502_USB_HCD
		if(ohci->flags & 0x80)
		{
			td_fill (ohci, info, data, data_len, dev, cnt++, urb, buffer);
		}
		else
#endif
		{
			td_fill (ohci, info, data, data_len, dev, cnt++, urb, NULL);
		}
		periodic = 1;
		break;
	case PIPE_BULK:
		info = usb_pipeout (pipe)?
			TD_CC | TD_DP_OUT : TD_CC | TD_DP_IN ;
		while(data_len > 4096) {
			td_fill (ohci, info | (cnt? TD_T_TOGGLE:toggle), data, 4096, dev, cnt, urb, NULL);
			data += 4096; data_len -= 4096; cnt++;
		}
		info = usb_pipeout (pipe)?
			TD_CC | TD_DP_OUT : TD_CC | TD_R | TD_DP_IN ;
		td_fill (ohci, info | (cnt? TD_T_TOGGLE:toggle), data, data_len, dev, cnt, urb, NULL);
		cnt++;

		if (!ohci->sleeping)
			writel (OHCI_BLF, &ohci->regs->cmdstatus); /* start bulk list */
		break;

	case PIPE_CONTROL:
		info = TD_CC | TD_DP_SETUP | TD_T_DATA0;
		memcpy(ohci->setup, setup, 8);
		td_fill (ohci, info, (void *)ohci->setup, 8, dev, cnt++, urb, NULL);
		if (data_len > 0) {
			info = usb_pipeout (pipe)?
				TD_CC | TD_R | TD_DP_OUT | TD_T_DATA1 : TD_CC | TD_R | TD_DP_IN | TD_T_DATA1;
			/* NOTE:  mishandles transfers >8K, some >4K */
			if(usb_pipeout(pipe)){
				memcpy(ohci->control_buf, data, data_len);
				td_fill (ohci, info, ohci->control_buf, data_len, dev, cnt++, urb, NULL);
			} else {
				td_fill (ohci, info, ohci->control_buf, data_len, dev, cnt++, urb, buffer);
			}
		}
		info = (usb_pipeout (pipe) || data_len ==0)?
			TD_CC | TD_DP_IN | TD_T_DATA1: TD_CC | TD_DP_OUT | TD_T_DATA1;
		td_fill (ohci, info, data, 0, dev, cnt++, urb, NULL);
		if (!ohci->sleeping)
			writel (OHCI_CLF, &ohci->regs->cmdstatus); /* start Control list */
		break;
	}

	if (periodic) {
		ohci->hc_control |= OHCI_CTRL_PLE|OHCI_CTRL_IE;
		writel(ohci->hc_control, &ohci->regs->control);
	}

	if (urb->length != cnt)
		dbg("TD LENGTH %d != CNT %d", urb->length, cnt);
}

/*-------------------------------------------------------------------------*
 * Done List handling functions
 *-------------------------------------------------------------------------*/
/*===========================================================================
*
*FUNTION: dl_transfer_length
*
*DESCRIPTION: This function is used to calculate the transfer length and update
*             the urb.
*
*PARAMETERS:
*          [IN] td:  a pointer to the stuct td which contains informaiton 
*                     about TD. 
*
*RETURN VALUE: none.
*                   
*===========================================================================*/
static void dl_transfer_length(td_t * td)
{
	u32 tdINFO, tdBE, tdCBP;
	urb_priv_t *lurb_priv = NULL;
	int length = 0;

    //QYL-2008-03-07
    u_int32_t dev_num,ed_num;
    struct usb_device *p_dev = NULL;
    ed_t *p_ed = NULL;

	tdINFO = m32_swap (td->hwINFO);
	tdBE   = m32_swap (td->hwBE);
#ifdef CONFIG_SM502_USB_HCD
	if(ohci->flags & 0x80)
	{
		tdCBP  = (u32)sm502_phystov(ohci, m32_swap (td->hwCBP));
	}
	else
#endif
	{
		tdCBP  = PHYS_TO_UNCACHED(m32_swap (td->hwCBP));
	}

    //QYL-2008-03-07
    if (td != NULL)
    {
        p_dev = td->usb_dev;
        for(dev_num = 0; dev_num < USB_MAX_DEVICE; dev_num++)
        {
            if (p_dev == &usb_dev[dev_num])
            {
                break;
            }
        }
        p_ed = td->ed;
        ed_num = (p_ed->hwINFO & 0x780) >> 7;
        lurb_priv = &ohci_urb[dev_num][ed_num];
    }

	if (!(usb_pipetype (lurb_priv->pipe) == PIPE_CONTROL &&
	    ((td->index == 0) || (td->index == lurb_priv->length - 1)))) {
		if (tdBE != 0) {
			if (td->hwCBP == 0){
#ifdef CONFIG_SM502_USB_HCD
				if(ohci->flags & 0x80)
				{
					/*FIXME */
					length = (unsigned long)sm502_phystov(ohci, tdBE) - td->data + 1;
				}
				else
#endif
				{
					length = PHYS_TO_UNCACHED(tdBE) - CACHED_TO_UNCACHED(td->data) + 1;
				}
				lurb_priv->actual_length += length;
				if(usb_pipecontrol(lurb_priv->pipe)&& usb_pipein(lurb_priv->pipe)){
#ifdef CONFIG_SM502_USB_HCD
					if(ohci->flags & 0x80)
					{
						memcpy((void *)td->retbuf, (void*)td->data, length);
					}
					else
#endif
					{
						memcpy((void *)td->retbuf, (void*)td->data, length);
					}
				}
#ifdef CONFIG_SM502_USB_HCD
			   	else 
				{
					if((ohci->flags & 0x80)
						&& !usb_pipecontrol(lurb_priv->pipe) 
						&& usb_pipein(lurb_priv->pipe)) 
					{
						memcpy((void *)td->retbuf, (void *)td->data, length);
					}
				}
#endif

			} else {
#ifdef CONFIG_SM502_USB_HCD
				if(ohci->flags & 0x80)
				{
					length= tdCBP - td->data; 
				}
				else
#endif
				{
					length = tdCBP - CACHED_TO_UNCACHED(td->data);
				}
				lurb_priv->actual_length += length;
				if(usb_pipein(lurb_priv->pipe) &&usb_pipecontrol(lurb_priv->pipe)){
					memcpy(td->retbuf, (void*)td->data, length);
				}
#ifdef CONFIG_SM502_USB_HCD
				else 
				{
					if((ohci->flags & 0x80)
						&& !usb_pipecontrol(lurb_priv->pipe) 
						&& usb_pipein(lurb_priv->pipe)) 
					{
						memcpy((void *)td->retbuf, (void *)td->data, length);
					}
				}
#endif
			}
		}
	}

#if 0
	if(usb_pipein(lurb_priv->pipe)){
		int i;
		printf("transfer_length=%d/%d\n", length, td->transfer_len);
		for(i=0; i<length; i++)
			printf("%02x ", ((unsigned char*)td->data)[i]);	
		printf("\n");
			
	}
#endif

}

/*===========================================================================
*
*FUNTION: dl_reverse_done_list
*
*DESCRIPTION: This function is used to reverse the reversed done-list,for replies
*             to the request have to be on a FIFO basis.
*
*PARAMETERS:
*          [IN] ohci: a pointer to the struct ohci which stores useful information
*                     of the USB OHCI controller.
*
*RETURN VALUE: a pointer to the struct td,which is the head of the td done-list 
*              chain.
*                   
*===========================================================================*/
static td_t * dl_reverse_done_list (ohci_t *ohci)
{
	u32 td_list_hc;
	td_t *td_rev = NULL;
	td_t *td_list = NULL;
	urb_priv_t *lurb_priv = NULL;

    //QYL-2008-03-07
    u_int32_t dev_num,ed_num;
    struct usb_device *p_dev = NULL;
    ed_t *p_ed = NULL;

	td_list_hc = ohci->hcca->done_head & 0xfffffff0;
	ohci->hcca->done_head = 0;

	while (td_list_hc) {

#ifdef CONFIG_SM502_USB_HCD
		if(ohci->flags & 0x80)
		{
			td_list = (td_t *)sm502_phystov(ohci, td_list_hc & 0x1fffffff);
		}
		else
#endif
		{

			td_list = (td_t *)PHYS_TO_UNCACHED(td_list_hc & 0x1fffffff);
		}
		td_list->hwINFO |= TD_DEL;

		if (TD_CC_GET (m32_swap (td_list->hwINFO))) {
            p_dev = td_list->usb_dev;
            for(dev_num = 0; dev_num < USB_MAX_DEVICE; dev_num++)
            {
                if (p_dev == &usb_dev[dev_num])
                {
                    break;
                }
            }
            p_ed = td_list->ed;
            ed_num = (p_ed->hwINFO & 0x780) >> 7;
            lurb_priv = &ohci_urb[dev_num][ed_num];
            
#if 0
			//FIXME Error Handling
			printf(" USB-error/status: %x : %p\n",
					TD_CC_GET (m32_swap (td_list->hwINFO)), td_list);
#endif
			if (td_list->ed->hwHeadP & m32_swap (0x1)) { //ED halted
				if (lurb_priv && ((td_list->index + 1) < lurb_priv->length)) {
					td_list->ed->hwHeadP =
						(lurb_priv->td[lurb_priv->length - 1]->hwNextTD & m32_swap (0xfffffff0)) |
									(td_list->ed->hwHeadP & m32_swap (0x2));
					lurb_priv->td_cnt += lurb_priv->length - td_list->index - 1;
				} else
					td_list->ed->hwHeadP &= m32_swap (0xfffffff2);
			}
		}

		td_list->next_dl_td = td_rev;
		td_rev = td_list;
		td_list_hc = m32_swap (td_list->hwNextTD) & 0xfffffff0;
	}
	return td_list;
}

/*===========================================================================
*
*FUNTION: dl_done_list
*
*DESCRIPTION: This function is used to deal with the td done list,and if the
*             pipe type is PIPE_INTERRUPT,here it needs to submit a new job
*             for PIPE_INTERRUPT,in fact,this feature is used for USB keypad.
*
*PARAMETERS:
*          [IN] ohci: a pointer to the struct ohci which stores useful information
*                     of the USB OHCI controller.
*          [IN] td_list: a pointer to the struct td,which is the head of the td done-list 
*                        chain.
*
*RETURN VALUE: zero :it is ok.
*              not zero:something wrong.
*                   
*===========================================================================*/
static int dl_done_list (ohci_t *ohci, td_t *td_list)
{
	td_t *td_list_next = NULL;
	ed_t *ed;
	int cc = 0;
	int stat = 0;
	struct usb_device *dev = NULL;
	/* urb_t *urb; */
	urb_priv_t *lurb_priv = NULL;
	u32 tdINFO, edHeadP, edTailP;
    u_int32_t dev_num,ed_num;
    struct usb_device *p_dev = NULL;
    ed_t *p_ed = NULL;

    //QYL-2008-03-07
    u_int32_t bPipeBulk = FALSE;
    urb_priv_t *pInt_urb_priv = NULL;
    struct usb_device *pInt_dev = NULL;
    ed_t *pInt_ed = NULL;
    
	while (td_list) {
        
		td_list_next = td_list->next_dl_td;
		//printf("td_list:%x\n",td_list);

        //QYL-2008-03-07
        //lurb_priv = &urb_priv;
        p_dev = td_list->usb_dev;
        for(dev_num = 0; dev_num < USB_MAX_DEVICE; dev_num++)
        {
            if (p_dev == &usb_dev[dev_num])
            {
                break;
            }
        }
        p_ed = td_list->ed;
        ed_num = (p_ed->hwINFO & 0x780) >> 7;
        lurb_priv = &ohci_urb[dev_num][ed_num];

        //QYL-2008-03-07
        if (p_ed->type == PIPE_INTERRUPT)
        {
            pInt_ed = p_ed;
            pInt_urb_priv = lurb_priv;
            pInt_dev = p_ed->usb_dev;
        }
        if (p_ed->type == PIPE_BULK)
        {
            bPipeBulk = TRUE;
        }
        
		tdINFO = m32_swap (td_list->hwINFO);

		ed = td_list->ed;
		dev = ed->usb_dev;

		dl_transfer_length(td_list);

		/* error code of transfer */
		cc = TD_CC_GET (tdINFO);
		if (cc != 0) {
#if 0
			//FIXME Error Handling	
			err("ConditionCode %x/%x", cc, td_list);
#endif
			stat = cc_to_error[cc];
		}

		if (ed->state != ED_NEW) {
			edHeadP = m32_swap (ed->hwHeadP) & 0xfffffff0;
			edTailP = m32_swap (ed->hwTailP);

			/* unlink eds if they are not busy */
			if ((edHeadP == edTailP) && (ed->state == ED_OPER)){
				ep_unlink (ohci, ed);
			}
		}
		dev->status = stat; // FIXME;

		td_list = td_list_next;
	}

    if (NULL != pInt_urb_priv) {
        if (pInt_dev && pInt_dev->irq_handle) {
			pInt_dev->irq_status = 0;
			pInt_dev->irq_act_len = pInt_urb_priv->actual_length;
			pInt_dev->irq_handle(pInt_dev);
		}

		pInt_urb_priv->actual_length = 0;
		pInt_dev->irq_act_len = 0;
		pInt_ed->hwINFO = pInt_ed->oINFO;
		ep_link(ohci, pInt_ed);
		td_submit_job(pInt_ed->usb_dev, pInt_urb_priv->pipe, pInt_urb_priv->trans_buffer, pInt_urb_priv->trans_length, pInt_urb_priv->setup_buffer, pInt_urb_priv, pInt_ed->int_interval);               
		pInt_urb_priv = NULL;
	}
	return stat;
}

//Felix-2008-05-05
static int dl_td_done_list (ohci_t *ohci, td_t *td_list)
{
	td_t *td_list_next = NULL;
	ed_t *ed;
	int cc = 0;
	int stat = 0;
	struct usb_device *dev = NULL;
	/* urb_t *urb; */
	urb_priv_t *lurb_priv = NULL;//&urb_priv;
	u32 tdINFO, edHeadP, edTailP;

    //QYL-2008-03-07
    u_int32_t dev_num,ed_num;
    struct usb_device *p_dev = NULL;
    ed_t *p_ed = NULL;

    //QYL-2008-03-07
    u_int32_t bPipeBulk = FALSE;
    urb_priv_t *pInt_urb_priv = NULL;
    struct usb_device *pInt_dev = NULL;
    ed_t *pInt_ed = NULL;
    
	while (td_list) {
        
		td_list_next = td_list->next_dl_td;

        p_dev = td_list->usb_dev;
        for(dev_num = 0; dev_num < USB_MAX_DEVICE; dev_num++)
        {
            if (p_dev == &usb_dev[dev_num])
            {
                break;
            }
        }
		if(dev_num == USB_MAX_DEVICE) {
			printf("Error not found device (%08x),%08x\n", td_list, p_dev);
			break;
		}
        p_ed = td_list->ed;
        ed_num = (p_ed->hwINFO & 0x780) >> 7;
        lurb_priv = &ohci_urb[dev_num][ed_num];

        //QYL-2008-03-07
        if (p_ed->type == PIPE_INTERRUPT)
        {
            pInt_ed = p_ed;
            pInt_urb_priv = lurb_priv;
            pInt_dev = p_ed->usb_dev;
        }
        if (p_ed->type == PIPE_BULK)
        {
            bPipeBulk = TRUE;
        }
        
		tdINFO = m32_swap (td_list->hwINFO);

		ed = td_list->ed;
		dev = ed->usb_dev;

		dl_transfer_length(td_list);

		/* error code of transfer */
		cc = TD_CC_GET (tdINFO);
		if (cc != 0) {
			err("ConditionCode %x/%x", cc, td_list);
			stat = cc_to_error[cc];
		}

		if (ed->state != ED_NEW) {
			edHeadP = m32_swap (ed->hwHeadP) & 0xfffffff0;
			edTailP = m32_swap (ed->hwTailP);

			/* unlink eds if they are not busy */
			if ((edHeadP == edTailP) && (ed->state == ED_OPER)){
				ep_unlink (ohci, ed);
			}
		}
		dev->status = stat; // FIXME, the transfer complete?

		td_list = td_list_next;
	}
    
    if ((NULL != pInt_urb_priv)) { /* FIXME */
		int i;
		for (i=0; i< MAX_INTS; i++) {
			if(ohci->g_pInt_dev[i] == NULL) {
				ohci->g_pInt_dev[i] = pInt_dev;
				ohci->g_pInt_ed[i] = pInt_ed;
				ohci->g_pInt_urb_priv[i] = pInt_urb_priv;
				break;
			}
		}
    }
	return stat;
}


/*-------------------------------------------------------------------------*
 * Virtual Root Hub
 *-------------------------------------------------------------------------*/
/* Device descriptor */
static u8 root_hub_dev_des[] =
{
	0x12,	    /*	__u8  bLength; */
	0x01,	    /*	__u8  bDescriptorType; Device */
	0x10,	    /*	__u16 bcdUSB; v1.1 */
	0x01,
	0x09,	    /*	__u8  bDeviceClass; HUB_CLASSCODE */
	0x00,	    /*	__u8  bDeviceSubClass; */
	0x00,	    /*	__u8  bDeviceProtocol; */
	0x08,	    /*	__u8  bMaxPacketSize0; 8 Bytes */
	0x00,	    /*	__u16 idVendor; */
	0x00,
	0x00,	    /*	__u16 idProduct; */
	0x00,
	0x00,	    /*	__u16 bcdDevice; */
	0x00,
	0x00,	    /*	__u8  iManufacturer; */
	0x01,	    /*	__u8  iProduct; */
	0x00,	    /*	__u8  iSerialNumber; */
	0x01	    /*	__u8  bNumConfigurations; */
};

/* Configuration descriptor */
static u8 root_hub_config_des[] =
{
	0x09,	    /*	__u8  bLength; */
	0x02,	    /*	__u8  bDescriptorType; Configuration */
	0x19,	    /*	__u16 wTotalLength; */
	0x00,
	0x01,	    /*	__u8  bNumInterfaces; */
	0x01,	    /*	__u8  bConfigurationValue; */
	0x00,	    /*	__u8  iConfiguration; */
	0x40,	    /*	__u8  bmAttributes;
		 Bit 7: Bus-powered, 6: Self-powered, 5 Remote-wakwup, 4..0: resvd */
	0x00,	    /*	__u8  MaxPower; */

	/* interface */
	0x09,	    /*	__u8  if_bLength; */
	0x04,	    /*	__u8  if_bDescriptorType; Interface */
	0x00,	    /*	__u8  if_bInterfaceNumber; */
	0x00,	    /*	__u8  if_bAlternateSetting; */
	0x01,	    /*	__u8  if_bNumEndpoints; */
	0x09,	    /*	__u8  if_bInterfaceClass; HUB_CLASSCODE */
	0x00,	    /*	__u8  if_bInterfaceSubClass; */
	0x00,	    /*	__u8  if_bInterfaceProtocol; */
	0x00,	    /*	__u8  if_iInterface; */

	/* endpoint */
	0x07,	    /*	__u8  ep_bLength; */
	0x05,	    /*	__u8  ep_bDescriptorType; Endpoint */
	0x81,	    /*	__u8  ep_bEndpointAddress; IN Endpoint 1 */
	0x03,	    /*	__u8  ep_bmAttributes; Interrupt */
	0x02,	    /*	__u16 ep_wMaxPacketSize; ((MAX_ROOT_PORTS + 1) / 8 */
	0x00,
	0xff	    /*	__u8  ep_bInterval; 255 ms */
};

static unsigned char root_hub_str_index0[] =
{
	0x04,			/*  __u8  bLength; */
	0x03,			/*  __u8  bDescriptorType; String-descriptor */
	0x09,			/*  __u8  lang ID */
	0x04,			/*  __u8  lang ID */
};

static unsigned char root_hub_str_index1[] =
{
	28,			/*  __u8  bLength; */
	0x03,			/*  __u8  bDescriptorType; String-descriptor */
	'O',			/*  __u8  Unicode */
	0,				/*  __u8  Unicode */
	'H',			/*  __u8  Unicode */
	0,				/*  __u8  Unicode */
	'C',			/*  __u8  Unicode */
	0,				/*  __u8  Unicode */
	'I',			/*  __u8  Unicode */
	0,				/*  __u8  Unicode */
	' ',			/*  __u8  Unicode */
	0,				/*  __u8  Unicode */
	'R',			/*  __u8  Unicode */
	0,				/*  __u8  Unicode */
	'o',			/*  __u8  Unicode */
	0,				/*  __u8  Unicode */
	'o',			/*  __u8  Unicode */
	0,				/*  __u8  Unicode */
	't',			/*  __u8  Unicode */
	0,				/*  __u8  Unicode */
	' ',			/*  __u8  Unicode */
	0,				/*  __u8  Unicode */
	'H',			/*  __u8  Unicode */
	0,				/*  __u8  Unicode */
	'u',			/*  __u8  Unicode */
	0,				/*  __u8  Unicode */
	'b',			/*  __u8  Unicode */
	0,				/*  __u8  Unicode */
};

/* Hub class-specific descriptor is constructed dynamically */
/*-------------------------------------------------------------------------*/

#define OK(x)			len = (x); break
#ifdef DEBUG
#define WR_RH_STAT(x)		{info("WR:status %#8x", (x));writel((x), &gohci->regs->roothub.status);}
#define WR_RH_PORTSTAT(x)	{info("WR:portstatus[%d] %#8x", wIndex-1, (x));writel((x), &gohci->regs->roothub.portstatus[wIndex-1]);}
#else
#define WR_RH_STAT(x)		writel((x), &gohci->regs->roothub.status)
#define WR_RH_PORTSTAT(x)	writel((x), &gohci->regs->roothub.portstatus[wIndex-1])
#endif
#define RD_RH_STAT		roothub_status(gohci)
#define RD_RH_PORTSTAT		roothub_portstatus(gohci,wIndex-1)

/*===========================================================================
*
*FUNTION: rh_check_port_status
*
*DESCRIPTION: This function is used to answer the request to the virtual root
*             hub,checking whether a device disconnected just now.
*
*PARAMETERS:
*          [IN] controller: a pointer to the struct ohci which stores useful 
*                           information of the USB OHCI controller.
*
*RETURN VALUE: zero :nothing happened.
*              > zero:some device disconnected.
*                   
*===========================================================================*/
int rh_check_port_status(ohci_t *controller)
{
	u32 temp, ndp, i;
	int res;

	res = -1;
	temp = roothub_a (controller);
	ndp = (temp & RH_A_NDP);
	for (i = 0; i < ndp; i++) {
		temp = roothub_portstatus (controller, i);
		/* check for a device disconnect */
		if (((temp & (RH_PS_PESC | RH_PS_CSC)) ==
			(RH_PS_PESC | RH_PS_CSC)) &&
			((temp & RH_PS_CCS) == 0)) {
			res = i;
			break;
		}
	}
	return res;
}

/*===========================================================================
*
*FUNTION: ohci_submit_rh_msg
*
*DESCRIPTION: This function is used to submit a message to virtual root hub
*             in fact,for this root hub is virtual,so it will return data(
*             if exists) immediatelly.
*
*PARAMETERS:
*          [IN] dev:  a pointer to the USB device( here root hub,a special USB
*                     device) the message belongs to.
*          [IN] pipe:  describe the property of a pipe,more details about it,
*                      please see the usb.h. 
*          [IN] buffer:an all-purpose pointer to the data that would be returned 
*                      from virtual root hub.
*          [IN] transfer_len: the length of data to be transfered.
*          [IN] cmd: an pointer to the struct devrequest *,which is used when
*                       the transfer type is setup of control transfer.
*
*RETURN VALUE: zero :indicates that root hub has not implemented PIPE_INTERRUPT.
*              not zero:indicates the status of this transfer.
*                   
*===========================================================================*/
static int ohci_submit_rh_msg(struct usb_device *dev, unsigned long pipe,
		void *buffer, int transfer_len, struct devrequest *cmd)
{
	void * data = buffer;
	int leni = transfer_len;
	int len = 0;
	int stat = 0;
	u32 datab[4];
	u8 *data_buf = (u8 *)datab;
	u16 bmRType_bReq;
	u16 wValue;
	u16 wIndex;
	u16 wLength;
	struct ohci *gohci = dev->hc_private;

	wait_ms(1);

	if ((pipe & PIPE_INTERRUPT) == PIPE_INTERRUPT) {
		info("Root-Hub submit IRQ: NOT implemented");
		return 0;
	}

	bmRType_bReq  = cmd->requesttype | (cmd->request << 8);
	wValue	      = m16_swap (cmd->value);
	wIndex	      = m16_swap (cmd->index);
	wLength	      = m16_swap (cmd->length);

	/*
	info("Root-Hub: adr: %2x cmd(%1x): %08x %04x %04x %04x",
		dev->devnum, 8, bmRType_bReq, wValue, wIndex, wLength);
	*/

	switch (bmRType_bReq) {
	/* Request Destination:
	   without flags: Device,
	   RH_INTERFACE: interface,
	   RH_ENDPOINT: endpoint,
	   RH_CLASS means HUB here,
	   RH_OTHER | RH_CLASS	almost ever means HUB_PORT here
	*/

	case RH_GET_STATUS:
			*(u16 *) data_buf = m16_swap (1); OK (2);
	case RH_GET_STATUS | RH_INTERFACE:
			*(u16 *) data_buf = m16_swap (0); OK (2);
	case RH_GET_STATUS | RH_ENDPOINT:
			*(u16 *) data_buf = m16_swap (0); OK (2);
	case RH_GET_STATUS | RH_CLASS:
			*(u32 *) data_buf = m32_swap (
				RD_RH_STAT & ~(RH_HS_CRWE | RH_HS_DRWE));
			OK (4);
	case RH_GET_STATUS | RH_OTHER | RH_CLASS:
			*(u32 *) data_buf = m32_swap (RD_RH_PORTSTAT); OK (4);

	case RH_CLEAR_FEATURE | RH_ENDPOINT:
		switch (wValue) {
			case (RH_ENDPOINT_STALL): OK (0);
		}
		break;

	case RH_CLEAR_FEATURE | RH_CLASS:
		switch (wValue) {
			case RH_C_HUB_LOCAL_POWER:
				OK(0);
			case (RH_C_HUB_OVER_CURRENT):
					WR_RH_STAT(RH_HS_OCIC); OK (0);
		}
		break;

	case RH_CLEAR_FEATURE | RH_OTHER | RH_CLASS:
		switch (wValue) {
			case (RH_PORT_ENABLE):
					WR_RH_PORTSTAT (RH_PS_CCS ); OK (0);
			case (RH_PORT_SUSPEND):
					WR_RH_PORTSTAT (RH_PS_POCI); OK (0);
			case (RH_PORT_POWER):
					WR_RH_PORTSTAT (RH_PS_LSDA); OK (0);
			case (RH_C_PORT_CONNECTION):
					WR_RH_PORTSTAT (RH_PS_CSC ); OK (0);
			case (RH_C_PORT_ENABLE):
					WR_RH_PORTSTAT (RH_PS_PESC); OK (0);
			case (RH_C_PORT_SUSPEND):
					WR_RH_PORTSTAT (RH_PS_PSSC); OK (0);
			case (RH_C_PORT_OVER_CURRENT):
					WR_RH_PORTSTAT (RH_PS_OCIC); OK (0);
			case (RH_C_PORT_RESET):
					WR_RH_PORTSTAT (RH_PS_PRSC); OK (0);
		}
		break;

	case RH_SET_FEATURE | RH_OTHER | RH_CLASS:
		switch (wValue) {
			case (RH_PORT_SUSPEND):
					WR_RH_PORTSTAT (RH_PS_PSS ); OK (0);
			case (RH_PORT_RESET): /* BUG IN HUP CODE *********/
					if (RD_RH_PORTSTAT & RH_PS_CCS)
					    WR_RH_PORTSTAT (RH_PS_PRS);
					OK (0);
			case (RH_PORT_POWER):
					WR_RH_PORTSTAT (RH_PS_PPS ); OK (0);
			case (RH_PORT_ENABLE): /* BUG IN HUP CODE *********/
					if (RD_RH_PORTSTAT & RH_PS_CCS)
					    WR_RH_PORTSTAT (RH_PS_PES );
					OK (0);
		}
		break;

	case RH_SET_ADDRESS: gohci->rh.devnum = wValue; OK(0);

	case RH_GET_DESCRIPTOR:
		switch ((wValue & 0xff00) >> 8) {
			case (0x01): /* device descriptor */
				len = min_t(unsigned int,
					  leni,
					  min_t(unsigned int,
					      sizeof (root_hub_dev_des),
					      wLength));
				data_buf = root_hub_dev_des; OK(len);
			case (0x02): /* configuration descriptor */
				len = min_t(unsigned int,
					  leni,
					  min_t(unsigned int,
					      sizeof (root_hub_config_des),
					      wLength));
				data_buf = root_hub_config_des; OK(len);
			case (0x03): /* string descriptors */
				if(wValue==0x0300) {
					len = min_t(unsigned int,
						  leni,
						  min_t(unsigned int,
						      sizeof (root_hub_str_index0),
						      wLength));
					data_buf = root_hub_str_index0;
					OK(len);
				}
				if(wValue==0x0301) {
					len = min_t(unsigned int,
						  leni,
						  min_t(unsigned int,
						      sizeof (root_hub_str_index1),
						      wLength));
					data_buf = root_hub_str_index1;
					OK(len);
			}
			default:
				stat = USB_ST_STALLED;
		}
		break;

	case RH_GET_DESCRIPTOR | RH_CLASS:
	    {
		    u32 temp = roothub_a (gohci);

		    data_buf [0] = 9;		/* min length; */
		    data_buf [1] = 0x29;
		    data_buf [2] = temp & RH_A_NDP;
		    data_buf [3] = 0;
		    if (temp & RH_A_PSM)	/* per-port power switching? */
			data_buf [3] |= 0x1;
		    if (temp & RH_A_NOCP)	/* no overcurrent reporting? */
			data_buf [3] |= 0x10;
		    else if (temp & RH_A_OCPM)	/* per-port overcurrent reporting? */
			data_buf [3] |= 0x8;

		    /* corresponds to data_buf[4-7] */
		    datab [1] = 0;
		    data_buf [5] = (temp & RH_A_POTPGT) >> 24;
		    temp = roothub_b (gohci);
		    data_buf [7] = temp & RH_B_DR;
		    if (data_buf [2] < 7) {
			data_buf [8] = 0xff;
		    } else {
			data_buf [0] += 2;
			data_buf [8] = (temp & RH_B_DR) >> 8;
			data_buf [10] = data_buf [9] = 0xff;
		    }

		    len = min_t(unsigned int, leni,
			      min_t(unsigned int, data_buf [0], wLength));
		    OK (len);
		}

	case RH_GET_CONFIGURATION:	*(u8 *) data_buf = 0x01; OK (1);

	case RH_SET_CONFIGURATION:	WR_RH_STAT (0x10000); OK (0);

	default:
		dbg ("unsupported root hub command");
		stat = USB_ST_STALLED;
	}

#ifdef	DEBUG
	//ohci_dump_roothub (gohci, 1);
#else
	wait_ms(1);
#endif

	len = min_t(int, len, leni);
	if (data != data_buf)
	    memcpy (data, data_buf, len);
	dev->act_len = len;
	dev->status = stat;

#ifdef DEBUG
	if (transfer_len)
		urb_priv.actual_length = transfer_len;
#else
	wait_ms(1);
#endif

	return stat;
}

/*===========================================================================
*
*FUNTION: reset_controller
*
*DESCRIPTION: This function is used to reset the USB OHCI controller.
*
*PARAMETERS:
*          [IN] hc_data: an all-purpose pointer to be cast to struct ohci*,
*                        points to the buffer which stores information of ohci.
*
*RETURN VALUE: none
*                   
*===========================================================================*/
void reset_controller(void *hc_data)
{
	ohci_t * ohci = hc_data;	
	memset(&ohci->rh, 0, sizeof(ohci->rh));
}

/*===========================================================================
*
*FUNTION: submit_common_msg
*
*DESCRIPTION: This function is the common code for handling submit messages,which
*             are used for all but root hub accesses.
*
*PARAMETERS:
*          [IN] dev:  a pointer to the USB device the messages belongs to.
*          [IN] pipe:  describe the property of a pipe,more details about it,
*                      please see the usb.h. 
*          [IN] buffer:an all-purpose pointer to the data that would be transfered
*                      through USB channel.
*          [IN] transfer_len: the length of data to be transfered.
*          [IN] setup: an pointer to the struct devrequest *,which is used when
*                       the transfer type is setup of control transfer.
*          [IN] interval: not used here.
*
*RETURN VALUE: 0 : indicates that device has been pulled or function return ok.
*              -1: indicates that pipesize is zero or sohci_submit_job failed.
*
*note:      unlike ohci_submit_rh_msg(),this function would access the real
*           OHCI controller hardware,so it needs to wait to check whether hardware 
*           has fininshed transfering TDs in limited timeouts by calling hc_check_
*           ohci_controller().
*                   
*===========================================================================*/
int submit_common_msg(struct usb_device *dev, unsigned long pipe, void *buffer,
		int transfer_len, struct devrequest *setup, int interval)
{
	int stat = 0;
	int maxsize = usb_maxpacket(dev, pipe);
	int timeout, i;
	struct ohci *gohci = dev->hc_private;

    //QYL-2008-03-07
    u_int32_t dev_num,ed_num;
    urb_priv_t *lurb_priv = NULL;

	/* device pulled? Shortcut the action. */
	if (devgone == dev) {
		dev->status = USB_ST_CRC_ERR;
		return 0;
	}

	if (!maxsize) {
		err("submit_common_message: pipesize for pipe %lx is zero",
			pipe);
		return -1;
	}

	if(pipe != PIPE_INTERRUPT) {
		gohci->transfer_lock ++;

		gohci->hc_control &= ~OHCI_CTRL_PLE;
		writel(gohci->hc_control, &gohci->regs->control);
	}

	if (sohci_submit_job(dev, pipe, buffer, transfer_len, setup, interval) < 0) {
		err("sohci_submit_job failed");
		return -1;
	}

	/* allow more time for a BULK device to react - some are slow */
#define BULK_TO	 500	/* timeout in milliseconds */
	if (usb_pipetype (pipe) == PIPE_BULK)
		timeout = BULK_TO;
	else
		timeout = 2000;

	timeout *= 100;

	/* wait for it to complete */
#if 0
	for (;;) {
		/* check whether the controller is done */
		stat = hc_interrupt(gohci);
		if(stat == NOTUSBIRQ)
			continue;
		if (stat < 0) {
			stat = USB_ST_CRC_ERR << 8;
			break;
		}

		if (stat >= 0 && stat != 0xff ) {
			/* 0xff is returned for an SF-interrupt */
			if(stat != 303 || stat != TD_CC_STALL ) {
				printf("OHCI: unexpected stat %x\n", stat);
				break;
			}
		}
	}
#else
    
	while(--timeout > 0) {
		if(!(dev->status & USB_ST_NOT_PROC)){
			break;
		}
		delay(200);
		hc_check_ohci_controller(gohci);
	}

	if(pipe != PIPE_INTERRUPT) {
		gohci->transfer_lock --;
		gohci->hc_control |= OHCI_CTRL_PLE;
		writel(gohci->hc_control, &gohci->regs->control);
	}

	for(i=0; i< MAX_INTS; i++) {
		struct usb_device *pInt_dev = NULL;
		urb_priv_t * pInt_urb_priv = NULL;
		ed_t * pInt_ed = NULL;

		pInt_dev = gohci->g_pInt_dev[i];
		pInt_urb_priv = gohci->g_pInt_urb_priv[i];
		pInt_ed = gohci->g_pInt_ed[i];

		if(pInt_dev == NULL || pInt_urb_priv == NULL|| pInt_ed == NULL)
			continue;

		if (pInt_dev->irq_handle) {
			pInt_dev->irq_status = 0;
			pInt_dev->irq_act_len = pInt_urb_priv->actual_length;
			pInt_dev->irq_handle(pInt_dev);
		}

		pInt_urb_priv->actual_length = 0;
		pInt_dev->irq_act_len = 0;
		pInt_ed->hwINFO = pInt_ed->oINFO;
		ep_link(gohci, pInt_ed);
		td_submit_job(pInt_ed->usb_dev, pInt_urb_priv->pipe, pInt_urb_priv->trans_buffer, pInt_urb_priv->trans_length, (struct devrequest *)pInt_urb_priv->setup_buffer, pInt_urb_priv, pInt_ed->int_interval);               

		gohci->g_pInt_dev[i] = NULL;
		gohci->g_pInt_urb_priv[i] = NULL;
		gohci->g_pInt_ed[i] = NULL;

	}


    if (timeout == 0) 
		printf("USB timeout dev:0x%x\n",(u_int32_t)dev);

#endif

	/* we got an Root Hub Status Change interrupt */
	if (got_rhsc) {
#if 0
		ohci_dump_roothub (gohci, 1);
#endif
		got_rhsc = 0;
		/* abuse timeout */
		timeout = rh_check_port_status(gohci);
		if (timeout >= 0) {
			/*
			 * XXX
			 * This is potentially dangerous because it assumes
			 * that only one device is ever plugged in!
			 */
			printf("device disconnected\n");
			devgone = dev;
		}
	}

    //QYL-2008-03-07
    for(dev_num = 0; dev_num < USB_MAX_DEVICE; dev_num++)
    {
        if (dev == &usb_dev[dev_num])
        {
            break;
        }
    }
    ed_num = usb_pipeendpoint(pipe);
    lurb_priv = &ohci_urb[dev_num][ed_num];

	if (usb_pipetype(pipe) != PIPE_INTERRUPT) { /*FIXME, might not done bulk*/
		dev->status = stat;
		dev->act_len = transfer_len;
		/* free TDs in urb_priv */
		urb_free_priv(lurb_priv);
		memset(lurb_priv, 0, sizeof(*lurb_priv));
	}

	return 0;
}

/*===========================================================================
*
*FUNTION: ohci_submit_bulk_msg
*
*DESCRIPTION: This function is used to submit a bulk message to OHCI controller.
*             This is a submit routine called from usb.c.
*
*PARAMETERS:
*          [IN] dev:  a pointer to the USB device the message belongs to.
*          [IN] pipe:  describe the property of a pipe,more details about it,
*                      please see the usb.h. 
*          [IN] buffer:an all-purpose pointer to the data that would be returned 
*                      through usb channel.
*          [IN] transfer_len: the length of data to be transfered.
*
*RETURN VALUE: same as the value returned from submit_common_msg().
*                   
*===========================================================================*/
static int ohci_submit_bulk_msg(struct usb_device *dev, unsigned long pipe, void *buffer,
		int transfer_len)
{
	int s;

	if(ohci_debug)printf("submit_bulk_msg %x/%d\n", buffer, transfer_len);
	if((u32)buffer & 0x0f)
		printf("bulk buffer %x/%d not aligned\n", buffer, transfer_len);	
	s = submit_common_msg(dev, pipe, buffer, transfer_len, NULL, 0);
	if(ohci_debug)printf("submit_bulk_msg END\n"); 

	return s;
}

/*===========================================================================
*
*FUNTION: ohci_submit_control_msg
*
*DESCRIPTION: This function is used to submit a control message to OHCI controller
*             or root hub. This is a submit routine called from usb.c.
*
*PARAMETERS:
*          [IN] dev:  a pointer to the USB device the message belongs to.
*          [IN] pipe:  describe the property of a pipe,more details about it,
*                      please see the usb.h. 
*          [IN] buffer:an all-purpose pointer to the data that would be returned 
*                      through usb channel.
*          [IN] transfer_len: the length of data to be transfered.
*          [IN] setup: an pointer to the struct devrequest *,which is used when
*                       the transfer type is setup of control transfer.
*
*RETURN VALUE: same as the value returned from submit_common_msg()or ohci_
*              _submit_rh_msg().
*                   
*===========================================================================*/
static int ohci_submit_control_msg(struct usb_device *dev, unsigned long pipe,
				void *buffer, int transfer_len, struct devrequest *setup)
{
	int maxsize = usb_maxpacket(dev, pipe);
	struct ohci *gohci = dev->hc_private;

	if(ohci_debug)printf("submit_control_msg(%d) %x/%d\n", (pipe>>8)&0x7f, buffer, transfer_len);
#if 1
	wait_ms(1);
#endif
	if (!maxsize) {
		err("submit_control_message: pipesize for pipe %lx is zero",
			pipe);
		return -1;
	}
	if (((pipe >> 8) & 0x7f) == gohci->rh.devnum) {
		gohci->rh.dev = dev;
		/* root hub - redirect */
		return ohci_submit_rh_msg(dev, pipe, buffer, transfer_len,
			setup);
	}

	return submit_common_msg(dev, pipe, buffer, transfer_len, setup, 0);
}


/*===========================================================================
*
*FUNTION: ohci_submit_int_msg
*
*DESCRIPTION: This function is used to submit a interrupt message to OHCI 
*             controller. This is a submit routine called from usb.c.
*
*PARAMETERS:
*          [IN] dev:  a pointer to the USB device the message belongs to.
*          [IN] pipe:  describe the property of a pipe,more details about it,
*                      please see the usb.h. 
*          [IN] buffer:an all-purpose pointer to the data that would be returned 
*                      through usb channel.
*          [IN] transfer_len: the length of data to be transfered.
*          [IN] interval: not used here.
*
*RETURN VALUE: same as the value returned from submit_common_msg().
*                   
*===========================================================================*/
static int ohci_submit_int_msg(struct usb_device *dev, unsigned long pipe, void *buffer,
		int transfer_len, int interval)
{
	int s;

	if(ohci_debug)printf("submit int(%08x)\n", dev);
	s = submit_common_msg(dev, pipe, buffer, transfer_len, NULL, interval);
	return s;
}

/*-------------------------------------------------------------------------*
 * HC functions
 *-------------------------------------------------------------------------*/

/*===========================================================================
*
*FUNTION: hc_reset
*
*DESCRIPTION: This function is used to reset the HC and BUS.
*
*PARAMETERS:
*          [IN] ohci: a pointer to the struct ohci which stores useful 
*                     information of the USB OHCI controller.
*
*RETURN VALUE: -1: indicates that USB HC TakeOver failed or USB HC reset time out.
*              0 : normal return. 
*
*===========================================================================*/
static int hc_reset (ohci_t *ohci)
{
	int timeout = 30;
	int smm_timeout = 50; /* 0,5 sec */

	if (readl (&ohci->regs->control) & OHCI_CTRL_IR) { /* SMM owns the HC */
		writel (OHCI_OCR, &ohci->regs->cmdstatus); /* request ownership */
		info("USB HC TakeOver from SMM");
		while (readl (&ohci->regs->control) & OHCI_CTRL_IR) {
			wait_ms (10);
			if (--smm_timeout == 0) {
				err("USB HC TakeOver failed!");
				return -1;
			}
		}
	}

	/* Disable HC interrupts */
	writel (OHCI_INTR_MIE, &ohci->regs->intrdisable);

	dbg("USB HC reset_hc usb-%s: ctrl = 0x%X ;",
		ohci->slot_name,
		readl (&ohci->regs->control));

	/* Reset USB (needed by some controllers) */
	writel (0, &ohci->regs->control);

	/* HC Reset requires max 10 us delay */
	writel (OHCI_HCR,  &ohci->regs->cmdstatus);
	while ((readl (&ohci->regs->cmdstatus) & OHCI_HCR) != 0) {
		if (--timeout == 0) {
			err("USB HC reset timed out!");
			return -1;
		}
		udelay (1);
	}
	return 0;
}

/*===========================================================================
*
*FUNTION: hc_start
*
*DESCRIPTION: This function is used to start an OHCI controller,set the BUS
*             operational ,enable interrupts and connect the virtual root hub.
*
*PARAMETERS:
*          [IN] ohci: a pointer to the struct ohci which stores useful 
*                     information of the USB OHCI controller.
*
*RETURN VALUE: always returns 0.     
*===========================================================================*/
static int hc_start (ohci_t * ohci)
{
	u32 mask;
	unsigned int fminterval;

	ohci->disabled = 1;

	/* Tell the controller where the control and bulk lists are
	 * The lists are empty now. */

	writel (0, &ohci->regs->ed_controlhead);
	writel (0, &ohci->regs->ed_bulkhead);
	writel (0, &ohci->regs->ed_periodcurrent);

#ifdef CONFIG_SM502_USB_HCD
	if(ohci->flags & 0x80)
		writel(sm502_vtophys(ohci, ohci->hcca), &ohci->regs->hcca);
	else
#endif
	{
		writel ((u32)(vtophys(ohci->hcca)), &ohci->regs->hcca); /* a reset clears this */
	}

	fminterval = 0x2edf;
	writel ((fminterval * 8) / 10, &ohci->regs->periodicstart);    
    fminterval |= ((((fminterval - 210) * 6) / 7) << 16);
	writel (fminterval, &ohci->regs->fminterval);
	writel (0x628, &ohci->regs->lsthresh);

	/* start controller operations */
	ohci->hc_control = OHCI_CONTROL_INIT | OHCI_USB_OPER;
	ohci->disabled = 0;
	writel (ohci->hc_control, &ohci->regs->control);

	/* disable all interrupts */
	mask = (OHCI_INTR_SO | OHCI_INTR_WDH | OHCI_INTR_SF | OHCI_INTR_RD |
			OHCI_INTR_UE | OHCI_INTR_FNO | OHCI_INTR_RHSC |
			OHCI_INTR_OC | OHCI_INTR_MIE);
	writel (mask, &ohci->regs->intrdisable);
#if 0 //zenglu
	/* clear all interrupts */
	mask &= ~OHCI_INTR_MIE;
	writel (mask, &ohci->regs->intrstatus);
	/* Choose the interrupts we care about now  - but w/o MIE */
	mask = OHCI_INTR_RHSC | OHCI_INTR_UE | OHCI_INTR_WDH | OHCI_INTR_SO | OHCI_INTR_MIE;
	writel (mask, &ohci->regs->intrenable);
#endif

#ifdef	OHCI_USE_NPS
	/* required for AMD-756 and some Mac platforms */
	writel ((roothub_a (ohci) | RH_A_NPS) & ~RH_A_PSM,
		&ohci->regs->roothub.a);
	writel (RH_HS_LPSC, &ohci->regs->roothub.status);
#endif	/* OHCI_USE_NPS */

#define mdelay(n) do {unsigned long msec=(n); while (msec--) udelay(1000);} while(0)
	/* POTPGT delay is bits 24-31, in 2 ms units. */
	mdelay((roothub_a (ohci) >> 23) & 0x1fe);

//	mdelay(1000);

	/* connect the virtual root hub */
	ohci->rh.devnum = 0;

	return 0;
}

/*===========================================================================
*
*FUNTION: hc_interrupt
*
*DESCRIPTION: This function is used to check whether OHCI controller has finished
*             transfering the PIPE_INTERRUPT TDs.It is a INTR-hanlder-like routine,
*             and will be called by system frequently.
*PARAMETERS:
*          [IN] hc_data: an all-purpose pointer to be cast to struct ohci*,
*                        points to the buffer which stores information of ohci.
*
*RETURN VALUE: 0 :   indicates that no TDs has been transfered.
*              stat: indicates that PIPE_INTERRUPT TDs has been transfered and be
*                    dealed with.*
*                   
*===========================================================================*/
static int hc_interrupt (void *hc_data)
{
	ohci_t *ohci = hc_data;
	struct ohci_regs *regs = ohci->regs;
	
	urb_priv_t *lurb_priv = NULL;//&urb_priv;
	td_t *td = NULL;
	int ints;

	int stat = NOTUSBIRQ;

    //QYL-2008-03-07
    u_int32_t dev_num,ed_num;
    struct usb_device *p_dev = NULL;
    ed_t *p_ed = NULL;

	if(ohci->transfer_lock)
		return stat;

	if ((ohci->hcca->done_head != 0) && 
			!(m32_swap (ohci->hcca->done_head) & 0x01)) {
		ints =	OHCI_INTR_WDH;
#ifdef CONFIG_SM502_USB_HCD
		if(ohci->flags & 0x80)
		{
			td = (td_t *)sm502_phystov(ohci, ohci->hcca->done_head);
		}
		else
#endif
		{
			td = (td_t *)CACHED_TO_UNCACHED(ohci->hcca->done_head);
		}
	} else {
		ints = readl (&regs->intrstatus);
		if (ints == ~0ul) 
			return 0;
		if (ints == 0)
			return 0;
	}

	if (ints & OHCI_INTR_RHSC) {
		writel(OHCI_INTR_RD | OHCI_INTR_RHSC, &regs->intrstatus);
		got_rhsc = 1;
	}

	if (got_rhsc) {
		int timeout;
#if 0
		ohci_dump_roothub (gohci, 1);
#endif
		got_rhsc = 0;
		/* abuse timeout */
		delay(250);
		timeout = rh_check_port_status(ohci);
		if (timeout >= 0) {
			/*
			 * XXX
			 * This is potentially dangerous because it assumes
			 * that only one device is ever plugged in!
			 */
			//printf("**hc_interrupt 2008-03-07** device disconnected\n");//QYL-2008-03-07
		}
	}


	if (ints & OHCI_INTR_UE) {
		ohci->disabled++;
		printf("Unrecoverable Error, controller usb-%s disabled\n",
			ohci->slot_name);
		/* e.g. due to PCI Master/Target Abort */
#ifdef	DEBUG
		ohci_dump (ohci, 1);
#endif
		/* FIXME: be optimistic, hope that bug won't repeat often. */
		/* Make some non-interrupt context restart the controller. */
		/* Count and limit the retries though; either hardware or */
		/* software errors can go forever... */
		hc_reset (ohci);
		ohci->disabled--;
		return -1;
	}

	if (ints & OHCI_INTR_WDH) {

		writel (OHCI_INTR_WDH, &regs->intrdisable);

		if (td == NULL){ 
#ifdef CONFIG_SM502_USB_HCD
			if(ohci->flags & 0x80)
			{
				td = (td_t *)sm502_phystov(ohci, ohci->hcca->done_head & ~0x1f);
			}
			else
#endif
			{
				td = (td_t *)CACHED_TO_UNCACHED(ohci->hcca->done_head & ~0x1f);
			}
		}

		if (td == NULL) {
			printf("Bad td in donehead\n");
		} else if ((td != NULL) && (td->ed != NULL) ) {      
            p_dev = td->usb_dev;
            for(dev_num = 0; dev_num < USB_MAX_DEVICE; dev_num++)
            {
                if (p_dev == &usb_dev[dev_num])
                {
                    break;
                }
            }
            p_ed = td->ed;
            ed_num = (p_ed->hwINFO & 0x780) >> 7;
            lurb_priv = &ohci_urb[dev_num][ed_num];

            if (td->index != lurb_priv->length -1){
				stat = dl_done_list (ohci, dl_reverse_done_list (ohci));
				printf("td index=%x/%x\n", td->index, lurb_priv->length);
			} else {
				stat = dl_done_list (ohci, dl_reverse_done_list (ohci));
		    }
        }

		writel (OHCI_INTR_WDH, &regs->intrenable);

	}

	if (ints & OHCI_INTR_SO) {
		printf("USB Schedule overrun\n");
		writel (OHCI_INTR_SO, &regs->intrenable);
		stat = -1;
	}

	/* FIXME:  this assumes SOF (1/ms) interrupts don't get lost... */
	if (ints & OHCI_INTR_SF) {
		unsigned int frame = m16_swap (ohci->hcca->frame_no) & 1;
		writel (OHCI_INTR_SF, &regs->intrdisable);
		if (ohci->ed_rm_list[frame] != NULL)
			writel (OHCI_INTR_SF, &regs->intrenable);
		stat = 0xff;
	}

	writel (ints, &regs->intrstatus);
	(void)readl(&regs->control);

	return stat;
}

/*===========================================================================
*
*FUNTION: hc_release_ohci
*
*DESCRIPTION: This function is used to De-allocate all resources.
*
*PARAMETERS:
*          [IN] ohci: a pointer to the struct ohci which stores useful 
*                     information of the USB OHCI controller.
*
*RETURN VALUE: none.     
*===========================================================================*/
static void hc_release_ohci (ohci_t *ohci)
{
	dbg ("USB HC release ohci usb-%s", ohci->slot_name);

	if (!ohci->disabled)
		hc_reset (ohci);
}

/*===========================================================================
*
*FUNTION: usb_lowlevel_init
*
*DESCRIPTION: This is the low level initialization routine,called from usb.c.
*
*PARAMETERS:
*          [IN] ghci: a pointer to the struct ohci which stores useful 
*                     information of the USB OHCI controller.
*
*RETURN VALUE: -1: indicates that HCCA not aligned or EDs not aligned or TDs
*                  not aligned or OHCI initialization error.  
*              0 : indicates that initialization finished successfully.
*===========================================================================*/
int usb_lowlevel_init(ohci_t *gohci)
{

	struct ohci_hcca *hcca = NULL;
	struct ohci_device *ohci_dev = NULL;
	td_t *gtd = NULL;
	unsigned char *tmpbuf;

	dbg("in usb_lowlevel_init\n");

#ifdef CONFIG_SM502_USB_HCD
	if(gohci->flags & 0x80){
		hcca = sm502_mem_alloc(gohci, sizeof(struct ohci_hcca));
		memset(hcca, 0, sizeof(*hcca));
	} 
	else 
#endif
	{
		hcca = malloc(sizeof(*gohci->hcca), M_DEVBUF, M_NOWAIT);
		memset(hcca, 0, sizeof(*hcca));
		pci_sync_cache(gohci->sc_pc, (vm_offset_t)hcca, sizeof(*hcca), SYNC_W);
	}

	/* align the storage */
	if ((u32)&hcca[0] & 0xff) {
		err("HCCA not aligned!! %x\n", hcca);
		return -1;
	}

#ifdef CONFIG_SM502_USB_HCD
	if(gohci->flags & 0x80) 
	{
		ohci_dev = sm502_mem_alloc(gohci, sizeof *ohci_dev);
		memset(ohci_dev, 0, sizeof(struct ohci_device));
	} 
	else
#endif
	{
		ohci_dev = malloc(sizeof *ohci_dev, M_DEVBUF, M_NOWAIT);
		memset(ohci_dev, 0, sizeof(struct ohci_device));
	}
	if ((u32)&ohci_dev->ed[0] & 31) {
		err("EDs not aligned!!");
		return -1;
	}

	if((gohci->flags & 0x80)) 
	{
		ohci_dev->cpu_ed = ohci_dev->ed;
	} 
	else 
	{
		pci_sync_cache(gohci->sc_pc, (vm_offset_t)ohci_dev->ed, sizeof(ohci_dev->ed),SYNC_W);
		ohci_dev->cpu_ed = (ed_t *)CACHED_TO_UNCACHED(&ohci_dev->ed);
	}

#ifdef CONFIG_SM502_USB_HCD
	if(gohci->flags & 0x80)
	{
		gtd = sm502_mem_alloc(gohci, sizeof(td_t) * (NUM_TD+1));
		memset(gtd, 0, sizeof(td_t) * (NUM_TD + 1));
	} else 
#endif
	{
		gtd = malloc(sizeof(td_t) * (NUM_TD+1), M_DEVBUF, M_NOWAIT);
		memset(gtd, 0, sizeof(td_t) * (NUM_TD + 1));
		pci_sync_cache(gohci->sc_pc, (vm_offset_t)gtd, sizeof(td_t)*(NUM_TD+1), SYNC_W);
	}

	if ((u32)gtd & 0x0f) {
		err("TDs not aligned!!");
		return -1;
	}


#ifdef CONFIG_SM502_USB_HCD
	if(gohci->flags & 0x80)
	{
		gohci->hcca = hcca;
		gohci->gtd = gtd;
		gohci->ohci_dev = ohci_dev;
	} 
	else
#endif
	{
		gohci->hcca = (struct ohci_hcca*)CACHED_TO_UNCACHED(hcca);
		gohci->gtd = (td_t *)CACHED_TO_UNCACHED(gtd);
		gohci->ohci_dev = ohci_dev;
	}

#ifdef CONFIG_SM502_USB_HCD
	if(gohci->flags & 0x80)
	{
		tmpbuf = sm502_mem_alloc(gohci, 512);
		if(tmpbuf == NULL){
			printf("sm502-usb: out of memory at %d\n", __LINE__);
			goto errout;
		}
		if((u32)tmpbuf & 0x1f)
			printf("Malloc return not cache line aligned\n");
		memset(tmpbuf, 0, 512);
		gohci->control_buf = (unsigned char*)tmpbuf;
	} 
	else
#endif
	{
		tmpbuf = malloc(512, M_DEVBUF, M_NOWAIT);
		if(tmpbuf == NULL){
			printf("No mem for control buffer\n");
			goto errout;
		}	
		if((u32)tmpbuf & 0x1f)
			printf("Malloc return not cache line aligned\n");
		memset(tmpbuf, 0, 512);
		pci_sync_cache(gohci->sc_pc, (vm_offset_t)tmpbuf,  512, SYNC_W);
		gohci->control_buf = (unsigned char*)CACHED_TO_UNCACHED(tmpbuf);
	}

#ifdef CONFIG_SM502_USB_HCD
	if(gohci->flags & 0x80) {
		tmpbuf = sm502_mem_alloc(gohci, 64);
		if(tmpbuf == NULL){
			printf("sm502-usb: out of memory at %d\n", __LINE__);
			goto errout;
		}	
		gohci->setup = (unsigned char *)tmpbuf;
	} else
#endif
	{
		tmpbuf = malloc(64, M_DEVBUF, M_NOWAIT);
		if(tmpbuf == NULL){
			printf("No mem for setup buffer\n");
			goto errout;
		}	
		if((u32)tmpbuf & 0x1f)
			printf("Malloc return not cache line aligned\n");
		memset(tmpbuf, 0, 64);
		pci_sync_cache(tmpbuf, (vm_offset_t)tmpbuf, 64, SYNC_W);
		gohci->setup = (unsigned char *)CACHED_TO_UNCACHED(tmpbuf);
	}

	gohci->disabled = 1;
	gohci->sleeping = 0;
	gohci->irq = -1;
	gohci->regs = (struct ohci_regs *)(gohci->sc_sh | 0xA0000000);

#ifdef CONFIG_SM502_USB_HCD
	if(gohci->flags & 0x80){
		sm502_bufs_init(gohci);
	}
#endif

	dbg("OHCI: regs base %x\n", gohci->regs);

	//gohci->flags = 0;
	gohci->slot_name = "Godson";

	dbg("OHCI revision: 0x%08x\n"
	       "  RH: a: 0x%08x b: 0x%08x\n",
	       readl(&gohci->regs->revision),
	       readl(&gohci->regs->roothub.a), readl(&gohci->regs->roothub.b));

	if (hc_reset (gohci) < 0)
		goto errout;

	/* FIXME this is a second HC reset; why?? */
	writel (gohci->hc_control = OHCI_USB_RESET, &gohci->regs->control);
	wait_ms (10);

	if (hc_start (gohci) < 0)
		goto errout;

#ifdef	DEBUG
	//ohci_dump (gohci, 1);
#else
	wait_ms(1);
#endif
	printf("OHCI %x initialized ok\n", gohci);
	return 0;

errout:
	err("OHCI initialization error\n");
	hc_release_ohci (gohci);
	/* Initialization failed */
	return -1;
}

/*===========================================================================
*
*FUNTION: usb_lowlevel_stop
*
*DESCRIPTION: This function is used to reset OHCI controller.
*
*PARAMETERS:
*          [IN] ohci: a pointer to the struct ohci which stores useful 
*                     information of the USB OHCI controller.
*
*RETURN VALUE: always return 0
*
*===========================================================================*/
int usb_lowlevel_stop(void *hc_data)
{
	/* this gets called really early - before the controller has */
	/* even been initialized! */
	struct ohci *ohci = hc_data;
	/* TODO release any interrupts, etc. */
	/* call hc_release_ohci() here ? */
	hc_reset (ohci);
	/* may not want to do this */
	/* Disable clock */
	return 0;
}

void usb_ohci_stop_one(ohci_t *ohci)
{
	int cmd;

	writel (0, &ohci->regs->control);
	writel (OHCI_HCR,  &ohci->regs->cmdstatus);
	(void) readl (&ohci->regs->cmdstatus);

	cmd = pci_conf_read(ohci->sc_pc, ohci->pa.pa_tag, 0x04);
	pci_conf_write(ohci->sc_pc, ohci->pa.pa_tag, 0x04, (cmd & ~0x7));
}

void usb_ohci_stop()
{
	int i;

	for(i=0; i< MAX_OHCI_C && usb_ohci_dev[i]; i++) 
		usb_ohci_stop_one(usb_ohci_dev[i]);
}

#ifdef DEBUG
#include  <pmon.h>
extern unsigned long strtoul(const char *nptr,char **endptr,int base);

int cmd_setdebug(int ac, char *av[])
{
	int val;

	if(ac == 1){
		if(ohci_debug)
			printf("ohci debug on\n");
		else
			printf("ohci debug off\n");
		return 0;
	}

	val = strtoul(av[1], 0, 0);	
	if(val)
		ohci_debug = 1;
	else 
		ohci_debug = 0;

	return 0;
}


static const struct Optdesc ohci_opts [] = {
	{"ohci controller", "debug" },
	{"ohci", ""},
	{}
};

static const struct Cmd Cmds[] = {
	{"ohci"},
	{"dohci", "",
		0,
		"swith debug for ohci driver", cmd_setdebug, 1, 2, 0},
	{0, 0},
};

static void init_cmd __P((void)) __attribute__((constructor));

static void
init_cmd()
{
	cmdlist_expand(Cmds, 1);	
}
#endif


//QYL-2008-03-07
/*===========================================================================
*
*FUNTION: hc_check_ohci_controller
*
*DESCRIPTION: This function is used to check whether OHCI controller has finished
*             transfering the TDs except PIPE_INTERRUPT TDs.
*PARAMETERS:
*          [IN] hc_data: an all-purpose pointer to be cast to struct ohci*,
*                        points to the buffer which stores information of ohci.
*
*RETURN VALUE: 0 :   indicates that no TDs has been transfered.
*              stat: indicates that PIPE_INTERRUPT TDs has been transfered and be
*                    dealed with.
*                   
*===========================================================================*/
static int hc_check_ohci_controller (void *hc_data)
{
	ohci_t *ohci = hc_data;
	struct ohci_regs *regs = ohci->regs;	
	urb_priv_t *lurb_priv = NULL;    
	td_t *td = NULL;
	int ints;
	int stat = NOTUSBIRQ;

    u_int32_t dev_num, ed_num;
    struct usb_device *p_dev = NULL;
    ed_t *p_ed = NULL;

	if ((ohci->hcca->done_head != 0) &&
			!(m32_swap (ohci->hcca->done_head) & 0x01)) {
		ints =	OHCI_INTR_WDH;
#ifdef CONFIG_SM502_USB_HCD
		if(ohci->flags & 0x80)
		{
			td = (td_t *)sm502_phystov(ohci, ohci->hcca->done_head);
		}
		else
#endif
		{
			td = (td_t *)CACHED_TO_UNCACHED(ohci->hcca->done_head);
		}
	} else {
		ints = readl (&regs->intrstatus);
		if (ints == ~0ul) 
			return 0;
		if (ints == 0)
			return 0;
	}

	if (ints & OHCI_INTR_RHSC) {
		writel(OHCI_INTR_RD | OHCI_INTR_RHSC, &regs->intrstatus);
		got_rhsc = 1;
	}

	if (got_rhsc) {
		int timeout;
#if 0
		ohci_dump_roothub (gohci, 1);
#endif
		got_rhsc = 0;
		/* abuse timeout */
		delay(250);
		timeout = rh_check_port_status(ohci);
		if (timeout >= 0) {
			/*
			 * XXX
			 * This is potentially dangerous because it assumes
			 * that only one device is ever plugged in!
			 */
			printf("**hc_check_ohci_controller** device disconnected\n");
		}
	}


	if (ints & OHCI_INTR_UE) {
		ohci->disabled++;
		printf("**hc_check_ohci_controller** Unrecoverable Error, controller usb-%s disabled\n",
			ohci->slot_name);
		/* e.g. due to PCI Master/Target Abort */
#ifdef	DEBUG
		ohci_dump (ohci, 1);
#endif
		/* FIXME: be optimistic, hope that bug won't repeat often. */
		/* Make some non-interrupt context restart the controller. */
		/* Count and limit the retries though; either hardware or */
		/* software errors can go forever... */
		hc_reset (ohci);
		ohci->disabled--;
		return -1;
	}

	if (ints & OHCI_INTR_WDH) {
        
		if (td == NULL) { 
#ifdef CONFIG_SM502_USB_HCD
			if(ohci->flags & 0x80)
			{
				td = (td_t *)sm502_phystov(ohci, ohci->hcca->done_head & ~0x1f);
			}
			else
#endif
			{
				td = (td_t *)CACHED_TO_UNCACHED(ohci->hcca->done_head & ~0x1f);
			}
		}

		if ((td != NULL)&&(td->ed != NULL)) { //&&(td->ed->type != PIPE_INTERRUPT)))
            writel (OHCI_INTR_WDH, &regs->intrdisable);
            p_dev = td->usb_dev;

            for(dev_num = 0; dev_num < USB_MAX_DEVICE; dev_num++)
            {
                if (p_dev == &usb_dev[dev_num])
                {
                    break;
                }
            }
            p_ed = td->ed;
            ed_num = (p_ed->hwINFO & 0x780) >> 7;//See OHCI1.1 spec Page 17 Endpoint Descriptor Field Definitions
            lurb_priv= &ohci_urb[dev_num][ed_num];

		    if (td->index != lurb_priv->length -1) {           
			    stat = dl_td_done_list (ohci, dl_reverse_done_list (ohci));
			    printf("**hc_check** td index=%x/%x, p_dev %x\n", td->index, lurb_priv->length, p_dev);
		    } else {
			    stat = dl_td_done_list (ohci, dl_reverse_done_list (ohci));
		    }
            writel (OHCI_INTR_WDH, &regs->intrenable);
        }
	}

	if (ints & OHCI_INTR_SO) {
		printf("USB Schedule overrun\n");
		writel (OHCI_INTR_SO, &regs->intrenable);
		stat = -1;
	}

	/* FIXME:  this assumes SOF (1/ms) interrupts don't get lost... */
	if (ints & OHCI_INTR_SF) {
		unsigned int frame = m16_swap (ohci->hcca->frame_no) & 1;
		writel (OHCI_INTR_SF, &regs->intrdisable);
		if (ohci->ed_rm_list[frame] != NULL)
			writel (OHCI_INTR_SF, &regs->intrenable);
		stat = 0xff;
	}
	writel (ints, &regs->intrstatus);
	(void)readl(&regs->control);

	return stat;
}

static void ohci_device_notify(struct usb_device *dev, int port)
{
	if(port == 2 && dev->match ){
		strcpy(((struct device *)dev->match)->dv_xname, "usbg0");
	}
}
