/* $XFree86: xc/programs/Xserver/hw/xfree86/int10/helper_exec.c,v 1.16 2001/04/30 14:34:57 tsi Exp $ */
/*
 *                   XFree86 int10 module
 *   execute BIOS int 10h calls in x86 real mode environment
 *                 Copyright 1999 Egbert Eich
 *
 *   Part of this is based on code taken from DOSEMU
 *   (C) Copyright 1992, ..., 1999 the "DOSEMU-Development-Team"
 */

/*
 * To debug port accesses define PRINT_PORT.
 * Note! You also have to comment out ioperm()
 * in xf86EnableIO(). Otherwise we won't trap
 * on PIO.
 */
#include <stdio.h>
#include <stdlib.h>
#include "xf86.h"

#define _INT10_PRIVATE
#include "xf86x86emu.h"
#include "xf86int10.h"
#include "linux/io.h"
#include <dev/pci/pcivar.h>

#if !defined (_PC) && !defined (_PC_PCI)
static int pciCfg1in(CARD16 addr, CARD32 *val,int type);
static int pciCfg1out(CARD16 addr, CARD32 val,int type);
#define PCI_BYTE  0
#define PCI_WORD  1
#define PCI_DWORD 2
#endif

#define REG pInt

int
setup_int(xf86Int10InfoPtr pInt)
{
    if (pInt != Int10Current) {
	if (!MapCurrentInt10(pInt))
	    return -1;
	Int10Current = pInt;
    }
    X86_EAX = (CARD32) pInt->ax;
    X86_EBX = (CARD32) pInt->bx;
    X86_ECX = (CARD32) pInt->cx;
    X86_EDX = (CARD32) pInt->dx;
    X86_ESI = (CARD32) pInt->si;
    X86_EDI = (CARD32) pInt->di;
    X86_EBP = (CARD32) pInt->bp;
    X86_ESP = 0x1000; X86_SS = pInt->stackseg >> 4;
    X86_EIP = 0x0600; X86_CS = 0x0;	/* address of 'hlt' */
    X86_DS = 0x40;			/* standard pc ds */
    X86_ES = pInt->es;
    X86_FS = 0;
    X86_GS = 0;
    X86_EFLAGS = X86_IF_MASK | X86_IOPL_MASK;

    return 0;
}

void
finish_int(xf86Int10InfoPtr pInt, int sig)
{
    pInt->ax = (CARD16) X86_EAX;
    pInt->bx = (CARD16) X86_EBX;
    pInt->cx = (CARD16) X86_ECX;
    pInt->dx = (CARD16) X86_EDX;
    pInt->si = (CARD16) X86_ESI;
    pInt->di = (CARD16) X86_EDI;
    pInt->es = (CARD16) X86_ES;
    pInt->bp = (CARD16) X86_EBP;
    pInt->flags = (CARD16) X86_FLAGS;
}

/* general software interrupt handler */
CARD32
getIntVect(xf86Int10InfoPtr pInt,int num)
{
    return MEM_RW(pInt, num << 2) + (MEM_RW(pInt, (num << 2) + 2) << 4);
}

void
pushw(xf86Int10InfoPtr pInt, CARD16 val)
{
    X86_ESP -= 2;
    MEM_WW(pInt, ((CARD32) X86_SS << 4) + X86_SP, val);
}

