/*	$Id: i386_pci.c,v 1.1.1.1 2006/09/14 01:59:08 root Exp $ */

/*
 * Copyright (c) 2002 Patrik Lindergren  (www.lindergren.com)
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
 *      This product includes software developed by Patrik Lindergren.
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

#include <sys/param.h>
#include <sys/device.h>
#include <sys/systm.h>

#include <dev/pci/pcivar.h>
#include <dev/pci/pcireg.h>
#include <pmon/dev/elan520.h>
#include <pmon.h>
#include <stdlib.h>

extern int _pciverbose;

/*
 *  Make pci tag from bus, device and function data.
 */
pcitag_t
_pci_make_tag(bus, device, function)
	int bus;
	int device;
	int function;
{
	pcitag_t tag;

	tag = (bus << 16) | (device << 11) | (function << 8);
	return(tag);
}

/*
 *  Break up a pci tag to bus, device function components.
 */
void
_pci_break_tag(tag, busp, devicep, functionp)
	pcitag_t tag;
	int *busp;
	int *devicep;
	int *functionp;
{
	if (busp) {
		*busp = (tag >> 16) & 255;
	}
	if (devicep) {
		*devicep = (tag >> 11) & 31;
	}
	if (functionp) {
		*functionp = (tag >> 8) & 7;
	}
}

/*
 *  Read a value form PCI configuration space.
 */
pcireg_t
_pci_conf_read(tag, reg)
	pcitag_t tag;
	int reg;
{
	pcireg_t data;
	u_int32_t addr;
	u_int32_t addrp;
	int bus, device, function;

	if (reg & 3 || reg < 0 || reg >= 0x100) {
		if (_pciverbose >= 1) {
			_pci_tagprintf (tag, "_pci_conf_read: bad reg 0x%x\r\n", reg);
		}
		return ~0;
	}

	_pci_break_tag (tag, &bus, &device, &function); 

	/* Type 1 configuration for PCI bus */
	if (bus > 255 || device > 31 || function > 7) {
		return ~0;		/* device out of range */
	}
	addr = (bus << 16) | (device << 11) | (function << 8) | reg;

	addrp = 0x80000000 + addr;
	outl(ELAN520_PCICFGADDR, addrp);
        
	data = (pcireg_t)inl(ELAN520_PCICFGDATA + (reg & 3));

/* XXX We should prolly check error status here but what the... */

	return data;
}

/*
 *  Write a value to PCI configuration space. Support for
 *  all three data sizes (byte, halfword and word) is provided.
 */
void
_pci_conf_write(tag, reg, data)
	pcitag_t tag;
	int reg;
	pcireg_t data;
{
	u_int32_t addr;
	u_int32_t addrp;
	int bus, device, function;

	if (reg & 3 || reg < 0 || reg >= 0x100) {
		if (_pciverbose >= 1) { 
			_pci_tagprintf(tag, "_pci_conf_write: bad reg %x\r\n", reg);
		}
		return;
	}

	_pci_break_tag (tag, &bus, &device, &function);

	/* Type 1 configuration for PCI bus */
	if (bus > 255 || device > 31 || function > 7) {
		return;		/* device out of range */
	}
	addr = (bus << 16) | (device << 11) | (function << 8) | reg;

	addrp = 0x80000000 + addr;
	outl(ELAN520_PCICFGADDR, addrp);

	outl(ELAN520_PCICFGDATA + (reg & 3), data);

/* XXX We should prolly check error status here but what the... */

}
