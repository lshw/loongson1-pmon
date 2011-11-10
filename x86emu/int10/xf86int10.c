/* $XFree86: xc/programs/Xserver/hw/xfree86/int10/xf86int10.c,v 1.8 2001/10/01 13:44:13 eich Exp $ */
/*
 *                   XFree86 int10 module
 *   execute BIOS int 10h calls in x86 real mode environment
 *                 Copyright 1999 Egbert Eich
 */

#include <stdio.h>
#include "xf86.h"
#define _INT10_PRIVATE
#include "xf86int10.h"
#include "xf86x86emu.h"
#include "linux/io.h"
#include <dev/pci/pcivar.h>

#define REG pInt

xf86Int10InfoPtr Int10Current = NULL;

static int int1A_handler(xf86Int10InfoPtr pInt);
//static int int11_handler(xf86Int10InfoPtr pInt);
#ifndef _PC
static int int15_handler(xf86Int10InfoPtr pInt);
static int int42_handler(xf86Int10InfoPtr pInt);
#endif
static int intE6_handler(xf86Int10InfoPtr pInt);

static int verbose = 1;

int
int_handler(xf86Int10InfoPtr pInt)
{
    int num = pInt->num;
    int ret = 0;

    printf("int_handler called,int=%x\n",num);

    switch (num) {
#ifndef _PC
    case 0x10:
    case 0x42:
    case 0x6D:
	if (getIntVect(pInt, num) == I_S_DEFAULT_INT_VECT) {
	    printf("default int10 called,intno=%x\n",num);
	    ret = int42_handler(pInt);
	}
	break;
    case 0x15:
        ret = int15_handler(pInt);
        break;
#endif
    case 0x1A:
	ret = int1A_handler(pInt);
	break;
    case 0xe6:
	ret = intE6_handler(pInt);
	break;
    default:
	break;
    }

    if (!ret) {
	ret = run_bios_int(num, pInt);
	//if(num==0x10) X86EMU_trace_on();
	printf("run_bios_int,intno=%x,ret=%x\n",num,ret);
    }

    if (!ret) {
	printf( "Halting on int 0x%2.2x!\n", num);
	dump_registers(pInt);
	stack_trace(pInt);
    }

    return ret;
}

#ifndef _PC

/*
static int 
int11_handler(xf86Int10InfoPtr pInt)
{
	M.x86.R_AX=0xc823;
	return 1;
}*/
static int 
int15_handler(xf86Int10InfoPtr pInt)
{
	switch(M.x86.R_AX){
		case 0x4e08:
			M.x86.R_AX=0x4e00;
			break;
		default:
			M.x86.R_AL=0x86;
			break;
	}
			
/*
        switch(M.x86.R_AX){
                case 0x7f03:
                        M.x86.R_BX=0x0;
                        break;
                case 0x7f06:
                        M.x86.R_AX=0x7f;
                        M.x86.R_BL=0x1;
                        break;
                case 0x7f07:
                        M.x86.R_AX=0x007f;
                        M.x86.R_BX=0x0720;
                        M.x86.R_CX=0x126f;
                        break;
                case 0x7f08:
                        M.x86.R_AX=0x007f;
                        M.x86.R_BL=0x3;
                        break;
                case 0x7f09:
                        M.x86.R_AX=0x007f;
                        M.x86.R_BL=0x0;
                        break;
                case 0x7f0a:
                        break;
                case 0x7f11:
                        M.x86.R_AX=0x007f;
                        M.x86.R_BL=0x3;
                        break;
                case 0x7f0f:
                        break;
                default:
                        printk("biosEmu/bios.int15: unknown function AH=%#02x, AL=%#02x, BL=%#02x\n",M.x86.R_AH, M.x86.R_AL, M.x86.R_BL);
        }
	return 1;*/
	return 1;
}

/*
 * This is derived from a number of PC system BIOS'es.  The intent here is to
 * provide very primitive video support, before an EGA/VGA BIOS installs its
 * own interrupt vector.  Here, "Ignored" calls should remain so.  "Not
 * Implemented" denotes functionality that can be implemented should the need
 * arise.  What are "Not Implemented" throughout are video memory accesses.
 * Also, very little input validity checking is done here.
 */
