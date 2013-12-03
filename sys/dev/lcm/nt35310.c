/* driver for the NT35310
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 */
#include <pmon.h>
#include <cpu.h>
#include <sys/types.h>
#include <termio.h>
#include <string.h>
#include <time.h>
#include <stdlib.h>
#include <stdio.h>

#include "include/gpio.h"

#if 0
#define DEBUGP(x, args...) printf("%s: " x, __FUNCTION__, ## args);
#define DEBUGPC(x, args...) printf(x, ## args);
#else
#define DEBUGP(x, args...) do { } while (0)
#define DEBUGPC(x, args...) do { } while (0)
#endif

/* FIXME: glofish definitions */
#if defined(LS1BSOC)
#define GPIO_SCLK	(24) 	/* GPIO24 */
#define GPIO_MOSI	(26)	/* GPIO26 */
#define GPIO_MISO	(25)	/* GPIO25 */
#define GPIO_CS	(30)	/* GPIO28 */
#define GPIO_REST	(39)	/* GPIO39 */
#elif defined(LS1CSOC)
#define GPIO_SCLK	(78) 	/* GPIO24 */
#define GPIO_MOSI	(79)	/* GPIO26 */
#define GPIO_MISO	(80)	/* GPIO25 */
#define GPIO_CS	(84)	/* GPIO28 */
#define GPIO_REST	(39)	/* GPIO39 */
#endif


/* 150uS minimum clock cycle, we have two of this plus our other
 * instructions */
#define SPI_DELAY	delay(10)	/* 200uS */
//#define SPI_DELAY

#define JBT_COMMAND	0x000
#define JBT_DATA	0x100

static int spi_read(int bitlen)
{
	int j;
	int tmpdout = 00;
	int ret;

	gpio_set_value(GPIO_CS, 0);
	SPI_DELAY;

/*	gpio_set_value(GPIO_SCLK, 0);
	SPI_DELAY;
	gpio_set_value(GPIO_SCLK, 1);
	SPI_DELAY;*/

	for (j = bitlen - 1; j >= 0; j--) {
		gpio_set_value(GPIO_SCLK, 0);
		SPI_DELAY;
		gpio_set_value(GPIO_SCLK, 1);
		SPI_DELAY;
		ret = gpio_get_value(GPIO_MISO);
		tmpdout |= ret << j;
		SPI_DELAY;
	}
	gpio_set_value(GPIO_CS, 1);

	return tmpdout;
}

static int spi_write(int bitlen, int value)
{
	int j;
	int tmpdout = value;

	gpio_set_value(GPIO_CS, 0);
	SPI_DELAY;

	for (j = 0; j < bitlen; j++) {
		gpio_set_value(GPIO_SCLK, 0);
		SPI_DELAY;
		if (tmpdout & (1 << bitlen-1)) {
			gpio_set_value(GPIO_MOSI, 1);
			DEBUGPC("1");
		} else {
			gpio_set_value(GPIO_MOSI, 0);
			DEBUGPC("0");
		}
		SPI_DELAY;
		gpio_set_value(GPIO_SCLK, 1);
		SPI_DELAY;
		tmpdout <<= 1;
	}
	gpio_set_value(GPIO_CS, 1);
}

static inline int spi_write_cmd(int value)
{
	spi_write(9, value);
}

static inline int spi_write_dat(int value)
{
	spi_write(9, value | 0x100);
}

static void nt35310_hw_init(void)
{
	DEBUGP("entering\n");

#if defined(CONFIG_PCA953X)
	pca953x_gpio_set_value(0x26, 12, 1);
	delay(100);
	pca953x_gpio_set_value(0x26, 12, 0);
	delay(10000);
	pca953x_gpio_set_value(0x26, 12, 1);
	delay(10000);
#else
//	gpio_set_value(GPIO_REST, 1);
//	delay(100);
//	gpio_set_value(GPIO_REST, 0);
//	delay(10000);
//	gpio_set_value(GPIO_REST, 1);
//	delay(10000);
#endif

	/* CMD2UNLOCK */
	spi_write_cmd(0xed);
	spi_write_dat(0x01);
	spi_write_dat(0xfe);

	/* SPI&RGB INTERFACE SETTING */
	spi_write_cmd(0xb3);
	spi_write_dat(0x21);

	/* EXIT_SLEEP_MODE */
	spi_write_cmd(0x11);
	delay(8000);

	/* RGB Interface Signal Control */
	spi_write_cmd(0x3b);
	spi_write_dat(0x0b);	/* Clock polarity极性 set for RGB Interface */
	spi_write_dat(0x0c);
	spi_write_dat(0x04);
	spi_write_dat(0x28);
	spi_write_dat(0x32);

	/* Memory Data Access Control */
	spi_write_cmd(0x36);
	spi_write_dat(0x00);
//	spi_write_dat(0x14);

	/* Display Inversion Off */
	spi_write_cmd(0x20);

	/* Set the Interface Pixel Format */
	spi_write_cmd(0x3A);
//	spi_write_dat(0x55);	/* 16-bits / pixel */
	spi_write_dat(0x66);	/* 16-bits / pixel */

	/* Tearing Effect Line ON */
//	spi_write_cmd(0x35);
//	spi_write_dat(0x00);

	/* Display On */
	spi_write_cmd(0x29);
}

void nt35310_exit(void)
{
	ls1x_gpio_free(GPIO_SCLK);
	ls1x_gpio_free(GPIO_MOSI);
//	ls1x_gpio_free(GPIO_MISO);
	ls1x_gpio_free(GPIO_CS);
//	ls1x_gpio_free(GPIO_REST);
}

int nt35310_init(void)
{
	/* initialize SPI for GPIO bitbang */
	/* FIXME: glofiish */
	ls1x_gpio_direction_output(GPIO_SCLK, 1);
	ls1x_gpio_direction_output(GPIO_MOSI, 1);
//	ls1x_gpio_direction_input(GPIO_MISO);
	ls1x_gpio_direction_output(GPIO_CS, 1);

	/* get LCM out of reset */
#if defined(CONFIG_PCA953X)
	pca953x_gpio_direction_output(0x26, 12);
#else
//	ls1x_gpio_direction_output(GPIO_REST, 1);
#endif

	/* according to data sheet: wait 50ms (Tpos of LCM). However, 50ms
	 * seems unreliable with later LCM batches, increasing to 90ms */
	nt35310_hw_init();
	nt35310_exit();

	return 0;
}

