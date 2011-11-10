/*	$Id: ds17285reg.h,v 1.1.1.1 2006/09/14 01:59:08 root Exp $ */

/*
 * Copyright (c) 2002 Opsycon AB  (www.opsycon.se / www.opsycon.com)
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
#ifndef _DS_17285REG_H_
#define _DS_17285REG_H_

/*
 *   Register definitions for Dallas Semiconductor DS17285/287 clock chips.
 */

#define	DS_REG_SEC	0x00		/* Seconds reg */
#define	DS_REG_ASEC	0x01		/* Alarm Seconds reg */
#define	DS_REG_MIN	0x02		/* Minutes reg */
#define	DS_REG_AMIN	0x03		/* Alarm Minutes reg */
#define	DS_REG_HOUR	0x04		/* Hours reg */
#define	DS_REG_AHOUR	0x05		/* Alarm Hours reg */
#define	DS_REG_WDAY	0x06		/* Day of week reg */
#define	DS_REG_DATE	0x07		/* Day of month reg */
#define	DS_REG_MONTH	0x08		/* Month reg */
#define	DS_REG_YEAR	0x09		/* Year reg */

#define	DS_REG_CTLA	0x0a		/* Control reg A */
#define	DS_REG_CTLB	0x0b		/* Control reg B */
#define	DS_REG_CTLC	0x0c		/* Control reg A */
#define	DS_REG_CTLD	0x0d		/* Control reg B */


#define	DS_MONTH_E32K	0x40		/* Enable 32Khz out when 0 */
#define	DS_MONTH_EOSC	0x80		/* Enable oscilator when 0 */

#define	DS_CTLA_RS0	0x01		/* Rate Select */
#define	DS_CTLA_RS1	0x02		/* Rate Select */
#define	DS_CTLA_RS2	0x04		/* Rate Select */
#define	DS_CTLA_RS3	0x08		/* Rate Select */
#define	DS_CTLA_DV0	0x10		/* Bank Select */
#define	DS_CTLA_DV1	0x20		/* Osc. Enable */
#define	DS_CTLA_DV2	0x40		/* Countdown Chain */
#define	DS_CTLA_UIP	0x80		/* Update In Progress flag 1 */

#define DS_CTLB_DSE	0x01		/* Daylight savings enable */
#define	DS_CTLB_24	0x02		/* 24hr enable */
#define	DS_CTLB_SQWE	0x04		/* Square wave enable */
#define	DS_CTLB_DM	0x08		/* Data mode */
#define	DS_CTLB_UIE	0x10		/* Update ended interrupt enable */
#define	DS_CTLB_AIE	0x20		/* Alarm interrupt enable */
#define	DS_CTLB_PIE	0x40		/* Periodic interrupt enable */
#define	DS_CTLB_SET	0x80		/* Set registers enable */

/*
 *  Registers in Bank 1
 */
#define	DS_EXRAM_LSB	0x50		/* Extended RAM LSB addr */
#define	DS_EXRAM_MSB	0x51		/* Extended RAM MSB addr */
#define	DS_EXRAM_DATA	0x53		/* Extended RAM Data port */

#endif /* _DS_17285REG_H_ */