int
run_bios_int(int num, xf86Int10InfoPtr pInt)
{
    CARD32 eflags;
#ifndef _PC
    /* check if bios vector is initialized */
    if (MEM_RW(pInt, (num << 2) + 2) == (SYS_BIOS >> 4)) { /* SYS_BIOS_SEG ?*/
      printf("Int CS=%X",MEM_RW(pInt,(num<<2)+2));
      printf("Card BIOS on non-PC like platform not loaded\n");
	//return 0;
	//X86EMU_trace_on();
    }
#endif
#ifdef PRINT_INT
    printf("calling card BIOS at: ");
#endif
    eflags = X86_EFLAGS;
#if 0
    eflags = eflags | IF_MASK;
    X86_EFLAGS = X86_EFLAGS  & ~(VIF_MASK | TF_MASK | IF_MASK | NT_MASK);
#endif
    pushw(pInt, eflags);
    pushw(pInt, X86_CS);
    pushw(pInt, X86_IP);
    X86_CS = MEM_RW(pInt, (num << 2) + 2);
    X86_IP = MEM_RW(pInt,  num << 2);
#ifdef PRINT_INT
    printf("0x%x:%lx\n", X86_CS, X86_EIP);
#endif
    return 1;
}

/* Debugging stuff */
void
dump_code(xf86Int10InfoPtr pInt)
{
    int i;
    CARD32 lina = SEG_ADR((CARD32), X86_CS, IP);

    printf("code at 0x%8.8x:\n", lina);
    for (i=0; i<0x10; i++)
	printf(" %2.2x", MEM_RB(pInt, lina + i));
    printf("\n");
    for (; i<0x20; i++)
	printf(" %2.2x", MEM_RB(pInt, lina + i));
    printf("\n");
}

void
dump_registers(xf86Int10InfoPtr pInt)
{
	printf("EAX=0x%8.8x, EBX=0x%8.8x, ECX=0x%8.8x, EDX=0x%8.8x\n",
			X86_EAX, X86_EBX, X86_ECX, X86_EDX);
	printf( "ESP=0x%8.8x, EBP=0x%8.8x, ESI=0x%8.8x, EDI=0x%8.8x\n",
			X86_ESP, X86_EBP, X86_ESI, X86_EDI);
	printf( "CS=0x%4.4x, SS=0x%4.4x,"
			" DS=0x%4.4x, ES=0x%4.4x, FS=0x%4.4x, GS=0x%4.4x\n",
			X86_CS, X86_SS, X86_DS, X86_ES, X86_FS, X86_GS);
	printf( "EIP=0x%8.8x, EFLAGS=0x%8.8x\n", X86_EIP, X86_EFLAGS);
}

void
stack_trace(xf86Int10InfoPtr pInt)
{
    int i = 0;
    CARD32 stack = SEG_ADR((CARD32), X86_SS, SP);
    CARD32 tail  = (CARD32)((X86_SS << 4) + 0x1000);

    if (stack >= tail) return;

    printf(" stack at 0x%8.8x:\n", stack);
    for (; stack < tail; stack++) {
	printf(" %2.2x", MEM_RB(pInt, stack));
	i = (i + 1) % 0x10;
	if (!i)
	printf( "\n");
    }
    if (i)
	printf("\n");
}



int
port_rep_inb(xf86Int10InfoPtr pInt,
	     CARD16 port, CARD32 base, int d_f, CARD32 count)
{
    register int inc = d_f ? -1 : 1;
    CARD32 dst = base;
#ifdef PRINT_PORT
    printf(" rep_insb(%#x) %d bytes at %p %s\n",
	     port, count, base, d_f ? "up" : "down");
#endif
    while (count--) {
	MEM_WB(pInt, dst, x_inb(port));
	dst += inc;
    }
    return dst - base;
}

int
port_rep_inw(xf86Int10InfoPtr pInt,
	     CARD16 port, CARD32 base, int d_f, CARD32 count)
{
    register int inc = d_f ? -2 : 2;
    CARD32 dst = base;
#ifdef PRINT_PORT
    printf(" rep_insw(%#x) %d bytes at %p %s\n",
	     port, count, base, d_f ? "up" : "down");
#endif
    while (count--) {
	MEM_WW(pInt, dst, x_inw(port));
	dst += inc;
    }
    return dst - base;
}

