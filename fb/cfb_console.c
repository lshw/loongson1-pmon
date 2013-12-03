/*
 * (C) Copyright 2002 ELTEC Elektronik AG
 * Frank Gottschling <fgottschling@eltec.de>
 *
 * See file CREDITS for list of people who contributed to this
 * project.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */

/*
 * cfb_console.c
 *
 * Color Framebuffer Console driver for 8/15/16/24/32 bits per pixel.
 *
 * At the moment only the 8x16 font is tested and the font fore- and
 * background color is limited to black/white/gray colors. The Linux
 * logo can be placed in the upper left corner and additional board
 * information strings (that normaly goes to serial port) can be drawed.
 *
 * The console driver can use the standard PC keyboard interface (i8042)
 * for character input. Character output goes to a memory mapped video
 * framebuffer with little or big-endian organisation.
 * With environment setting 'console=serial' the console i/o can be
 * forced to serial port.

 The driver uses graphic specific defines/parameters/functions:

 (for SMI LynxE graphic chip)

 CONFIG_VIDEO_SMI_LYNXEM - use graphic driver for SMI 710,712,810
 VIDEO_FB_LITTLE_ENDIAN	 - framebuffer organisation default: big endian
 VIDEO_HW_RECTFILL	 - graphic driver supports hardware rectangle fill
 VIDEO_HW_BITBLT	 - graphic driver supports hardware bit blt

 Console Parameters are set by graphic drivers global struct:

 VIDEO_VISIBLE_COLS	     - x resolution
 VIDEO_VISIBLE_ROWS	     - y resolution
 VIDEO_PIXEL_SIZE	     - storage size in byte per pixel
 VIDEO_DATA_FORMAT	     - graphical data format GDF
 VIDEO_FB_ADRS		     - start of video memory

 CONFIG_I8042_KBD	     - AT Keyboard driver for i8042
 VIDEO_KBD_INIT_FCT	     - init function for keyboard
 VIDEO_TSTC_FCT		     - keyboard_tstc function
 VIDEO_GETC_FCT		     - keyboard_getc function

 CONFIG_CONSOLE_CURSOR	     - on/off drawing cursor is done with delay
			       loop in VIDEO_TSTC_FCT (i8042)
 CFG_CONSOLE_BLINK_COUNT     - value for delay loop - blink rate
 CONFIG_CONSOLE_TIME	     - display time/date in upper right corner,
			       needs CFG_CMD_DATE and CONFIG_CONSOLE_CURSOR
 CONFIG_VIDEO_LOGO	     - display Linux Logo in upper left corner
 CONFIG_VIDEO_BMP_LOGO	     - use bmp_logo instead of linux_logo
 CONFIG_CONSOLE_EXTRA_INFO   - display additional board information strings
			       that normaly goes to serial port. This define
			       requires a board specific function:
			       video_drawstring (VIDEO_INFO_X,
						 VIDEO_INFO_Y + i*VIDEO_FONT_HEIGHT,
						 info);
			       that fills a info buffer at i=row.
			       s.a: board/eltec/bab7xx.
CONFIG_VGA_AS_SINGLE_DEVICE  - If set the framebuffer device will be initialised
			       as an output only device. The Keyboard driver
			       will not be set-up. This may be used, if you
			       have none or more than one Keyboard devices
			       (USB Keyboard, AT Keyboard).

CONFIG_VIDEO_SW_CURSOR:	     - Draws a cursor after the last character. No
			       blinking is provided. Uses the macros CURSOR_SET
			       and CURSOR_OFF.
CONFIG_VIDEO_HW_CURSOR:	     - Uses the hardware cursor capability of the
			       graphic chip. Uses the macro CURSOR_SET.
			       ATTENTION: If booting an OS, the display driver
			       must disable the hardware register of the graphic
			       chip. Otherwise a blinking field is displayed
*/
/*
	This file is modified by Zhouhe Ustc 0411
	disableoutput() enableoutput() added.
	cursor bug fixed.
*/

/* this is software sursor shape.it can be modified*/
#define SOFTWARECURSOR	219

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <pmon.h>
#include <cpu.h>
#include "mod_sisfb.h"
#include <dev/pci/pcivar.h>

#ifdef RADEON7000
//#define VIDEO_FB_LITTLE_ENDIAN
#define CONFIG_VIDEO_SW_CURSOR
#define CONFIG_VIDEO_LOGO
#define	CONFIG_VIDEO_BMP_LOGO
//#define DEBUG_CFG_CONSOLE
#define VIDEO_HW_BITBLT
#elif defined(SMI712)
#define CONFIG_VIDEO_SW_CURSOR
//#define CONFIG_VIDEO_LOGO
//#define	CONFIG_VIDEO_BMP_LOGO
#define VIDEO_HW_BITBLT
#define VIDEO_HW_RECTFILL
#elif defined(SMI502) && !defined(DEVBD2F_SM502_GOLDING)
#define CONFIG_VIDEO_SW_CURSOR
#define VIDEO_HW_BITBLT
#define VIDEO_HW_RECTFILL
//#define CONFIG_VIDEO_LOGO
//#define CONFIG_VIDEO_BMP_LOGO
#elif defined(GC300)
#define CONFIG_VIDEO_SW_CURSOR
#define VIDEO_HW_BITBLT
#define VIDEO_HW_RECTFILL
#endif

/*****************************************************************************/
/* Include video_fb.h after definitions of VIDEO_HW_RECTFILL etc	     */
/*****************************************************************************/
#include "video_fb.h"

/*****************************************************************************/
/* some Macros								     */
/*****************************************************************************/
#define VIDEO_VISIBLE_COLS	(pGD->winSizeX)
#define VIDEO_VISIBLE_ROWS	(pGD->winSizeY)
#define VIDEO_PIXEL_SIZE	(pGD->gdfBytesPP)
#define VIDEO_DATA_FORMAT	(pGD->gdfIndex)
#define VIDEO_FB_ADRS		(pGD->frameAdrs)

/*****************************************************************************/
/* Console device defines with i8042 keyboard controller		     */
/* Any other keyboard controller must change this section		     */
/*****************************************************************************/

#ifdef	CONFIG_I8042_KBD
#include <i8042.h>

#define VIDEO_KBD_INIT_FCT	i8042_kbd_init()
#define VIDEO_TSTC_FCT		i8042_tstc
#define VIDEO_GETC_FCT		i8042_getc
#endif

/*****************************************************************************/
/* Console device							     */
/*****************************************************************************/

#include "video_font.h"
#ifdef CFG_CMD_DATE
#include <rtc.h>

#endif

#if (CONFIG_COMMANDS & CFG_CMD_BMP) || defined(CONFIG_SPLASH_SCREEN)
//#include <watchdog.h>
#include "bmp_layout.h"
#endif /* (CONFIG_COMMANDS & CFG_CMD_BMP) || CONFIG_SPLASH_SCREEN */

/*****************************************************************************/
/* Cursor definition:							     */
/* CONFIG_CONSOLE_CURSOR:  Uses a timer function (see drivers/i8042.c) to    */
/*			   let the cursor blink. Uses the macros CURSOR_OFF  */
/*			   and CURSOR_ON.				     */
/* CONFIG_VIDEO_SW_CURSOR: Draws a cursor after the last character. No	     */
/*			   blinking is provided. Uses the macros CURSOR_SET  */
/*			   and CURSOR_OFF.				     */
/* CONFIG_VIDEO_HW_CURSOR: Uses the hardware cursor capability of the	     */
/*			   graphic chip. Uses the macro CURSOR_SET.	     */
/*			   ATTENTION: If booting an OS, the display driver   */
/*			   must disable the hardware register of the graphic */
/*			   chip. Otherwise a blinking field is displayed     */
/*****************************************************************************/
#if !defined(CONFIG_CONSOLE_CURSOR) && \
    !defined(CONFIG_VIDEO_SW_CURSOR) && \
    !defined(CONFIG_VIDEO_HW_CURSOR)
/* no Cursor defined */
#define CURSOR_ON
#define CURSOR_OFF
#define CURSOR_SET
#endif

#ifdef	CONFIG_CONSOLE_CURSOR
#ifdef	CURSOR_ON
#error	only one of CONFIG_CONSOLE_CURSOR,CONFIG_VIDEO_SW_CURSOR,CONFIG_VIDEO_HW_CURSOR can be defined
#endif
void	console_cursor (int state);
#define CURSOR_ON  console_cursor(1);
#define CURSOR_OFF console_cursor(0);
#define CURSOR_SET
#ifndef CONFIG_I8042_KBD
#warning Cursor drawing on/off needs timer function s.a. drivers/i8042.c
#endif
#else
#ifdef	CONFIG_CONSOLE_TIME
#error	CONFIG_CONSOLE_CURSOR must be defined for CONFIG_CONSOLE_TIME
#endif
#endif /* CONFIG_CONSOLE_CURSOR */

#ifdef	CONFIG_VIDEO_SW_CURSOR
#ifdef	CURSOR_ON
#error	only one of CONFIG_CONSOLE_CURSOR,CONFIG_VIDEO_SW_CURSOR,CONFIG_VIDEO_HW_CURSOR can be defined
#endif
#define CURSOR_ON
#define CURSOR_OFF video_putchar_xor(console_col * VIDEO_FONT_WIDTH,\
				 console_row * VIDEO_FONT_HEIGHT, SOFTWARECURSOR);
#define CURSOR_SET video_putchar_xor(console_col * VIDEO_FONT_WIDTH,\
				 console_row * VIDEO_FONT_HEIGHT, SOFTWARECURSOR);
#endif /* CONFIG_VIDEO_SW_CURSOR */


#ifdef CONFIG_VIDEO_HW_CURSOR
#ifdef	CURSOR_ON
#error	only one of CONFIG_CONSOLE_CURSOR,CONFIG_VIDEO_SW_CURSOR,CONFIG_VIDEO_HW_CURSOR can be defined
#endif
#define CURSOR_ON
#define CURSOR_OFF
#define CURSOR_SET video_set_hw_cursor(console_col * VIDEO_FONT_WIDTH, \
		  (console_row * VIDEO_FONT_HEIGHT) + VIDEO_LOGO_HEIGHT);
