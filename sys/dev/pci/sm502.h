/*
 * (C) Copyright 2002
 * Stäubli Faverges - <www.staubli.com>
 * Pierre AUBERT  p.aubert@staubli.com
 *
 * (C) Copyright 2005
 * Martin Krause TQ-Systems GmbH martin.krause@tqs.de
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
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */

/*
 * Basic video support for SMI SM501 "Voyager" graphic controller
 */

#ifndef _SM501_H_
#define _SM501_H_
#endif

#define PCI_VENDOR_SM		0x126f
#define PCI_DEVICE_SM501	0x0501

typedef struct {
	unsigned int Index;
	unsigned int Value;
} SMI_REGS;

/* Board specific functions                                                  */
int board_video_init (void);
void board_validate_screen (unsigned int base);
const SMI_REGS *board_get_regs (void);
int board_get_width (void);
int board_get_height (void);
int board_video_get_fb (void);



#if 0
//#ifdef CONFIG_VIDEO_SM501_32BPP
 const SMI_REGS init_regs [] =
{
#if 0 /* CRT only */
        {0x00004, 0x0},
        {0x00048, 0x00021807},
        {0x0004C, 0x10090a01},
        {0x00054, 0x1},
        {0x00040, 0x00021807},
        {0x00044, 0x10090a01},
        {0x00054, 0x0},
        {0x80200, 0x00010000},
        {0x80204, 0x0},
        {0x80208, 0x0A000A00},
        {0x8020C, 0x02fa027f},
        {0x80210, 0x004a028b},
        {0x80214, 0x020c01df},
        {0x80218, 0x000201e9},
        {0x80200, 0x00013306},
#else  /* panel + CRT */
        {0x00004, 0x0},
        {0x00048, 0x00021807},
        {0x0004C, 0x091a0a01},
        {0x00054, 0x1},
        {0x00040, 0x00021807},
        {0x00044, 0x091a0a01},
        {0x00054, 0x0},
        {0x80000, 0x0f013106},
        {0x80004, 0xc428bb17},
        {0x8000C, 0x00000000},
        {0x80010, 0x0a000a00},
        {0x80014, 0x02800000},
        {0x80018, 0x01e00000},
        {0x8001C, 0x00000000},
        {0x80020, 0x01e00280},
        {0x80024, 0x02fa027f},
        {0x80028, 0x004a028b},
        {0x8002C, 0x020c01df},
        {0x80030, 0x000201e9},
        {0x80200, 0x00010000},
#endif
        {0, 0}
};


#endif /* _SM501_H_ */
