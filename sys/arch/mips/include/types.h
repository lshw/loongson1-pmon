/*	$OpenBSD: types.h,v 1.6 1999/01/27 04:46:06 imp Exp $	*/

/*-
 * Copyright (c) 1992, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * Ralph Campbell.
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
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 *	@(#)types.h	8.3 (Berkeley) 1/5/94
 */

#ifndef	_MIPS_TYPES_H_
#define	_MIPS_TYPES_H_

#include <sys/cdefs.h>


typedef unsigned long long u64;
typedef unsigned int u32;
typedef unsigned short u16;
typedef unsigned char u8;
typedef signed long long s64;
typedef signed long s32;
typedef signed short s16;
typedef signed char s8;
typedef int bool;
typedef unsigned long dma_addr_t;

#define K1BASE 0xa0000000
#define _KSEG1(addr) ((void *)(K1BASE | (u32)(addr)))

#define KSEG1_STORE8(addr, value) *(volatile u8 *)(_KSEG1(addr)) = (value)
#define KSEG1_STORE16(addr, value) *(volatile u16 *)(_KSEG1(addr)) = (value)
#define KSEG1_STORE32(addr, value) *(volatile u32 *)(_KSEG1(addr)) = (value)

#define KSEG1_LOAD8(addr) *(volatile u8 *)(_KSEG1(addr))
#define KSEG1_LOAD16(addr) *(volatile u16 *)(_KSEG1(addr))
#define KSEG1_LOAD32(addr) *(volatile u32 *)(_KSEG1(addr))

#define STORE8(addr, value) *(volatile u8 *)(addr) = value
#define STORE16(addr, value) *(volatile u16 *)(addr) = value
#define STORE32(addr, value) *(volatile u32 *)(addr) = value

#define LOAD8(addr) *(volatile u8 *)(addr)
#define LOAD16(addr) *(volatile u16 *)(addr)
#define LOAD32(addr) *(volatile u32 *)(addr)

#define PHY(addr) \
    (((u32)addr >= 0x80000000 && (u32)addr < 0xa0000000)? \
    (u32)addr-0x80000000:(u32)addr >= 0xa0000000? \
    (u32)addr-0xa0000000:(u32)addr)


typedef	unsigned long	vm_offset_t;
typedef	unsigned long	vm_size_t;

typedef unsigned long	vaddr_t;
typedef unsigned long	paddr_t;
typedef unsigned long	vsize_t;
typedef unsigned long	psize_t;

/*
 * Basic integral types.  Omit the typedef if
 * not possible for a machine/compiler combination.
 */
#define	__BIT_TYPES_DEFINED__
typedef	__signed char		   int8_t;
typedef	unsigned char		 u_int8_t;
typedef	unsigned char		  uint8_t;
typedef	short			  int16_t;
typedef	unsigned short		u_int16_t;
typedef	unsigned short		 uint16_t;
typedef	int			  int32_t;
typedef	unsigned int		u_int32_t;
typedef	unsigned int		 uint32_t;
/* LONGLONG */
typedef	long long		  int64_t;
/* LONGLONG */
typedef	unsigned long long	u_int64_t;
/* LONGLONG */
typedef	unsigned long long	 uint64_t;

#if __mips >= 3
#define	HAVE_QUAD
typedef int64_t		       register_t;
typedef int64_t		     f_register_t;
#else
typedef int32_t		       register_t;
typedef int32_t		     f_register_t;
#endif

#if !defined(_ANSI_SOURCE) && !defined(_POSIX_SOURCE)
typedef struct _physadr {
	int r[1];
} *physadr;

typedef struct label_t {
	register_t val[12];
} label_t;
#endif

#define	__SWAP_BROKEN

#endif	/* !_MIPS_TYPES_H_ */
