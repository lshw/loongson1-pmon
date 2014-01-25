/* driver for the ST7565
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 */

#include <termio.h>
#include <string.h>
#include <time.h>
#include <stdlib.h>
#include <stdio.h>

#include <pmon.h>
#include <cpu.h>
#include <machine/pio.h>

#include <target/gpio.h>

#define udelay(x) delay(x)

#define LS1X_GPIO_OUT0	((void *)0xbfd010f0)
#define RES	17
#define CS	16
#define DC	18
#define RD	20
#define WR	19

#define D0	8
#define D1	9
#define D2	10
#define D3	11
#define D4	12
#define D5	13
#define D6	14
#define D7	15

#define X_OFFSET		0
#define ST7565FB_WIDTH		128
#define ST7565FB_HEIGHT		64

#define ST7565FB_DATA			1
#define ST7565FB_COMMAND		0

#define ST7565FB_CONTRAST		0x81
#define SSD1307FB_ADC_NOR		0xa0
#define SSD1307FB_ADC_REV		0xa1
#define SSD1307FB_RESET			0xe2
#define ST7565FB_LCD_19BIAS		0xa2
#define ST7565FB_LCD_17BIAS		0xa3
#define ST7565FB_DISPLAY_ALL_NOR		0xa4
#define ST7565FB_DISPLAY_ALL_REV		0xa5
#define ST7565FB_DISPLAY_NOR		0xa6
#define ST7565FB_DISPLAY_REV		0xa7
#define ST7565FB_DISPLAY_OFF		0xae
#define ST7565FB_DISPLAY_ON		0xaf
#define ST7565FB_SCAN_NOR		0xc0
#define ST7565FB_SCAN_REV		0xc8
#define ST7565FB_START_PAGE_ADDRESS	0xb0

#include "logo_mono.c"

struct st7565fb_par {
	void *reg_addr;
	unsigned char *screen_base;
};

static int st7565fb_write_array(struct st7565fb_par *par, u8 type, u8 valu)
{
	/* 根据显示屏使用的gpio引脚，简化代码，减少操作时间（gpio_set_value等函数相对占用时间多） */
	u32 ret;

	ret = readl(par->reg_addr);
	ret &= 0xfff200ff;
//	ret |= (1 << 20)/*rd*/;	/* rd 只在读时才用到，不需要读的话可以在初始化时置1即可 */
	ret = ret | (type << 18)/*dc*/ | (valu << 8);
	writel(ret, par->reg_addr);

//	ret |= (1 << 16)/*cs*/;	/* 片选可以一直有效 */
	ret = ret | (1 << 19)/*wr*/ | (1 << 18)/*dc*/;
	writel(ret, par->reg_addr);
}

static inline void st7565fb_write_cmd(struct st7565fb_par *par, u8 cmd)
{
	st7565fb_write_array(par, ST7565FB_COMMAND, cmd);
}

static inline void st7565fb_write_data(struct st7565fb_par *par, u8 data)
{
	st7565fb_write_array(par, ST7565FB_DATA, data);
}

static void st7565fb_update_display(struct st7565fb_par *par)
{
	u8 *vmem = par->screen_base;
	int i, j, k;

	/*
	 * The screen is divided in pages, each having a height of 8
	 * pixels, and the width of the screen. When sending a byte of
	 * data to the controller, it gives the 8 bits for the current
	 * column. I.e, the first byte are the 8 bits of the first
	 * column, then the 8 bits for the second column, etc.
	 *
	 *
	 * Representation of the screen, assuming it is 5 bits
	 * wide. Each letter-number combination is a bit that controls
	 * one pixel.
	 *
	 * A0 A1 A2 A3 A4
	 * B0 B1 B2 B3 B4
	 * C0 C1 C2 C3 C4
	 * D0 D1 D2 D3 D4
	 * E0 E1 E2 E3 E4
	 * F0 F1 F2 F3 F4
	 * G0 G1 G2 G3 G4
	 * H0 H1 H2 H3 H4
	 *
	 * If you want to update this screen, you need to send 5 bytes:
	 *  (1) A0 B0 C0 D0 E0 F0 G0 H0
	 *  (2) A1 B1 C1 D1 E1 F1 G1 H1
	 *  (3) A2 B2 C2 D2 E2 F2 G2 H2
	 *  (4) A3 B3 C3 D3 E3 F3 G3 H3
	 *  (5) A4 B4 C4 D4 E4 F4 G4 H4
	 */

	for (i = 0; i < (ST7565FB_HEIGHT / 8); i++) {
		st7565fb_write_cmd(par, ST7565FB_START_PAGE_ADDRESS + i);
		st7565fb_write_cmd(par, 0x00+X_OFFSET%16);
		st7565fb_write_cmd(par, 0x10+X_OFFSET/16);

		for (j = 0; j < ST7565FB_WIDTH; j++) {
			u8 buf = 0;
			for (k = 0; k < 8; k++) {
				u32 page_length = ST7565FB_WIDTH * i;
				u32 index = page_length + (ST7565FB_WIDTH * k + j) / 8;
				u8 byte = *(vmem + index);
//				u8 bit = byte & (1 << (j % 8));
//				bit = bit >> (j % 8);
				u8 bit = byte & (1 << (7 - (j % 8)));
				bit = bit >> (7 - (j % 8));
				buf |= bit << k;
			}
			st7565fb_write_data(par, buf);
		}
	}
}