static int
int42_handler(xf86Int10InfoPtr pInt)
{
    switch (X86_AH) {
    case 0x00:
	/* Set Video Mode                                     */
	/* Enter:  AL = video mode number                     */
	/* Leave:  Nothing                                    */
	/* Implemented (except for clearing the screen)       */
	{                                         /* Localise */
	    int i;
	    CARD16 ioport, int1d, regvals, tmp;
	    CARD8 mode, cgamode, cgacolour;

		printf("setting video mode %d\n",X86_AL);
	    /*
	     * Ignore all mode numbers but 0x00-0x13.  Some systems also ignore
	     * 0x0B and 0x0C, but don't do that here.
	     */
	    if (X86_AL > 0x13)
		break;

	    /*
	     * You didn't think that was really the mode set, did you?  There
	     * are only so many slots in the video parameter table...
	     */
	    mode = X86_AL;
	    ioport = 0x03D4;
	    switch (MEM_RB(pInt, 0x0410) & 0x30) {
	    case 0x30:                  /* MDA */
		mode = 0x07;            /* Force mode to 0x07 */
		ioport = 0x03B4;
		break;
	    case 0x10:                  /* CGA 40x25 */
		if (mode >= 0x07)
		    mode = 0x01;
		break;
	    case 0x20:                  /* CGA 80x25 (MCGA?) */
		if (mode >= 0x07)
		    mode = 0x03;
		break;
	    case 0x00:                  /* EGA/VGA */
		if (mode >= 0x07)       /* Don't try MDA timings */
		    mode = 0x01;        /* !?!?! */
		break;
	    }

	    /* Locate data in video parameter table */
	    int1d = MEM_RW(pInt, 0x1d << 2);
	    regvals = ((mode >> 1) << 4) + int1d;
	    cgacolour = 0x30;
	    if (mode == 0x06) {
		regvals -= 0x10;
		cgacolour = 0x3F;
	    }

	    /** Update BIOS Data Area **/

	    /* Video mode */
	    MEM_WB(pInt, 0x0449, mode);

	    /* Columns */
	    tmp = MEM_RB(pInt, mode + int1d + 0x48);
	    MEM_WW(pInt, 0x044A, tmp);

	    /* Page length */
	    tmp = MEM_RW(pInt, (mode & 0x06) + int1d + 0x40);
	    MEM_WW(pInt, 0x044C, tmp);

	    /* Start Address */
	    MEM_WW(pInt, 0x044E, 0);

	    /* Cursor positions, one for each display page */
	    for (i = 0x0450; i < 0x0460; i += 2)
		MEM_WW(pInt, i, 0);

	    /* Cursor start & end scanlines */
	    tmp = MEM_RB(pInt, regvals + 0x0B);
	    MEM_WB(pInt, 0x0460, tmp);
	    tmp = MEM_RB(pInt, regvals + 0x0A);
	    MEM_WB(pInt, 0x0461, tmp);

	    /* Current display page number */
	    MEM_WB(pInt, 0x0462, 0);

	    /* CRTC I/O address */
	    MEM_WW(pInt, 0x0463, ioport);

	    /* CGA Mode register value */
	    cgamode = MEM_RB(pInt, mode + int1d + 0x50);
	    MEM_WB(pInt, 0x0465, cgamode);

	    /* CGA Colour register value */
	    MEM_WB(pInt, 0x0466, cgacolour);

	    /* Rows */
	    MEM_WB(pInt, 0x0484, (25 - 1));

	    /* Programme the mode */
	    linux_outb(cgamode & 0x37,ioport + 4);   /* Turn off screen */
	    for (i = 0; i < 0x10; i++) {
		tmp = MEM_RB(pInt, regvals + i);
		linux_outb(i, ioport);
		linux_outb(tmp, ioport + 1);
	    }
	    linux_outb(cgacolour, ioport + 5);        /* Select colour mode */
	    linux_outb(cgamode, ioport +4);          /* Turn on screen */
	}
	break;

    case 0x01:
	/* Set Cursor Type                                    */
	/* Enter:  CH = starting line for cursor              */
	/*         CL = ending line for cursor                */
	/* Leave:  Nothing                                    */
	/* Implemented                                        */
	{                                         /* Localise */
	    CARD16 ioport = MEM_RW(pInt, 0x0463);

	    MEM_WB(pInt, 0x0460, X86_CL);
	    MEM_WB(pInt, 0x0461, X86_CH);

	    linux_outb(0x0A, ioport);
	    linux_outb(X86_CH, ioport + 1);
	    linux_outb(0x0B, ioport);
	    linux_outb(X86_CL,ioport + 1);
	}
	break;

    case 0x02:
	/* Set Cursor Position                                */
	/* Enter:  BH = display page number                   */
	/*         DH = row                                   */
	/*         DL = column                                */
	/* Leave:  Nothing                                    */
	/* Implemented                                        */
	{                                         /* Localise */
	    CARD16 offset, ioport;

	    MEM_WB(pInt, (X86_BH << 1) + 0x0450, X86_DL);
	    MEM_WB(pInt, (X86_BH << 1) + 0x0451, X86_DH);

	    if (X86_BH != MEM_RB(pInt, 0x0462))
		break;

	    offset = (X86_DH * MEM_RW(pInt, 0x044A)) + X86_DL;
	    offset += MEM_RW(pInt, 0x044E) << 1;

	    ioport = MEM_RW(pInt, 0x0463);
	    linux_outb(0x0E, ioport);
	    linux_outb(offset >> 8,ioport + 1);
	    linux_outb(0x0F,ioport);
	    linux_outb(offset & 0xFF,ioport + 1);
	}
	break;

    case 0x03:
	/* Get Cursor Position                                */
	/* Enter:  BH = display page number                   */
	/* Leave:  CH = starting line for cursor              */
	/*         CL = ending line for cursor                */
	/*         DH = row                                   */
	/*         DL = column                                */
	/* Implemented                                        */
	{                                         /* Localise */
	    X86_CL = MEM_RB(pInt, 0x0460);
	    X86_CH = MEM_RB(pInt, 0x0461);
	    X86_DL = MEM_RB(pInt, (X86_BH << 1) + 0x0450);
	    X86_DH = MEM_RB(pInt, (X86_BH << 1) + 0x0451);
	}
	break;

    case 0x04:
	/* Get Light Pen Position                             */
	/* Enter:  Nothing                                    */
	/* Leave:  AH = 0x01 (down/triggered) or 0x00 (not)   */
	/*         BX = pixel column                          */
	/*         CX = pixel row                             */
	/*         DH = character row                         */
	/*         DL = character column                      */
	/* Not Implemented                                    */
	{                                         /* Localise */
	    printf("int 0x%2.2x(AH=0x04) -- Get Light Pen Position\n", pInt->num);
	    if (verbose > 3) {
		dump_registers(pInt);
		stack_trace(pInt);
	    }
	    X86_AH = X86_BX = X86_CX = X86_DX = 0;
	}
	break;

    case 0x05:
	/* Set Display Page                                   */
	/* Enter:  AL = display page number                   */
	/* Leave:  Nothing                                    */
	/* Implemented                                        */
	{                                         /* Localise */
	    CARD16 start, ioport = MEM_RW(pInt, 0x0463);
	    CARD8 x, y;

	    /* Calculate new start address */
	    MEM_WB(pInt, 0x0462, X86_AL);
	    start = X86_AL * MEM_RW(pInt, 0x044C);
	    MEM_WW(pInt, 0x044E, start);
	    start <<= 1;

	    /* Update start address */
	    linux_outb(0x0C, ioport);
	    linux_outb(start >> 8,ioport + 1);
	    linux_outb(0x0D,ioport);
	    linux_outb(start & 0xFF,ioport + 1);

	    /* Switch cursor position */
	    y = MEM_RB(pInt, (X86_AL << 1) + 0x0450);
	    x = MEM_RB(pInt, (X86_AL << 1) + 0x0451);
	    start += (y * MEM_RW(pInt, 0x044A)) + x;

	    /* Update cursor position */
	    linux_outb(0x0E, ioport);
	    linux_outb(start >> 8,ioport + 1);
	    linux_outb(0x0F, ioport);
	    linux_outb(start & 0xFF,ioport +1);
	}
	break;

    case 0x06:
	/* Initialise or Scroll Window Up                     */
	/* Enter:  AL = lines to scroll up                    */
	/*         BH = attribute for blank                   */
	/*         CH = upper y of window                     */
	/*         CL = left x of window                      */
	/*         DH = lower y of window                     */
	/*         DL = right x of window                     */
	/* Leave:  Nothing                                    */
	/* Not Implemented                                    */
	{                                         /* Localise */
		printf("int 0x%2.2x(AH=0x06) -- Initialise or Scroll Window Up\n",
				pInt->num);
		printf( " AL=0x%2.2x, BH=0x%2.2x,"
				" CH=0x%2.2x, CL=0x%2.2x, DH=0x%2.2x, DL=0x%2.2x\n",
				X86_AL, X86_BH, X86_CH, X86_CL, X86_DH, X86_DL);
		dump_registers(pInt);
		stack_trace(pInt);
	}
	break;

    case 0x07:
	/* Initialise or Scroll Window Down                   */
	/* Enter:  AL = lines to scroll down                  */
	/*         BH = attribute for blank                   */
	/*         CH = upper y of window                     */
	/*         CL = left x of window                      */
	/*         DH = lower y of window                     */
	/*         DL = right x of window                     */
	/* Leave:  Nothing                                    */
	/* Not Implemented                                    */
	{                                         /* Localise */
		printf(
				"int 0x%2.2x(AH=0x07) -- Initialise or Scroll Window Down\n",
				pInt->num);
		printf(
				" AL=0x%2.2x, BH=0x%2.2x,"
				" CH=0x%2.2x, CL=0x%2.2x, DH=0x%2.2x, DL=0x%2.2x\n",
				X86_AL, X86_BH, X86_CH, X86_CL, X86_DH, X86_DL);
		if (verbose > 3) {
			dump_registers(pInt);
			stack_trace(pInt);
		}
	}
	break;

    case 0x08:
	/* Read Character and Attribute at Cursor             */
	/* Enter:  BH = display page number                   */
	/* Leave:  AH = attribute                             */
	/*         AL = character                             */
	/* Not Implemented                                    */
	{                                         /* Localise */
	    printf(
		"int 0x%2.2x(AH=0x08) -- Read Character and Attribute at"
		" Cursor\n", pInt->num);
            printf(
		"BH=0x%2.2x\n", X86_BH);
	    if (verbose > 3) {
		dump_registers(pInt);
		stack_trace(pInt);
	    }
	    X86_AX = 0;
	}
	break;

    case 0x09:
	/* Write Character and Attribute at Cursor            */
	/* Enter:  AL = character                             */
	/*         BH = display page number                   */
	/*         BL = attribute (text) or colour (graphics) */
	/*         CX = replication count                     */
	/* Leave:  Nothing                                    */
	/* Not Implemented                                    */
	{                                         /* Localise */
	    if (verbose>2) printf( "int 0x%2.2x(AH=0x09) -- Write Character and Attribute at"
		" Cursor\n", pInt->num);
	    if (verbose>2) printf( "AL=0x%2.2x, BH=0x%2.2x, BL=0x%2.2x, CX=0x%4.4x\n",
		X86_AL, X86_BH, X86_BL, X86_CX);
	    if (verbose > 3) {
		dump_registers(pInt);
		stack_trace(pInt);
	    }
	}
	break;

    case 0x0a:
	/* Write Character at Cursor                          */
	/* Enter:  AL = character                             */
	/*         BH = display page number                   */
	/*         BL = colour                                */
	/*         CX = replication count                     */
	/* Leave:  Nothing                                    */
	/* Not Implemented                                    */
	{                                         /* Localise */
            if (verbose>=2) printf(
		"int 0x%2.2x(AH=0x0A) -- Write Character at Cursor\n",
		pInt->num);
            if (verbose>=3) printf(
		"AL=0x%2.2x, BH=0x%2.2x, BL=0x%2.2x, CX=0x%4.4x\n",
		X86_AL, X86_BH, X86_BL, X86_CX);
	    if (verbose > 3) {
		dump_registers(pInt);
		stack_trace(pInt);
	    }
	}
	break;

    case 0x0b:
	/* Set Palette, Background or Border                  */
	/* Enter:  BH = 0x00 or 0x01                          */
	/*         BL = colour or palette (respectively)      */
	/* Leave:  Nothing                                    */
	/* Implemented                                        */
	{                                         /* Localise */
	    CARD16 ioport = MEM_RW(pInt, 0x0463) + 5;
	    CARD8 cgacolour = MEM_RB(pInt, 0x0466);

	    if (X86_BH) {
		cgacolour &= 0xDF;
		cgacolour |= (X86_BL & 0x01) << 5;
	    } else {
		cgacolour &= 0xE0;
		cgacolour |= X86_BL & 0x1F;
	    }

	    MEM_WB(pInt, 0x0466, cgacolour);
	    linux_outb(cgacolour,ioport);
	}
	break;

    case 0x0c:
	/* Write Graphics Pixel                               */
	/* Enter:  AL = pixel value                           */
	/*         BH = display page number                   */
	/*         CX = column                                */
	/*         DX = row                                   */
	/* Leave:  Nothing                                    */
	/* Not Implemented                                    */
	{                                         /* Localise */
	    if (verbose>=2) printf( "int 0x%2.2x(AH=0x0C) -- Write Graphics Pixel\n", pInt->num);
	    if (verbose>=3) printf(
		"AL=0x%2.2x, BH=0x%2.2x, CX=0x%4.4x, DX=0x%4.4x\n",
		X86_AL, X86_BH, X86_CX, X86_DX);
	    if (verbose > 3) {
		dump_registers(pInt);
		stack_trace(pInt);
	    }
	}
	break;

    case 0x0d:
	/* Read Graphics Pixel                                */
	/* Enter:  BH = display page number                   */
	/*         CX = column                                */
	/*         DX = row                                   */
	/* Leave:  AL = pixel value                           */
	/* Not Implemented                                    */
	{                                         /* Localise */
	    if (verbose>=2) printf(
		"int 0x%2.2x(AH=0x0D) -- Read Graphics Pixel\n", pInt->num);
	    if (verbose>=3) printf(
		"BH=0x%2.2x, CX=0x%4.4x, DX=0x%4.4x\n",
		X86_BH, X86_CX, X86_DX);
	    if (verbose > 3) {
		dump_registers(pInt);
		stack_trace(pInt);
	    }
	    X86_AL = 0;
	}
	break;

    case 0x0e:
	/* Write Character in Teletype Mode                   */
	/* Enter:  AL = character                             */
	/*         BH = display page number                   */
	/*         BL = foreground colour                     */
	/* Leave:  Nothing                                    */
	/* Not Implemented                                    */
	/* WARNING:  Emulation of BEL characters will require */
	/*           emulation of RTC and PC speaker I/O.     */
	/*           Also, this recurses through int 0x10     */
	/*           which might or might not have been       */
	/*           installed yet.                           */
	{                                         /* Localise */
	    if (verbose>=2) printf(
		"int 0x%2.2x(AH=0x0E) -- Write Character in Teletype Mode\n",
		pInt->num);
	    if (verbose>=3) printf(
		"AL=0x%2.2x, BH=0x%2.2x, BL=0x%2.2x\n",
		X86_AL, X86_BH, X86_BL);
	    if (verbose > 3) {
		dump_registers(pInt);
		stack_trace(pInt);
	    }
	}
	break;

    case 0x0f:
	/* Get Video Mode                                     */
	/* Enter:  Nothing                                    */
	/* Leave:  AH = number of columns                     */
	/*         AL = video mode number                     */
	/*         BH = display page number                   */
	/* Implemented                                        */
	{                                         /* Localise */
	    X86_AH = MEM_RW(pInt, 0x044A);
	    X86_AL = MEM_RB(pInt, 0x0449);
	    X86_BH = MEM_RB(pInt, 0x0462);
	}
	break;

    case 0x10:
	/* Colour Control (subfunction in AL)                 */
	/* Enter:  Various                                    */
	/* Leave:  Various                                    */
	/* Ignored                                            */
	break;

    case 0x11:
	/* Font Control (subfunction in AL)                   */
	/* Enter:  Various                                    */
	/* Leave:  Various                                    */
	/* Ignored                                            */
	break;

    case 0x12:
	/* Miscellaneous (subfunction in BL)                  */
	/* Enter:  Various                                    */
	/* Leave:  Various                                    */
	/* Ignored.  Previous code here optionally allowed    */
	/* the enabling and disabling of VGA, but no system   */
	/* BIOS I've come across actually implements it.      */
	break;

    case 0x13:
	/* Write String in Teletype Mode                      */
	/* Enter:  AL = write mode                            */
	/*         BL = attribute (if (AL & 0x02) == 0)       */
	/*         CX = string length                         */
	/*         DH = row                                   */
	/*         DL = column                                */
	/*         ES:BP = string segment:offset              */
	/* Leave:  Nothing                                    */
	/* Not Implemented                                    */
	/* WARNING:  Emulation of BEL characters will require */
	/*           emulation of RTC and PC speaker I/O.     */
	/*           Also, this recurses through int 0x10     */
	/*           which might or might not have been       */
	/*           installed yet.                           */
	{                                         /* Localise */
	    if (verbose>=2) printf(
		"int 0x%2.2x(AH=0x13) -- Write String in Teletype Mode\n",
		pInt->num);
	    if (verbose>=3) printf(
		"AL=0x%2.2x, BL=0x%2.2x, CX=0x%4.4x,"
		" DH=0x%2.2x, DL=0x%2.2x, ES:BP=0x%4.4x:0x%4.4x\n",
		X86_AL, X86_BL, X86_CX, X86_DH, X86_DL, X86_ES, X86_BP);
	    if (verbose > 3) {
		dump_registers(pInt);
		stack_trace(pInt);
	    }
	}
	break;

    default:
	/* Various extensions                                 */
	/* Enter:  Various                                    */
	/* Leave:  Various                                    */
	/* Ignored                                            */
	break;
    }

    return 1;
}
#endif

