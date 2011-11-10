/* $Id: mc146818.c,v 1.1.1.1 2006/09/14 01:59:08 root Exp $ */

/*
 * Copyright (c) 2001 ipUnplugged AB (www.ipunplugged.com)
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
 *	This product includes software developed for Patrik Lindergren, by
 *	ipUnplugged AB, Sweden.
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
#include <stdio.h>
#include <setjmp.h>
#include <termio.h>
#include <string.h>
#include <time.h>

#include <machine/cpu.h>
#include <machine/pio.h>
#include <pmon/dev/mc146818reg.h>
#include <pmon.h>

static int clkread __P((int));
static void clkwrite __P((int, int));

static int
clkread(reg)
	int reg;
{
	outb(CLOCK_ADDR, reg);
	return(inb(CLOCK_DATA));
}

static void
clkwrite(reg, data)
	int reg;
	int data;
{
	outb(CLOCK_ADDR, reg);
	outb(CLOCK_DATA, data);
}

#define FROMBCD(x)      (((x) >> 4) * 10 + ((x) & 0xf))
#define TOBCD(x)        (((x) / 10 * 16) + ((x) % 10)) 

/*
 * Initialize Real-time Clock
 */
void
tgt_rtinit()
{
	/* Enable register writing */
	clkwrite(MC_REGB, clkread(MC_REGB) | MC_REGB_SET);

        clkwrite(MC_REGA, (clkread(MC_REGA) & 0x8F) | MC_BASE_32_KHz);
        clkwrite(MC_REGB, clkread(MC_REGB) | MC_REGB_24HR);
        clkwrite(MC_REGB, clkread(MC_REGB) & ~MC_REGB_BINARY);

	/* Transfer new time to counters */
	clkwrite(MC_REGB, clkread(MC_REGB) & ~MC_REGB_SET);
}

int
tgt_onesecond(int timeout)
{
	int second;

	second = clkread(MC_SEC);
	while (timeout--) {
		if (second != clkread(MC_SEC))
			break;
	}
	return(timeout);
}

int
tgt_clockram_write(char *data, int offset, int nbytes)
{
	while (nbytes--) {
		clkwrite(offset, *data);
		data++;
		offset++;
	}
	return(0);
}

int
tgt_clockram_read(char *data, int offset, int nbytes)
{
	while (nbytes--) {
		*data++ = clkread(offset);
		offset++;
	}
	return(0);
}


time_t
tgt_gettime()
{
	struct tm tm;
	time_t t;
	int trys = 100000;

	/* Wait for end of update in progress */
	while(trys-- > 0 && clkread(MC_REGA) & MC_REGA_UIP);

	tm.tm_sec = FROMBCD(clkread(MC_SEC));
	tm.tm_min = FROMBCD(clkread(MC_MIN));
	tm.tm_hour = FROMBCD(clkread(MC_HOUR));
	tm.tm_wday = FROMBCD(clkread(MC_DOW));
	tm.tm_mday = FROMBCD(clkread(MC_DOM));
	tm.tm_mon = FROMBCD(clkread(MC_MONTH)) - 1;
	tm.tm_year = FROMBCD(clkread(MC_YEAR));

	tm.tm_isdst = tm.tm_gmtoff = 0;
	if(tm.tm_year < 90) {
		tm.tm_year += 100;
	}
	t = gmmktime(&tm);
        return(t);
}

/*
 *  Set the current time if a TOD clock is present
 */
void
tgt_settime(time_t t)
{
	struct tm *tm;

	tm = gmtime(&t);

	/* Enable register writing */
	clkwrite(MC_REGB, clkread(MC_REGB) | MC_REGB_SET);

        clkwrite(MC_YEAR, TOBCD(tm->tm_year % 100));
        clkwrite(MC_MONTH, TOBCD(tm->tm_mon + 1));
        clkwrite(MC_DOM, TOBCD(tm->tm_mday));
        clkwrite(MC_DOW, TOBCD(tm->tm_wday));
        clkwrite(MC_HOUR, TOBCD(tm->tm_hour));
        clkwrite(MC_MIN, TOBCD(tm->tm_min));
        clkwrite(MC_SEC, TOBCD(tm->tm_sec));

	/* Transfer new time to counters */
	clkwrite(MC_REGB, clkread(MC_REGB) & ~MC_REGB_SET);
}

