/****************************************************************************
*
*                        BIOS emulator and interface
*                      to Realmode X86 Emulator Library
*
*               Copyright (C) 1996-1999 SciTech Software, Inc.
*
*  ========================================================================
*
*  Permission to use, copy, modify, distribute, and sell this software and
*  its documentation for any purpose is hereby granted without fee,
*  provided that the above copyright notice appear in all copies and that
*  both that copyright notice and this permission notice appear in
*  supporting documentation, and that the name of the authors not be used
*  in advertising or publicity pertaining to distribution of the software
*  without specific, written prior permission.  The authors makes no
*  representations about the suitability of this software for any purpose.
*  It is provided "as is" without express or implied warranty.
*
*  THE AUTHORS DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
*  INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO
*  EVENT SHALL THE AUTHORS BE LIABLE FOR ANY SPECIAL, INDIRECT OR
*  CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF
*  USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR
*  OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
*  PERFORMANCE OF THIS SOFTWARE.
*
*  ========================================================================
*
* Language:     ANSI C
* Environment:  Any
* Developer:    Kendall Bennett
*
* Description:  Internal header file for the BIOS emulator library.
*
****************************************************************************/

#ifndef __BIOSEMUI_H
#define __BIOSEMUI_H

#include "biosemu.h"

#ifdef IN_LINUX_KERNEL
#include <linux/kernel.h> /* for printk */
#endif

/*---------------------- Macros and type definitions ----------------------*/

#ifdef DEBUG_EMU_VGA
#define DB(x)   x
#else
#define DB(x)
#endif

#define BIOS_SEG        0xfff0

#define SYS_SIZE 0x100000 /* 1M real mode memory */

#define SYS_BIOS 0xF0000
#define BIOS_SIZE 0x10000

#define V_RAM 0xA0000
#define VRAM_SIZE 0x20000

#define V_BIOS_SIZE 0x10000
#define V_BIOS  0xC0000

#define M               _X86EMU_env

/*-------------------------- Function Prototypes --------------------------*/

/* bios.c */

void    _BE_bios_init(u32 *intrTab);
void    _BE_setup_funcs(void);
void    LockLegacyVGA(void);
void    UnlockLegacyVGA(void);
int     fillin_key_datas(void);

/* besys.c */

u8       BE_rdb(u32 addr);
u16      BE_rdw(u32 addr);
u32      BE_rdl(u32 addr);
void     BE_wrb(u32 addr,u8 val);
void     BE_wrw(u32 addr,u16 val);
void     BE_wrl(u32 addr,u32 val);
u8       BE_inb(int port);
u16      BE_inw(int port);
u32      BE_inl(int port);
void     BE_outb(int port, u8 val);
void     BE_outw(int port, u16 val);
void     BE_outl(int port, u32 val);

#endif /* __BIOSEMUI_H */