#endif	/* CONFIG_VIDEO_HW_CURSOR */

#ifdef	CONFIG_VIDEO_LOGO
#ifdef	CONFIG_VIDEO_BMP_LOGO
#ifdef DEVBD2F_SM502_GOLDING
#include "sict_logo.h"
#else
#include "bmp_logo.h"
#endif
#define VIDEO_LOGO_WIDTH	BMP_LOGO_WIDTH
#define VIDEO_LOGO_HEIGHT	BMP_LOGO_HEIGHT
#define VIDEO_LOGO_LUT_OFFSET	BMP_LOGO_OFFSET
#define VIDEO_LOGO_COLORS	BMP_LOGO_COLORS

#else	/* CONFIG_VIDEO_BMP_LOGO */
#define LINUX_LOGO_WIDTH	80
#define LINUX_LOGO_HEIGHT	80
#define LINUX_LOGO_COLORS	214
#define LINUX_LOGO_LUT_OFFSET	0x20
#define __initdata
#include "linux_logo.h"
#define VIDEO_LOGO_WIDTH	LINUX_LOGO_WIDTH
#define VIDEO_LOGO_HEIGHT	LINUX_LOGO_HEIGHT
#define VIDEO_LOGO_LUT_OFFSET	LINUX_LOGO_LUT_OFFSET
#define VIDEO_LOGO_COLORS	LINUX_LOGO_COLORS
#endif	/* CONFIG_VIDEO_BMP_LOGO */
#define VIDEO_INFO_X		(VIDEO_LOGO_WIDTH)
#define VIDEO_INFO_Y		(VIDEO_FONT_HEIGHT/2)
#else	/* CONFIG_VIDEO_LOGO */
#define VIDEO_LOGO_WIDTH	0
#define VIDEO_LOGO_HEIGHT	0
#endif	/* CONFIG_VIDEO_LOGO */

#define VIDEO_COLS		VIDEO_VISIBLE_COLS
#define VIDEO_ROWS		VIDEO_VISIBLE_ROWS
#ifdef  CONFIG_VIDEO_1BPP       
#define VIDEO_SIZE              (VIDEO_ROWS*VIDEO_COLS*VIDEO_PIXEL_SIZE/8)
#define VIDEO_LINE_LEN		(VIDEO_COLS*VIDEO_PIXEL_SIZE/8)
#elif   CONFIG_VIDEO_2BPP
#define VIDEO_SIZE              (VIDEO_ROWS*VIDEO_COLS*VIDEO_PIXEL_SIZE/4)
#define VIDEO_LINE_LEN		(VIDEO_COLS*VIDEO_PIXEL_SIZE/4)
#elif   CONFIG_VIDEO_4BPP
#define VIDEO_SIZE              (VIDEO_ROWS*VIDEO_COLS*VIDEO_PIXEL_SIZE/2)
#define VIDEO_LINE_LEN		(VIDEO_COLS*VIDEO_PIXEL_SIZE/2)
#else
#define VIDEO_SIZE		(VIDEO_ROWS*VIDEO_COLS*VIDEO_PIXEL_SIZE)
#define VIDEO_LINE_LEN		(VIDEO_COLS*VIDEO_PIXEL_SIZE)
#endif
#define VIDEO_PIX_BLOCKS	(VIDEO_SIZE >> 2)
#define VIDEO_BURST_LEN		(VIDEO_COLS/8)

#ifdef	CONFIG_VIDEO_LOGO
#define CONSOLE_ROWS		((VIDEO_ROWS - VIDEO_LOGO_HEIGHT) / VIDEO_FONT_HEIGHT)
#else
#define CONSOLE_ROWS		(VIDEO_ROWS / VIDEO_FONT_HEIGHT)
#endif

#define CONSOLE_COLS		(VIDEO_COLS / VIDEO_FONT_WIDTH)
#define CONSOLE_ROW_SIZE	(VIDEO_FONT_HEIGHT * VIDEO_LINE_LEN)
#define CONSOLE_ROW_FIRST	(video_console_address)
#define CONSOLE_ROW_SECOND	(video_console_address + CONSOLE_ROW_SIZE)
//#define CONSOLE_ROW_LAST	(video_console_address + CONSOLE_SIZE - CONSOLE_ROW_SIZE)
#define CONSOLE_ROW_LAST	(video_console_address + (CONSOLE_ROWS-1)* VIDEO_FONT_HEIGHT*VIDEO_LINE_LEN)
//#define CONSOLE_SIZE		(CONSOLE_ROW_SIZE * CONSOLE_ROWS)
#ifdef CONFIG_VIDEO_LOGO
#define CONSOLE_SIZE		(VIDEO_COLS * (VIDEO_ROWS - VIDEO_LOGO_HEIGHT) * VIDEO_PIXEL_SIZE)
#else
#ifdef  CONFIG_VIDEO_1BPP
#define CONSOLE_SIZE            (VIDEO_COLS * VIDEO_ROWS * VIDEO_PIXEL_SIZE/8)
#elif   CONFIG_VIDEO_2BPP
#define CONSOLE_SIZE            (VIDEO_COLS * VIDEO_ROWS * VIDEO_PIXEL_SIZE/4)
#elif   CONFIG_VIDEO_4BPP
#define CONSOLE_SIZE            (VIDEO_COLS * VIDEO_ROWS * VIDEO_PIXEL_SIZE/2)
#else
#define CONSOLE_SIZE		(VIDEO_COLS * VIDEO_ROWS * VIDEO_PIXEL_SIZE)
#endif
#endif
#define CONSOLE_SCROLL_SIZE	(CONSOLE_SIZE - CONSOLE_ROW_SIZE)

/* Macros */
#ifdef	VIDEO_FB_LITTLE_ENDIAN
#define BYTESWAP32(x) 	(x)
#define SWAP16(x)	 ((((x) & 0x00ff) << 8) | ( (x) >> 8))
#define SWAP32(x)	 ((((x) & 0x000000ff) << 24) | (((x) & 0x0000ff00) << 8)|\
			  (((x) & 0x00ff0000) >>  8) | (((x) & 0xff000000) >> 24) )
#define SHORTSWAP32(x)	 ((((x) & 0x000000ff) <<  8) | (((x) & 0x0000ff00) >> 8)|\
			  (((x) & 0x00ff0000) <<  8) | (((x) & 0xff000000) >> 8) )
#else
#define BYTESWAP32(x)    ((((x) & 0x000000ff) <<  24) | (((x) & 0x0000ff00) << 8)|\
                          (((x) & 0x00ff0000) >>  8) | (((x) & 0xff000000) >> 24) )
#define SWAP16(x)	 (x)
#define SWAP32(x)	 (x)
#define SHORTSWAP32(x)	 (x)
#endif


#if defined(DEBUG) || defined(DEBUG_CFB_CONSOLE)
#define PRINTD(x)	  printf(x)
#else
#define PRINTD(x)
#endif


#ifdef CONFIG_CONSOLE_EXTRA_INFO
extern void video_get_info_str (    /* setup a board string: type, speed, etc. */
    int line_number,	    /* location to place info string beside logo */
    char *info		    /* buffer for info string */
    );

#endif

/* Locals */
static GraphicDevice *pGD,GD;	/* Pointer to Graphic array */

static void *video_fb_address;		/* frame buffer address */
static void *video_console_address;	/* console buffer start address */

#ifndef VIDEO_HW_BITBLT
#ifndef MEM_PRINTTO_VIDEO
#define MEM_PRINTTO_VIDEO
static char *memfb;
#endif
#endif

static int console_col = 0; /* cursor col */
static int console_row = 0; /* cursor row */
//static \B8ĳ\C9extern
unsigned int eorx, fgx, bgx;  /* color pats */

static const char video_font_draw_table2[] = {
	0x00, 0x03, 0x0c, 0x0f, 
	0x30, 0x33, 0x3c, 0x3f,
	0xc0, 0xc3, 0xcc, 0xcf,
	0xf0, 0xf3, 0xfc, 0xff
};

static const char video_font_draw_table4[] = {
	0x00, 0x0f, 0xf0, 0xff
};

static const int video_font_draw_table8[] = {
	0x00000000, 0x000000ff, 0x0000ff00, 0x0000ffff,
	0x00ff0000, 0x00ff00ff, 0x00ffff00, 0x00ffffff,
	0xff000000, 0xff0000ff, 0xff00ff00, 0xff00ffff,
	0xffff0000, 0xffff00ff, 0xffffff00, 0xffffffff
};

static const int video_font_draw_table12[] = {
	0x00000000, 0x0fff0000, 0x00000fff, 0x7fff7fff
};

static const int video_font_draw_table15[] = {
	0x00000000, 0x7fff0000, 0x00007fff, 0x7fff7fff
};
//	0x00000000, 0x00007fff, 0x7fff0000, 0x7fff7fff };

static const int video_font_draw_table16[] = {
	0x00000000, 0xffff0000, 0x0000ffff, 0xffffffff
};

static const int video_font_draw_table24[16][3] = {
	{ 0x00000000, 0x00000000, 0x00000000 },
	{ 0x00000000, 0x00000000, 0x00ffffff },
	{ 0x00000000, 0x0000ffff, 0xff000000 },
	{ 0x00000000, 0x0000ffff, 0xffffffff },
	{ 0x000000ff, 0xffff0000, 0x00000000 },
	{ 0x000000ff, 0xffff0000, 0x00ffffff },
	{ 0x000000ff, 0xffffffff, 0xff000000 },
	{ 0x000000ff, 0xffffffff, 0xffffffff },
	{ 0xffffff00, 0x00000000, 0x00000000 },
	{ 0xffffff00, 0x00000000, 0x00ffffff },
	{ 0xffffff00, 0x0000ffff, 0xff000000 },
	{ 0xffffff00, 0x0000ffff, 0xffffffff },
	{ 0xffffffff, 0xffff0000, 0x00000000 },
	{ 0xffffffff, 0xffff0000, 0x00ffffff },
	{ 0xffffffff, 0xffffffff, 0xff000000 },
	{ 0xffffffff, 0xffffffff, 0xffffffff }
};

