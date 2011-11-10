/*	$Id: ds15x1reg.h,v 1.1.1.1 2006/09/14 01:59:08 root Exp $ */

/*
 * Copyright (c) 2001 Opsycon AB  (www.opsycon.se / www.opsycon.com)
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
#ifndef _DS_15x1REG_H_
#define _DS_15x1REG_H_

/*
 *   Register definitions for Dallas Semiconductor DS15x1 clock chips.
 */

#define	DS_REG_SEC	0x00		/* Seconds reg */
#define	DS_REG_MIN	0x01		/* Minutes reg */
#define	DS_REG_HOUR	0x02		/* Hours reg */
#define	DS_REG_WDAY	0x03		/* Day of week reg */
#define	DS_REG_DATE	0x04		/* Day of month reg */
#define	DS_REG_MONTH	0x05		/* Month reg */
#define	DS_REG_YEAR	0x06		/* Year reg */
#define	DS_REG_CENT	0x07		/* Century reg */

#define	DS_REG_ASEC	0x08		/* Seconds reg */
#define	DS_REG_AMIN	0x09		/* Minutes reg */
#define	DS_REG_AHOUR	0x0a		/* Hours reg */
#define	DS_REG_ADATE	0x0b		/* Date reg */

#define	DS_REG_WDMS	0x0c		/* Watchdog millisec */
#define	DS_REG_WDSEC	0x0d		/* Watchdog seconds */

#define	DS_MONTH_E32K	0x40		/* Enable 32Khz out when 0 */
#define	DS_MONTH_EOSC	0x80		/* Enable oscilator when 0 */

#define	DS_REG_CTLA	0x0e		/* Control reg A */
#define	DS_REG_CTLB	0x0f		/* Control reg B */

#define	DS_CTLA_IRQF	0x01		/* IRQ Flag */
#define	DS_CTLA_WDF	0x02		/* Watchdog Flag */
#define	DS_CTLA_KSF	0x04		/* Kickstart Flag */
#define	DS_CTLA_TDF	0x08		/* TOD Flag */
#define	DS_CTLA_PAB	0x10		/* Power Active Bar Flag */
#define	DS_CTLA_PRS	0x20		/* PAB reset Select */
#define	DS_CTLA_BLF2	0x40		/* Battery level flag 2 */
#define	DS_CTLA_BLF1	0x80		/* Battery level flag 1 */

#define DS_CTLB_WDS	0x01		/* Watchdog route IRQ/RESET */
#define	DS_CTLB_WDE	0x02		/* Watchdog enable */
#define	DS_CTLB_KIE	0x04		/* 'Kickstart' interrupt enable */
#define	DS_CTLB_TIE	0x08		/* TOD Alarm interrupt enable */
#define	DS_CTLB_TPE	0x10		/* TOD Alarm power enable */
#define	DS_CTLB_BME	0x20		/* Burst mode enable */
#define	DS_CTLB_CS	0x40		/* Crystal select 6pf/12.5pf */
#define	DS_CTLB_TE	0x80		/* Transfer enable */

#endif /* _DS_15x1REG_H_ */
