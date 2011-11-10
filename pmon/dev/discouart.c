/* $Id: discouart.c,v 1.1.1.1 2006/09/14 01:59:08 root Exp $ */

/*
 * Copyright (c) 2002 Opsycon AB  (www.opsycon.se)
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
 *	This product includes software developed by Opsycon AB, Sweden.
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
#include <termio.h>
#include <machine/pio.h>
#include <machine/bus.h>
#include <pmon.h>

extern int getbaudval __P((int));

int discouart (int op, struct DevEntry *dev, unsigned long param, int data);

static int
discoinit (volatile void *dp)
{
	return 0;
}

static int
discoprogram (volatile void *dp, unsigned long freq, int baudrate)
{
#if 0
static int rates[] = {
	50, 75, 110, 134, 150, 200, 300, 600, 1200, 1800, 2400,
	4800, 9600, 19200, 38400, 57600, 115200, 0 };

	unsigned short brtc;
	int timeout, *p;

	/* wait for Tx fifo to completely drain */
	timeout = 1000000;
	while (!(inb(&dp->lsr) & LSR_TSRE)) {
		if (--timeout == 0)
			break;
	}

	baudrate = getbaudval (baudrate);
	for (p = rates; *p; p++)
		if (*p == baudrate)
			break;
	if (*p == 0)
		return 1;

	brtc = (freq) / (16*(baudrate));

	outb(&dp->cfcr, CFCR_DLAB);
	outb(&dp->data, brtc & 0xff);
	outb(&dp->ier, brtc >> 8);
	outb(&dp->cfcr, CFCR_8BITS);
	outb(&dp->mcr, MCR_DTR | MCR_RTS);
	outb(&dp->ier, 0);
#endif
	return 0;
}


int
discouart (int op, struct DevEntry *dev, unsigned long param, int data)
{
	volatile void *dp = (volatile void *) dev->sio;
	int tmp;

	switch (op) {
		case OP_INIT:
			/* Kill any SDMA activity on this channel */
			GT_WRITE(CHANNEL0_COMMAND_REGISTER, 0x00008000);
			/* Setup debug tty */
			GT_WRITE(CHANNEL0_REGISTER4, 0x20000000);
			GT_WRITE(CHANNEL0_REGISTER5, 0x00009000);
			GT_WRITE(CHANNEL0_REGISTER2,
				 GT_READ(CHANNEL0_REGISTER2) | 0x80000000);
			return discoinit (dp);

		case OP_XBAUD:
		case OP_BAUD:
			return discoprogram (dp, dev->freq, data);

		case OP_TXRDY:
			return (!(GT_READ(CHANNEL0_REGISTER2) & 0x200));

		case OP_TX:
			GT_WRITE(CHANNEL0_REGISTER1, data);
			GT_WRITE(CHANNEL0_REGISTER2,
				 GT_READ(CHANNEL0_REGISTER2) | 0x200);
			break;

		case OP_RXRDY:
			return(GT_READ(MPSC0_CAUSE) & 0x0040);

		case OP_RX:
			tmp = GT_READ(CHANNEL0_REGISTER10);
			GT_WRITE(CHANNEL0_REGISTER10, tmp);
			GT_WRITE(MPSC0_CAUSE, 0xffffffbf);
			return (tmp >> 16) & 0xff;

#if 0
		case OP_RXSTOP:
			if (data)
				outb(&dp->mcr, inb(&dp->mcr) & ~MCR_RTS);
			else
				outb(&dp->mcr, inb(&dp->mcr) | MCR_RTS);
			break;
#endif
	}
	return 0;
}