int
port_rep_inl(xf86Int10InfoPtr pInt,
	     CARD16 port, CARD32 base, int d_f, CARD32 count)
{
    register int inc = d_f ? -4 : 4;
    CARD32 dst = base;
#ifdef PRINT_PORT
    printf(" rep_insl(%#x) %d bytes at %p %s\n",
	     port, count, base, d_f ? "up" : "down");
#endif
    while (count--) {
	MEM_WL(pInt, dst, x_inl(port));
	dst += inc;
    }
    return dst - base;
}

int
port_rep_outb(xf86Int10InfoPtr pInt,
	      CARD16 port, CARD32 base, int d_f, CARD32 count)
{
    register int inc = d_f ? -1 : 1;
    CARD32 dst = base;
#ifdef PRINT_PORT
    printf(" rep_outb(%#x) %d bytes at %p %s\n",
	     port, count, base, d_f ? "up" : "down");
#endif
    while (count--) {
	x_outb(port, MEM_RB(pInt, dst));
	dst += inc;
    }
    return dst - base;
}

int
port_rep_outw(xf86Int10InfoPtr pInt,
	      CARD16 port, CARD32 base, int d_f, CARD32 count)
{
    register int inc = d_f ? -2 : 2;
    CARD32 dst = base;
#ifdef PRINT_PORT
    printf(" rep_outw(%#x) %d bytes at %p %s\n",
	     port, count, base, d_f ? "up" : "down");
#endif
    while (count--) {
	x_outw(port, MEM_RW(pInt, dst));
	dst += inc;
    }
    return dst - base;
}

int
port_rep_outl(xf86Int10InfoPtr pInt,
	      CARD16 port, CARD32 base, int d_f, CARD32 count)
{
    register int inc = d_f ? -4 : 4;
    CARD32 dst = base;
#ifdef PRINT_PORT
    printf(" rep_outl(%#x) %d bytes at %p %s\n",
	     port, count, base, d_f ? "up" : "down");
#endif
    while (count--) {
	x_outl(port, MEM_RL(pInt, dst));
	dst += inc;
    }
    return dst - base;
}

CARD8
x_inb(CARD16 port)
{
    CARD8 val;
    if (port == 0x40) {
	Int10Current->inb40time++;
	val = (CARD8)(Int10Current->inb40time >>
		      ((Int10Current->inb40time & 1) << 3));
#ifdef PRINT_PORT
	printf(" inb(%#x) = %2.2x\n", port, val);
#endif
#ifdef __NOT_YET__
    } else if (port < 0x0100) {		/* Don't interfere with mainboard */
	val = 0;
	printf( "inb 0x%4.4x\n", port);
	dump_registers(Int10Current);
	stack_trace(Int10Current);
#endif /* __NOT_YET__ */
    } else {
	val = linux_inb(port);
#ifdef PRINT_PORT
	if(port!=0x3da&& port!=0x3ba&&port!=0x61) printf(" inb(%#x) = %2.2x\n", port, val);
#endif
    }
    return val;
}

CARD16
x_inw(CARD16 port)
{
    CARD16 val;

    if (port == 0x5c) {
	/*
	 * Emulate a PC98's timer.  Typical resolution is 3.26 usec.
	 * Approximate this by dividing by 3.
	*	TODO: need complete!
	*/
    }else {
	val = linux_inw(port);
    }
#ifdef PRINT_PORT
    printf(" inw(%#x) = %4.4x\n", port, val);
#endif
    /*if(port==0x100a&&val==0x18f){
	 M.x86.debug=3;
    }*/
    return val;
}

void
x_outb(CARD16 port, CARD8 val)
{
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
#ifdef __NOT_YET__
    } else if (port < 0x0100) {		/* Don't interfere with mainboard */
	printf( "outb 0x%4.4x,0x%2.2x\n", port, val);
	dump_registers(Int10Current);
	stack_trace(Int10Current);
#endif /* __NOT_YET__ */
    } else {
#ifdef PRINT_PORT
	printf(" outb(%#x, %2.2x)\n", port, val);
#endif
	linux_outb(val,port);
    }
}

