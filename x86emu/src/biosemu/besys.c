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
* Description:  This file includes BIOS emulator I/O and memory access
*               functions.
*
****************************************************************************/
/* BE CAREFUL: outb here is using linux style outb(value,addr)
 * while the same function in pmon&xfree86 are different
 */

#include <stdio.h>
#include <dev/pci/pcivar.h>
#include <linux/types.h>
#include <linux/pci.h>
#include <linux/io.h>
#include "biosemui.h"

#if !defined (_PC) && !defined (_PC_PCI)
static int pciCfg1in(u16 addr, u32 *val,int type);
static int pciCfg1out(u16 addr, u32 val,int type);
#endif

/*------------------------------- Macros ----------------------------------*/

/* Macros to read and write values to x86 bus memory. Replace these as
 * necessary if you need to do something special to access memory over
 * the bus on a particular processor family.
 */

/* Use char read/write to avoid unaligned access */

#define READ8(addr,val) { val = *(u8*)(addr) ; }
#define READ16(addr,val) { if ((addr)&1) \
	                 val = ((*(u8*)(addr)) | ((*(u8*)((addr)+1)) << 8)); \
			else \
	                 val = *(u16*)(addr); \
		     }
#define READ32(addr,val) { if ( (addr)&3 ) \
	                 val = ( (*(u8*)(addr)) | ((*(u8*)((addr)+1)) << 8) | ( (*(u8*)((addr)+2))<<16) | ((*(u8*)((addr)+3))<<24) ); \
			else \
	                 val = *(u32*)(addr); \
		     }
#define WRITE8(addr,val) { *(u8*)(addr) = val; }
#define WRITE16(addr,val) { if ((addr)&1) { \
	                     *(u8*)(addr) = val; \
	                     *(u8*)((addr)+1) = val>>8; \
			   } else \
	                     *(u16*)(addr) = val; \
		     }
#define WRITE32(addr,val) { if ((addr) & 3) { \
	                       *(u8*)(addr) = val; \
	                       *(u8*)((addr)+1) = val>>8; \
	                       *(u8*)((addr)+2) = val>>16; \
	                       *(u8*)((addr)+3) = val>>24; \
			     } else \
	                       *(u32*)(addr) = val; \
		          }

/*----------------------------- Implementation ----------------------------*/

#ifdef DEBUG_EMU_VGA
# define DEBUG_MEM()        (M.x86.debug & DEBUG_MEM_TRACE_F)
#else
# define DEBUG_MEM()
#endif

/****************************************************************************
PARAMETERS:
addr    - Emulator memory address to read

RETURNS:
Byte value read from emulator memory.

REMARKS:
Reads a byte value from the emulator memory. We have three distinct memory
regions that are handled differently, which this function handles.
****************************************************************************/
u8 X86API BE_rdb(
    u32 addr)
{
    u8 val = 0;

    if (addr < M.mem_size) {
        READ8(M.mem_base + addr, val);
    } else if (addr >= 0xC0000 && addr <= _BE_env.biosmem_limit) {
        READ8(_BE_env.biosmem_base + addr - 0xC0000, val);
    } else if (addr >= 0xA0000 && addr < 0xC0000) {
        READ8(_BE_env.busmem_base + addr - 0xA0000, val);
    } else if (addr >= 0xf0000 && addr < SYS_SIZE) {
        READ8(_BE_env.sysmem_base + addr - 0xf0000, val);
    }else {
	printf("mem_read: address %#x out of range!\n", addr);
        HALT_SYS();
    }
DB( if (DEBUG_MEM())
        printf("%#08x 1 -> %#x\n", addr, val);)
    return val;
}

/****************************************************************************
PARAMETERS:
addr    - Emulator memory address to read

RETURNS:
Word value read from emulator memory.

REMARKS:
Reads a word value from the emulator memory. We have three distinct memory
regions that are handled differently, which this function handles.
****************************************************************************/
u16 X86API BE_rdw(
    u32 addr)
{
    u16 val = 0;

    if (addr < M.mem_size - 1 ) {
         READ16(M.mem_base + addr, val);
    } else if (addr >= 0xC0000 && addr <= _BE_env.biosmem_limit) {
         READ16(_BE_env.biosmem_base + addr - 0xC0000, val);
    } else if (addr >= 0xA0000 && addr < 0xC0000) {
         READ16(_BE_env.busmem_base + addr - 0xA0000, val);
    } else if (addr >= 0xf0000 && addr < SYS_SIZE) {
         READ16(_BE_env.sysmem_base + addr - 0xf0000, val);
    }else {
	printf("mem_read: address %#x out of range!\n", addr);
        HALT_SYS();
    }
DB( if (DEBUG_MEM())
        printf("%#08x 2 -> %#x\n", addr, val);)
    return val;
}