static const int video_font_draw_table32[16][4] = {
	{ 0x00000000, 0x00000000, 0x00000000, 0x00000000 },
	{ 0x00000000, 0x00000000, 0x00000000, 0x00ffffff },
	{ 0x00000000, 0x00000000, 0x00ffffff, 0x00000000 },
	{ 0x00000000, 0x00000000, 0x00ffffff, 0x00ffffff },
	{ 0x00000000, 0x00ffffff, 0x00000000, 0x00000000 },
	{ 0x00000000, 0x00ffffff, 0x00000000, 0x00ffffff },
	{ 0x00000000, 0x00ffffff, 0x00ffffff, 0x00000000 },
	{ 0x00000000, 0x00ffffff, 0x00ffffff, 0x00ffffff },
	{ 0x00ffffff, 0x00000000, 0x00000000, 0x00000000 },
	{ 0x00ffffff, 0x00000000, 0x00000000, 0x00ffffff },
	{ 0x00ffffff, 0x00000000, 0x00ffffff, 0x00000000 },
	{ 0x00ffffff, 0x00000000, 0x00ffffff, 0x00ffffff },
	{ 0x00ffffff, 0x00ffffff, 0x00000000, 0x00000000 },
	{ 0x00ffffff, 0x00ffffff, 0x00000000, 0x00ffffff },
	{ 0x00ffffff, 0x00ffffff, 0x00ffffff, 0x00000000 },
	{ 0x00ffffff, 0x00ffffff, 0x00ffffff, 0x00ffffff }
};

int gunzip(void *, int, unsigned char *, unsigned long *);
static void memsetl (int *p, int c, int v);
void video_putchar (int xx, int yy, unsigned char c);
static void memcpyl (int *d, int *s, int c);

extern int vga_available;

static int disableoutput = 0;
void inline video_disableoutput(void)
{
	disableoutput = 1;
}

void inline video_enableoutput(void)
{
	disableoutput = 0;
}

void video_drawchars_xor (int xx, int yy, unsigned char *s, int count)
{
	unsigned char *cdat, *dest, *dest0;
	int rows, offset, c;
	int i;
	
	if(disableoutput)return;
	
	offset = yy * VIDEO_LINE_LEN + xx * VIDEO_PIXEL_SIZE;
	dest0 = video_fb_address + offset;

	switch (VIDEO_DATA_FORMAT) {
	case GDF__1BIT:
		while (count--) {
			c = *s;
			cdat = video_fontdata + c * VIDEO_FONT_HEIGHT;

			for (i=0;i<VIDEO_FONT_HEIGHT;i++) {
				offset = ((yy+i)*(VIDEO_VISIBLE_COLS) + xx)/8;
				*(unsigned char *)(video_fb_address + offset)=((unsigned char *)cdat)[i];
			}
			s++;
		}
		break;
	case GDF__8BIT_INDEX:
	case GDF__8BIT_332RGB:
		while (count--) {
			c = *s;
			cdat = video_fontdata + c * VIDEO_FONT_HEIGHT;
			for (rows = VIDEO_FONT_HEIGHT, dest = dest0;
			     rows--;
			     dest += VIDEO_LINE_LEN) {
				unsigned char bits = *cdat++;

				((unsigned int *) dest)[0] ^= BYTESWAP32((video_font_draw_table8[bits >> 4] & eorx) ^ bgx);
				((unsigned int *) dest)[1] ^= BYTESWAP32((video_font_draw_table8[bits & 15] & eorx) ^ bgx);
			}
			dest0 += VIDEO_FONT_WIDTH * VIDEO_PIXEL_SIZE;
			s++;
		}
		break;

	case GDF_12BIT_444RGB:
		while (count--) {
			c = *s;
			cdat = video_fontdata + c * VIDEO_FONT_HEIGHT;
			for (rows = VIDEO_FONT_HEIGHT, dest = dest0;
			     rows--;
			     dest += VIDEO_LINE_LEN) {
				unsigned char bits = *cdat++;

				((unsigned int *) dest)[0] ^= SHORTSWAP32 ((video_font_draw_table12 [bits >> 6] & eorx) ^ bgx);
				((unsigned int *) dest)[1] ^= SHORTSWAP32 ((video_font_draw_table12 [bits >> 4 & 3] & eorx) ^ bgx);
				((unsigned int *) dest)[2] ^= SHORTSWAP32 ((video_font_draw_table12 [bits >> 2 & 3] & eorx) ^ bgx);
				((unsigned int *) dest)[3] ^= SHORTSWAP32 ((video_font_draw_table12 [bits & 3] & eorx) ^ bgx);
			}
			dest0 += VIDEO_FONT_WIDTH * VIDEO_PIXEL_SIZE;
			s++;
		}
		break;

	case GDF_15BIT_555RGB:
		while (count--) {
			c = *s;
			cdat = video_fontdata + c * VIDEO_FONT_HEIGHT;
			for (rows = VIDEO_FONT_HEIGHT, dest = dest0;
			     rows--;
			     dest += VIDEO_LINE_LEN) {
				unsigned char bits = *cdat++;

				((unsigned int *) dest)[0] ^= SHORTSWAP32 ((video_font_draw_table15 [bits >> 6] & eorx) ^ bgx);
				((unsigned int *) dest)[1] ^= SHORTSWAP32 ((video_font_draw_table15 [bits >> 4 & 3] & eorx) ^ bgx);
				((unsigned int *) dest)[2] ^= SHORTSWAP32 ((video_font_draw_table15 [bits >> 2 & 3] & eorx) ^ bgx);
				((unsigned int *) dest)[3] ^= SHORTSWAP32 ((video_font_draw_table15 [bits & 3] & eorx) ^ bgx);
			}
			dest0 += VIDEO_FONT_WIDTH * VIDEO_PIXEL_SIZE;
			s++;
		}
		break;

	//here
	case GDF_16BIT_565RGB:
		while (count--) {
			c = *s;
			cdat = video_fontdata + c * VIDEO_FONT_HEIGHT;
			for (rows = VIDEO_FONT_HEIGHT, dest = dest0;
			     rows--;
			     dest += VIDEO_LINE_LEN) {
				unsigned char bits = *cdat++;

				((unsigned int *) dest)[0] ^= SHORTSWAP32 ((video_font_draw_table16 [bits >> 6] & eorx) ^ bgx);
				((unsigned int *) dest)[1] ^= SHORTSWAP32 ((video_font_draw_table16 [bits >> 4 & 3] & eorx) ^ bgx);
				((unsigned int *) dest)[2] ^= SHORTSWAP32 ((video_font_draw_table16 [bits >> 2 & 3] & eorx) ^ bgx);
				((unsigned int *) dest)[3] ^= SHORTSWAP32 ((video_font_draw_table16 [bits & 3] & eorx) ^ bgx);
			}
			dest0 += VIDEO_FONT_WIDTH * VIDEO_PIXEL_SIZE;
			s++;
		}
		break;

	case GDF_32BIT_X888RGB:
		while (count--) {
			c = *s;
			cdat = video_fontdata + c * VIDEO_FONT_HEIGHT;
			for (rows = VIDEO_FONT_HEIGHT, dest = dest0;
			     rows--;
			     dest += VIDEO_LINE_LEN) {
				unsigned char bits = *cdat++;

				((unsigned int *) dest)[0] ^= SWAP32 ((video_font_draw_table32 [bits >> 4][0] & eorx) ^ bgx);
				((unsigned int *) dest)[1] ^= SWAP32 ((video_font_draw_table32 [bits >> 4][1] & eorx) ^ bgx);
				((unsigned int *) dest)[2] ^= SWAP32 ((video_font_draw_table32 [bits >> 4][2] & eorx) ^ bgx);
				((unsigned int *) dest)[3] ^= SWAP32 ((video_font_draw_table32 [bits >> 4][3] & eorx) ^ bgx);
				((unsigned int *) dest)[4] ^= SWAP32 ((video_font_draw_table32 [bits & 15][0] & eorx) ^ bgx);
				((unsigned int *) dest)[5] ^= SWAP32 ((video_font_draw_table32 [bits & 15][1] & eorx) ^ bgx);
				((unsigned int *) dest)[6] ^= SWAP32 ((video_font_draw_table32 [bits & 15][2] & eorx) ^ bgx);
				((unsigned int *) dest)[7] ^= SWAP32 ((video_font_draw_table32 [bits & 15][3] & eorx) ^ bgx);
			}
			dest0 += VIDEO_FONT_WIDTH * VIDEO_PIXEL_SIZE;
			s++;
		}
		break;

	case GDF_24BIT_888RGB:
		while (count--) {
			c = *s;
			cdat = video_fontdata + c * VIDEO_FONT_HEIGHT;
			for (rows = VIDEO_FONT_HEIGHT, dest = dest0;
			     rows--;
			     dest += VIDEO_LINE_LEN) {
				unsigned char bits = *cdat++;

				((unsigned int *) dest)[0] ^= (video_font_draw_table24[bits >> 4][0] & eorx) ^ bgx;
				((unsigned int *) dest)[1] ^= (video_font_draw_table24[bits >> 4][1] & eorx) ^ bgx;
				((unsigned int *) dest)[2] ^= (video_font_draw_table24[bits >> 4][2] & eorx) ^ bgx;
				((unsigned int *) dest)[3] ^= (video_font_draw_table24[bits & 15][0] & eorx) ^ bgx;
				((unsigned int *) dest)[4] ^= (video_font_draw_table24[bits & 15][1] & eorx) ^ bgx;
				((unsigned int *) dest)[5] ^= (video_font_draw_table24[bits & 15][2] & eorx) ^ bgx;
			}
			dest0 += VIDEO_FONT_WIDTH * VIDEO_PIXEL_SIZE;
			s++;
		}
		break;
	}
}