void
x_outw(CARD16 port, CARD16 val)
{
#ifdef PRINT_PORT
    printf(" outw(%#x, %4.4x)\n", port, val);
#endif
#ifndef VGA_LYNX_0712
    linux_outw(val,port);
#else
    if(port==0xd9c7){
    }else linux_outw(val,port);
#endif
}

CARD32
x_inl(CARD16 port)
{
    CARD32 val;

#if !defined(_PC) && !defined(_PC_PCI)
    if (!pciCfg1in(port, &val, PCI_DWORD))
#endif
    val = linux_inl(port);

#ifdef PRINT_PORT
    if(port!=0xcf8&&port!=0xcfc) printf(" inl(%#x) = %8.8x\n", port, val);
#endif
    return val;
}

void
x_outl(CARD16 port, CARD32 val)
{
#ifdef PRINT_PORT
    if(port!=0xcfc&&port!=0xcf8) printf(" outl(%#x, %8.8x)\n", port, val);
#endif

#if !defined(_PC) && !defined(_PC_PCI)
    if (!pciCfg1out(port, val, PCI_DWORD))
#endif
    linux_outl(val,port);
}

CARD8
Mem_rb(int addr)
{
    return (*Int10Current->mem->rb)(Int10Current, addr);
}

CARD16
Mem_rw(int addr)
{
    return (*Int10Current->mem->rw)(Int10Current, addr);
}

CARD32
Mem_rl(int addr)
{
    return (*Int10Current->mem->rl)(Int10Current, addr);
}

void
Mem_wb(int addr, CARD8 val)
{
    (*Int10Current->mem->wb)(Int10Current, addr, val);
}

void
Mem_ww(int addr, CARD16 val)
{
    (*Int10Current->mem->ww)(Int10Current, addr, val);
}

void
Mem_wl(int addr, CARD32 val)
{
    (*Int10Current->mem->wl)(Int10Current, addr, val);
}


#if !defined(_PC) && !defined(_PC_PCI)
static u32 PciCfg1Addr = 0;

#define BUS(Cfg1Addr) ((Cfg1Addr & 0xff0000) >> 16)
#define DEVFN(Cfg1Addr) ((Cfg1Addr & 0xff00) >> 8)
#define OFFSET(Cfg1Addr) (Cfg1Addr & 0xff)

static int
pciCfg1in(CARD16 addr, CARD32 *val,int type)
{
    if (addr == 0xCF8) {
	*(u32*)val = PciCfg1Addr;
	return 1;
    }
    if (addr == 0xCFC) {
	if (type==0)  {

          *val=_pci_conf_readn(_pci_make_tag(BUS(PciCfg1Addr),((DEVFN(PciCfg1Addr)>>3)&0x1f),(DEVFN(PciCfg1Addr)&0x7)),OFFSET(PciCfg1Addr),1);
#ifdef DEBUG_EMU_VGA
          printk(" byte read configuration space,addr=%x,val=%x\n",PciCfg1Addr,*(u8*)val);
#endif
	}else if (type==1) {
          *val=_pci_conf_readn(_pci_make_tag(BUS(PciCfg1Addr),((DEVFN(PciCfg1Addr)>>3)&0x1f),(DEVFN(PciCfg1Addr)&0x7)),OFFSET(PciCfg1Addr),2);
#ifdef DEBUG_EMU_VGA
          printk("word read configuration space,addr=%x,val=%x\n",PciCfg1Addr,*(u16*)val);
#endif
	}else if (type==2) {
          *val=_pci_conf_read(_pci_make_tag(BUS(PciCfg1Addr),((DEVFN(PciCfg1Addr)>>3)&0x1f),(DEVFN(PciCfg1Addr)&0x7)),OFFSET(PciCfg1Addr));
#ifdef DEBUG_EMU_VGA
//          printk(" dword read configuration space,addr=%x,val=%x\n",PciCfg1Addr,*(u32*)val);
#endif
	}else{
          *val=_pci_conf_read(_pci_make_tag(BUS(PciCfg1Addr),((DEVFN(PciCfg1Addr)>>3)&0x1f),(DEVFN(PciCfg1Addr)&0x7)),OFFSET(PciCfg1Addr));
	  //printk("wrong type for pci config op\n");
	}
	return 1;
    }
    return 0;
}

