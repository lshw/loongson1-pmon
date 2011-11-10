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
* Description:  Header file for the real mode x86 BIOS emulator, which is
*               used to warmboot any number of VGA compatible PCI/AGP
*               controllers under any OS, on any processor family that
*               supports PCI. We also allow the user application to call
*               real mode BIOS functions and Int 10h functions (including
*               the VESA BIOS).
*
****************************************************************************/

#ifndef __BIOSEMU_H
#define __BIOSEMU_H

#include "include/x86emu.h"

/*---------------------- Macros and type definitions ----------------------*/

/* ??? this will lead to unaligned accesses
#pragma pack(1)
*/

/****************************************************************************
REMARKS:
Data structure used to describe the details specific to a particular VGA
controller. This information is used to allow the VGA controller to be
swapped on the fly within the BIOS emulator.

HEADER:
biosemu.h

MEMBERS:
pciInfo         - PCI device information block for the controller
BIOSImage       - Pointer to a read/write copy of the BIOS image
BIOSImageLen    - Length of the BIOS image
LowMem          - Copy of key low memory areas
****************************************************************************/

#ifndef uchar
#define uchar unsigned char
#endif

typedef struct {
    struct pci_device *pciInfo;
    void            *BIOSImage;
    ulong           BIOSImageLen;
    uchar           LowMem[1536];
    uchar           save_msr;
    uchar           save_pos102;
    uchar           save_vse;
    uchar           save_46e8;
    } BE_VGAInfo;

/****************************************************************************
REMARKS:
Data structure used to describe the details for the BIOS emulator system
environment as used by the X86 emulator library.

HEADER:
biosemu.h

MEMBERS:
vgaInfo         - VGA BIOS information structure
biosmem_base    - Base of the BIOS image
biosmem_limit   - Limit of the BIOS image
busmem_base     - Base of the VGA bus memory
****************************************************************************/

typedef struct {
    BE_VGAInfo      vgaInfo;
    ulong           biosmem_base;
    ulong           biosmem_limit;
    ulong           busmem_base;
    ulong           sysmem_base;
    } BE_sysEnv;

struct _PMDWORDREGS {
    ulong   eax,ebx,ecx,edx,esi,edi,cflag;
    };

struct _PMWORDREGS {
    ushort  ax,ax_hi;
    ushort  bx,bx_hi;
    ushort  cx,cx_hi;
    ushort  dx,dx_hi;
    ushort  si,si_hi;
    ushort  di,di_hi;
    ushort  cflag,cflag_hi;
    };

struct _PMBYTEREGS {
    uchar   al, ah; ushort ax_hi;
    uchar   bl, bh; ushort bx_hi;
    uchar   cl, ch; ushort cx_hi;
    uchar   dl, dh; ushort dx_hi;
    };

typedef union {
    struct  _PMDWORDREGS e;
    struct  _PMWORDREGS  x;
    struct  _PMBYTEREGS  h;
    } PMREGS;

typedef struct {
    ushort  es;
    ushort  cs;
    ushort  ss;
    ushort  ds;
    ushort  fs;
    ushort  gs;
    } PMSREGS;

/* Provide definitions for the real mode register structures passed to
 * the PM_int86() and PM_int86x() routines. Note that we provide our own
 * functions to do this for 16-bit code that calls the PM_int386 functions.
 */

typedef PMREGS  RMREGS;
typedef PMSREGS RMSREGS;

/*
#pragma pack()
*/

/*---------------------------- Global variables ---------------------------*/

#ifdef  __cplusplus
extern "C" {                        /* Use "C" linkage when in C++ mode */
#endif

/* {secret} Global BIOS emulator system environment */
extern BE_sysEnv _BE_env;

/*-------------------------- Function Prototypes --------------------------*/

/* BIOS emulator library entry points */

int	 BE_init(u32 debugFlags,int memSize,BE_VGAInfo *info);
void     BE_setVGA(BE_VGAInfo *info);
void     BE_getVGA(BE_VGAInfo *info);
void     BE_setDebugFlags(u32 debugFlags);
void *   BE_mapRealPointer(uint r_seg,uint r_off);
void *   BE_getVESABuf(uint *len,uint *rseg,uint *roff);
void     BE_callRealMode(uint seg,uint off,RMREGS *regs,RMSREGS *sregs);
int      BE_int86(int intno,RMREGS *in,RMREGS *out);
int      BE_int86x(int intno,RMREGS *in,RMREGS *out,RMSREGS *sregs);
void     BE_exit(void);

#ifdef  __cplusplus
}                                   /* End of "C" linkage for C++       */
#endif

#endif /* __BIOSEMU_H */

