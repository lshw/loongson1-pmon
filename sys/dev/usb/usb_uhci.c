/*
 * Part of this code has been derived from linux:
 * Universal Host Controller Interface driver for USB (take II).
 *
 * (c) 1999-2001 Georg Acher, acher@in.tum.de (executive slave) (base guitar)
 *               Deti Fliegl, deti@fliegl.de (executive slave) (lead voice)
 *               Thomas Sailer, sailer@ife.ee.ethz.ch (chief consultant) (cheer leader)
 *               Roman Weissgaerber, weissg@vienna.at (virt root hub) (studio porter)
 * (c) 2000      Yggdrasil Computing, Inc. (port of new PCI interface support
 *               from usb-ohci.c by Adam Richter, adam@yggdrasil.com).
 * (C) 2000      David Brownell, david-b@pacbell.net (usb-ohci.c)
 *
 * HW-initalization based on material of
 *
 * (C) Copyright 1999 Linus Torvalds
 * (C) Copyright 1999 Johannes Erdfelt
 * (C) Copyright 1999 Randy Dunlap
 * (C) Copyright 1999 Gregory P. Smith
 *
 *
 * Adapted for U-Boot:
 * (C) Copyright 2001 Denis Peter, MPL AG Switzerland
 *
 * Adapted for Godson pmon
 * (C) Copyright 2006 yanhua@ict.ac.cn
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
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 *
 *
 */

/**********************************************************************
 * How it works:
 * -------------
 * The framelist / Transfer descriptor / Queue Heads are similar like
 * in the linux usb_uhci.c.
 *
 * During initialization, the following skeleton is allocated in init_skel:
 *
 *         framespecific           |           common chain
 *
 * framelist[]
 * [  0 ]-----> TD ---------\
 * [  1 ]-----> TD ----------> TD ------> QH -------> QH -------> QH ---> NULL
 *   ...        TD ---------/
 * [1023]-----> TD --------/
 *
 *              ^^             ^^         ^^          ^^          ^^
 *              7 TDs for      1 TD for   Start of    Start of    End Chain
 *              INT (2-128ms)  1ms-INT    CTRL Chain  BULK Chain
 *
 *
 * Since this is a bootloader, the isochronous transfer descriptor have been removed.
 *
 * Interrupt Transfers.
 * --------------------
 * For Interupt transfers USB_MAX_TEMP_INT_TD Transfer descriptor are available. They
 * will be inserted after the appropriate (depending the interval setting) skeleton TD.
 * If an interrupt has been detected the dev->irqhandler is called. The status and number
 * of transfered bytes is stored in dev->irq_status resp. dev->irq_act_len. If the
 * dev->irqhandler returns 0, the interrupt TD is removed and disabled. If an 1 is returned,
 * the interrupt TD will be reactivated.
 *
 * Control Transfers
 * -----------------
 * Control Transfers are issued by filling the tmp_td with the appropriate data and connect
 * them to the qh_cntrl queue header. Before other control/bulk transfers can be issued,
 * the programm has to wait for completion. This does not allows asynchronous data transfer.
 *
 * Bulk Transfers
 * --------------
 * Bulk Transfers are issued by filling the tmp_td with the appropriate data and connect
 * them to the qh_bulk queue header. Before other control/bulk transfers can be issued,
 * the programm has to wait for completion. This does not allows asynchronous data transfer.
 *
 *
 */

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

#include "usb.h"
#include "usb_uhci.h"



//#undef USB_UHCI_DEBUG
#define USB_UHCI_DEBUG

#ifdef	USB_UHCI_DEBUG
#define	USB_UHCI_PRINTF(fmt,args...)	printf (fmt ,##args)
#else
#define USB_UHCI_PRINTF(fmt,args...)
#endif


#define out8r(addr, val)  *((volatile u8*)(addr)) = (val)
#define out16r(addr, val) *((volatile u16*)(addr)) = (val)
#define out32r(addr, val) *((volatile u32*)(addr)) = (val)
#define in16r(addr)  *((volatile u16*)(addr))
#define in32r(addr)  *((volatile u32*)(addr))

static int uhci_stop = 1;

static int uhci_match(struct device *parent, void *match, void *aux);
static void uhci_attach(struct device *parent, struct device *self, void *aux);


static int uhci_submit_bulk_msg(struct usb_device *dev, unsigned long pipe, void *buffer,int transfer_len);

static int uhci_submit_control_msg(struct usb_device *dev, unsigned long pipe, void *buffer,
										int transfer_len,struct devrequest *setup);

static int uhci_submit_int_msg(struct usb_device *dev, unsigned long pipe, void *buffer,int transfer_len, int interval);

static int usb_lowlevel_init(void * hc_data);
		
int handle_usb_interrupt(void *);

struct cfattach uhci_ca = {
	sizeof(struct uhci), uhci_match, uhci_attach
};

struct cfdriver uhci_cd = {
	NULL, "usb-uhci", DV_DULL
};

struct usb_ops uhci_op = {
	.submit_bulk_msg = uhci_submit_bulk_msg,
	.submit_control_msg = uhci_submit_control_msg,
	.submit_int_msg  = uhci_submit_int_msg
};


static int uhci_match(struct device *parent, void *match, void *aux)
{
	struct pci_attach_args *pa = aux;
	static int addr=0xc000;
	
	if(PCI_CLASS(pa->pa_class) == PCI_CLASS_SERIALBUS && 
		  PCI_SUBCLASS(pa->pa_class) == PCI_SUBCLASS_SERIALBUS_USB) {
		if(((pa->pa_class >>8) & 0xff) == 0x00){
			printf("usb %d/%d\n", pa->pa_device, pa->pa_function);
#if 0
			if(!(pa->pa_function ==2))
				return 0;
			addr=_pci_allocate_io(_pci_head,0x20);
			pci_conf_write(0, pa->pa_tag, 0x20, addr);
#endif
			printf("Found usb uhci controller %x\n", 
				pci_conf_read(0, pa->pa_tag, 0x20));
			return 1; 
		}
	}

	return 0;
}


static void uhci_attach(struct device *parent, struct device *self, void *aux)
{

	uhci_t *uhci = (struct uhci*)self;
	struct pci_attach_args *pa = (struct pci_attach_args *)aux;

	const char *intrstr;
	pci_chipset_tag_t pc = pa->pa_pc;
	bus_space_tag_t iot = pa->pa_iot;
//	bus_space_tag_t memt = pa->pa_memt;
	pci_intr_handle_t ih;
	bus_addr_t iobase; 
	bus_addr_t iosize;

#if 0
	my_uhci = uhci;
#endif

	if(pci_io_find(pc, pa->pa_tag, PCI_MEMBASE_1, &iobase, &iosize)){
		printf("Can not find i/o space\n");		
		return ;
	}

	if(bus_space_map(iot, iobase, iosize, 0, &uhci->sc_sh)){
		printf("Can not map i/o space\n");
		return ;
	}

	printf("%s: iobase = %x/%x \n", self->dv_xname, iobase, uhci->sc_sh);
	uhci->io_addr = uhci->sc_sh;
	uhci->io_size = iosize;	
	uhci->sc_st = iot;
	uhci->sc_pc = pc; 

	intrstr = pci_intr_string(pc, ih);
	
	if(pci_intr_map(pc, pa->pa_intrtag, pa->pa_intrpin,
					pa->pa_intrline, &ih)) {
		printf(": couldn't map interrupt\n");
		return;
	}
	
	usb_lowlevel_init(self);

	uhci->hc.uop = &uhci_op;

#if 1	
	uhci->sc_ih = pci_intr_establish(pc, ih, IPL_BIO, handle_usb_interrupt, uhci,
	    self->dv_xname);
	if (uhci->sc_ih == NULL) {
		printf(": couldn't establish interrupt");
		if (intrstr != NULL)
			printf(" at %s\n", intrstr);
		return;
	}
#endif

	uhci->rdev = usb_alloc_new_device(uhci);
	usb_new_device(uhci->rdev);
	
	//usb_scan_devices(uhci);
}