void video_drawchars(int xx, int yy, unsigned char *s, int count)
{
	unsigned char *cdat, *dest, *dest0;
	int rows, offset, c;
	int i;

	if(disableoutput)return;
	
	offset = yy * VIDEO_LINE_LEN + xx * VIDEO_PIXEL_SIZE;
	dest0 = video_fb_address + offset;

	switch (VIDEO_DATA_FORMAT) {
	case GDF__1BIT:
		while (count--) {
			c = *s;
			cdat = video_fontdata + c * VIDEO_FONT_HEIGHT;

			for (i=0; i<VIDEO_FONT_HEIGHT; i++) {
				offset = ((yy+i)*(VIDEO_VISIBLE_COLS) + xx)/8;
				*(unsigned char *)(video_fb_address + offset)=((unsigned char *)cdat)[i];
			}
			s++;
		}
		break;

	case GDF__2BIT:
		while (count--) {
			c = *s;
			cdat = video_fontdata + c * VIDEO_FONT_HEIGHT;

			for (i=0; i<VIDEO_FONT_HEIGHT; i++) {
				unsigned char bits = *cdat++;
				offset = ((yy+i)*(VIDEO_VISIBLE_COLS) + xx)/4;
				dest = (unsigned char *)(video_fb_address + offset);
				dest[0] = video_font_draw_table2[bits >> 4] ;
				dest[1] = video_font_draw_table2[bits & 15] ;
			}
			s++;
		}
		break;

	case GDF__4BIT:
		while (count--) {
			c = *s;
			cdat = video_fontdata + c * VIDEO_FONT_HEIGHT;
			for (i=0; i<VIDEO_FONT_HEIGHT; i++) {
				unsigned char bits = *cdat++;
				offset = ((yy+i)*(VIDEO_VISIBLE_COLS) + xx)/2;
				dest = (unsigned char *)(video_fb_address + offset);
				dest[0] = video_font_draw_table4[bits >> 6] ;
				dest[1] = video_font_draw_table4[bits >> 4 & 3] ;
				dest[2] = video_font_draw_table4[bits >> 2 & 3] ;
				dest[3] = video_font_draw_table4[bits & 3] ;
			}
			s++;
		}
		break;

	case GDF__8BIT_INDEX:
	case GDF__8BIT_332RGB:
		while (count--) {
			c = *s;
			cdat = video_fontdata + c * VIDEO_FONT_HEIGHT;
			for (rows = VIDEO_FONT_HEIGHT, dest = dest0;
			     rows--;
			     dest += VIDEO_LINE_LEN) {
				unsigned char bits = *cdat++;

				((unsigned int *) dest)[0] = BYTESWAP32((video_font_draw_table8[bits >> 4] & eorx) ^ bgx);
				((unsigned int *) dest)[1] = BYTESWAP32((video_font_draw_table8[bits & 15] & eorx) ^ bgx);
			}
			dest0 += VIDEO_FONT_WIDTH * VIDEO_PIXEL_SIZE;
			s++;
		}
		break;

	case GDF_12BIT_444RGB:
		while (count--) {
			c = *s;
			cdat = video_fontdata + c * VIDEO_FONT_HEIGHT;
			for (rows = VIDEO_FONT_HEIGHT, dest = dest0;
			     rows--;
			     dest += VIDEO_LINE_LEN) {
				unsigned char bits = *cdat++;

				((unsigned int *) dest)[0] = SHORTSWAP32 ((video_font_draw_table15 [bits >> 6] & eorx) ^ bgx);
				((unsigned int *) dest)[1] = SHORTSWAP32 ((video_font_draw_table15 [bits >> 4 & 3] & eorx) ^ bgx);
				((unsigned int *) dest)[2] = SHORTSWAP32 ((video_font_draw_table15 [bits >> 2 & 3] & eorx) ^ bgx);
				((unsigned int *) dest)[3] = SHORTSWAP32 ((video_font_draw_table15 [bits & 3] & eorx) ^ bgx);
			}
			dest0 += VIDEO_FONT_WIDTH * VIDEO_PIXEL_SIZE;
			s++;
		}
		break;

	case GDF_15BIT_555RGB:
		while (count--) {
			c = *s;
			cdat = video_fontdata + c * VIDEO_FONT_HEIGHT;
			for (rows = VIDEO_FONT_HEIGHT, dest = dest0;
			     rows--;
			     dest += VIDEO_LINE_LEN) {
				unsigned char bits = *cdat++;

				((unsigned int *) dest)[0] = SHORTSWAP32 ((video_font_draw_table15 [bits >> 6] & eorx) ^ bgx);
				((unsigned int *) dest)[1] = SHORTSWAP32 ((video_font_draw_table15 [bits >> 4 & 3] & eorx) ^ bgx);
				((unsigned int *) dest)[2] = SHORTSWAP32 ((video_font_draw_table15 [bits >> 2 & 3] & eorx) ^ bgx);
				((unsigned int *) dest)[3] = SHORTSWAP32 ((video_font_draw_table15 [bits & 3] & eorx) ^ bgx);
			}
			dest0 += VIDEO_FONT_WIDTH * VIDEO_PIXEL_SIZE;
			s++;
		}
		break;

	//here
	case GDF_16BIT_565RGB:
		while (count--) {
			c = *s;
			cdat = video_fontdata + c * VIDEO_FONT_HEIGHT;
			for (rows = VIDEO_FONT_HEIGHT, dest = dest0;
			     rows--;
			     dest += VIDEO_LINE_LEN) {
				unsigned char bits = *cdat++;

				((unsigned int *) dest)[0] = SHORTSWAP32 ((video_font_draw_table16 [bits >> 6] & eorx) ^ bgx);
				((unsigned int *) dest)[1] = SHORTSWAP32 ((video_font_draw_table16 [bits >> 4 & 3] & eorx) ^ bgx);
				((unsigned int *) dest)[2] = SHORTSWAP32 ((video_font_draw_table16 [bits >> 2 & 3] & eorx) ^ bgx);
				((unsigned int *) dest)[3] = SHORTSWAP32 ((video_font_draw_table16 [bits & 3] & eorx) ^ bgx);
			}
			dest0 += VIDEO_FONT_WIDTH * VIDEO_PIXEL_SIZE;
			s++;
		}
		break;

	case GDF_32BIT_X888RGB:
		while (count--) {
			c = *s;
			cdat = video_fontdata + c * VIDEO_FONT_HEIGHT;
			for (rows = VIDEO_FONT_HEIGHT, dest = dest0;
			     rows--;
			     dest += VIDEO_LINE_LEN) {
				unsigned char bits = *cdat++;

				((unsigned int *) dest)[0] = SWAP32 ((video_font_draw_table32 [bits >> 4][0] & eorx) ^ bgx);
				((unsigned int *) dest)[1] = SWAP32 ((video_font_draw_table32 [bits >> 4][1] & eorx) ^ bgx);
				((unsigned int *) dest)[2] = SWAP32 ((video_font_draw_table32 [bits >> 4][2] & eorx) ^ bgx);
				((unsigned int *) dest)[3] = SWAP32 ((video_font_draw_table32 [bits >> 4][3] & eorx) ^ bgx);
				((unsigned int *) dest)[4] = SWAP32 ((video_font_draw_table32 [bits & 15][0] & eorx) ^ bgx);
				((unsigned int *) dest)[5] = SWAP32 ((video_font_draw_table32 [bits & 15][1] & eorx) ^ bgx);
				((unsigned int *) dest)[6] = SWAP32 ((video_font_draw_table32 [bits & 15][2] & eorx) ^ bgx);
				((unsigned int *) dest)[7] = SWAP32 ((video_font_draw_table32 [bits & 15][3] & eorx) ^ bgx);
			}
			dest0 += VIDEO_FONT_WIDTH * VIDEO_PIXEL_SIZE;
			s++;
		}
		break;

	case GDF_24BIT_888RGB:
		while (count--) {
			c = *s;
			cdat = video_fontdata + c * VIDEO_FONT_HEIGHT;
			for (rows = VIDEO_FONT_HEIGHT, dest = dest0;
			     rows--;
			     dest += VIDEO_LINE_LEN) {
				unsigned char bits = *cdat++;

				((unsigned int *) dest)[0] = (video_font_draw_table24[bits >> 4][0] & eorx) ^ bgx;
				((unsigned int *) dest)[1] = (video_font_draw_table24[bits >> 4][1] & eorx) ^ bgx;
				((unsigned int *) dest)[2] = (video_font_draw_table24[bits >> 4][2] & eorx) ^ bgx;
				((unsigned int *) dest)[3] = (video_font_draw_table24[bits & 15][0] & eorx) ^ bgx;
				((unsigned int *) dest)[4] = (video_font_draw_table24[bits & 15][1] & eorx) ^ bgx;
				((unsigned int *) dest)[5] = (video_font_draw_table24[bits & 15][2] & eorx) ^ bgx;
			}
			dest0 += VIDEO_FONT_WIDTH * VIDEO_PIXEL_SIZE;
			s++;
		}
		break;
	}
}

void inline video_drawstring(int xx, int yy, unsigned char *s)
{
	video_drawchars(xx, yy, s, strlen ((char *)s));
}

#ifndef VIDEO_HW_BITBLT
void video_drawsline(char *str, int rows, int cols)
{
	int xx, yy;
	int pos;

	for (yy = 1; yy < rows; yy++) {
		pos = yy * cols;
		for (xx = 0; xx < cols; xx++) {
			if(str[pos + xx] != str[pos - cols + xx])
				video_putchar(xx * VIDEO_FONT_WIDTH, (yy-1) * VIDEO_FONT_HEIGHT, str[pos + xx]);
		}
	}
	
	if (cols&3) {
		memcpy (str, &str[cols], (rows - 1) *cols);
		memset (&str[(rows - 1) * cols], CONSOLE_BG_COL, cols);
	} else {
		memcpyl (str, &str[cols], (rows - 1) *cols >> 2);
		memsetl (&str[(rows - 1) * cols], cols >> 2, CONSOLE_BG_COL);
	}
}

#endif

void inline video_putchar(int xx, int yy, unsigned char c)
{
	video_drawchars(xx, yy + VIDEO_LOGO_HEIGHT, &c, 1);
}

void inline video_putchar_xor(int xx, int yy, unsigned char c)
{
	video_drawchars_xor(xx, yy + VIDEO_LOGO_HEIGHT, &c, 1);
}

