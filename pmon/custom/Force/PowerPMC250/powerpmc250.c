/*	$Id: powerpmc250.c,v 1.1.1.1 2006/09/14 01:59:08 root Exp $ */

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

static struct pci_intline_routing pri_pci_bus = {
   0, 0, 0, /* Northbridge controller */
   { {PCI_INT_0, PCI_INT_0, PCI_INT_0, PCI_INT_0},	/* PCI Slot  0: */
   {PCI_INT_0, PCI_INT_0, PCI_INT_0, PCI_INT_0},	/* PCI Slot  1: */
   {PCI_INT_0, PCI_INT_0, PCI_INT_0, PCI_INT_0},	/* PCI Slot  2: */
   {PCI_INT_0, PCI_INT_0, PCI_INT_0, PCI_INT_0}, 	/* PCI Slot  3: */
   {PCI_INT_0, PCI_INT_0, PCI_INT_0, PCI_INT_0},	/* PCI Slot  4: */
   {PCI_INT_0, PCI_INT_0, PCI_INT_0, PCI_INT_0}, 	/* PCI Slot  5: */
   {PCI_INT_0, PCI_INT_0, PCI_INT_0, PCI_INT_0},	/* PCI Slot  6: */
   {PCI_INT_0, PCI_INT_0, PCI_INT_0, PCI_INT_0},	/* PCI Slot  7: */
   {PCI_INT_0, PCI_INT_0, PCI_INT_0, PCI_INT_0},	/* PCI Slot  8: */
   {PCI_INT_0, PCI_INT_0, PCI_INT_0, PCI_INT_0},	/* PCI Slot  9: */
   {PCI_INT_0, PCI_INT_0, PCI_INT_0, PCI_INT_0},	/* PCI Slot 10: */
   {PCI_INT_0, PCI_INT_0, PCI_INT_0, PCI_INT_0},	/* PCI Slot 11: */
   {PCI_INT_0, PCI_INT_0, PCI_INT_0, PCI_INT_0},	/* PCI Slot 12: */
   {PCI_INT_0, PCI_INT_0, PCI_INT_0, PCI_INT_0},	/* PCI Slot 13: */
   {PCI_INT_0, PCI_INT_0, PCI_INT_0, PCI_INT_0},	/* PCI Slot 14: */
   {PCI_INT_0, PCI_INT_0, PCI_INT_0, PCI_INT_0},	/* PCI Slot 15: */
   {PCI_INT_0, PCI_INT_0, PCI_INT_0, PCI_INT_0},	/* PCI Slot 16: */
   {PCI_INT_0, PCI_INT_0, PCI_INT_0, PCI_INT_0},	/* PCI Slot 17: */
   {PCI_INT_0, PCI_INT_0, PCI_INT_0, PCI_INT_0},	/* PCI Slot 18: */
   {PCI_INT_0, PCI_INT_0, PCI_INT_0, PCI_INT_0},	/* PCI Slot 19: */
   {PCI_INT_0, PCI_INT_0, PCI_INT_0, PCI_INT_0},	/* PCI Slot 20: */
   {PCI_INT_0, PCI_INT_0, PCI_INT_0, PCI_INT_0},	/* PCI Slot 21: */
   {PCI_INT_0, PCI_INT_0, PCI_INT_0, PCI_INT_0},	/* PCI Slot 22: */
   {PCI_INT_0, PCI_INT_0, PCI_INT_0, PCI_INT_0},	/* PCI Slot 23: */
   {PCI_INT_0, PCI_INT_0, PCI_INT_0, PCI_INT_0},	/* PCI Slot 24: */
   {PCI_INT_0, PCI_INT_0, PCI_INT_0, PCI_INT_0},	/* PCI Slot 25: */
   {PCI_INT_0, PCI_INT_0, PCI_INT_0, PCI_INT_0},	/* PCI Slot 26: */
   {PCI_INT_0, PCI_INT_0, PCI_INT_0, PCI_INT_0},	/* PCI Slot 27: */
   {PCI_INT_0, PCI_INT_0, PCI_INT_0, PCI_INT_0},	/* PCI Slot 28: */
   {PCI_INT_0, PCI_INT_0, PCI_INT_0, PCI_INT_0},	/* PCI Slot 29: */
   {PCI_INT_0, PCI_INT_0, PCI_INT_0, PCI_INT_0},	/* PCI Slot 30: */
   {PCI_INT_0, PCI_INT_0, PCI_INT_0, PCI_INT_0} },	/* PCI Slot 31: */
   NULL
};

int InitInterruptRouting(void)
{
      _pci_inthead = &pri_pci_bus;

      return(0);
}