/* temporary tds */
//FIXME 

static struct virt_root_hub rh;   /* struct for root hub */

/**********************************************************************
 * some forward decleration
 */
static int uhci_submit_rh_msg(struct usb_device *dev, unsigned long pipe,
						void *buffer, int transfer_len,struct devrequest *setup);

/* fill a td with the approproiate data. Link, status, info and buffer
 * are used by the USB controller itselfes, dev is used to identify the
 * "connected" device
 */
void usb_fill_td(uhci_td_t* td,unsigned long link,unsigned long status,
					unsigned long info, unsigned long buffer, unsigned long dev)
{
	uhci_td_t *ptd = (uhci_td_t *)CACHED_TO_UNCACHED(td);	

	ptd->link=swap_32(link);
	ptd->status=swap_32(status);
	ptd->info=swap_32(info);
	ptd->buffer=buffer;
	//ptd->buffer=vtophys(buffer);
	ptd->dev_ptr=dev;
}

/* fill a qh with the approproiate data. Head and element are used by the USB controller
 * itselfes. As soon as a valid dev_ptr is filled, a td chain is connected to the qh.
 * Please note, that after completion of the td chain, the entry element is removed /
 * marked invalid by the USB controller.
 */
void usb_fill_qh(uhci_qh_t* qh,unsigned long head,unsigned long element)
{
	uhci_qh_t *pqh = (uhci_qh_t *)CACHED_TO_UNCACHED(qh);

	pqh->head=swap_32(head);
	pqh->element=(element);
	pqh->dev_ptr=0L;
}

/* get the status of a td->status
 */
unsigned long usb_uhci_td_stat(unsigned long status)
{
	unsigned long result=0;
	result |= (status & TD_CTRL_NAK)      ? USB_ST_NAK_REC : 0;
	result |= (status & TD_CTRL_STALLED)  ? USB_ST_STALLED : 0;
	result |= (status & TD_CTRL_DBUFERR)  ? USB_ST_BUF_ERR : 0;
	result |= (status & TD_CTRL_BABBLE)   ? USB_ST_BABBLE_DET : 0;
	result |= (status & TD_CTRL_CRCTIMEO) ? USB_ST_CRC_ERR : 0;
	result |= (status & TD_CTRL_BITSTUFF) ? USB_ST_BIT_ERR : 0;
	result |= (status & TD_CTRL_ACTIVE)   ? USB_ST_NOT_PROC : 0;
	return result;
}

/* get the status and the transfered len of a td chain.
 * called from the completion handler
 */
int usb_get_td_status(uhci_td_t *td,struct usb_device *dev)
{
	unsigned long temp,info;
	unsigned long stat;
	int tlen;
	uhci_td_t *mytd=(uhci_td_t *)CACHED_TO_UNCACHED(td);

	if(dev->devnum==rh.devnum)
		return 0;
	dev->act_len=0;
	stat=0;
	do {
retry:	
		temp=swap_32(mytd->status);
		wbflush();
		stat=usb_uhci_td_stat(temp);
		info=swap_32(mytd->info);
		//printf("%x, stat =%x temp=%x\n", mytd, stat, temp);
		if(stat & USB_ST_NOT_PROC){
#if 0
			printf("dump qh\n");
			{
				uhci_td_t *int0 = (uhci_td_t *)CACHED_TO_UNCACHED(&td_int[0]);
				uhci_qh_t *qh = (uhci_qh_t *)CACHED_TO_UNCACHED(&qh_cntrl);
				unsigned long ltd = qh->element;
				uhci_td_t *td;
				printf("int0 %x\n", int0);
				printf(" link= %x\n", int0->link);
				printf(" status =%x\n", int0->status);
				while(ltd != UHCI_PTR_TERM){
					td = (uhci_td_t *)CACHED_TO_UNCACHED(ltd);
					printf("td=%x \n", td);
					printf("ltd= %x\n", ltd);
					printf("  status %x\n", td->status);
					printf("  info %x\n", td->info);
					ltd = td->link;
				}
				
			}	
#endif
			delay(10);	
			goto retry;
		}

		if(((info & 0xff)!= USB_PID_SETUP) &&
				(((info >> 21) & 0x7ff)!= 0x7ff))
		{  /* if not setup and not null data pack */
			dev->act_len+= tlen = (temp +1 )& 0x7FF ; /* the transfered len is act_len + 1 */
			//printf("act_len %d link %x\n", dev->act_len, mytd->link);
			if(usb_pipecontrol(mytd->pipe) && usb_pipein(mytd->pipe)){
				if(mytd->data)
					memcpy(mytd->data, (void*)CACHED_TO_UNCACHED(mytd->buffer), tlen);
				mytd->data = NULL;
			}

			if(usb_pipecontrol(mytd->pipe) && (tlen < (((info >>21) + 1) & 0x7ff ))){
				if((info & 0xff) == USB_PID_IN){
					uhci_qh_t *qh = (uhci_qh_t *)CACHED_TO_UNCACHED(dev->qpriv);
					uhci_td_t *ltd = qh->last_td;
					qh->element = vtophys(ltd);
					mytd=CACHED_TO_UNCACHED(qh->last_td);
					continue;
				}
				stat = 0;
				break;
			}
			//otherwise bulk or int
		}
		if(stat) {           /* status no ok */
			dev->status=stat;
			return -1;
		}
		temp=swap_32((unsigned long)mytd->link);
		mytd=(uhci_td_t *)CACHED_TO_UNCACHED(temp & 0xfffffff0);
	}while((temp & 0x1)==0); /* process all TDs */
	dev->status=stat;
	return 0; /* Ok */
}


/*-------------------------------------------------------------------
 *                         LOW LEVEL STUFF
 *          assembles QHs und TDs for control, bulk and iso
 *-------------------------------------------------------------------*/

/* Submits a control message. That is a Setup, Data and Status transfer.
 * Routine does not wait for completion.
 */
