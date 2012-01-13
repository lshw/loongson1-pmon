/*
 * (C) Copyright 2009 Freescale Semiconductor, Inc.
 *
 * (C) Copyright 2008, Excito Elektronik i Sk=E5ne AB
 *
 * Author: Tor Krill tor@excito.com
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */

#if 1
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

#else
#include <common.h>
#include <pci.h>
#include <usb.h>
#include <asm/io.h>
#endif


#include "ehci-fsl.h"
#include "ehci.h"
#include "ehci-core.h"

/*
 * Create the appropriate control structures to manage
 * a new EHCI host controller.
 *
 * Excerpts from linux ehci fsl driver.
 */

#define SYS_FSL_USB_ADDR  	0xbfe00000   //anson add  08-24
#define	EHCI_OPERATIONAL	0xbfe00010


void delay_us(microseconds)
	int microseconds;
{
	int total, start ,start1;
#if 1
/*
	start = CPU_GetCOUNT();
	start1 = CPU_GetCOUNT();
	if(start!=start1)
	{
		total = microseconds * clkperusec;
		while(total > (CPU_GetCOUNT() - start));
	}
	else	*/
	 	for(start1=0;start1<microseconds;start1++)
	 		*(volatile char *)0xbfc00000;
#else
	total = microseconds * 200;
	loopNinstr(total);
#endif

}
void delay_ms(int ms){
     int i=1000;
	 for(i=1000;i>0;i--){
		 delay_us(ms);

	 }

}


int ehci_hcd_init(void)
{
	struct usb_ehci *ehci;

//	ehci = (struct usb_ehci *)CONFIG_SYS_FSL_USB_ADDR;
//	hccr = (struct ehci_hccr *)((uint32_t)&ehci->caplength);
//	hcor = (struct ehci_hcor *)((uint32_t) hccr + HC_LENGTH(ehci_readl(&hccr->cr_capbase)));
	hccr = (struct ehci_hccr *)((uint32_t)SYS_FSL_USB_ADDR);	//lxy
	hcor = (struct ehci_hcor *)((uint32_t) hccr + HC_LENGTH(ehci_readl(&hccr->cr_capbase)));

	*(volatile unsigned int *)0xbfd00424 &= ~(0x80000000);
	*(volatile unsigned int *)0xbfd00424;
	delay_ms(1);
	/*ls1b usb reset stop*/
	*(volatile unsigned int *)0xbfd00424 |= 0x80000000;
//	hcor = (struct ehci_hcor *)((uint32_t)EHCI_OPERATIONAL);
//	ehci_writel(&hcor->or_usbcmd, CMD_ASE | CMD_RUN);		//lxy
	

    /*                                                               //anson mask 08-25
	// Set to Host mode 
	setbits_le32(&ehci->usbmode, CM_HOST);

	out_be32(&ehci->snoop1, SNOOP_SIZE_2GB);
	out_be32(&ehci->snoop2, 0x80000000 | SNOOP_SIZE_2GB);

	// Init phy 
	if (!strcmp(getenv("usb_phy_type"), "utmi"))
		out_le32(&(hcor->or_portsc[0]), PORT_PTS_UTMI);
	else
		out_le32(&(hcor->or_portsc[0]), PORT_PTS_ULPI);

	// Enable interface.
	setbits_be32(&ehci->control, USB_EN);

	out_be32(&ehci->prictrl, 0x0000000c);
	out_be32(&ehci->age_cnt_limit, 0x00000040);
	out_be32(&ehci->sictrl, 0x00000001);

	in_le32(&ehci->usbmode);
*/
	return 0;
}

/*
 * Destroy the appropriate control structures corresponding
 * the the EHCI host controller.
 */
int ehci_hcd_stop(void)
{
	return 0;
}