static void st7565fb_hw_init(struct st7565fb_par *par)
{

	/* Reset the screen */
	gpio_set_value(RES, 0);
	udelay(2000);
	gpio_set_value(RES, 1);

	st7565fb_write_cmd(par, SSD1307FB_RESET);	//寄存器复位
	udelay(1000);

	/* 设置ADC和SCAN的模式可以对显示进行旋转 */
	/* Sets the display RAM address SEG output correspondence */
	st7565fb_write_cmd(par, SSD1307FB_ADC_NOR);
	/* Common output mode select */
	st7565fb_write_cmd(par, ST7565FB_SCAN_REV);

	/* Sets the LCD drive voltage bias ratio */
	st7565fb_write_cmd(par, ST7565FB_LCD_19BIAS);
	/* Select internal resistor ratio(Rb/Ra) mode */
	st7565fb_write_cmd(par, 0x24);
	/* Select internal power supply operating mode */
	st7565fb_write_cmd(par, 0x2f);
	/* Set the V0 output voltage electronic volume register */
	st7565fb_write_cmd(par, 0x81);
	st7565fb_write_cmd(par, 0x3f);
	/* Display start line set st7565r */
	st7565fb_write_cmd(par, 0x60);
	/* Booster ratio set */
	st7565fb_write_cmd(par, 0xf8);
	st7565fb_write_cmd(par, 0x03);
	/* Sets the LCD display normal/ reverse */
	st7565fb_write_cmd(par, ST7565FB_DISPLAY_NOR);
	st7565fb_write_cmd(par, ST7565FB_DISPLAY_ALL_NOR);
	st7565fb_write_cmd(par, ST7565FB_DISPLAY_ON);

	st7565fb_update_display(par);
}

static void st7565_exit(void)
{
/*	ls1x_gpio_free(RES);
	ls1x_gpio_free(CS);
	ls1x_gpio_free(DC);
	ls1x_gpio_free(RD);
	ls1x_gpio_free(WR);

	ls1x_gpio_free(D0);
	ls1x_gpio_free(D1);
	ls1x_gpio_free(D2);
	ls1x_gpio_free(D3);
	ls1x_gpio_free(D4);
	ls1x_gpio_free(D5);
	ls1x_gpio_free(D6);
	ls1x_gpio_free(D7);*/
}

int st7565_init(void)
{
	struct st7565fb_par par;

	par.reg_addr = LS1X_GPIO_OUT0;
	par.screen_base = logo_linux_mono_data;

	ls1x_gpio_direction_output(RES, 1);
	ls1x_gpio_direction_output(CS, 1);
	ls1x_gpio_direction_output(DC, 1);
	ls1x_gpio_direction_output(RD, 1);
	ls1x_gpio_direction_output(WR, 1);
	ls1x_gpio_direction_output(D0, 1);
	ls1x_gpio_direction_output(D1, 1);
	ls1x_gpio_direction_output(D2, 1);
	ls1x_gpio_direction_output(D3, 1);
	ls1x_gpio_direction_output(D4, 1);
	ls1x_gpio_direction_output(D5, 1);
	ls1x_gpio_direction_output(D6, 1);
	ls1x_gpio_direction_output(D7, 1);

	st7565fb_hw_init(&par);
	st7565_exit();

	return 0;
}