/****************************************************************************
PARAMETERS:
addr    - Emulator memory address to read

RETURNS:
Long value read from emulator memory.

REMARKS:
Reads a long value from the emulator memory. We have three distinct memory
regions that are handled differently, which this function handles.
****************************************************************************/
u32 X86API BE_rdl(
    u32 addr)
{
    u32 val = 0;

    if (addr < M.mem_size - 3 ) {
         READ32(M.mem_base + addr, val);
    } else if (addr >= 0xC0000 && addr <= _BE_env.biosmem_limit) {
         READ32(_BE_env.biosmem_base + addr - 0xC0000, val);
    } else if (addr >= 0xA0000 && addr < 0xC0000) {
         READ32(_BE_env.busmem_base + addr - 0xA0000, val);
    } else if (addr >= 0xf0000 && addr < SYS_SIZE) {
         READ32(_BE_env.sysmem_base + addr - 0xf0000, val);
    }else {
#ifdef DEBUG_EMU_VGA
	printf("mem_read: address %#x out of range!\n", addr);
#endif
        HALT_SYS();
    }
DB( if (DEBUG_MEM())
        printf("%#08x 4 -> %#x\n", addr, val);)
    return val;
}

/****************************************************************************
PARAMETERS:
addr    - Emulator memory address to read
val     - Value to store

REMARKS:
Writes a byte value to emulator memory. We have three distinct memory
regions that are handled differently, which this function handles.
****************************************************************************/
void X86API BE_wrb(
    u32 addr,
    u8 val)
{
DB( if (DEBUG_MEM())
        printf("%#08x 1 <- %#x\n", addr, val);)

    if (addr < M.mem_size) {
        WRITE8(M.mem_base + addr , val);
    } else if (addr >= 0xC0000 && addr <= _BE_env.biosmem_limit) {
        WRITE8(_BE_env.biosmem_base + addr - 0xC0000 , val);
    } else if (addr >= 0xA0000 && addr < 0xC0000) {
        WRITE8(_BE_env.busmem_base + addr - 0xA0000 , val);
    } else if (addr >= 0xf0000 && addr < SYS_SIZE) {
        WRITE8(_BE_env.sysmem_base + addr - 0xf0000 , val);
    }else {
#ifdef DEBUG_EMU_VGA
	printf("mem_write: address %#x out of range!\n", addr);
#endif
        HALT_SYS();
    }
}

/****************************************************************************
PARAMETERS:
addr    - Emulator memory address to read
val     - Value to store

REMARKS:
Writes a word value to emulator memory. We have three distinct memory
regions that are handled differently, which this function handles.
****************************************************************************/
void X86API BE_wrw(
    u32 addr,
    u16 val)
{
DB( if (DEBUG_MEM())
        printf("%#08x 2 <- %#x\n", addr, val);)
    if (addr < M.mem_size - 1) {
        WRITE16(M.mem_base + addr , val);
    } else if (addr >= 0xC0000 && addr <= _BE_env.biosmem_limit) {
        WRITE16(_BE_env.biosmem_base + addr - 0xC0000 , val);
    } else if (addr >= 0xA0000 && addr < 0xC0000) {
        WRITE16(_BE_env.busmem_base + addr - 0xA0000 , val);
    } else if (addr >= 0xf0000 && addr < SYS_SIZE) {
        WRITE16(_BE_env.sysmem_base + addr - 0xf0000 , val);
    }else {
#ifdef DEBUG_EMU_VGA
#endif
	printf("mem_write: address %#x out of range!\n", addr);
        HALT_SYS();
    }
}

/****************************************************************************
PARAMETERS:
addr    - Emulator memory address to read
val     - Value to store

REMARKS:
Writes a long value to emulator memory. We have three distinct memory
regions that are handled differently, which this function handles.
****************************************************************************/
void X86API BE_wrl(
    u32 addr,
    u32 val)
{
DB( if (DEBUG_MEM())
        printf("%#08x 4 <- %#x\n", addr, val);)
    if (addr < M.mem_size - 3) {
        WRITE32(M.mem_base + addr , val);
    } else if (addr >= 0xC0000 && addr <= _BE_env.biosmem_limit) {
        WRITE32(_BE_env.biosmem_base + addr - 0xC0000 , val);
    } else if (addr >= 0xA0000 && addr < 0xC0000) {
        WRITE32(_BE_env.busmem_base + addr - 0xA0000 , val);
    } else if (addr >= 0xf0000 && addr < SYS_SIZE) {
        WRITE32(_BE_env.sysmem_base + addr - 0xf0000 , val);
    }else {
	printf("mem_write: address %#x out of range!\n", addr);
        HALT_SYS();
    }
}