#define SUCCESSFUL              0x00
#define DEVICE_NOT_FOUND        0x86
#define BAD_REGISTER_NUMBER     0x87


static int
int1A_handler(xf86Int10InfoPtr pInt)
{
    u32 pciSlot;
    u32 mconfig; 
    struct pci_device *pdev;

    /* Fail if no PCI device information has been registered */
    if (!( pdev = pInt->pdev))
        return 0;

     pciSlot=pdev->pa.pa_tag;

     pciSlot = (pdev->pa.pa_bus<<8) | ( (pdev->pa.pa_device<<3) | pdev->pa.pa_function);
//     printk("int 0x1a: ax=0x%x bx=0x%x cx=0x%x dx=0x%x di=0x%x es=0x%,pciSlot=%x\n",
//		                   M.x86.R_AX, M.x86.R_BX,M.x86.R_CX,M.x86.R_DX,M.x86.R_DI,M.x86.R_ES,pciSlot);

    switch (M.x86.R_AX) {
        case 0xB101:                    /* PCI bios present? */
            M.x86.R_EAX  &= 0xFF00;         /* no config space/special cycle generation support */
            M.x86.R_EDX = 0x20494350;   /* " ICP" */
            M.x86.R_BX  = 0x0210;       /* Version 2.10 */
            M.x86.R_ECX  &= 0xff00;            /* Max bus number in system */
            M.x86.R_ECX  |= 0xff;            /* Max bus number in system */
            CLEAR_FLAG(F_CF);
	    M.x86.debug|=0x2000;
	    //X86EMU_trace_on();
            break;
        case 0xB102:                    /* Find PCI device */
            M.x86.R_AH = DEVICE_NOT_FOUND;
            if (M.x86.R_DX == PCI_VENDOR(pdev->pa.pa_id) &&
                    M.x86.R_CX == pdev->pa.pa_device &&
                    M.x86.R_SI == 0) {
                M.x86.R_AH = SUCCESSFUL;
                M.x86.R_EBX = pciSlot;
                }
            CONDITIONAL_SET_FLAG((M.x86.R_AH != SUCCESSFUL), F_CF);
            break;
        case 0xB103:                    /* Find PCI class code */
            M.x86.R_AH = DEVICE_NOT_FOUND;
            if (1) {
               M.x86.R_AH = SUCCESSFUL;
               M.x86.R_EBX = pciSlot;
            }
            CONDITIONAL_SET_FLAG((M.x86.R_AH != SUCCESSFUL), F_CF);
            break;
        case 0xB108:                    /* Read configuration byte */
            M.x86.R_AH = BAD_REGISTER_NUMBER;
            if (M.x86.R_EBX == pciSlot) {
                M.x86.R_AH = SUCCESSFUL;
		mconfig=_pci_conf_read(pdev->pa.pa_tag,M.x86.R_DI&0xfc);
		if((M.x86.R_DI&(~0xfc))==1)
		{
			mconfig=mconfig&0x0000ff00;
			mconfig>>=8;
		}else if((M.x86.R_DI&(~0xfc))==2)
		{
			mconfig=mconfig&0x00ff0000;
			mconfig>>=16;
		}else if((M.x86.R_DI&(~0xfc))==3)
		{
			mconfig=mconfig&0xff000000;
			mconfig>>=24;
		}else{}
		M.x86.R_CL=(u8)mconfig;	
		//M.x86.R_CL=_pci_conf_readn(pdev->pa.pa_tag,M.x86.R_DI,1);
                printf("read_config_byte(0x%x)=0x%x\n",M.x86.R_DI, M.x86.R_CL);
                }

            CONDITIONAL_SET_FLAG((M.x86.R_AH != SUCCESSFUL), F_CF);
            break;
        case 0xB109:                    /* Read configuration word */
            M.x86.R_AH = BAD_REGISTER_NUMBER;
            if (M.x86.R_EBX == pciSlot) {
                M.x86.R_AH = SUCCESSFUL;
		mconfig=_pci_conf_read(pdev->pa.pa_tag,M.x86.R_DI&0xfc);
		if((M.x86.R_DI&(~0xfc))==2)
		{
			mconfig&=0xffff0000;
			mconfig>>=16;
		}else if((M.x86.R_DI&(~0xfc))==1)
		{
			mconfig&=0x00ffff00;
			mconfig>>=8;
		}else{}
		
		M.x86.R_CX=(u16)mconfig;
		//M.x86.R_CX=_pci_conf_readn(pdev->pa.pa_tag,M.x86.R_DI,2);
               printf("read_config_word(0x%x)=0x%x\n",M.x86.R_DI, M.x86.R_CX);

                }
            CONDITIONAL_SET_FLAG((M.x86.R_AH != SUCCESSFUL), F_CF);
            break;
        case 0xB10A:                    /* Read configuration dword */
            M.x86.R_AH = BAD_REGISTER_NUMBER;
            if (M.x86.R_EBX == pciSlot) {
                M.x86.R_AH = SUCCESSFUL;
		M.x86.R_ECX=_pci_conf_read(pdev->pa.pa_tag,M.x86.R_DI);
                printf("read_config_dword(0x%x)=0x%x\n",M.x86.R_DI, M.x86.R_ECX);

                }
            CONDITIONAL_SET_FLAG((M.x86.R_AH != SUCCESSFUL), F_CF);
	    //X86EMU_trace_on();
            break;
        case 0xB10B:                    /* Write configuration byte */
            M.x86.R_AH = BAD_REGISTER_NUMBER;
            if (M.x86.R_EBX == pciSlot) {
                M.x86.R_AH = SUCCESSFUL;
		_pci_conf_writen(pdev->pa.pa_tag,M.x86.R_DI,M.x86.R_CL,1);
               printf("write_config_byte(0x%x)=0x%x\n",M.x86.R_DI, M.x86.R_CL);

                }
            CONDITIONAL_SET_FLAG((M.x86.R_AH != SUCCESSFUL), F_CF);
            break;
        case 0xB10C:                    /* Write configuration word */
            M.x86.R_AH = BAD_REGISTER_NUMBER;
            if (M.x86.R_EBX == pciSlot) {
                M.x86.R_AH = SUCCESSFUL;
		_pci_conf_writen(pdev->pa.pa_tag,M.x86.R_DI,M.x86.R_CX,2);
                }
            CONDITIONAL_SET_FLAG((M.x86.R_AH != SUCCESSFUL), F_CF);
            break;
        case 0xB10D:                    /* Write configuration dword */
            M.x86.R_AH = BAD_REGISTER_NUMBER;
            if (M.x86.R_EBX == pciSlot) {
                M.x86.R_AH = SUCCESSFUL;
		_pci_conf_write(pdev->pa.pa_tag,M.x86.R_DI,M.x86.R_ECX);
                }
            CONDITIONAL_SET_FLAG((M.x86.R_AH != SUCCESSFUL), F_CF);
            break;
        default:
            printk("biosEmu/bios.int1a: unknown function AX=%#04x\n", M.x86.R_AX);
        }
	return 1;
}

/*
 * handle initialization
 */
static int
intE6_handler(xf86Int10InfoPtr pInt)
{
    struct pci_device *pdev;

    if ((pdev = pInt->pdev))
	X86_AX = (pdev->pa.pa_bus << 8) | (pdev->pa.pa_device << 3) | (pdev->pa.pa_function & 0x7);
    pushw(pInt, X86_CS);
    pushw(pInt, X86_IP);
    X86_CS = pInt->BIOSseg;
    X86_EIP = 0x0003;
    X86_ES = 0;                  /* standard pc es */
    return 1;
}