#ifdef CONFIG_CONSOLE_CURSOR
void console_cursor(int state)
{
	static int last_state = 0;

#ifdef CONFIG_CONSOLE_TIME
	struct rtc_time tm;
	char info[16];

	/* time update only if cursor is on (faster scroll) */
	if (state) {
		rtc_get (&tm);

		sprintf (info, " %02d:%02d:%02d ", tm.tm_hour, tm.tm_min,
			 tm.tm_sec);
		video_drawstring (VIDEO_VISIBLE_COLS - 10 * VIDEO_FONT_WIDTH,
				  VIDEO_INFO_Y, (unsigned char *)info);

		sprintf (info, "%02d.%02d.%04d", tm.tm_mday, tm.tm_mon,
			 tm.tm_year);
		video_drawstring (VIDEO_VISIBLE_COLS - 10 * VIDEO_FONT_WIDTH,
				  VIDEO_INFO_Y + 1 * VIDEO_FONT_HEIGHT, (unsigned char *)info);
	}
#endif

	if (last_state != state) {
		video_putchar_xor (console_col * VIDEO_FONT_WIDTH,
			       console_row * VIDEO_FONT_HEIGHT,
			       SOFTWARECURSOR);
	}

	last_state = state;
}
#endif

#if 1
//#ifndef VIDEO_HW_RECTFILL
static void inline memsetl(int *p, int c, int v)
{
	while (c--)
		*(p++) = v;
}
#endif

#ifndef VIDEO_HW_BITBLT
static void inline memcpyl(int *d, int *s, int c)
{
	while (c--)
		*(d++) = *(s++);
}
#endif

static void console_scrollup (void)
{
	/* copy up rows ignoring the first one */
	if(disableoutput)return;
#ifdef VIDEO_HW_BITBLT

#ifdef RADEON7000
	video_hw_bitblt (VIDEO_PIXEL_SIZE,	/* bytes per pixel */
			 0,	/* source pos x */
			 VIDEO_LOGO_HEIGHT + VIDEO_FONT_HEIGHT, /* source pos y */
			 0,	/* dest pos x */
			 VIDEO_LOGO_HEIGHT,	/* dest pos y */
			 VIDEO_VISIBLE_COLS,	/* frame width */
			 VIDEO_VISIBLE_ROWS - VIDEO_LOGO_HEIGHT - VIDEO_FONT_HEIGHT	/* frame height */
		);
#endif

#if (defined(SMI502)||defined(SMI712))
    AutodeCopyModify((pGD->gdfBytesPP * 8));
#endif
#if NMOD_SISFB
{
void sisfb_copyarea(int sx,int sy,int dx,int dy,int width,int height);
	sisfb_copyarea(0,VIDEO_LOGO_HEIGHT + VIDEO_FONT_HEIGHT,0,VIDEO_LOGO_HEIGHT,VIDEO_VISIBLE_COLS,VIDEO_VISIBLE_ROWS - VIDEO_LOGO_HEIGHT - VIDEO_FONT_HEIGHT);
}
#endif

#if (defined(GC300))
gc300_hw_bitblt(pGD->gdfBytesPP,pGD->winSizeX,pGD->winSizeY, VIDEO_FONT_HEIGHT );
#endif

#else

#if defined(MEM_PRINTTO_VIDEO)
	video_drawsline(memfb, CONSOLE_ROWS, CONSOLE_COLS);
#else

#ifdef __mips__
#define UNCACHED_TO_CACHED(x) PHYS_TO_CACHED(UNCACHED_TO_PHYS(x))
	if(CONSOLE_ROW_FIRST<0xb0000000 && CONSOLE_ROW_FIRST>=0xa0000000)
	{
	memcpyl (UNCACHED_TO_CACHED(CONSOLE_ROW_FIRST),UNCACHED_TO_CACHED(CONSOLE_ROW_SECOND),
		 CONSOLE_SCROLL_SIZE >> 2);
    pci_sync_cache(0, (vm_offset_t)UNCACHED_TO_CACHED(CONSOLE_ROW_FIRST), CONSOLE_SCROLL_SIZE >> 2, SYNC_W);
	}
	else
#endif
	memcpyl (CONSOLE_ROW_FIRST, CONSOLE_ROW_SECOND,
		 CONSOLE_SCROLL_SIZE >> 2);
#endif
#endif

	/* clear the last one */
#ifdef VIDEO_HW_RECTFILL

#ifdef RADEON7000	
	video_hw_rectfill (VIDEO_PIXEL_SIZE,	/* bytes per pixel */
			   0,	/* dest pos x */
			   VIDEO_VISIBLE_ROWS - VIDEO_FONT_HEIGHT,	/* dest pos y */
			   VIDEO_VISIBLE_COLS,	/* frame width */
			   VIDEO_FONT_HEIGHT,	/* frame height */
			   CONSOLE_BG_COL	/* fill color */
		);

#elif (defined(GC300))
    gc300_hw_rectfill(pGD->gdfBytesPP,pGD->winSizeX,pGD->winSizeY, VIDEO_FONT_HEIGHT);

#elif (defined(SMI502)||defined(SMI712))
    AutodeFillRectModify(CONSOLE_BG_COL);
#ifdef X800x480
	memsetl (CONSOLE_ROW_LAST, CONSOLE_ROW_SIZE >> 2, CONSOLE_BG_COL);
#endif

#else
	memsetl (CONSOLE_ROW_LAST, CONSOLE_ROW_SIZE >> 2, CONSOLE_BG_COL);

#endif

//#elif X800x600
//	memsetl (CONSOLE_ROW_LAST - CONSOLE_ROW_SIZE/2, CONSOLE_ROW_SIZE >> 2, CONSOLE_BG_COL);
#else
	memsetl (CONSOLE_ROW_LAST, CONSOLE_ROW_SIZE >> 2, CONSOLE_BG_COL);
#endif
}

static void console_back(void)
{
	console_col--;

	if (console_col < 0) {
		console_col = CONSOLE_COLS - 1;
		console_row--;
		if (console_row < 0)
			console_row = 0;
	}
}

static void console_newline (void)
{
	console_row++;
	console_col = 0;

	/* Check if we need to scroll the terminal */
	if (console_row >= CONSOLE_ROWS) {
		/* Scroll everything up */
		console_scrollup ();

		/* Decrement row number */
		console_row--;
	}
}

void video_putc (const char c)
{
	switch (c) {
	case 13:		/* ignore */
		CURSOR_OFF
		break;

	case '\n':		/* next line */
		CURSOR_OFF
		console_newline ();
		break;

	case 9:		/* tab 8 */
		CURSOR_OFF 
		console_col += 0x0008;
		console_col &= ~0x0007;

		if (console_col >= CONSOLE_COLS)
			console_newline ();
		break;

	case 8:		/* backspace */
		CURSOR_OFF
		console_back ();
		break;

	default:		/* draw the char */
		video_putchar (console_col * VIDEO_FONT_WIDTH,
			       console_row * VIDEO_FONT_HEIGHT,
			       c);
#ifdef MEM_PRINTTO_VIDEO
		memfb[console_row * CONSOLE_COLS + console_col] = c;
#endif
		console_col++;

		/* check for newline */
		if (console_col >= CONSOLE_COLS)
			console_newline ();
	}
	CURSOR_SET
}

void video_puts(const char *s)
{
	int count = strlen(s);

	while (count--)
		video_putc(*s++);
}

#if (CONFIG_COMMANDS & CFG_CMD_BMP) || defined(CONFIG_SPLASH_SCREEN)

#define FILL_8BIT_332RGB(r,g,b)	{			\
	*fb = ((r>>5)<<5) | ((g>>5)<<2) | (b>>6);	\
	fb ++;						\
}

#define FILL_15BIT_555RGB(r,g,b) {			\
	*(unsigned short *)fb = SWAP16((unsigned short)(((r>>3)<<10) | ((g>>3)<<5) | (b>>3))); \
	fb += 2;					\
}

#define FILL_12BIT_444RGB(r,g,b) {			\
	*(unsigned short *)fb = SWAP16((unsigned short)(((r>>4)<<8) | ((g>>4)<<4) | (b>>4))); \
	fb += 2;					\
}

#define FILL_16BIT_565RGB(r,g,b) {			\
	*(unsigned short *)fb = SWAP16((unsigned short)((((r)>>3)<<11) | (((g)>>2)<<5) | ((b)>>3))); \
	fb += 2;					\
}

#define FILL_32BIT_X888RGB(r,g,b) {			\
	*(unsigned long *)fb = SWAP32((unsigned long)(((r<<16) | (g<<8) | b))); \
	fb += 4;					\
}

#ifdef VIDEO_FB_LITTLE_ENDIAN
#define FILL_24BIT_888RGB(r,g,b) {			\
	fb[0] = b;					\
	fb[1] = g;					\
	fb[2] = r;					\
	fb += 3;					\
}
#else
#define FILL_24BIT_888RGB(r,g,b) {			\
	fb[0] = r;					\
	fb[1] = g;					\
	fb[2] = b;					\
	fb += 3;					\
}
#endif

#define le32_to_cpu(x) (x)
#define le16_to_cpu(x) (x)

/*
 * Display the BMP file located at address bmp_image.
 * Only uncompressed
 */