static int uhci_submit_control_msg(struct usb_device *dev, unsigned long pipe, void *buffer,
										int transfer_len,struct devrequest *setup)
{
	unsigned long destination, status;
	int maxsze = usb_maxpacket(dev, pipe);
	unsigned long dataptr;
	int len;
	int pktsze;
	int i=0;
	int s;
	uhci_t * uhci = dev->hc_private;
	((uhci_qh_t*)CACHED_TO_UNCACHED(&qh_cntrl))->element =UHCI_PTR_TERM;//0xffffffffL;
	usb_fill_td(&td_int0,vtophys(&qh_cntrl) | UHCI_PTR_QH,0,0,0,0L);
	memset(tmp_td, 0, sizeof(tmp_td));
	pci_sync_cache(uhci->sc_pc, (vm_offset_t)&tmp_td, sizeof(tmp_td), SYNC_W);

	if (!maxsze) {
		USB_UHCI_PRINTF("uhci_submit_control_urb: pipesize for pipe %lx is zero\n", pipe);
		return -1;
	}

	if(((pipe>>8)&0x7f)==rh.devnum) {
		/* this is the root hub -> redirect it */
		return uhci_submit_rh_msg(dev,pipe,buffer,transfer_len,setup);
	}
	USB_UHCI_PRINTF("uhci_submit_control start len %x, maxsize %x\n",transfer_len,maxsze);
	s = splimp();
	memset(tmp_td, 0, sizeof(tmp_td));
	pci_sync_cache(uhci->sc_pc, (vm_offset_t)&tmp_td, sizeof(tmp_td), SYNC_W);

	/* The "pipe" thing contains the destination in bits 8--18 */
	destination = (pipe & PIPE_DEVEP_MASK) | USB_PID_SETUP; /* Setup stage */
	/* 3 errors */
	status = (pipe & TD_CTRL_LS) | TD_CTRL_ACTIVE | (3 << 27) |TD_CTRL_SPD; 
	/*  Build the TD for the control request, try forever, 8 bytes of data */
	memcpy(uhci->setup, setup, sizeof(*setup));
	usb_fill_td(&tmp_td[i],UHCI_PTR_TERM ,status, destination | (7 << 21),vtophys((unsigned long)uhci->setup),(unsigned long)dev);
#if 0
	{
		char *sp=(char *)setup;
		printf("SETUP to pipe %lx: %x %x %x %x %x %x %x %x\n", pipe,
		    sp[0],sp[1],sp[2],sp[3],sp[4],sp[5],sp[6],sp[7]);
	}
#endif
	if(usb_pipeout(pipe))
		memcpy(uhci->control_buf, buffer, transfer_len);

	if(transfer_len)
	{
		if(usb_pipeout(pipe))
		pci_sync_cache(uhci->sc_pc, (vm_offset_t)uhci->control_buf, transfer_len, SYNC_W);
		else 
		pci_sync_cache(uhci->sc_pc, (vm_offset_t)uhci->control_buf, transfer_len, SYNC_R);

		dataptr = vtophys((unsigned long)uhci->control_buf);
	}
	else
		dataptr = 0;
	len=transfer_len;

	/* If direction is "send", change the frame from SETUP (0x2D)
	   to OUT (0xE1). Else change it from SETUP to IN (0x69). */
	destination = (pipe & PIPE_DEVEP_MASK) | ((pipe & USB_DIR_IN)==0 ? USB_PID_OUT : USB_PID_IN);
	while (len > 0) {
		/* data stage */
		pktsze = len;
		i++;
		if (pktsze > maxsze)
			pktsze = maxsze;
		if(i>=USB_MAX_TEMP_TD){while(1)printf("there is no tds,please set USB_MAX_TEMP_TD larger than %d\n",USB_MAX_TEMP_TD);}
		destination ^= 1 << TD_TOKEN_TOGGLE;	/* toggle DATA0/1 */
		usb_fill_td(&tmp_td[i],UHCI_PTR_TERM, status, destination | ((pktsze - 1) << 21),dataptr,(unsigned long)dev);	/* Status, pktsze bytes of data */
		((uhci_td_t *)CACHED_TO_UNCACHED(&tmp_td[i-1]))->link=vtophys((unsigned long)&tmp_td[i]);
		((uhci_td_t *)CACHED_TO_UNCACHED(&tmp_td[i]))->pipe = pipe;
		((uhci_td_t *)CACHED_TO_UNCACHED(&tmp_td[i]))->data = buffer;

		dataptr += pktsze;
		buffer  += pktsze;
		len -= pktsze;
	}

	/*  Build the final TD for control status */
	/* It's only IN if the pipe is out AND we aren't expecting data */

	destination &= ~UHCI_PID;
	if (((pipe & USB_DIR_IN)==0) || (transfer_len == 0))
		destination |= USB_PID_IN;
	else
		destination |= USB_PID_OUT;
	destination |= 1 << TD_TOKEN_TOGGLE;	/* End in Data1 */
	i++;
	status &= ~TD_CTRL_SPD;
	/* no limit on errors on final packet , 0 bytes of data */
	usb_fill_td(&tmp_td[i],UHCI_PTR_TERM, status | TD_CTRL_IOC, destination | (UHCI_NULL_DATA_SIZE << 21),0,(unsigned long)dev);
	((uhci_td_t*)CACHED_TO_UNCACHED(&tmp_td[i-1]))->link=vtophys((unsigned long)&tmp_td[i]);	/* queue status td */
	//	usb_show_td(i+1);
	USB_UHCI_PRINTF("uhci_submit_control end (%d tmp_tds used)\n",i);
	/* first mark the control QH element terminated */
	((uhci_qh_t*)CACHED_TO_UNCACHED(&qh_cntrl))->element =UHCI_PTR_TERM;//0xffffffffL;
	/* set qh active */
	((uhci_qh_t*)CACHED_TO_UNCACHED(&qh_cntrl))->dev_ptr=(unsigned long)dev;
	dev->status = USB_ST_NOT_PROC;
	/* fill in tmp_td_chain */
	((uhci_qh_t*)CACHED_TO_UNCACHED(&qh_cntrl))->element=vtophys((unsigned long)&tmp_td[0]);

	((uhci_qh_t*)CACHED_TO_UNCACHED(&qh_cntrl))->last_td = &tmp_td[i];
	dev->qpriv = &qh_cntrl;
#if 1	
	s = spl0();
	while(1){
		if(!(dev->status & USB_ST_NOT_PROC)){
			break;
		}
		spl0();
	}
#else
	while(1){
		handle_usb_interrupt(uhci);
		if(!(dev->status & USB_ST_NOT_PROC))
			break;	
	}
#endif
	splx(s);
	return 0;
}

/*-------------------------------------------------------------------
 * Prepare TDs for bulk transfers.
 */
static int uhci_submit_bulk_msg(struct usb_device *dev, unsigned long pipe, void *buffer,int transfer_len)
{
	unsigned long destination, status,info;
	unsigned long dataptr;
	int maxsze = usb_maxpacket(dev, pipe);
	int len;
	int i=0;
	int s;
	uhci_t *uhci = dev->hc_private;

	((uhci_qh_t *)CACHED_TO_UNCACHED(&qh_bulk))->element =UHCI_PTR_TERM;//0xffffffffL;
	usb_fill_td(&td_int0,vtophys(&qh_bulk) | UHCI_PTR_QH,0,0,0,0L);
	memset(tmp_td, 0, sizeof(tmp_td));
	pci_sync_cache(uhci->sc_pc, (vm_offset_t)&tmp_td, sizeof(tmp_td), SYNC_W);
	
	if(transfer_len < 0) {
		printf("Negative transfer length in submit_bulk\n");
		return -1;
	}
	//printf("uhci_submit_bulk_msg: transfer_len %x\n", transfer_len);
	if (!maxsze)
		return -1;
	/* The "pipe" thing contains the destination in bits 8--18. */
	s = splimp();
	memset(tmp_td, 0, sizeof(tmp_td));
	pci_sync_cache(uhci->sc_pc, (vm_offset_t)&tmp_td, sizeof(tmp_td), SYNC_W);
	destination = (pipe & PIPE_DEVEP_MASK) | usb_packetid (pipe);
	/* 3 errors */
	status = (pipe & TD_CTRL_LS) | TD_CTRL_ACTIVE | (3 << 27);
	/*	((urb->transfer_flags & USB_DISABLE_SPD) ? 0 : TD_CTRL_SPD) | (3 << 27); */
	/* Build the TDs for the bulk request */
	len = transfer_len;
	dataptr = vtophys((unsigned long)buffer);
	if(usb_pipeout(pipe))
	pci_sync_cache(uhci->sc_pc, (vm_offset_t)buffer, transfer_len, SYNC_W);
	else
	pci_sync_cache(uhci->sc_pc, (vm_offset_t)buffer, transfer_len, SYNC_R);

	do {
		int pktsze = len;
		if (pktsze > maxsze)
			pktsze = maxsze;
		/* pktsze bytes of data  */
		if(i>=USB_MAX_TEMP_TD){while(1)printf("there is no tds,please set USB_MAX_TEMP_TD larger than %d\n",USB_MAX_TEMP_TD);}
		info = destination | (((pktsze - 1)&UHCI_NULL_DATA_SIZE) << 21) |
			(usb_gettoggle (dev, usb_pipeendpoint (pipe), usb_pipeout (pipe)) << TD_TOKEN_TOGGLE);

		if((len-pktsze)==0)
			status |= TD_CTRL_IOC;	/* last one generates INT */

		usb_fill_td(&tmp_td[i],UHCI_PTR_TERM, status, info,dataptr,(unsigned long)dev);	/* Status, pktsze bytes of data */
		if(i>0)
			((uhci_td_t *)CACHED_TO_UNCACHED(&tmp_td[i-1]))->link=vtophys((unsigned long)&tmp_td[i] | 0x0004);
		i++;
		dataptr += pktsze;
		len -= pktsze;
		usb_dotoggle (dev, usb_pipeendpoint (pipe), usb_pipeout (pipe));
	} while (len > 0);
	/* first mark the bulk QH element terminated */
	((uhci_qh_t *)CACHED_TO_UNCACHED(&qh_bulk))->element =UHCI_PTR_TERM;//0xffffffffL;
	/* set qh active */
	((uhci_qh_t *)CACHED_TO_UNCACHED(&qh_bulk))->dev_ptr=(unsigned long)dev;
	dev->status = USB_ST_NOT_PROC;
	/* fill in tmp_td_chain */
	((uhci_qh_t *)CACHED_TO_UNCACHED(&qh_bulk))->element=vtophys((unsigned long)&tmp_td[0] | 0x0004);
	dev->qpriv = &qh_bulk;

#if 1
	s = spl0();
	while(1){
		if(!(dev->status & USB_ST_NOT_PROC)){
			break;
		}
		spl0();
	}
#else
	while(1){
		handle_usb_interrupt(uhci);
		if(!(dev->status & USB_ST_NOT_PROC))
			break;	
	}
#endif
	splx(s);
	return 0;
}