static int
pciCfg1out(CARD16 addr, CARD32 val,int type)
{
    if (addr == 0xCF8) {
	PciCfg1Addr = val;
	return 1;
    }
    if (addr == 0xCFC) {
#ifdef DEBUG_EMU_VGA
    printk("write configuration space,addr=%x,val=%x,type=%d\n",PciCfg1Addr,val,type);
#endif
      if (type==0) {
          _pci_conf_writen(_pci_make_tag(BUS(PciCfg1Addr),((DEVFN(PciCfg1Addr)>>3)&0x1f),(DEVFN(PciCfg1Addr)&0x7)),OFFSET(PciCfg1Addr),(u8)val,1);
      }else if (type==1) {
          _pci_conf_writen(_pci_make_tag(BUS(PciCfg1Addr),((DEVFN(PciCfg1Addr)>>3)&0x1f),(DEVFN(PciCfg1Addr)&0x7)),OFFSET(PciCfg1Addr),(u8)val,2);
      }else if (type==2) {
          _pci_conf_write(_pci_make_tag(BUS(PciCfg1Addr),((DEVFN(PciCfg1Addr)>>3)&0x1f),(DEVFN(PciCfg1Addr)&0x7)),OFFSET(PciCfg1Addr),(u8)val);
      }else {
          _pci_conf_write(_pci_make_tag(BUS(PciCfg1Addr),((DEVFN(PciCfg1Addr)>>3)&0x1f),(DEVFN(PciCfg1Addr)&0x7)),OFFSET(PciCfg1Addr),(u8)val);
	  //printk("wrong type for pci config op\n");
      }
      return 1;
    }
    return 0;
}
#endif

CARD8
bios_checksum(CARD8 *start, int size)
{
    CARD8 sum = 0;

    while (size-- > 0)
	sum += *start++;
    return sum;
}

/*
 * Lock/Unlock legacy VGA. Some Bioses try to be very clever and make
 * an attempt to detect a legacy ISA card. If they find one they might
 * act very strange: for example they might configure the card as a
 * monochrome card. This might cause some drivers to choke.
 * To avoid this we attempt legacy VGA by writing to all know VGA
 * disable registers before we call the BIOS initialization and
 * restore the original values afterwards. In beween we hold our
 * breath. To get to a (possibly exising) ISA card need to disable
 * our current PCI card.
 */
/*
 * This is just for booting: we just want to catch pure
 * legacy vga therefore we don't worry about mmio etc.
 * This stuff should really go into vgaHW.c. However then
 * the driver would have to load the vga-module prior to
 * doing int10.
 */
void
LockLegacyVGA(int screenIndex,legacyVGAPtr vga)
{
    vga->save_msr = linux_inb(0x3CC);
    vga->save_vse = linux_inb(0x3C3);
    vga->save_46e8 = linux_inb(0x46e8);
    vga->save_pos102 = linux_inb(0x102);
    linux_outb(~(CARD8)0x03 & vga->save_msr,0x3C2);
    linux_outb(~(CARD8)0x01 & vga->save_vse,0x3C3);
    linux_outb(~(CARD8)0x08 & vga->save_46e8,0x4e68);
    linux_outb(~(CARD8)0x01 & vga->save_pos102,0x102);
}

void
UnlockLegacyVGA(int screenIndex, legacyVGAPtr vga)
{
    linux_outb(vga->save_pos102,0x102);
    linux_outb(vga->save_46e8,0x46e8);
    linux_outb(vga->save_vse,0x3C3);
    linux_outb(vga->save_msr,0x3C2);
}