int video_display_bitmap (ulong bmp_image, int x, int y)
{
	ushort xcount, ycount;
	unsigned char *fb;
	bmp_image_t *bmp = (bmp_image_t *) bmp_image;
	unsigned char *bmap;
	ushort padded_line;
	unsigned long width, height, bpp;
	unsigned colors;
	unsigned long compression;
	bmp_color_table_entry_t cte;
#ifdef CONFIG_VIDEO_BMP_GZIP
	unsigned char *dst = NULL;
	ulong len;
#endif

	if (!((bmp->header.signature[0] == 'B') &&
	      (bmp->header.signature[1] == 'M'))) {

#ifdef CONFIG_VIDEO_BMP_GZIP
		/*
		 * Could be a gzipped bmp image, try to decrompress...
		 */
		len = CFG_VIDEO_LOGO_MAX_SIZE;
		dst = malloc(CFG_VIDEO_LOGO_MAX_SIZE);
		if (dst == NULL) {
			printf("Error: malloc in gunzip failed!\n");
			return(1);
		}
		if (gunzip(dst, CFG_VIDEO_LOGO_MAX_SIZE, (unsigned char *)bmp_image, &len) != 0) {
			printf ("Error: no valid bmp or bmp.gz image at %lx\n", bmp_image);
			free(dst);
			return 1;
		}
		if (len == CFG_VIDEO_LOGO_MAX_SIZE) {
			printf("Image could be truncated (increase CFG_VIDEO_LOGO_MAX_SIZE)!\n");
		}

		/*
		 * Set addr to decompressed image
		 */
		bmp = (bmp_image_t *)dst;

		if (!((bmp->header.signature[0] == 'B') &&
		      (bmp->header.signature[1] == 'M'))) {
			printf ("Error: no valid bmp.gz image at %lx\n", bmp_image);
			return 1;
		}
#else
		printf ("Error: no valid bmp image at %lx\n", bmp_image);
		return 1;
#endif /* CONFIG_VIDEO_BMP_GZIP */
	}

	width = le32_to_cpu (bmp->header.width);
	height = le32_to_cpu (bmp->header.height);
	bpp = le16_to_cpu (bmp->header.bit_count);
	colors = le32_to_cpu (bmp->header.colors_used);
	compression = le32_to_cpu (bmp->header.compression);

	if (compression != BMP_BI_RGB) {
		printf ("Error: compression type %ld not supported\n",
			compression);
		return 1;
	}

	padded_line = (((width * bpp + 7) / 8) + 3) & ~0x3;

	if ((x + width) > VIDEO_VISIBLE_COLS)
		width = VIDEO_VISIBLE_COLS - x;
	if ((y + height) > VIDEO_VISIBLE_ROWS)
		height = VIDEO_VISIBLE_ROWS - y;

	bmap = (unsigned char *) bmp + le32_to_cpu (bmp->header.data_offset);
	fb = (unsigned char *) (video_fb_address +
			((y + height - 1) * VIDEO_COLS * VIDEO_PIXEL_SIZE) +
			x * VIDEO_PIXEL_SIZE);

	/* We handle only 8bpp or 24 bpp bitmap */
	switch (le16_to_cpu (bmp->header.bit_count)) {
	case 8:
		padded_line -= width;
		if (VIDEO_DATA_FORMAT == GDF__8BIT_INDEX) {
#if 1
			/* Copy colormap					     */
			for (xcount = 0; xcount < colors; ++xcount) {
				cte = bmp->color_table[xcount];
				video_set_lut (xcount, cte.red, cte.green, cte.blue);
			}
#endif
		}
		ycount = height;
		switch (VIDEO_DATA_FORMAT) {
		case GDF__8BIT_INDEX:
			while (ycount--) {
				xcount = width;
				while (xcount--) {
					*fb++ = *bmap++;
				}
				bmap += padded_line;
				fb -= (VIDEO_VISIBLE_COLS + width) * VIDEO_PIXEL_SIZE;
			}
			break;
		case GDF__8BIT_332RGB:
			while (ycount--) {
				xcount = width;
				while (xcount--) {
					cte = bmp->color_table[*bmap++];
					FILL_8BIT_332RGB (cte.red, cte.green, cte.blue);
				}
				bmap += padded_line;
				fb -= (VIDEO_VISIBLE_COLS + width) * VIDEO_PIXEL_SIZE;
			}
			break;
		case GDF_12BIT_444RGB:
			while (ycount--) {
				xcount = width;
				while (xcount--) {
					cte = bmp->color_table[*bmap++];
					FILL_12BIT_444RGB (cte.red, cte.green, cte.blue);
				}
				bmap += padded_line;
				fb -= (VIDEO_VISIBLE_COLS + width) * VIDEO_PIXEL_SIZE;
			}
			break;
		case GDF_15BIT_555RGB:
			while (ycount--) {
				xcount = width;
				while (xcount--) {
					cte = bmp->color_table[*bmap++];
					FILL_15BIT_555RGB (cte.red, cte.green, cte.blue);
				}
				bmap += padded_line;
				fb -= (VIDEO_VISIBLE_COLS + width) * VIDEO_PIXEL_SIZE;
			}
			break;
		case GDF_16BIT_565RGB:
			while (ycount--) {
				xcount = width;
				while (xcount--) {
					cte = bmp->color_table[*bmap++];
					FILL_16BIT_565RGB (cte.red, cte.green, cte.blue);
				}
				bmap += padded_line;
				fb -= (VIDEO_VISIBLE_COLS + width) * VIDEO_PIXEL_SIZE;
			}
			break;
		case GDF_32BIT_X888RGB:
			while (ycount--) {
				xcount = width;
				while (xcount--) {
					cte = bmp->color_table[*bmap++];
					FILL_32BIT_X888RGB (cte.red, cte.green, cte.blue);
				}
				bmap += padded_line;
				fb -= (VIDEO_VISIBLE_COLS + width) * VIDEO_PIXEL_SIZE;
			}
			break;
		case GDF_24BIT_888RGB:
			while (ycount--) {
				xcount = width;
				while (xcount--) {
					cte = bmp->color_table[*bmap++];
					FILL_24BIT_888RGB (cte.red, cte.green, cte.blue);
				}
				bmap += padded_line;
				fb -= (VIDEO_VISIBLE_COLS + width) * VIDEO_PIXEL_SIZE;
			}
			break;
		}
		break;
	case 24:
		padded_line -= 3 * width;
		ycount = height;
		switch (VIDEO_DATA_FORMAT) {
		case GDF__8BIT_332RGB:
			while (ycount--) {
				xcount = width;
				while (xcount--) {
					FILL_8BIT_332RGB (bmap[2], bmap[1], bmap[0]);
					bmap += 3;
				}
				bmap += padded_line;
				fb -= (VIDEO_VISIBLE_COLS + width) * VIDEO_PIXEL_SIZE;
			}
			break;
		case GDF_12BIT_444RGB:
			while (ycount--) {
				xcount = width;
				while (xcount--) {
					FILL_12BIT_444RGB (bmap[2], bmap[1], bmap[0]);
					bmap += 3;
				}
				bmap += padded_line;
				fb -= (VIDEO_VISIBLE_COLS + width) * VIDEO_PIXEL_SIZE;
			}
			break;
		case GDF_15BIT_555RGB:
			while (ycount--) {
				xcount = width;
				while (xcount--) {
					FILL_15BIT_555RGB (bmap[2], bmap[1], bmap[0]);
					bmap += 3;
				}
				bmap += padded_line;
				fb -= (VIDEO_VISIBLE_COLS + width) * VIDEO_PIXEL_SIZE;
			}
			break;
		case GDF_16BIT_565RGB:
			while (ycount--) {
				xcount = width;
				while (xcount--) {
					FILL_16BIT_565RGB (bmap[2], bmap[1], bmap[0]);
					bmap += 3;
				}
				bmap += padded_line;
				fb -= (VIDEO_VISIBLE_COLS + width) * VIDEO_PIXEL_SIZE;
			}
			break;
		case GDF_32BIT_X888RGB:
			while (ycount--) {
				xcount = width;
				while (xcount--) {
					FILL_32BIT_X888RGB (bmap[2], bmap[1], bmap[0]);
					bmap += 3;
				}
				bmap += padded_line;
				fb -= (VIDEO_VISIBLE_COLS + width) * VIDEO_PIXEL_SIZE;
			}
			break;
		case GDF_24BIT_888RGB:
			while (ycount--) {
				xcount = width;
				while (xcount--) {
					FILL_24BIT_888RGB (bmap[2], bmap[1], bmap[0]);
					bmap += 3;
				}
				bmap += padded_line;
				fb -= (VIDEO_VISIBLE_COLS + width) * VIDEO_PIXEL_SIZE;
			}
			break;
		default:
			printf ("Error: 24 bits/pixel bitmap incompatible with current video mode\n");
			break;
		}
		break;
	default:
		printf ("Error: %d bit/pixel bitmaps not supported by U-Boot\n",
			le16_to_cpu (bmp->header.bit_count));
		break;
	}

#ifdef CONFIG_VIDEO_BMP_GZIP
	if (dst) {
		free(dst);
	}
#endif

	return (0);
}

#include "custom_bmp.h"

int video_display_bmp(int argc, char **argv)
{
	unsigned char *addr;
	int x, y;

	addr = &custom_bmp_bits;
	x = 300;
	y = 300;
	video_display_bitmap(addr, x, y);
	return 0;
}

static const Cmd Cmds[] =
{
	{"MyCmds"},
	{"display_bmp", "imgaddr x y", 0, "hardware test", video_display_bmp, 0, 99, CMD_REPEAT},
	{0,0}
};

static void init_cmd __P((void)) __attribute__ ((constructor));
static void init_cmd()
{
	cmdlist_expand(Cmds, 1);
}

#endif /* (CONFIG_COMMANDS & CFG_CMD_BMP) || CONFIG_SPLASH_SCREEN */