/* search a free interrupt td
 */
uhci_td_t *uhci_alloc_int_td(uhci_t *uhci)
{
	int i;
	for(i=0;i<USB_MAX_TEMP_INT_TD;i++) {
		if(((uhci_td_t *)CACHED_TO_UNCACHED(&tmp_int_td[i]))->dev_ptr==0) 
			return &tmp_int_td[i];
	}
	return NULL;
}

#if 0
void uhci_show_temp_int_td(void)
{
	int i;
	for(i=0;i<USB_MAX_TEMP_INT_TD;i++) {
		if((tmp_int_td[i].dev_ptr&0x01)!=0x1L) /* no device assigned -> free TD */
			printf("temp_td %d is assigned to dev %lx\n",i,tmp_int_td[i].dev_ptr);
	}
	printf("all others temp_tds are free\n");
}
#endif
/*-------------------------------------------------------------------
 * submits USB interrupt (ie. polling ;-)
 */
static int uhci_submit_int_msg(struct usb_device *dev, unsigned long pipe, void *buffer,int transfer_len, int interval)
{
	int nint, n;
	unsigned long status, destination;
	unsigned long info,tmp;
	uhci_td_t *mytd;
	uhci_t *uhci = dev->hc_private;


	if (interval < 0 || interval >= 256)
		return -1;

	if (interval == 0)
		nint = 0;
	else {
		for (nint = 0, n = 1; nint <= 8; nint++, n += n)	/* round interval down to 2^n */
		 {
			if(interval < n) {
				interval = n / 2;
				break;
			}
		}
		nint--;
	}

	USB_UHCI_PRINTF("Rounded interval to %d, chain  %d\n", interval, nint);
	mytd=uhci_alloc_int_td(uhci);
	if(mytd==NULL) {
		printf("No free INT TDs found\n");
		return -1;
	}


	status = (pipe & TD_CTRL_LS) | TD_CTRL_ACTIVE | TD_CTRL_IOC | (3 << 27);
/*		(urb->transfer_flags & USB_DISABLE_SPD ? 0 : TD_CTRL_SPD) | (3 << 27);
*/

	destination =(pipe & PIPE_DEVEP_MASK) | usb_packetid (pipe) | (((transfer_len - 1) & 0x7ff) << 21);

	info = destination | (usb_gettoggle(dev, usb_pipeendpoint(pipe), usb_pipeout(pipe)) << TD_TOKEN_TOGGLE);
	tmp = ((uhci_td_t *)CACHED_TO_UNCACHED(&td_int[nint]))->link;
	/*only for small data < 64 byte, it shoutld be ok*/
	if(usb_pipeout(pipe))
	pci_sync_cache(uhci->sc_pc, (vm_offset_t)buffer, transfer_len, SYNC_W); //wb
	else
	pci_sync_cache(uhci->sc_pc, (vm_offset_t)buffer, transfer_len, SYNC_R); //wb
	usb_fill_td(mytd,tmp,status, info,vtophys(buffer),(unsigned long)dev);
	//usb_fill_td(mytd, tmp,status, info,(vm_offset_t)(buffer),(unsigned long)dev);
	/* Link it */
	tmp = swap_32((unsigned long)mytd);
	((uhci_td_t *)CACHED_TO_UNCACHED(&td_int[nint]))->link=vtophys(tmp);

	usb_dotoggle (dev, usb_pipeendpoint (pipe), usb_pipeout (pipe));

	return 0;
}

/**********************************************************************
 * Low Level functions
 */


void reset_hc(uhci_t *uhci)
{

	/* Global reset for 100ms */
	out16r( usb_base_addr + USBPORTSC1,0x0204);
	out16r( usb_base_addr + USBPORTSC2,0x0204);
	out16r( usb_base_addr + USBCMD,USBCMD_GRESET | USBCMD_RS);
	/* Turn off all interrupts */
	out16r(usb_base_addr + USBINTR,0);
	wait_ms(50);
	out16r( usb_base_addr + USBCMD,0);
	wait_ms(10);
}

void start_hc(uhci_t *uhci)
{
	int timeout = 1000;

	while(in16r(usb_base_addr + USBCMD) & USBCMD_HCRESET) {
		if (!--timeout) {
			printf("USBCMD_HCRESET timed out!\n");
			break;
		}
	}

	out8r(usb_base_addr + USBSOF, 64);
	/* Turn on all interrupts */
	//out16r(usb_base_addr + USBINTR,USBINTR_TIMEOUT | USBINTR_RESUME | USBINTR_IOC | USBINTR_SP);
	/* Start at frame 0 */
	out16r(usb_base_addr + USBFRNUM,0);
	/* set Framebuffer base address */
	out32r(usb_base_addr+USBFLBASEADD,vtophys(&framelist));
	/* Run and mark it configured with a 64-byte max packet */
	out16r(usb_base_addr + USBCMD,USBCMD_RS | USBCMD_CF | USBCMD_MAXP);
}

/* Initialize the skeleton
 */
