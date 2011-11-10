/*	$Id: cpc700_iic.c,v 1.1.1.1 2006/09/14 01:59:08 root Exp $ */

/*
 * Copyright (c) 2000 Opsycon AB  (www.opsycon.se)
 * Copyright (c) 2000 Rtmx, Inc   (www.rtmx.com)
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
 *	This product includes software developed for Rtmx, Inc by
 *	Opsycon Open System Consulting AB, Sweden.
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

#include <sys/types.h>
#include <machine/pio.h>

#include <pmon/dev/cpc700.h>
#include <pmon/dev/cpc700_iic.h>

/*
 *  Functions to read and write the EEPROM located on the CPC700 IIC bus.
 */

static __inline int
cpc700_iic_wrdy(volatile u_char *iic_base, int offset)
{
	int tout;

	outb(iic_base + CPC700_IIC_STS, 0x0a);
	tout = 2000;
	while(tout-- != 0 && inb(iic_base + CPC700_IIC_STS) & 1);
		if(tout <= 0) {
		return(-1);
	}
	outb(iic_base + CPC700_IIC_MDCNTL, inb(iic_base + CPC700_IIC_MDCNTL) | 0x40);
	outb(iic_base + CPC700_IIC_LMADR, 0xa0 | (offset & 0x700) >> 7);
	outb(iic_base + CPC700_IIC_MDBUF, 0x00);
	outb(iic_base + CPC700_IIC_CNTL, 0x01);

	outb(iic_base + CPC700_IIC_STS, 0x0a);
	tout = 2000;
	while(tout-- != 0 && inb(iic_base + CPC700_IIC_STS) & 1);
	if(tout <= 0 || inb(iic_base + CPC700_IIC_STS) != 8) {
		return(-1);
	}
	return(0);
}

int
cpc700_iic_read(char *buf, int offset, int length)
{
	volatile u_char *iic_base;
	int tout;
	int byte;

	iic_base = (volatile u_char *)CPC700_IIC0_MDBUF;

	for(byte = offset; byte < offset + length; byte++) {
		outb(iic_base + CPC700_IIC_STS, 0x0a);
		tout = 2000;
		while(tout-- != 0 && inb(iic_base + CPC700_IIC_STS) & 1);
		if(tout <= 0) {
			return(-1);
		}

		outb(iic_base + CPC700_IIC_MDCNTL, inb(iic_base + CPC700_IIC_MDCNTL) | 0x40);
		outb(iic_base + CPC700_IIC_LMADR, 0xa0 | (byte & 0x700) >> 7);
		outb(iic_base + CPC700_IIC_MDBUF, byte & 0x0ff);
		outb(iic_base + CPC700_IIC_CNTL, 0x01);
		tout = 2000;
		while(tout-- != 0 && inb(iic_base + CPC700_IIC_STS) & 1);
		if(tout <= 0) {
			return(-1);
		}
		outb(iic_base + CPC700_IIC_STS, 0x0a);
		tout = 2000;
		while(tout-- != 0 && inb(iic_base + CPC700_IIC_STS) & 1);
		if(tout <= 0) {
			return(-1);
		}
		outb(iic_base + CPC700_IIC_MDCNTL, inb(iic_base + CPC700_IIC_MDCNTL) | 0x40);
		outb(iic_base + CPC700_IIC_LMADR, 0xa1 | (byte & 0x700) >> 7);
		outb(iic_base + CPC700_IIC_CNTL, 0x0b);
		tout = 2000;
		while(tout-- != 0 && inb(iic_base + CPC700_IIC_STS) & 1);
		if(tout <= 0) {
			outb(iic_base + CPC700_IIC_XTCNTLSS, 0x01);
			outb(iic_base + CPC700_IIC_XTCNTLSS, 0x00);
			outb(iic_base + CPC700_IIC_EXTSTS, 0x8f);
			outb(iic_base + CPC700_IIC_CLKDIV, 0x03);
			outb(iic_base + CPC700_IIC_MDCNTL, 0x53);
			return(-1);
		}
		*buf++ = inb(iic_base + CPC700_IIC_MDBUF);
	}
	return(0);
}

int
cpc700_iic_write(char *buf, int offset, int length)
{
	volatile u_char *iic_base;
	int tout;
	int byte;

	iic_base = (volatile u_char *)CPC700_IIC0_MDBUF;

	for(byte = offset; byte < offset + length; byte++) {
		
		tout = 2000;
		while(tout-- != 0 && cpc700_iic_wrdy(iic_base, byte));

		outb(iic_base + CPC700_IIC_STS, 0x0a);
		tout = 2000;
		while(tout-- != 0 && inb(iic_base + CPC700_IIC_STS) & 1);
		if(tout <= 0) {
			return(-1);
		}
		outb(iic_base + CPC700_IIC_MDCNTL, inb(iic_base + CPC700_IIC_MDCNTL) | 0x40);
		outb(iic_base + CPC700_IIC_LMADR, 0xa0 | (byte & 0x700) >> 7);
		outb(iic_base + CPC700_IIC_MDBUF, byte & 0x0ff);
		outb(iic_base + CPC700_IIC_MDBUF, *buf++);
		outb(iic_base + CPC700_IIC_CNTL, 0x11);
		tout = 2000;
		while(tout-- != 0 && inb(iic_base + CPC700_IIC_STS) & 1);
		if(tout <= 0) {
			return(-1);
		}
	}
	return(0);
}