#ifdef DEBUG_EMU_VGA
#define DEBUG_IO()          (M.x86.debug & DEBUG_IO_TRACE_F)
void check_io(int port,int read,int val)
{
	return;
	if(port==0x40||port==0x43) return;
	//if(port>0x3ff) return;
/*	static int printed[1024];
	static int count=0;
	int i;
	for (i=0;i<count;i++)
	  if (printed[i]==port) return;
	printed[count] = port;
	count++;
	if (count>=1024) count=1023;*/
	if(port!=0xcf8&&port!=0xcfc&&port!=0x3da&&port!=0x3ba&&port!=61)
           printf("%4X:%4X %s io port %x, val %x\n",M.x86.R_CS,M.x86.R_IP,read?"write":"read",port,val);
}
#else
#define check_io(port,type,val) 
#endif

u8 X86API BE_inb(int port)
{
    u8 val;

#ifdef PLAY_SAFE
    if (port < 0 || port > 0x10000) {
	    printf("Invalid ioport %x\n",port);
	    return 0xff;
    }
#endif

#ifdef MY40IO
{
static unsigned short Int10Current_inb40time=0;
    if (port == 0x40) {
	Int10Current_inb40time++;
	val = (u8)(Int10Current_inb40time >>
		      ((Int10Current_inb40time & 1) << 3));
#ifdef PRINT_PORT
	printf(" inb(%#x) = %2.2x\n", port, val);
#endif
	return val;
    } 
}
#endif

#ifdef MY61IO
{
static unsigned short Int10Current_inb61time=0;
    if (port == 0x61) {
	Int10Current_inb61time++;
	val = (u8)(Int10Current_inb61time>>3); 
#ifdef PRINT_PORT
	printf(" inb(%#x) = %2.2x\n", port, val);
#endif
	return val;
    } 
}
#endif

#if !defined(_PC) && !defined(_PC_PCI)
    if (!pciCfg1in(port,(u32 *)&val,1))
#endif
    val = linux_inb(port);
#ifdef DEBUG_EMU_VGA
    if (DEBUG_IO());
#endif
//	printf("inb.%04X -> %02X\n", (ushort)port, val);
    check_io(port,0,val);
    return val;
}

u16 X86API BE_inw(int port)
{
    u16 val;

#ifdef PLAY_SAFE
    if (port < 0 || port > 0x10000) {
	    printf("Invalid ioport %x\n",port);
	    return 0xffff;
    }
#endif
#if !defined(_PC) && !defined(_PC_PCI)
    if (!pciCfg1in(port,(u32 *)&val,2))
#endif
#if 0
    if (port == 0x5c) {
	/* Emulate a PC98's timer */
       long sec,usec;
       (void)getsecs(&sec,&usec);
       val = (u16)(usec / 3 );
    }else
#endif
    val = linux_inw(port);

#ifdef DEBUG_EMU_VGA
    if (DEBUG_IO());
#endif
//	printf("inw.%04X -> %04X\n", (ushort)port, val);
    check_io(port,0,val);
    return val;
}

u32 X86API BE_inl(int port)
{
    u32 val;
    
#ifdef PLAY_SAFE
    if (port < 0 || port > 0x10000) {
	    printf("Invalid ioport %x\n",port);
	    return 0xffffffff;
    }
#endif
#if !defined(_PC) && !defined(_PC_PCI)
    if (!pciCfg1in(port,&val,4))
#endif
    val = linux_inl(port);
#ifdef DEBUG_EMU_VGA
    if (DEBUG_IO());
#endif
//	printf("inl.%04X -> %08X\n", (ushort)port, val);
    check_io(port,0,val);
    return val;
}

void X86API BE_outb(int port, u8 val)
{
#ifdef DEBUG_EMU_VGA
    if (DEBUG_IO());
#endif
#ifdef PLAY_SAFE
    if (port < 0 || port > 0x10000) {
	    printf("Invalid ioport %x\n",port);
	    return;
    }
#endif

#ifdef MY40IO
    if ((port == 0x43) && (val == 0)) {
	/*
	 * Emulate a PC's timer 0.  Such timers typically have a resolution of
	 * some .838 usec per tick, but this can only provide 1 usec per tick.
	 * (Not that this matters much, given inherent emulation delays.)  Use
	 * the bottom bit as a byte select.  See inb(0x40) above.
	 */
	 //TODO need complete
#ifdef PRINT_PORT
	printf(" outb(%#x, %2.2x)\n", port, val);
#endif
	return;
    } 
#endif

#if !defined(_PC) && !defined(_PC_PCI)
    if (!pciCfg1out(port,val,1))
#endif
    //if (port == 0x20 || port == 0x21 || port==0xa0 || port == 0xa1) return;
    linux_outb(val,port);
    check_io(port,1,val);
//printf("outb.%04X <- %02X\n",(ushort)port, val);
}