#ifdef CONFIG_VIDEO_LOGO
void logo_plot(void *screen, int width, int x, int y)
{

	int xcount, i;
	int skip   = (width - VIDEO_LOGO_WIDTH) * VIDEO_PIXEL_SIZE;
	int ycount = VIDEO_LOGO_HEIGHT;
	unsigned char r, g, b, *logo_red, *logo_blue, *logo_green;
	unsigned char *source;
	unsigned char *dest = (unsigned char *)screen + ((y * width * VIDEO_PIXEL_SIZE) + x);

#ifdef CONFIG_VIDEO_BMP_LOGO
	source = bmp_logo_bitmap;

	/* Allocate temporary space for computing colormap			 */
	logo_red = malloc (BMP_LOGO_COLORS);
	logo_green = malloc (BMP_LOGO_COLORS);
	logo_blue = malloc (BMP_LOGO_COLORS);
	/* Compute color map							 */
	for (i = 0; i < VIDEO_LOGO_COLORS; i++) {
		logo_red[i] = (bmp_logo_palette[i] & 0x0f00) >> 4;
		logo_green[i] = (bmp_logo_palette[i] & 0x00f0);
		logo_blue[i] = (bmp_logo_palette[i] & 0x000f) << 4;
	}
#else
	source = linux_logo;
	logo_red = linux_logo_red;
	logo_green = linux_logo_green;
	logo_blue = linux_logo_blue;
#endif

	if (VIDEO_DATA_FORMAT == GDF__8BIT_INDEX) {
#if 0
		for (i = 0; i < VIDEO_LOGO_COLORS; i++) {
			video_set_lut (i + VIDEO_LOGO_LUT_OFFSET,
				       logo_red[i], logo_green[i], logo_blue[i]);
		}
#endif
	}

	while (ycount--) {
		xcount = VIDEO_LOGO_WIDTH;
		while (xcount--) {
			r = logo_red[*source - VIDEO_LOGO_LUT_OFFSET];
			g = logo_green[*source - VIDEO_LOGO_LUT_OFFSET];
			b = logo_blue[*source - VIDEO_LOGO_LUT_OFFSET];

			switch (VIDEO_DATA_FORMAT) {
			case GDF__8BIT_INDEX:
				*dest = *source;
				break;
			case GDF__8BIT_332RGB:
				*dest = ((r >> 5) << 5) | ((g >> 5) << 2) | (b >> 6);
				break;
			case GDF_12BIT_444RGB:
				*(unsigned short *) dest =
					SWAP16 ((unsigned short) (((r >> 4) << 8) | ((g >> 4) << 4) | (b >> 4)));
				break;
			case GDF_15BIT_555RGB:
				*(unsigned short *) dest =
					SWAP16 ((unsigned short) (((r >> 3) << 10) | ((g >> 3) << 5) | (b >> 3)));
				break;
			case GDF_16BIT_565RGB:
				*(unsigned short *) dest =
					SWAP16 ((unsigned short) (((r >> 3) << 11) | ((g >> 2) << 5) | (b >> 3)));
				break;
			case GDF_32BIT_X888RGB:
				*(unsigned long *) dest =
					SWAP32 ((unsigned long) ((r << 16) | (g << 8) | b));
				break;
			case GDF_24BIT_888RGB:
#ifdef VIDEO_FB_LITTLE_ENDIAN
				dest[0] = b;
				dest[1] = g;
				dest[2] = r;
#else
				dest[0] = r;
				dest[1] = g;
				dest[2] = b;
#endif
				break;
			}
			source++;
			dest += VIDEO_PIXEL_SIZE;
		}
		dest += skip;
	}
#ifdef CONFIG_VIDEO_BMP_LOGO
	free (logo_red);
	free (logo_green);
	free (logo_blue);
#endif
}

static void *video_logo(void)
{
#ifdef CONFIG_SPLASH_SCREEN
	char *s;
	ulong addr;

	if ((s = getenv ("splashimage")) != NULL) {
//		addr = simple_strtoul (s, NULL, 16);
		addr = NULL;
		if (video_display_bitmap (addr, 0, 0) == 0) {
			return ((void *) (video_fb_address));
		}
	}
#endif /* CONFIG_SPLASH_SCREEN */

	logo_plot(video_fb_address, VIDEO_COLS, (VIDEO_COLS - VIDEO_LOGO_WIDTH)* VIDEO_PIXEL_SIZE, 0);

	//video_drawstring (VIDEO_INFO_X, VIDEO_INFO_Y, (unsigned char *)info);

#ifdef CONFIG_CONSOLE_EXTRA_INFO
	{
		char info[128] = " PMON on Loongson-ICT2E,build r31,2007.8.24.";
		int i, n = ((VIDEO_LOGO_HEIGHT - VIDEO_FONT_HEIGHT) / VIDEO_FONT_HEIGHT);

		for (i = 1; i < n; i++) {
			video_get_info_str (i, info);
			if (*info)
				video_drawstring (VIDEO_INFO_X,
						  VIDEO_INFO_Y + i * VIDEO_FONT_HEIGHT,
						  (unsigned char *)info);
		}
	}
#endif

	return (video_fb_address + VIDEO_LOGO_HEIGHT * VIDEO_LINE_LEN);
}
#endif

void video_cls(void)
{
	memsetl (video_fb_address + VIDEO_LOGO_HEIGHT * VIDEO_LINE_LEN, 
		CONSOLE_SIZE>>2, 
		CONSOLE_BG_COL);
#ifdef MEM_PRINTTO_VIDEO
	memsetl (memfb, CONSOLE_ROWS * CONSOLE_COLS >> 2, CONSOLE_BG_COL);
#endif
}

void inline video_cls_all(void)
{
	if(disableoutput)
		return;
	memsetl(video_fb_address, CONSOLE_SIZE, CONSOLE_BG_COL);
}

void inline video_fill(int color)
{
	memsetl(video_fb_address, VIDEO_COLS * VIDEO_ROWS * VIDEO_PIXEL_SIZE / 4, color);
}

void video_set_background(unsigned char r, unsigned char g, unsigned char b)
{
	int cnt = CONSOLE_SIZE - VIDEO_LOGO_HEIGHT * VIDEO_LINE_LEN;
	unsigned short *p = video_fb_address + VIDEO_LOGO_HEIGHT * VIDEO_LINE_LEN;
	unsigned short color = SWAP16((unsigned short) (((r >> 3) << 11) | ((g >> 2) << 5) | (b >> 3)));

	while (cnt>0) {	
		*(p++) = color;
		cnt -= 2;		
	}
}

static int record = 0;
#ifdef CONFIG_FB_DYN
char console_buffer[2][1280/VIDEO_FONT_HEIGHT+1][1024/VIDEO_FONT_WIDTH+1]={32};
#else
char console_buffer[2][FB_YSIZE/VIDEO_FONT_HEIGHT+1][FB_XSIZE/VIDEO_FONT_WIDTH+1]={32};
#endif

void video_console_print(int console_col, int console_row, unsigned char *s)
{
	int count = strlen (s);

	while (count--){
		video_putchar(console_col * VIDEO_FONT_WIDTH,
			       console_row * VIDEO_FONT_HEIGHT,
			       *s);
		if (record) {
			console_buffer[0][console_row][console_col] = *s;
			console_buffer[1][console_row][console_col] = (char)bgx;
		}
		console_col++;
		s++;
	}
}

void inline begin_record(void)
{
	record = 1;
}

void inline stop_record(void)
{
	record = 0;
}

char inline video_get_console_char(int console_col, int console_row)
{
	return console_buffer[0][console_row][console_col];
}

char inline video_get_console_bgcolor(int console_col, int console_row)
{
	return console_buffer[1][console_row][console_col];
}

void video_set_bg(unsigned char r, unsigned char g, unsigned char b)
{
	bgx = SWAP16 ((unsigned short) (((r >> 3) << 11) | ((g >> 2) << 5) | (b >> 3)));
	bgx |= (bgx << 16);
	fgx =  SWAP16 ((unsigned short) (((128 >> 3) << 11) | ((128 >> 2) << 5) | (128 >> 3)));
	fgx |= (fgx << 16);
	eorx = fgx ^ bgx;
}

unsigned short pallete[16] = {
	SWAP16 ((unsigned short) (((0   >> 3) << 11) | ((0   >> 2) << 5) | (0   >> 3))),
	SWAP16 ((unsigned short) (((0   >> 3) << 11) | ((0   >> 2) << 5) | (128 >> 3))),
	SWAP16 ((unsigned short) (((0   >> 3) << 11) | ((128 >> 2) << 5) | (0   >> 3))),
	SWAP16 ((unsigned short) (((0   >> 3) << 11) | ((128 >> 2) << 5) | (128 >> 3))),
	SWAP16 ((unsigned short) (((128 >> 3) << 11) | ((0   >> 2) << 5) | (0   >> 3))),
	SWAP16 ((unsigned short) (((128 >> 3) << 11) | ((0   >> 2) << 5) | (128 >> 3))),
	SWAP16 ((unsigned short) (((128 >> 3) << 11) | ((128 >> 2) << 5) | (0   >> 3))),
	SWAP16 ((unsigned short) (((0xc0>> 3) << 11) | ((0xc0>> 2) << 5) | (0xc0>> 3))),
	SWAP16 ((unsigned short) (((128 >> 3) << 11) | ((128 >> 2) << 5) | (128 >> 3))),
	SWAP16 ((unsigned short) (((0   >> 3) << 11) | ((0   >> 2) << 5) | (255 >> 3))),
	SWAP16 ((unsigned short) (((0   >> 3) << 11) | ((255 >> 2) << 5) | (0   >> 3))),
	SWAP16 ((unsigned short) (((0   >> 3) << 11) | ((255 >> 2) << 5) | (255 >> 3))),
	SWAP16 ((unsigned short) (((255 >> 3) << 11) | ((0   >> 2) << 5) | (0   >> 3))),
	SWAP16 ((unsigned short) (((255 >> 3) << 11) | ((0   >> 2) << 5) | (255 >> 3))),
	SWAP16 ((unsigned short) (((255 >> 3) << 11) | ((255 >> 2) << 5) | (0   >> 3))),
	SWAP16 ((unsigned short) (((255 >> 3) << 11) | ((255 >> 2) << 5) | (255 >> 3)))
};

static void __cprint(int y, int x,int width,char color, const char *buf)
{
	begin_record();
#ifndef FB_MENU_NOCLOLOR
	bgx = pallete[color>>4];
	bgx |= (bgx << 16);
	fgx =  pallete[color&0xf];
	fgx |= (fgx << 16);
	eorx = fgx ^ bgx;
#endif
	video_console_print(x, y, buf);
#ifndef FB_MENU_NOCLOLOR
	bgx = 0;	
	fgx =  SWAP16 ((unsigned short) (((128 >> 3) << 11) | ((128 >> 2) << 5) | (128 >> 3)));
	fgx |= (fgx << 16);
	eorx = fgx ^ bgx;
#endif
}

