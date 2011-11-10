/*	$Id: pocono.c,v 1.1.1.1 2006/09/14 01:59:08 root Exp $ */

/*
 * Copyright (c) 2001 ipUnplugged AB   (www.ipunplugged.com)
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
 *	This product includes software developed by ipUnplugged AB
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
 */

#include <sys/param.h>
#include <sys/device.h>
#include <sys/systm.h>

#include <dev/pci/pcivar.h>
#include <dev/pci/pcireg.h>

#include <pmon.h>

#include <stdlib.h>

#ifdef CPC700
static struct pci_intline_routing pri_pci_bus = {
   0, 0, 0, /* Northbridge controller */
   /*
   { Dev 0, Dev 1, Dev 2, Dev 3, Dev 4, Dev 5, Dev 6, Dev 7,} 
   {     0,     0,  PMC2,     0, Eth 0, Eth 1, 371AB,   PPB,}
   {     0,     0,    #D,     0,    #B,    #A,    #D,     0,}*/
   { PCI_INT_0, PCI_INT_0, PCI_INT_C, PCI_INT_0, PCI_INT_B, PCI_INT_A, PCI_INT_D, PCI_INT_0,
     PCI_INT_0, PCI_INT_0, PCI_INT_0, PCI_INT_0, PCI_INT_0, PCI_INT_0, PCI_INT_0, PCI_INT_0,
     PCI_INT_0, PCI_INT_0, PCI_INT_0, PCI_INT_0, PCI_INT_0, PCI_INT_0, PCI_INT_0, PCI_INT_0,
     PCI_INT_0, PCI_INT_0, PCI_INT_0, PCI_INT_0, PCI_INT_0, PCI_INT_0, PCI_INT_0, PCI_INT_0 },
   NULL
};

static struct pci_intline_routing sec_pci_bus = {
   0, 7, 0, /* PCI-PCI bridge on Main board */
   /*
   { Dev 0,     Dev 1,      Dev 2,     Dev 3,     Dev 4,    Dev 5,     Dev 6,     Dev 7,} 
   {     0,      PMC0,       PMC1,     Eth 2,     Eth 3,        0,         0,         0,}
   {     0,        #D,         #C,        #B,        #A,        0,         0,         0,}*/
   { PCI_INT_0, PCI_INT_D, PCI_INT_C, PCI_INT_B, PCI_INT_A, PCI_INT_0, PCI_INT_0, PCI_INT_0,
     PCI_INT_0, PCI_INT_0, PCI_INT_0, PCI_INT_0, PCI_INT_0, PCI_INT_0, PCI_INT_0, PCI_INT_0,
     PCI_INT_0, PCI_INT_0, PCI_INT_0, PCI_INT_0, PCI_INT_0, PCI_INT_0, PCI_INT_0, PCI_INT_0,
     PCI_INT_0, PCI_INT_0, PCI_INT_0, PCI_INT_0, PCI_INT_0, PCI_INT_0, PCI_INT_0, PCI_INT_0 },
   NULL

};
#elif defined(MPC107)
static struct pci_intline_routing pri_pci_bus = {
   0, 0, 0, /* Northbridge controller */
   { PCI_INT_0, PCI_INT_0, PCI_INT_0, PCI_INT_0, PCI_INT_0, PCI_INT_0, PCI_INT_0, PCI_INT_0,
     PCI_INT_0, PCI_INT_0, PCI_INT_0, PCI_INT_0, PCI_INT_C, PCI_INT_0, PCI_INT_B, PCI_INT_A,
     PCI_INT_D, PCI_INT_0, PCI_INT_0, PCI_INT_0, PCI_INT_0, PCI_INT_0, PCI_INT_0, PCI_INT_0,
     PCI_INT_0, PCI_INT_0, PCI_INT_0, PCI_INT_0, PCI_INT_0, PCI_INT_0, PCI_INT_0, PCI_INT_0 },
   NULL
};

static struct pci_intline_routing sec_pci_bus = {
   0, 17, 0, /* PCI-PCI bridge on Main board */
   { PCI_INT_0, PCI_INT_D, PCI_INT_C, PCI_INT_B, PCI_INT_A, PCI_INT_0, PCI_INT_0, PCI_INT_0,
     PCI_INT_0, PCI_INT_0, PCI_INT_0, PCI_INT_0, PCI_INT_0, PCI_INT_0, PCI_INT_0, PCI_INT_0,
     PCI_INT_0, PCI_INT_0, PCI_INT_0, PCI_INT_0, PCI_INT_0, PCI_INT_0, PCI_INT_0, PCI_INT_0,
     PCI_INT_0, PCI_INT_0, PCI_INT_0, PCI_INT_0, PCI_INT_0, PCI_INT_0, PCI_INT_0, PCI_INT_0 },
   NULL

};
#else
#error "You need to define atleast one Host-Bridge"
#endif

int InitInterruptRouting(void)
{
      pri_pci_bus.next = &sec_pci_bus;
      _pci_inthead = &pri_pci_bus;

      return(0);
}