void X86API BE_outw(int port, u16 val)
{
#ifdef DEBUG_EMU_VGA
    if (DEBUG_IO());
#endif
#ifdef PLAY_SAFE
    if (port < 0 || port > 0x10000) {
	    printf("Invalid ioport %x\n",port);
	    return;
    }
#endif
#if !defined(_PC) && !defined(_PC_PCI)
    if (!pciCfg1out(port,val,2))
#endif
    linux_outw(val,port);
    check_io(port,1,val);
    //printf("outw.%04X <- %04X\n", (ushort)port, val);
}

void X86API BE_outl(int port, u32 val)
{
#ifdef DEBUG_EMU_VGA
    if (DEBUG_IO());
#endif
#if !defined(_PC) && !defined(_PC_PCI)
    if (!pciCfg1out(port,val,4))
#endif
#ifdef PLAY_SAFE
    if (port < 0 || port > 0x10000) {
	    printf("Invalid ioport %x\n",port);
	    return;
    }
#endif
    linux_outl(val,port);
    check_io(port,1,val);
    //printf("outl.%04X <- %08X\n", (ushort)port, val);
}

#if !defined(_PC) && !defined(_PC_PCI)
static u32 PciCfg1Addr = 0;

#define BUS(Cfg1Addr) ((Cfg1Addr & 0xff0000) >> 16)
#define DEVFN(Cfg1Addr) ((Cfg1Addr & 0xff00) >> 8)
#define OFFSET(Cfg1Addr) (Cfg1Addr & 0xff)

void pcibios_read_config_dword(int bus,int devfn,int offset,int * val)
{
	*val=_pci_conf_read(_pci_make_tag(bus,((devfn>>3)&0x1f),(devfn&0x7)),offset);
}
void pcibios_read_config_byte(int bus,int devfn,int offset,int * val)
{
	*val=_pci_conf_readn(_pci_make_tag(bus,((devfn>>3)&0x1f),(devfn&0x7)),offset,1);
}
void pcibios_read_config_word(int bus,int devfn,int offset,int * val)
{
	*val=_pci_conf_readn(_pci_make_tag(bus,((devfn>>3)&0x1f),(devfn&0x7)),offset,2);
}
void pcibios_write_config_dword(int bus,int devfn,int offset,int val)
{
	_pci_conf_write(_pci_make_tag(bus,((devfn>>3)&0x1f),(devfn&0x7)),offset,val);
}
void pcibios_write_config_byte(int bus,int devfn,int offset,int val)
{
	_pci_conf_writen(_pci_make_tag(bus,((devfn>>3)&0x1f),(devfn&0x7)),offset,val,1);
}

void pcibios_write_config_word(int bus,int devfn,int offset,int val)
{
	_pci_conf_writen(_pci_make_tag(bus,((devfn>>3)&0x1f),(devfn&0x7)),offset,val,2);
}
static int
pciCfg1in(u16 addr, u32 *val,int type)
{
    if (addr == 0xCF8) {
	*val = PciCfg1Addr;
	return 1;
    }
    if (addr == 0xCFC) {
	if(type==0){
		pcibios_read_config_byte(BUS(PciCfg1Addr),DEVFN(PciCfg1Addr),OFFSET(PciCfg1Addr),(u8*)val);
	}else if(type==1){
		pcibios_read_config_word(BUS(PciCfg1Addr),DEVFN(PciCfg1Addr),OFFSET(PciCfg1Addr),(u16*)val);
	}else if(type==2){
		pcibios_read_config_dword(BUS(PciCfg1Addr),DEVFN(PciCfg1Addr),OFFSET(PciCfg1Addr),(u32*)val);
	}else{
		pcibios_read_config_dword(BUS(PciCfg1Addr),DEVFN(PciCfg1Addr),OFFSET(PciCfg1Addr),(u32*)val);
  	   //printf("wrong type for pci config op\n");
	}
	return 1;
    }
    return 0;
}

static int
pciCfg1out(u16 addr, u32 val,int type)
{
    if (addr == 0xCF8) {
	PciCfg1Addr = val;
	return 1;
    }
    if (addr == 0xCFC) {
	if(type==0){
		pcibios_write_config_byte(BUS(PciCfg1Addr),DEVFN(PciCfg1Addr),OFFSET(PciCfg1Addr),val);
	}else if(type==1){
		pcibios_write_config_word(BUS(PciCfg1Addr),DEVFN(PciCfg1Addr),OFFSET(PciCfg1Addr),val);
	}else if(type==2){
		pcibios_write_config_dword(BUS(PciCfg1Addr),DEVFN(PciCfg1Addr),OFFSET(PciCfg1Addr),val);
	}else{
		pcibios_write_config_dword(BUS(PciCfg1Addr),DEVFN(PciCfg1Addr),OFFSET(PciCfg1Addr),val);
  	   //printf("wrong type for pci config op\n");
	}
	return 1;
    }
    return 0;
}
#endif