void cprintfb(int y, int x, int width, char color, const char *text)
{
	int i,l;
	unsigned char buf[200];

	if (width==0 && text)
		width = strlen(text);
	if (text && (l=strlen(text)))
		memcpy(buf, text, (width && l>width) ? width : l);
	else
		l = 0;
	if (l<width)
		for (i=0; i<width-l; i++)
			buf[l+i] = 0x20;
	if (width) {
		buf[width] = 0;
		__cprint(y, x, width, color, buf);
	}
}

void set_cursor_fb(unsigned char x, unsigned char y)
{
	console_col = x;
	console_row = y;
}

int fb_init (unsigned long fbbase,unsigned long iobase)
{
	unsigned char color8;

    int i,j;
	pGD = &GD;
#if defined(VGA_NOTEBOOK_V1)
	pGD->winSizeX  = 1280;
	pGD->winSizeY  = 800;
#elif defined(VGA_NOTEBOOK_V2)
	pGD->winSizeX  = 1024;
	pGD->winSizeY  = 768;
#else
	pGD->winSizeX  = 640;
	pGD->winSizeY  = 480;
#endif
#if defined(X800x600)
	pGD->winSizeX  = 800;
	pGD->winSizeY  = 600;
#elif defined(X1440x900)
	pGD->winSizeX  = 1440;
	pGD->winSizeY  = 900;
#elif defined(X1024x768)
	pGD->winSizeX  = 1024;
	pGD->winSizeY  = 768;
#elif defined(X1280x1024)
	pGD->winSizeX  = 1280;
	pGD->winSizeY  = 1024;
#elif defined(X800x480)
	pGD->winSizeX  = 800;
	pGD->winSizeY  = 480;
#elif defined(X480x640)
	pGD->winSizeX  = 480;
	pGD->winSizeY  = 640;
#elif defined(X480x272)
	pGD->winSizeX  = 480;
	pGD->winSizeY  = 272;
#elif defined(X320x240)
	pGD->winSizeX  = 320;
	pGD->winSizeY  = 240;
#elif defined(X240x320)
	pGD->winSizeX  = 240;
	pGD->winSizeY  = 320;
#else
	pGD->winSizeX  = FB_XSIZE;
	pGD->winSizeY  = FB_YSIZE;
#endif			
#ifdef CONFIG_FB_DYN
	pGD->winSizeX  = getenv("xres")? strtoul(getenv("xres"),0,0):FB_XSIZE;
	pGD->winSizeY  = getenv("yres")? strtoul(getenv("yres"),0,0):FB_YSIZE;
#endif
	
#if defined(CONFIG_VIDEO_1BPP)
	pGD->gdfIndex  = GDF__1BIT;
	pGD->gdfBytesPP= 1;
#elif defined(CONFIG_VIDEO_2BPP)
	pGD->gdfIndex  = GDF__2BIT;
	pGD->gdfBytesPP= 1;
#elif defined(CONFIG_VIDEO_4BPP)
	pGD->gdfIndex  = GDF__4BIT;
	pGD->gdfBytesPP= 1;
#elif defined(CONFIG_VIDEO_8BPP_INDEX)
	pGD->gdfBytesPP= 1;
	pGD->gdfIndex  = GDF__8BIT_INDEX;
#elif defined(CONFIG_VIDEO_8BPP)
	pGD->gdfBytesPP= 1;
	pGD->gdfIndex  = GDF__8BIT_332RGB;
#elif defined(CONFIG_VIDEO_12BPP)
	pGD->gdfBytesPP= 2;
	pGD->gdfIndex  = GDF_12BIT_444RGB;
#elif defined(CONFIG_VIDEO_15BPP)
	pGD->gdfBytesPP= 2;
	pGD->gdfIndex  = GDF_15BIT_555RGB;
#elif defined(CONFIG_VIDEO_16BPP)
	pGD->gdfBytesPP= 2;
	pGD->gdfIndex  = GDF_16BIT_565RGB;
#elif defined(CONFIG_VIDE0_24BPP)
	pGD->gdfBytesPP= 3;
	pGD->gdfIndex=GDF_24BIT_888RGB;
#elif defined(CONFIG_VIDEO_32BPP)
	pGD->gdfBytesPP= 4;
	pGD->gdfIndex  = GDF_32BIT_X888RGB;
#else
	pGD->gdfBytesPP= 2;
	pGD->gdfIndex  = GDF_16BIT_565RGB;
#endif
	if (fbbase<0x20000000)
		pGD->frameAdrs = 0xb0000000 | fbbase;
	else
		pGD->frameAdrs = fbbase;

	printf("cfb_console init, fb=0x%x\n", pGD->frameAdrs);

	video_fb_address = (void *) VIDEO_FB_ADRS;
#ifdef CONFIG_VIDEO_HW_CURSOR
	video_init_hw_cursor (VIDEO_FONT_WIDTH, VIDEO_FONT_HEIGHT);
#endif

	/* Init drawing pats */
	switch (VIDEO_DATA_FORMAT) {
#if 1
	case GDF__8BIT_INDEX:
		video_set_lut(0x01, CONSOLE_FG_COL, CONSOLE_FG_COL, CONSOLE_FG_COL);
		video_set_lut(0x00, CONSOLE_BG_COL, CONSOLE_BG_COL, CONSOLE_BG_COL);
		fgx = 0x01010101;
		bgx = 0x00000000;
	break;
#endif
	case GDF__1BIT:
		fgx=1;
		bgx=0;
	break;
	case GDF__2BIT:
		fgx=3;
		bgx=0;
	break;
	case GDF__4BIT:
		fgx=0xf;
		bgx=0;
	break;
	case GDF__8BIT_332RGB:
		color8 = ((CONSOLE_FG_COL & 0xe0) |
				((CONSOLE_FG_COL >> 3) & 0x1c) | CONSOLE_FG_COL >> 6);
		fgx = (color8 << 24) | (color8 << 16) | (color8 << 8) | color8;
		color8 = ((CONSOLE_BG_COL & 0xe0) |
				((CONSOLE_BG_COL >> 3) & 0x1c) | CONSOLE_BG_COL >> 6);
		bgx = (color8 << 24) | (color8 << 16) | (color8 << 8) | color8;
	break;
	case GDF_12BIT_444RGB:
		fgx = (((CONSOLE_FG_COL >> 4) << 28) |
		        ((CONSOLE_FG_COL >> 4) << 24) | ((CONSOLE_FG_COL >> 4) << 20) |
				((CONSOLE_FG_COL >> 4) << 16) | ((CONSOLE_FG_COL >> 4) << 12) |
				((CONSOLE_FG_COL >> 4) << 8) | ((CONSOLE_FG_COL >> 4) << 4) |
				(CONSOLE_FG_COL >> 4));
		bgx = (((CONSOLE_BG_COL >> 4) << 28) |
                ((CONSOLE_BG_COL >> 4) << 24) | ((CONSOLE_BG_COL >> 4) << 20) |
				((CONSOLE_BG_COL >> 4) << 16) | ((CONSOLE_BG_COL >> 4) << 12) |
				((CONSOLE_BG_COL >> 4) << 8) | ((CONSOLE_BG_COL >> 4) << 4) |
				(CONSOLE_BG_COL >> 4));
	break;
	case GDF_15BIT_555RGB:
		fgx = (((CONSOLE_FG_COL >> 3) << 26) |
				((CONSOLE_FG_COL >> 3) << 21) | ((CONSOLE_FG_COL >> 3) << 16) |
				((CONSOLE_FG_COL >> 3) << 10) | ((CONSOLE_FG_COL >> 3) << 5) |
				(CONSOLE_FG_COL >> 3));
		bgx = (((CONSOLE_BG_COL >> 3) << 26) |
				((CONSOLE_BG_COL >> 3) << 21) | ((CONSOLE_BG_COL >> 3) << 16) |
				((CONSOLE_BG_COL >> 3) << 10) | ((CONSOLE_BG_COL >> 3) << 5) |
				(CONSOLE_BG_COL >> 3));
	break;
	case GDF_16BIT_565RGB:
		fgx = (((CONSOLE_FG_COL >> 3) << 27) |
				((CONSOLE_FG_COL >> 2) << 21) | ((CONSOLE_FG_COL >> 3) << 16) |
				((CONSOLE_FG_COL >> 3) << 11) | ((CONSOLE_FG_COL >> 2) << 5) |
				(CONSOLE_FG_COL >> 3));
		bgx = (((CONSOLE_BG_COL >> 3) << 27) |
				((CONSOLE_BG_COL >> 2) << 21) | ((CONSOLE_BG_COL >> 3) << 16) |
				((CONSOLE_BG_COL >> 3) << 11) | ((CONSOLE_BG_COL >> 2) << 5) |
				(CONSOLE_BG_COL >> 3));
	break;
	case GDF_32BIT_X888RGB:
		fgx = (CONSOLE_FG_COL << 16) | (CONSOLE_FG_COL << 8) | CONSOLE_FG_COL;
		bgx = (CONSOLE_BG_COL << 16) | (CONSOLE_BG_COL << 8) | CONSOLE_BG_COL;
	break;
	case GDF_24BIT_888RGB:
		fgx = (CONSOLE_FG_COL << 24) | (CONSOLE_FG_COL << 16) |
			(CONSOLE_FG_COL << 8) | CONSOLE_FG_COL;
		bgx = (CONSOLE_BG_COL << 24) | (CONSOLE_BG_COL << 16) |
			(CONSOLE_BG_COL << 8) | CONSOLE_BG_COL;
	break;
	}
	eorx = fgx ^ bgx;

//	memsetl(video_fb_address, VIDEO_COLS * VIDEO_ROWS * VIDEO_PIXEL_SIZE / 4, CONSOLE_BG_COL);
#ifdef CONFIG_VIDEO_LOGO
	/* Plot the logo and get start point of console */
	video_console_address = video_logo();
#else
	video_console_address = video_fb_address;
#endif

	/* Initialize the console */
	console_col = 0;
	console_row = 0;

//	memset(console_buffer, ' ', sizeof console_buffer);
#if defined(MEM_PRINTTO_VIDEO)
	memfb = malloc(CONSOLE_ROWS * CONSOLE_COLS);
//	memset(memfb, 0, CONSOLE_ROWS * CONSOLE_COLS);
#endif
	return 0;
}