void usb_init_skel(void *data)
{
	unsigned long temp;
	int n;
	uhci_t *uhci =data;

	for(n=0;n<USB_MAX_TEMP_INT_TD;n++)
		tmp_int_td[n].dev_ptr=0L; /* no devices connected */
	
	pci_sync_cache(uhci->sc_pc, (vm_offset_t)&tmp_int_td, sizeof(tmp_int_td), SYNC_W);
	/* last td */
	memset(&td_last, 0, sizeof(td_last));
	pci_sync_cache(uhci->sc_pc, (vm_offset_t)&td_last, sizeof(td_last), SYNC_W);
	
	usb_fill_td(&td_last,UHCI_PTR_TERM,TD_CTRL_IOC ,0,0,0L);
  /* usb_fill_td(&td_last,UHCI_PTR_TERM,0,0,0); */
	/* End Queue Header */

	memset(&qh_end, 0, sizeof(qh_end));
	pci_sync_cache(uhci->sc_pc, (vm_offset_t)&qh_end, sizeof(qh_end), SYNC_W);

	usb_fill_qh(&qh_end,UHCI_PTR_TERM,vtophys((unsigned long)&td_last));
	/* Bulk Queue Header */

	memset(&qh_bulk, 0, sizeof(qh_bulk));
	pci_sync_cache(uhci->sc_pc, (vm_offset_t)&qh_bulk, sizeof(qh_bulk), SYNC_W);
	
	temp=(unsigned long)&qh_end;
	usb_fill_qh(&qh_bulk,vtophys(temp) | UHCI_PTR_QH,UHCI_PTR_TERM);
	/* Control Queue Header */

	memset(&qh_cntrl, 0, sizeof(qh_cntrl));
	pci_sync_cache(uhci->sc_pc, (vm_offset_t)&qh_cntrl, sizeof(qh_cntrl), SYNC_W);
		
//	temp=(unsigned long)&qh_bulk;
	temp=(unsigned long)&qh_end;
	usb_fill_qh(&qh_cntrl, vtophys(temp) | UHCI_PTR_QH,UHCI_PTR_TERM);
	/* 1ms Interrupt td */

	memset(td_int, 0, sizeof(td_int));
	pci_sync_cache(uhci->sc_pc, (vm_offset_t)&td_int, sizeof(td_int), SYNC_W);
		
	memset(&td_int0, 0, sizeof(td_int0));
	pci_sync_cache(uhci->sc_pc, (vm_offset_t)&td_int0, sizeof(td_int0), SYNC_W);

	temp=(unsigned long)&qh_cntrl;
	usb_fill_td(&td_int0,vtophys(temp) | UHCI_PTR_QH,0,0,0,0L);

	temp=(unsigned long)&td_int0;
	usb_fill_td(&td_int[0],vtophys(temp) ,0,0,0,0L);

	memset(tmp_td, 0, sizeof(tmp_td));
	pci_sync_cache(uhci->sc_pc, (vm_offset_t)&tmp_td, sizeof(tmp_td), SYNC_W);

	temp=(unsigned long)&td_int[0];
	for(n=1; n<8; n++)
		usb_fill_td(&td_int[n],vtophys(temp),0,0,0,0L);
	for (n = 0; n < 1024; n++) {
	/* link all framelist pointers to one of the interrupts */
		int m, o;
		if ((n&127)==127)
			framelist[n]= vtophys(swap_32((unsigned long)&td_int[0]));
		else
			for (o = 1, m = 2; m <= 128; o++, m += m)
				if ((n & (m - 1)) == ((m - 1) / 2))
						framelist[n]= vtophys(swap_32((unsigned long)&td_int[o]));
	}
	pci_sync_cache(uhci->sc_pc, (vm_offset_t)framelist, sizeof(framelist), SYNC_W);

}

/* check the common skeleton for completed transfers, and update the status
 * of the "connected" device. Called from the IRQ routine.
 */
void usb_check_skel(uhci_t *uhci)
{
	struct usb_device *dev;
	/* start with the control qh */
	uhci_qh_t *qcntrl = (uhci_qh_t *)CACHED_TO_UNCACHED(&qh_cntrl);
	uhci_qh_t *qbulk = (uhci_qh_t *)CACHED_TO_UNCACHED(&qh_bulk);
			
	if(qcntrl->dev_ptr!=0) /* it's a device assigned check if this caused IRQ */
	{
		//printf("check qcntrl\n");
		dev=(struct usb_device *)qcntrl->dev_ptr;
		usb_get_td_status(&tmp_td[0],dev); /* update status */
		if(!(dev->status & USB_ST_NOT_PROC)) { /* is not active anymore, disconnect devices */
			qcntrl->dev_ptr=0;
		if(!(qcntrl->element&1))qcntrl->element=UHCI_PTR_TERM;
		}
	}
	/* now process the bulk */
	if(qbulk->dev_ptr!=0) /* it's a device assigned check if this caused IRQ */
	{
		//printf("check qbulk\n");	
		dev=(struct usb_device *)qbulk->dev_ptr;
		usb_get_td_status(&tmp_td[0],dev); /* update status */
		if(!(dev->status & USB_ST_NOT_PROC)) { /* is not active anymore, disconnect devices */
			qbulk->dev_ptr=0;
		if(!(qbulk->element&1))qbulk->element=UHCI_PTR_TERM;
		}
	}
}

/* check the interrupt chain, ubdate the status of the appropriate device,
 * call the appropriate irqhandler and reactivate the TD if the irqhandler
 * returns with 1
 */
void usb_check_int_chain(uhci_t *uhci)
{
	int i,res;
	unsigned long link,status;
	struct usb_device *dev;
	uhci_td_t *td,*prevtd;

	for(i=0;i<8;i++) {
		prevtd= (uhci_td_t *)CACHED_TO_UNCACHED(&td_int[i]); /* the first previous td is the skeleton td */
		link=swap_32(((uhci_td_t *)CACHED_TO_UNCACHED(&td_int[i]))->link) & 0xfffffff0; /* next in chain */
		td=(uhci_td_t *)CACHED_TO_UNCACHED(link); /* assign it */
		/* all interrupt TDs are finally linked to the td_int[0].
 		 * so we process all until we find the td_int[0].
		 * if int0 chain points to a QH, we're also done
	   */
		while(((i>0) && (CACHED_TO_PHYS(link) != (unsigned long)CACHED_TO_PHYS(&td_int[0]))) ||
					((i==0) && !(swap_32(td->link) &  UHCI_PTR_QH)))
		{
			/* check if a device is assigned with this td */
			int len;
			status=swap_32(td->status);
			if((td->dev_ptr!=0L) && !(status & TD_CTRL_ACTIVE)) {
				/* td is not active and a device is assigned -> call irqhandler */
				dev=(struct usb_device *)td->dev_ptr;
				dev->irq_act_len= len = ((status & 0x7FF)==0x7FF) ? 0 : (status & 0x7FF) + 1; /* transfered length */
				dev->irq_status=usb_uhci_td_stat(status); /* get status */
				res=dev->irq_handle(dev); /* call irqhandler */
				if(res==1) {
					/* reactivate */
					status|=TD_CTRL_ACTIVE;
					td->status=swap_32(status);
					prevtd=td; /* previous td = this td */
				}
				else {
					prevtd->link=td->link; /* link previous td directly to the nex td -> unlinked */
					/* remove device pointer */
					td->dev_ptr=0L;
				}
			} /* if we call the irq handler */
			link=swap_32(td->link) & 0xfffffff0; /* next in chain */
			td=(uhci_td_t *)CACHED_TO_UNCACHED(link); /* assign it */
		} /* process all td in this int chain */
	} /* next interrupt chain */
}

#if 0
void usb_unlink_inactive(uhci_qh_t *head)
{
	uhci_qh_t *qh;
	uhci_td_t *td, *ptd = NULL;
	unsigned long link;

	qh = (uhci_qh_t *)CACHED_TO_UNCACHED(head);

	if(!(qh->element & UHCI_PTR_QH)){
		link = qh->element;
		td = (uhci_td_t *)CACHED_TO_UNCACHED(link & 0xfffffff0);
		while(link != UHCI_PTR_TERM ){
			printf("unlink: link %x\n", link);
			printf("   status %x\n", td->status);
			if(td->status & TD_CTRL_ACTIVE)
				break;
			printf("unlink %x\n", link);
			link = td->link;
			td = (uhci_td_t *)CACHED_TO_UNCACHED((td->link & 0xfffffff0));
		}
		qh->element = link;
	}
}
#endif
/* usb interrupt service routine.
 */
