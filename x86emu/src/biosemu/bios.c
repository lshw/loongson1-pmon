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
* Description:  Module implementing the BIOS specific functions.
*
****************************************************************************/
/* BE CAREFUL: outb here is using linux style outb(value,addr)
 * while pmon&xfree86 are different
 */
#include <stdio.h>
#include <dev/pci/pcivar.h>
#include <linux/types.h>
#include <linux/pci.h>
#include <linux/io.h>

#include "biosemui.h"

/****************************************************************************
PARAMETERS:
intno   - Interrupt number being serviced

REMARKS:
Handler for undefined interrupts.
****************************************************************************/
static void  undefined_intr(
    int intno)
{
    if (BE_rdw(intno * 4 + 2) == BIOS_SEG)
        printf("biosEmu: undefined interrupt %xh called!\n",intno);
    else
        X86EMU_prepareForInt(intno);
}

/****************************************************************************
PARAMETERS:
intno   - Interrupt number being serviced

REMARKS:
This function handles the default system BIOS Int 10h (the default is stored
in the Int 42h vector by the system BIOS at bootup). We only need to handle
a small number of special functions used by the BIOS during POST time.
****************************************************************************/
static void  int42(
    int intno)
{
    switch (M.x86.R_AH) {
    case 0x00:
	/* Set Video Mode                                     */
	/* Enter:  AL = video mode number                     */
	/* Leave:  Nothing                                    */
	/* Implemented (except for clearing the screen)       */
	{                                         /* Localise */
	    int i;
	    u16 ioport, int1d, regvals, tmp;
	    u8 mode, cgamode, cgacolour;

	    /*
	     * Ignore all mode numbers but 0x00-0x13.  Some systems also ignore
	     * 0x0B and 0x0C, but don't do that here.
	     */
	    if (M.x86.R_AL > 0x13)
		break;

	    /*
	     * You didn't think that was really the mode set, did you?  There
	     * are only so many slots in the video parameter table...
	     */
	    mode = M.x86.R_AL;
	    ioport = 0x03D4;
	    switch (BE_rdb( 0x0410) & 0x30) {
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
	    int1d = BE_rdw( 0x1d << 2);
	    regvals = ((mode >> 1) << 4) + int1d;
	    cgacolour = 0x30;
	    if (mode == 0x06) {
		regvals -= 0x10;
		cgacolour = 0x3F;
	    }

	    /** Update BIOS Data Area **/

	    /* Video mode */
	    BE_wrb( 0x0449, mode);

	    /* Columns */
	    tmp = BE_rdb( mode + int1d + 0x48);
	    BE_wrw( 0x044A, tmp);

	    /* Page length */
	    tmp = BE_rdw( (mode & 0x06) + int1d + 0x40);
	    BE_wrw( 0x044C, tmp);

	    /* Start Address */
	    BE_wrw( 0x044E, 0);

	    /* Cursor positions, one for each display page */
	    for (i = 0x0450; i < 0x0460; i += 2)
		BE_wrw( i, 0);

	    /* Cursor start & end scanlines */
	    tmp = BE_rdb( regvals + 0x0B);
	    BE_wrb( 0x0460, tmp);
	    tmp = BE_rdb( regvals + 0x0A);
	    BE_wrb( 0x0461, tmp);

	    /* Current display page number */
	    BE_wrb( 0x0462, 0);

	    /* CRTC I/O address */
	    BE_wrw( 0x0463, ioport);

	    /* CGA Mode register value */
	    cgamode = BE_rdb( mode + int1d + 0x50);
	    BE_wrb( 0x0465, cgamode);

	    /* CGA Colour register value */
	    BE_wrb( 0x0466, cgacolour);

	    /* Rows */
	    BE_wrb( 0x0484, (25 - 1));

	    /* Programme the mode */
	    linux_outb(cgamode & 0x37, ioport + 4);   /* Turn off screen */
	    for (i = 0; i < 0x10; i++) {
		tmp = BE_rdb( regvals + i);
		linux_outb(i, ioport);
		linux_outb(tmp, ioport + 1);
	    }
	    linux_outb(cgacolour, ioport + 5);        /* Select colour mode */
	    linux_outb(cgamode, ioport + 4);          /* Turn on screen */
	}
	break;

    case 0x01:
	/* Set Cursor Type                                    */
	/* Enter:  CH = starting line for cursor              */
	/*         CL = ending line for cursor                */
	/* Leave:  Nothing                                    */
	/* Implemented                                        */
	{                                         /* Localise */
	    u16 ioport = BE_rdw( 0x0463);

	    BE_wrb( 0x0460, M.x86.R_CL);
	    BE_wrb( 0x0461, M.x86.R_CH);

	    linux_outb(0x0A, ioport);
	    linux_outb(M.x86.R_CH, ioport + 1);
	    linux_outb(0x0B, ioport);
	    linux_outb(M.x86.R_CL, ioport + 1);
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
	    u16 offset, ioport;

	    BE_wrb( (M.x86.R_BH << 1) + 0x0450, M.x86.R_DL);
	    BE_wrb( (M.x86.R_BH << 1) + 0x0451, M.x86.R_DH);

	    if (M.x86.R_BH != BE_rdb( 0x0462))
		break;

	    offset = (M.x86.R_DH * BE_rdw( 0x044A)) + M.x86.R_DL;
	    offset += BE_rdw( 0x044E) << 1;

	    ioport = BE_rdw( 0x0463);
	    linux_outb(0x0E, ioport);
	    linux_outb(offset >> 8, ioport + 1);
	    linux_outb(0x0F, ioport);
	    linux_outb(offset&0xFF,ioport + 1);
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
	    M.x86.R_CL = BE_rdb( 0x0460);
	    M.x86.R_CH = BE_rdb( 0x0461);
	    M.x86.R_DL = BE_rdb( (M.x86.R_BH << 1) + 0x0450);
	    M.x86.R_DH = BE_rdb( (M.x86.R_BH << 1) + 0x0451);
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
	    printf("Not implemented: Get Light Pen Position\n");
	    M.x86.R_AH = M.x86.R_BX = M.x86.R_CX = M.x86.R_DX = 0;
	}
	break;

    case 0x05:
	/* Set Display Page                                   */
	/* Enter:  AL = display page number                   */
	/* Leave:  Nothing                                    */
	/* Implemented                                        */
	{                                         /* Localise */
	    u16 start, ioport = BE_rdw( 0x0463);
	    u8 x, y;

	    /* Calculate new start address */
	    BE_wrb( 0x0462, M.x86.R_AL);
	    start = M.x86.R_AL * BE_rdw( 0x044C);
	    BE_wrw( 0x044E, start);
	    start <<= 1;

	    /* Update start address */
	    linux_outb(0x0C, ioport);
	    linux_outb(start>>8,ioport + 1);
	    linux_outb(0x0D, ioport);
	    linux_outb(start&0xFF,ioport + 1);

	    /* Switch cursor position */
	    y = BE_rdb( (M.x86.R_AL << 1) + 0x0450);
	    x = BE_rdb( (M.x86.R_AL << 1) + 0x0451);
	    start += (y * BE_rdw( 0x044A)) + x;

	    /* Update cursor position */
	    linux_outb(0x0E, ioport);
	    linux_outb(start >> 8,ioport + 1); 
	    linux_outb(0x0F, ioport); 
	    linux_outb(start & 0xFF, ioport + 1); 
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
	    printf("Not implemented:Initialise or Scroll window up\n");
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
	    printf("Not implemented:Initialise or Scroll window up\n");
	}
	break;

    case 0x08:
	/* Read Character and Attribute at Cursor             */
	/* Enter:  BH = display page number                   */
	/* Leave:  AH = attribute                             */
	/*         AL = character                             */
	/* Not Implemented                                    */
	{                                         /* Localise */
	    printf("Not implemented:Initialise or Scroll window up\n");
	    M.x86.R_AX = 0;
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
	    printf("Not implemented:Write Character and Attibute at Cursor\n");
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
	    printf("Not implemented:Write Character at Cursor\n");
	}
	break;

    case 0x0b:
	/* Set Palette, Background or Border                  */
	/* Enter:  BH = 0x00 or 0x01                          */
	/*         BL = colour or palette (respectively)      */
	/* Leave:  Nothing                                    */
	/* Implemented                                        */
	{                                         /* Localise */
	    u16 ioport = BE_rdw( 0x0463) + 5;
	    u8 cgacolour = BE_rdb( 0x0466);

	    if (M.x86.R_BH) {
		cgacolour &= 0xDF;
		cgacolour |= (M.x86.R_BL & 0x01) << 5;
	    } else {
		cgacolour &= 0xE0;
		cgacolour |= M.x86.R_BL & 0x1F;
	    }

	    BE_wrb( 0x0466, cgacolour);
	    linux_outb(cgacolour, ioport);
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
	    printf("Not implemented: Write Graphics Pixel\n");
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
	    printf("Not implemented: Read Graphics Pixel\n");
	    M.x86.R_AL = 0;
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
	    printf("Not implemented: Write Character in Telebyte Mode\n");
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
	    M.x86.R_AH = BE_rdw( 0x044A);
	    M.x86.R_AL = BE_rdb( 0x0449);
	    M.x86.R_BH = BE_rdb( 0x0462);
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
	    printf("Not implemented: Write String in Telebyte Mode\n");
	}
	break;

    default:
        printf("biosEmu/bios.int42: unknown function AH=%#02x, AL=%#02x, BL=%#02x\n",M.x86.R_AH, M.x86.R_AL, M.x86.R_BL);
	/* Various extensions                                 */
	/* Enter:  Various                                    */
	/* Leave:  Various                                    */
	/* Ignored                                            */
	break;
    }
}

/****************************************************************************
PARAMETERS:
intno   - Interrupt number being serviced

REMARKS:
This function handles the default system BIOS Int 10h. If the POST code
has not yet re-vectored the Int 10h BIOS interrupt vector, we handle this
by simply calling the int42 interrupt handler above. Very early in the
BIOS POST process, the vector gets replaced and we simply let the real
mode interrupt handler process the interrupt.
****************************************************************************/
static void  int10(
    int intno)
{
    if (BE_rdw(intno * 4 + 2) == BIOS_SEG)
        int42(intno);
    else
        X86EMU_prepareForInt(intno);
}



static void  int6d(
    int intno)
{
	int10(intno);
}


/* Result codes returned by the PCI BIOS */

#define SUCCESSFUL          0x00
#define FUNC_NOT_SUPPORT    0x81
#define BAD_VENDOR_ID       0x83
#define DEVICE_NOT_FOUND    0x86
#define BAD_REGISTER_NUMBER 0x87
#define SET_FAILED          0x88
#define BUFFER_TOO_SMALL    0x89

/****************************************************************************
PARAMETERS:
intno   - Interrupt number being serviced

REMARKS:
This function handles the default Int 1Ah interrupt handler for the real
mode code, which provides support for the PCI BIOS functions. Since we only
want to allow the real mode BIOS code *only* see the PCI config space for
its own device, we only return information for the specific PCI config
space that we have passed in to the init function. This solves problems
when using the BIOS to warm boot a secondary adapter when there is an
identical adapter before it on the bus (some BIOS'es get confused in this
case).
****************************************************************************/
static void  int1A(int unused)
{
    u16 pciSlot;

    /* Fail if no PCI device information has been registered */
    if (!_BE_env.vgaInfo.pciInfo)
        return;
    pciSlot = (u16)((_BE_env.vgaInfo.pciInfo->pa.pa_bus<<8)|(_BE_env.vgaInfo.pciInfo->pa.pa_device<<3)|(_BE_env.vgaInfo.pciInfo->pa.pa_function&0x7));
#ifdef DEBUG_EMU_VGA
printf("int 0x1a: ax=0x%x bx=0x%x cx=0x%x dx=0x%x di=0x%x es=0x%x\n",
					   M.x86.R_EAX, M.x86.R_EBX,M.x86.R_ECX,M.x86.R_EDX,M.x86.R_EDI,M.x86.R_ES);
#endif
    switch (M.x86.R_AX) {
        case 0xB101:                    /* PCI bios present? */
            M.x86.R_EAX  &= 0xFF00;         /* no config space/special cycle generation support */
            M.x86.R_EDX = 0x20494350;   /* " ICP" */
            M.x86.R_BX  = 0x0210;       /* Version 2.10 */
            M.x86.R_ECX  &= 0xff00;            /* Max bus number in system */
            CLEAR_FLAG(F_CF);
            break;
        case 0xB102:                    /* Find PCI device */
            M.x86.R_AH = DEVICE_NOT_FOUND;
            if (M.x86.R_DX == PCI_VENDOR(_BE_env.vgaInfo.pciInfo->pa.pa_id) &&
                    M.x86.R_CX == _BE_env.vgaInfo.pciInfo->pa.pa_device &&
                    M.x86.R_SI == 0) {
                M.x86.R_AH = SUCCESSFUL;
                M.x86.R_BX = pciSlot;
                }
            CONDITIONAL_SET_FLAG((M.x86.R_AH != SUCCESSFUL), F_CF);
            break;
        case 0xB103:                    /* Find PCI class code */
            M.x86.R_AH = DEVICE_NOT_FOUND;
            if (M.x86.R_ECX == _BE_env.vgaInfo.pciInfo->pa.pa_class) {
               M.x86.R_AH = SUCCESSFUL;
               M.x86.R_BX = pciSlot;
            }
            CONDITIONAL_SET_FLAG((M.x86.R_AH != SUCCESSFUL), F_CF);
            break;
        case 0xB108:                    /* Read configuration byte */
            M.x86.R_AH = BAD_REGISTER_NUMBER;
            if (M.x86.R_BX == pciSlot) {
                M.x86.R_AH = SUCCESSFUL;
		pci_read_config_byte(_BE_env.vgaInfo.pciInfo,M.x86.R_DI,&M.x86.R_CL);
		printf("read_config_byte(0x%x)=0x%x\n",M.x86.R_DI, M.x86.R_CL);
                }
            CONDITIONAL_SET_FLAG((M.x86.R_AH != SUCCESSFUL), F_CF);
            break;
        case 0xB109:                    /* Read configuration word */
            M.x86.R_AH = BAD_REGISTER_NUMBER;
            if (M.x86.R_BX == pciSlot) {
                M.x86.R_AH = SUCCESSFUL;
		pci_read_config_word(_BE_env.vgaInfo.pciInfo,M.x86.R_DI,&M.x86.R_CX);
		printf("read_config_word(0x%x)=0x%x\n",M.x86.R_DI, M.x86.R_CX);
                }
            CONDITIONAL_SET_FLAG((M.x86.R_AH != SUCCESSFUL), F_CF);
            break;
        case 0xB10A:                    /* Read configuration dword */
            M.x86.R_AH = BAD_REGISTER_NUMBER;
            if (M.x86.R_BX == pciSlot) {
                M.x86.R_AH = SUCCESSFUL;
		pci_read_config_dword(_BE_env.vgaInfo.pciInfo,M.x86.R_DI,&M.x86.R_ECX);
		printf("read_config_dword(0x%x)=0x%x\n",M.x86.R_DI, M.x86.R_ECX);
                }
            CONDITIONAL_SET_FLAG((M.x86.R_AH != SUCCESSFUL), F_CF);
            break;
        case 0xB10B:                    /* Write configuration byte */
            M.x86.R_AH = BAD_REGISTER_NUMBER;
            if (M.x86.R_BX == pciSlot) {
                M.x86.R_AH = SUCCESSFUL;
		pci_write_config_byte(_BE_env.vgaInfo.pciInfo,M.x86.R_DI,M.x86.R_CL);
		printf("write_config_byte(0x%x)=0x%x\n",M.x86.R_DI, M.x86.R_CL);
                }
            CONDITIONAL_SET_FLAG((M.x86.R_AH != SUCCESSFUL), F_CF);
            break;
        case 0xB10C:                    /* Write configuration word */
            M.x86.R_AH = BAD_REGISTER_NUMBER;
            if (M.x86.R_BX == pciSlot) {
                M.x86.R_AH = SUCCESSFUL;
		pci_write_config_word(_BE_env.vgaInfo.pciInfo,M.x86.R_DI,M.x86.R_CX);
                }
            CONDITIONAL_SET_FLAG((M.x86.R_AH != SUCCESSFUL), F_CF);
            break;
        case 0xB10D:                    /* Write configuration dword */
            M.x86.R_AH = BAD_REGISTER_NUMBER;
            if (M.x86.R_BX == pciSlot) {
                M.x86.R_AH = SUCCESSFUL;
		pci_write_config_dword(_BE_env.vgaInfo.pciInfo,M.x86.R_DI,M.x86.R_ECX);
                }
            CONDITIONAL_SET_FLAG((M.x86.R_AH != SUCCESSFUL), F_CF);
            break;
        default:
            printf("biosEmu/bios.int1a: unknown function AX=%#04x\n", M.x86.R_AX);
        }
}


/* inte6: handle initialization */
static void  inte6(int unused)
{
	printf("int E6: not implemented\n");
}
#if 1
static void  int15(int intno)
{
    switch (M.x86.R_AX) {
	case 0x7f01:
		printf("int 15:AX=%x\n",M.x86.R_AX);
		M.x86.R_AX=0x007f;
		M.x86.R_BL=0x0;
		break;
	case 0x7f03:
		printf("int 15:AX=%x\n",M.x86.R_AX);
		M.x86.R_BX=0x0;
		break;
	case 0x7f05:
		printf("int 15:AX=%x\n",M.x86.R_AX);
		M.x86.R_AX=0x007f;
		break;
	case 0x7f06:
		printf("int 15:AX=%x\n",M.x86.R_AX);
		M.x86.R_AX=0x007f;
		M.x86.R_BL=0x0;
		break;
	case 0x7f07:
		printf("int 15:AX=%x\n",M.x86.R_AX);
		M.x86.R_AX=0x007f;
		pci_read_config_word(_BE_env.vgaInfo.pciInfo,0x0,&M.x86.R_BX);
		pci_read_config_word(_BE_env.vgaInfo.pciInfo,0x2,&M.x86.R_CX);
		break;
	case 0x7f08:
		printf("int 15:AX=%x\n",M.x86.R_AX);
		M.x86.R_AX=0x007f;
		M.x86.R_BL=M.x86.R_BL & 0xf0;
		break;
	case 0x7f09:
		printf("int 15:AX=%x\n",M.x86.R_AX);
		M.x86.R_AX=0x007f;
		M.x86.R_BL=0x0;
		break;
	case 0x7f0a:
		printf("int 15:AX=%x\n",M.x86.R_AX);
		M.x86.R_AX=0x007f;
		M.x86.R_BL=0x0;
		break;
	case 0x7f14:
		printf("int 15:AX=%x\n",M.x86.R_AX);
		M.x86.R_AX=0x007f;
		M.x86.R_BL=0x1;
		break;
         default:
		printf("int 15:AX=%x is not emulation\n",M.x86.R_AX);
    }		
}
#endif
/****************************************************************************
REMARKS:
This function initialises the BIOS emulation functions for the specific
PCI display device. We insulate the real mode BIOS from any other devices
on the bus, so that it will work correctly thinking that it is the only
device present on the bus (ie: avoiding any adapters present in from of
the device we are trying to control).
****************************************************************************/
void _BE_bios_init(
    u32 *intrTab)
{
    int                 i;
    X86EMU_intrFuncs    bios_intr_tab[256];

    for (i = 0; i < 256; ++i) {
        intrTab[i] = BIOS_SEG << 16;
        bios_intr_tab[i] = undefined_intr;
    }
    bios_intr_tab[0x10] = int10;
    bios_intr_tab[0x1A] = int1A;
    bios_intr_tab[0x42] = int42;
    bios_intr_tab[0x6d] = int6d;
    bios_intr_tab[0xe6] = inte6;
    bios_intr_tab[0x15] = int15;
	
    X86EMU_setupIntrFuncs(bios_intr_tab);
}

/* fill into emulator memory values that a vga bios might expect */
int
fillin_key_datas(void)
{
    /*
     * This table is normally located at 0xF000:0xF0A4.  However, int 0x42,
     * function 0 (Mode Set) expects it (or a copy) somewhere in the bottom
     * 64kB.  Note that because this data doesn't survive POST, int 0x42 should
     * only be used during EGA/VGA BIOS initialisation.
     */
    const char VideoParms[] = {
	/* Timing for modes 0x00 & 0x01 */
	0x38, 0x28, 0x2d, 0x0a, 0x1f, 0x06, 0x19, 0x1c,
	0x02, 0x07, 0x06, 0x07, 0x00, 0x00, 0x00, 0x00,
	/* Timing for modes 0x02 & 0x03 */
	0x71, 0x50, 0x5a, 0x0a, 0x1f, 0x06, 0x19, 0x1c,
	0x02, 0x07, 0x06, 0x07, 0x00, 0x00, 0x00, 0x00,
	/* Timing for modes 0x04, 0x05 & 0x06 */
	0x38, 0x28, 0x2d, 0x0a, 0x7f, 0x06, 0x64, 0x70,
	0x02, 0x01, 0x06, 0x07, 0x00, 0x00, 0x00, 0x00,
	/* Timing for mode 0x07 */
	0x61, 0x50, 0x52, 0x0f, 0x19, 0x06, 0x19, 0x19,
	0x02, 0x0d, 0x0b, 0x0c, 0x00, 0x00, 0x00, 0x00,
	/* Display page lengths in little endian order */
	0x00, 0x08, /* Modes 0x00 and 0x01 */
	0x00, 0x10, /* Modes 0x02 and 0x03 */
	0x00, 0x40, /* Modes 0x04 and 0x05 */
	0x00, 0x40, /* Modes 0x06 and 0x07 */
	/* Number of columns for each mode */
	40, 40, 80, 80, 40, 40, 80, 80,
	/* CGA Mode register value for each mode */
	0x2c, 0x28, 0x2d, 0x29, 0x2a, 0x2e, 0x1e, 0x29,
	/* Padding */
	0x00, 0x00, 0x00, 0x00
	};
    int i;
    unsigned char *sys_base,*mem_base;

    mem_base = (char *)M.mem_base;
    sys_base = (char *)_BE_env.sysmem_base;

    for (i = 0; i < sizeof(VideoParms); i++)
	    mem_base[i + 0x1000 - sizeof(VideoParms)] = VideoParms[i];


    BE_wrw( 0x1d<<2 , 0x1000 - sizeof(VideoParms)); /* ?? */
    BE_wrw((0x1d<<2) + 2, 0);

    /* font tables default location (int 1F) */
    BE_wrw(0x1f<<2,0xfa6e);

    /* int 11 default location (Get Equipment Configuration) */
    BE_wrw( 0x11 << 2, 0xf84d);
    /* int 12 default location (Get Conventional Memory Size) */
    BE_wrw( 0x12 << 2, 0xf841);
    /* int 15 default location (I/O System Extensions) */
    BE_wrw( 0x15 << 2, 0xf859);
    /* int 1A default location (RTC, PCI and others) */
    BE_wrw( 0x1a << 2, 0xff6e);
    /* int 05 default location (Bound Exceeded) */
    BE_wrw( 0x05 << 2, 0xff54);
    /* int 08 default location (Double Fault) */
    BE_wrw( 0x08 << 2, 0xfea5);
    /* int 13 default location (Disk) */
    BE_wrw( 0x13 << 2, 0xec59);
    /* int 0E default location (Page Fault) */
    BE_wrw( 0x0e << 2, 0xef57);
    /* int 17 default location (Parallel Port) */
    BE_wrw( 0x17 << 2, 0xefd2);
    /* fdd table default location (int 1e) */
    BE_wrw( 0x1e << 2, 0xefc7);

    /* Set Equipment flag to VGA?? */
    i = BE_rdb(0x0410) & 0xCF; 
    BE_wrb(0x0410, 0);
    /* XXX Perhaps setup more of the BDA here.  See also int42(0x00). */

    /* set bios date */
    strcpy(sys_base + 0x0FFF5, "06/11/99");
    /* set up eisa ident string */
    strcpy(sys_base + 0x0FFD9, "PCI_ISA");
    /* write system model id for IBM-AT */
    *((unsigned char *)(sys_base + 0x0FFFE)) = 0xfc;

    /*printf("M.membase=%lx,BE_env.sysmem_base=%lx,sys_base=%p,mem_base=%p\n",M.mem_base,_BE_env.sysmem_base,sys_base,mem_base);*/
    printf("%c%c\n",BE_rdb(0xFFFD9),BE_rdb(0xFFFDA));
    return 1;
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
LockLegacyVGA(void)
{
#if 0
    _BE_env.vgaInfo.save_msr = linux_inb(0x3CC);
    _BE_env.vgaInfo.save_vse = linux_inb(0x3C3);
    _BE_env.vgaInfo.save_46e8 =linux_inb(0x46e8);
    _BE_env.vgaInfo.save_pos102 = linux_inb(0x102);
    linux_outb(~0x03 & _BE_env.vgaInfo.save_msr,0x3c2);
    linux_outb(~0x01 & _BE_env.vgaInfo.save_vse,0x3c3);
    linux_outb(~0x08 & _BE_env.vgaInfo.save_46e8,0x46e8);
    linux_outb(~0x01 & _BE_env.vgaInfo.save_pos102,0x102);
#endif
}

void
UnlockLegacyVGA(void)
{
#if 0
    linux_outb(_BE_env.vgaInfo.save_pos102,0x102);
    linux_outb(_BE_env.vgaInfo.save_46e8,0x46e8);
    linux_outb(_BE_env.vgaInfo.save_vse,0x3c3);
    linux_outb(_BE_env.vgaInfo.save_msr,0x3c2);
#endif
}