int handle_usb_interrupt(void *hc_data)
{
	unsigned short status;
	int s;
	static int count = 0,count1=0;
	uhci_t *uhci=hc_data;

	s = splimp();
//	if (count++%0x80000==0) printf("handle_usb_interrupt,count=%d\n",count);
	/*
	 * Read the interrupt status, and write it back to clear the
	 * interrupt cause
	 */
	if (uhci_stop) return 0;

	status = in16r(usb_base_addr + USBSTS);

	if (!status)		/* shared interrupt, not mine */
		return 0;

//	if (count1++%0x1000==0) printf("status=%x,count=%d\n",status,count);
	if (status != 1) {
		/* remove host controller halted state */
#if 1
		if ((status&0x20) && ((in16r(usb_base_addr+USBCMD) && USBCMD_RS)==0)) {
			out16r(usb_base_addr + USBCMD, USBCMD_RS | in16r(usb_base_addr + USBCMD));
		}
#else
		out16r(usb_base_addr + USBCMD, USBCMD_RS | in16r(usb_base_addr + USBCMD));
#endif
	}

	usb_check_int_chain(uhci); /* call interrupt handlers for int tds */
	usb_check_skel(uhci); /* call completion handler for common transfer routines */
	out16r(usb_base_addr+USBSTS,status);
	
	splx(s);
	return 0;
}


/* init uhci
 */
static int usb_lowlevel_init(void * hc_data)
{
	uhci_t *uhci = hc_data;

	usb_base_addr = uhci->io_addr;
	printf("usb_base_addr =%x\n", usb_base_addr);

	rh.devnum = 0;
	usb_init_skel(uhci);
	
	memset(hc_setup, 0, sizeof(hc_setup));
	memset(control_buf0, 0, sizeof(control_buf0));
	pci_sync_cache(uhci->sc_pc, (vm_offset_t)hc_setup, sizeof(hc_setup), SYNC_W);
	pci_sync_cache(uhci->sc_pc, (vm_offset_t)control_buf0, sizeof(control_buf0), SYNC_W);

	uhci->setup = (unsigned char*)CACHED_TO_UNCACHED(hc_setup);
	uhci->control_buf = (unsigned char*)CACHED_TO_UNCACHED(control_buf0);
	
	reset_hc(uhci);
	start_hc(uhci);

	uhci_stop = 0;

	return 0;
}

/* stop uhci
 */
void usb_uhci_stop_one(uhci_t *uhci)
{
	uhci_stop = 1;
	reset_hc(uhci);
}

void usb_uhci_stop()
{
        int cmd;
	uhci_t *tmp_uhci;
	struct device *dev, *next_dev;

	for (dev  = TAILQ_FIRST(&alldevs); dev != NULL; dev = next_dev) {
		next_dev = TAILQ_NEXT(dev, dv_list);
		if((dev->dv_xname[4] == 'u') && (dev->dv_xname[5] == 'h') && (dev->dv_xname[6] == 'c') && (dev->dv_xname[7] == 'i'))
		{
		  tmp_uhci = (struct uhci*)dev;
                  usb_uhci_stop_one(tmp_uhci);
	          out16r( tmp_uhci->io_addr + USBPORTSC1,0);
	          wait_ms(50);
	          out16r( tmp_uhci->io_addr + USBPORTSC2,0);
	          wait_ms(50);
                  cmd=pci_conf_read(0, tmp_uhci->pa.pa_tag, 0x04);
	          pci_conf_write(0, tmp_uhci->pa.pa_tag, 0x04, (cmd & ~0x7));
		}
	}
}
/*******************************************************************************************
 * Virtual Root Hub
 * Since the uhci does not have a real HUB, we simulate one ;-)
 */
#define	USB_RH_DEBUG

#ifdef	USB_RH_DEBUG
#define	USB_RH_PRINTF(fmt,args...)	printf (fmt ,##args)
static void usb_display_wValue(unsigned short wValue,unsigned short wIndex);
static void usb_display_Req(unsigned short req);
#else
#define USB_RH_PRINTF(fmt,args...)
static void usb_display_wValue(unsigned short wValue,unsigned short wIndex) {}
static void usb_display_Req(unsigned short req) {}
#endif

static unsigned char root_hub_dev_des[] =
{
	0x12,			/*  __u8  bLength; */
	0x01,			/*  __u8  bDescriptorType; Device */
	0x00,			/*  __u16 bcdUSB; v1.0 */
	0x01,
	0x09,			/*  __u8  bDeviceClass; HUB_CLASSCODE */
	0x00,			/*  __u8  bDeviceSubClass; */
	0x00,			/*  __u8  bDeviceProtocol; */
	0x08,			/*  __u8  bMaxPacketSize0; 8 Bytes */
	0x00,			/*  __u16 idVendor; */
	0x00,
	0x00,			/*  __u16 idProduct; */
	0x00,
	0x00,			/*  __u16 bcdDevice; */
	0x00,
	0x01,			/*  __u8  iManufacturer; */
	0x00,			/*  __u8  iProduct; */
	0x00,			/*  __u8  iSerialNumber; */
	0x01			/*  __u8  bNumConfigurations; */
};


/* Configuration descriptor */
static unsigned char root_hub_config_des[] =
{
	0x09,			/*  __u8  bLength; */
	0x02,			/*  __u8  bDescriptorType; Configuration */
	0x19,			/*  __u16 wTotalLength; */
	0x00,
	0x01,			/*  __u8  bNumInterfaces; */
	0x01,			/*  __u8  bConfigurationValue; */
	0x00,			/*  __u8  iConfiguration; */
	0x40,			/*  __u8  bmAttributes;
				   Bit 7: Bus-powered, 6: Self-powered, 5 Remote-wakwup, 4..0: resvd */
	0x00,			/*  __u8  MaxPower; */

     /* interface */
	0x09,			/*  __u8  if_bLength; */
	0x04,			/*  __u8  if_bDescriptorType; Interface */
	0x00,			/*  __u8  if_bInterfaceNumber; */
	0x00,			/*  __u8  if_bAlternateSetting; */
	0x01,			/*  __u8  if_bNumEndpoints; */
	0x09,			/*  __u8  if_bInterfaceClass; HUB_CLASSCODE */
	0x00,			/*  __u8  if_bInterfaceSubClass; */
	0x00,			/*  __u8  if_bInterfaceProtocol; */
	0x00,			/*  __u8  if_iInterface; */

     /* endpoint */
	0x07,			/*  __u8  ep_bLength; */
	0x05,			/*  __u8  ep_bDescriptorType; Endpoint */
	0x81,			/*  __u8  ep_bEndpointAddress; IN Endpoint 1 */
	0x03,			/*  __u8  ep_bmAttributes; Interrupt */
	0x08,			/*  __u16 ep_wMaxPacketSize; 8 Bytes */
	0x00,
	0xff			/*  __u8  ep_bInterval; 255 ms */
};


static unsigned char root_hub_hub_des[] =
{
	0x09,			/*  __u8  bLength; */
	0x29,			/*  __u8  bDescriptorType; Hub-descriptor */
	0x02,			/*  __u8  bNbrPorts; */
	0x00,			/* __u16  wHubCharacteristics; */
	0x00,
	0x01,			/*  __u8  bPwrOn2pwrGood; 2ms */
	0x00,			/*  __u8  bHubContrCurrent; 0 mA */
	0x00,			/*  __u8  DeviceRemovable; *** 7 Ports max *** */
	0xff			/*  __u8  PortPwrCtrlMask; *** 7 ports max *** */
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
	'U',			/*  __u8  Unicode */
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


/*
 * Root Hub Control Pipe (interrupt Pipes are not supported)
 */


int uhci_submit_rh_msg(struct usb_device *dev, unsigned long pipe, void *buffer,int transfer_len,struct devrequest *cmd)
{
	void *data = buffer;
	int leni = transfer_len;
	int len = 0;
	int status = 0;
	int stat = 0;
	int i;

	unsigned short cstatus;

	unsigned short bmRType_bReq;
	unsigned short wValue;
	unsigned short wIndex;
	unsigned short wLength;
	uhci_t * uhci = dev->hc_private;

	if ((pipe & PIPE_INTERRUPT) == PIPE_INTERRUPT) {
		printf("Root-Hub submit IRQ: NOT implemented\n");
#if 0
		uhci->rh.urb = urb;
		uhci->rh.send = 1;
		uhci->rh.interval = urb->interval;
		rh_init_int_timer (urb);
#endif
		return 0;
	}
	bmRType_bReq = cmd->requesttype | cmd->request << 8;
	wValue = swap_16(cmd->value);
	wIndex = swap_16(cmd->index);
	wLength = swap_16(cmd->length);
	usb_display_Req(bmRType_bReq);
	for (i = 0; i < 8; i++)
		rh.c_p_r[i] = 0;
	USB_RH_PRINTF("Root-Hub: adr: %2x cmd(%1x): %02x%02x %04x %04x %04x\n",
	     dev->devnum, 8, cmd->requesttype,cmd->request, wValue, wIndex, wLength);

	switch (bmRType_bReq) {
		/* Request Destination:
		   without flags: Device,
		   RH_INTERFACE: interface,
		   RH_ENDPOINT: endpoint,
		   RH_CLASS means HUB here,
		   RH_OTHER | RH_CLASS  almost ever means HUB_PORT here
		 */

	case RH_GET_STATUS:
		*(unsigned short *) data = swap_16(1);
		len=2;
		break;
	case RH_GET_STATUS | RH_INTERFACE:
		*(unsigned short *) data = swap_16(0);
		len=2;
		break;
	case RH_GET_STATUS | RH_ENDPOINT:
		*(unsigned short *) data = swap_16(0);
		len=2;
		break;
	case RH_GET_STATUS | RH_CLASS:
		*(unsigned long *) data = swap_32(0);
		len=4;
		break;	/* hub power ** */
	case RH_GET_STATUS | RH_OTHER | RH_CLASS:

		status = in16r(usb_base_addr + USBPORTSC1 + 2 * (wIndex - 1));
		cstatus = ((status & USBPORTSC_CSC) >> (1 - 0)) |
			((status & USBPORTSC_PEC) >> (3 - 1)) |
			(rh.c_p_r[wIndex - 1] << (0 + 4));
		status = (status & USBPORTSC_CCS) |
			((status & USBPORTSC_PE) >> (2 - 1)) |
			((status & USBPORTSC_SUSP) >> (12 - 2)) |
			((status & USBPORTSC_PR) >> (9 - 4)) |
			(1 << 8) |	/* power on ** */
			((status & USBPORTSC_LSDA) << (-8 + 9));

		*(unsigned short *) data = swap_16(status);
		*(unsigned short *) (data + 2) = swap_16(cstatus);
		len=4;
		break;
	case RH_CLEAR_FEATURE | RH_ENDPOINT:
		switch (wValue) {
		case (RH_ENDPOINT_STALL):
			len=0;
			break;
		}
		break;

	case RH_CLEAR_FEATURE | RH_CLASS:
		switch (wValue) {
		case (RH_C_HUB_OVER_CURRENT):
			len=0;	/* hub power over current ** */
			break;
		}
		break;

	case RH_CLEAR_FEATURE | RH_OTHER | RH_CLASS:
		usb_display_wValue(wValue,wIndex);
		switch (wValue) {
		case (RH_PORT_ENABLE):
			status = in16r(usb_base_addr+USBPORTSC1+2*(wIndex-1));
			status = (status & 0xfff5) & ~USBPORTSC_PE;
			out16r(usb_base_addr+USBPORTSC1+2*(wIndex-1),status);
			len=0;
			break;
		case (RH_PORT_SUSPEND):
			status = in16r(usb_base_addr+USBPORTSC1+2*(wIndex-1));
			status = (status & 0xfff5) & ~USBPORTSC_SUSP;
			out16r(usb_base_addr+USBPORTSC1+2*(wIndex-1),status);
			len=0;
			break;
		case (RH_PORT_POWER):
			len=0;	/* port power ** */
			break;
		case (RH_C_PORT_CONNECTION):
			status = in16r(usb_base_addr+USBPORTSC1+2*(wIndex-1));
			status = (status & 0xfff5) | USBPORTSC_CSC;
			out16r(usb_base_addr+USBPORTSC1+2*(wIndex-1),status);
			len=0;
			break;
		case (RH_C_PORT_ENABLE):
			status = in16r(usb_base_addr+USBPORTSC1+2*(wIndex-1));
			status = (status & 0xfff5) | USBPORTSC_PEC;
			out16r(usb_base_addr+USBPORTSC1+2*(wIndex-1),status);
			len=0;
			break;
		case (RH_C_PORT_SUSPEND):
/*** WR_RH_PORTSTAT(RH_PS_PSSC); */
			len=0;
			break;
		case (RH_C_PORT_OVER_CURRENT):
			len=0;
			break;
		case (RH_C_PORT_RESET):
			rh.c_p_r[wIndex - 1] = 0;
			len=0;
			break;
		}
		break;
	case RH_SET_FEATURE | RH_OTHER | RH_CLASS:
		usb_display_wValue(wValue,wIndex);
		switch (wValue) {
		case (RH_PORT_SUSPEND):
			status = in16r(usb_base_addr+USBPORTSC1+2*(wIndex-1));
			status = (status & 0xfff5) | USBPORTSC_SUSP;
			out16r(usb_base_addr+USBPORTSC1+2*(wIndex-1),status);
			len=0;
			break;
		case (RH_PORT_RESET):
			status = in16r(usb_base_addr+USBPORTSC1+2*(wIndex-1));
			status = (status & 0xfff5) | USBPORTSC_PR;
			out16r(usb_base_addr+USBPORTSC1+2*(wIndex-1),status);
			wait_ms(10);
			status = (status & 0xfff5) & ~USBPORTSC_PR;
			out16r(usb_base_addr+USBPORTSC1+2*(wIndex-1),status);
			udelay(10);
			status = (status & 0xfff5) | USBPORTSC_PE;
			out16r(usb_base_addr+USBPORTSC1+2*(wIndex-1),status);
			wait_ms(10);
			status = (status & 0xfff5) | 0xa;
			out16r(usb_base_addr+USBPORTSC1+2*(wIndex-1),status);
			len=0;
			break;
		case (RH_PORT_POWER):
			len=0;	/* port power ** */
			break;
		case (RH_PORT_ENABLE):
			status = in16r(usb_base_addr+USBPORTSC1+2*(wIndex-1));
			status = (status & 0xfff5) | USBPORTSC_PE;
			out16r(usb_base_addr+USBPORTSC1+2*(wIndex-1),status);
			len=0;
			break;
		}
		break;

	case RH_SET_ADDRESS:
		rh.devnum = wValue;
		len=0;
		break;
	case RH_GET_DESCRIPTOR:
		switch ((wValue & 0xff00) >> 8) {
		case (0x01):	/* device descriptor */
			i=sizeof(root_hub_config_des);
			status=i > wLength ? wLength : i;
			len = leni > status ? status : leni;
			memcpy (data, root_hub_dev_des, len);
			break;
		case (0x02):	/* configuration descriptor */
			i=sizeof(root_hub_config_des);
			status=i > wLength ? wLength : i;
			len = leni > status ? status : leni;
			memcpy (data, root_hub_config_des, len);
			break;
		case (0x03):	/*string descriptors */
			if(wValue==0x0300) {
				i=sizeof(root_hub_str_index0);
				status = i > wLength ? wLength : i;
				len = leni > status ? status : leni;
				memcpy (data, root_hub_str_index0, len);
				break;
			}
			if(wValue==0x0301) {
				i=sizeof(root_hub_str_index1);
				status = i > wLength ? wLength : i;
				len = leni > status ? status : leni;
				memcpy (data, root_hub_str_index1, len);
				break;
			}
			stat = USB_ST_STALLED;
		}
		break;

	case RH_GET_DESCRIPTOR | RH_CLASS:
		root_hub_hub_des[2] = 2;
		i=sizeof(root_hub_hub_des);
		status= i > wLength ? wLength : i;
		len = leni > status ? status : leni;
		memcpy (data, root_hub_hub_des, len);
		break;
	case RH_GET_CONFIGURATION:
		*(unsigned char *) data = 0x01;
		len = 1;
		break;
	case RH_SET_CONFIGURATION:
		len=0;
		break;
	default:
		stat = USB_ST_STALLED;
	}
	USB_RH_PRINTF("Root-Hub stat %lx port1: %x port2: %x\n\n",stat,
	     in16r(usb_base_addr + USBPORTSC1), in16r(usb_base_addr + USBPORTSC2));
	dev->act_len=len;
	dev->status=stat;
	return stat;

}

/********************************************************************************
 * Some Debug Routines
 */

#ifdef	USB_RH_DEBUG

static void usb_display_Req(unsigned short req)
{
	USB_RH_PRINTF("- Root-Hub Request: ");
	switch (req) {
	case RH_GET_STATUS:
		USB_RH_PRINTF("Get Status ");
		break;
	case RH_GET_STATUS | RH_INTERFACE:
		USB_RH_PRINTF("Get Status Interface ");
		break;
	case RH_GET_STATUS | RH_ENDPOINT:
		USB_RH_PRINTF("Get Status Endpoint ");
		break;
	case RH_GET_STATUS | RH_CLASS:
		USB_RH_PRINTF("Get Status Class");
		break;	/* hub power ** */
	case RH_GET_STATUS | RH_OTHER | RH_CLASS:
		USB_RH_PRINTF("Get Status Class Others");
		break;
	case RH_CLEAR_FEATURE | RH_ENDPOINT:
		USB_RH_PRINTF("Clear Feature Endpoint ");
		break;
	case RH_CLEAR_FEATURE | RH_CLASS:
		USB_RH_PRINTF("Clear Feature Class ");
		break;
	case RH_CLEAR_FEATURE | RH_OTHER | RH_CLASS:
		USB_RH_PRINTF("Clear Feature Other Class ");
		break;
	case RH_SET_FEATURE | RH_OTHER | RH_CLASS:
		USB_RH_PRINTF("Set Feature Other Class ");
		break;
	case RH_SET_ADDRESS:
		USB_RH_PRINTF("Set Address ");
		break;
	case RH_GET_DESCRIPTOR:
		USB_RH_PRINTF("Get Descriptor ");
		break;
	case RH_GET_DESCRIPTOR | RH_CLASS:
		USB_RH_PRINTF("Get Descriptor Class ");
		break;
	case RH_GET_CONFIGURATION:
		USB_RH_PRINTF("Get Configuration ");
		break;
	case RH_SET_CONFIGURATION:
		USB_RH_PRINTF("Get Configuration ");
		break;
	default:
		USB_RH_PRINTF("****UNKNOWN**** 0x%04X ",req);
	}
	USB_RH_PRINTF("\n");

}

static void usb_display_wValue(unsigned short wValue,unsigned short wIndex)
{
	switch (wValue) {
		case (RH_PORT_ENABLE):
			USB_RH_PRINTF("Root-Hub: Enable Port %d\n",wIndex);
			break;
		case (RH_PORT_SUSPEND):
			USB_RH_PRINTF("Root-Hub: Suspend Port %d\n",wIndex);
			break;
		case (RH_PORT_POWER):
			USB_RH_PRINTF("Root-Hub: Port Power %d\n",wIndex);
			break;
		case (RH_C_PORT_CONNECTION):
			USB_RH_PRINTF("Root-Hub: C Port Connection Port %d\n",wIndex);
			break;
		case (RH_C_PORT_ENABLE):
			USB_RH_PRINTF("Root-Hub: C Port Enable Port %d\n",wIndex);
			break;
		case (RH_C_PORT_SUSPEND):
			USB_RH_PRINTF("Root-Hub: C Port Suspend Port %d\n",wIndex);
			break;
		case (RH_C_PORT_OVER_CURRENT):
			USB_RH_PRINTF("Root-Hub: C Port Over Current Port %d\n",wIndex);
			break;
		case (RH_C_PORT_RESET):
			USB_RH_PRINTF("Root-Hub: C Port reset Port %d\n",wIndex);
			break;
		default:
			USB_RH_PRINTF("Root-Hub: unknown %x %x\n",wValue,wIndex);
			break;
	}
}

#endif


#ifdef	USB_UHCI_DEBUG

static int usb_display_td(uhci_td_t *td)
{
	unsigned long tmp;
	int valid;

	printf("TD at %p:\n",td);

	tmp=swap_32(td->link);
	printf("Link points to 0x%08lX, %s first, %s, %s\n",tmp&0xfffffff0,
		((tmp & 0x4)==0x4) ? "Depth" : "Breath",
		((tmp & 0x2)==0x2) ? "QH" : "TD",
		((tmp & 0x1)==0x1) ? "invalid" : "valid");
	valid=((tmp & 0x1)==0x0);
	tmp=swap_32(td->status);
	printf("     %s %ld Errors %s %s %s \n     %s %s %s %s %s %s\n     Len 0x%lX\n",
		(((tmp>>29)&0x1)==0x1) ? "SPD Enable" : "SPD Disable",
		((tmp>>28)&0x3),
		(((tmp>>26)&0x1)==0x1) ? "Low Speed" : "Full Speed",
		(((tmp>>25)&0x1)==0x1) ? "ISO " : "",
		(((tmp>>24)&0x1)==0x1) ? "IOC " : "",
		(((tmp>>23)&0x1)==0x1) ? "Active " : "Inactive ",
		(((tmp>>22)&0x1)==0x1) ? "Stalled" : "",
		(((tmp>>21)&0x1)==0x1) ? "Data Buffer Error" : "",
		(((tmp>>20)&0x1)==0x1) ? "Babble" : "",
		(((tmp>>19)&0x1)==0x1) ? "NAK" : "",
		(((tmp>>18)&0x1)==0x1) ? "Bitstuff Error" : "",
		(tmp&0x7ff));
	tmp=swap_32(td->info);
	printf("     MaxLen 0x%lX\n",((tmp>>21)&0x7FF));
	printf("     %s Endpoint 0x%lX Dev Addr 0x%lX PID 0x%lX\n",((tmp>>19)&0x1)==0x1 ? "TOGGLE" : "",
		((tmp>>15)&0xF),((tmp>>8)&0x7F),tmp&0xFF);
	tmp=swap_32(td->buffer);
	printf("     Buffer 0x%08lX\n",tmp);
	printf("     DEV %08lX\n",td->dev_ptr);
	return valid;
}


void usb_show_td(uhci_t *uhci,int max)
{
	int i;
	if(max>0) {
		for(i=0;i<max;i++) {
			usb_display_td(&tmp_td[i]);
		}
	}
	else {
		i=0;
		do {
			printf("tmp_td[%d]\n",i);
		}while(usb_display_td(&tmp_td[i++]));
	}
}

#endif

#include <pmon.h>

static int uhci_debug (int argc, char *argv[])
{
	if(argc < 2){
		if(!uhci_stop)
			printf("uhci is running\n");
		return 0;
	}	

	if(!strcmp(argv[1],"start")){
		uhci_stop = 0;
	}
	else if(!strcmp(argv[1], "stop"))
	{
	        usb_uhci_stop();
		uhci_stop = 1;
	}
	else {
		printf("usage: uhci [stop|start]\n");
	}
	return 0;
}

static const Cmd Cmds[] = {
	{"UHCI debug"},
	{"uhci", "", NULL, "uhci debugger commands", uhci_debug, 1, 2, 0},
	{0, 0}
};

static void init_cmd(void) __attribute__((constructor));

static void init_cmd(void)
{
	printf("init uhci cmd called\n");
	cmdlist_expand(Cmds, 0);
}
/* EOF */
