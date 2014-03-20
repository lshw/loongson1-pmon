/*
 *  Copyright (c) 2014 Tang, Haifeng <tanghaifeng-gz@loongson.cn>
 *
 *  This file is subject to the terms and conditions of the GNU General Public
 *  License. See the file COPYING in the main directory of this archive for
 *  more details.
 */

#include <stdlib.h>
#include <stdio.h>

#include <pmon.h>
#include <machine/pio.h>
#include <target/ls1x_spi.h>

#define DIV_ROUND_UP(n,d) (((n) + (d) - 1) / (d))

extern inline int fls(int x);
extern int tgt_apbfreq(void);

struct ls1x_spi spi0;
struct spi_device spi_flash;

static inline struct ls1x_spi *ls1x_spi_to_hw(struct spi_device *sdev)
{
	return sdev->hw;
}

static unsigned int ls1x_spi_div(struct spi_device *spi, unsigned int hz)
{
	struct ls1x_spi *hw = ls1x_spi_to_hw(spi);
	unsigned int div, div_tmp, bit;
	unsigned long clk;

	clk = hw->clk;
	div = DIV_ROUND_UP(clk, hz);

	if (div < 2)
		div = 2;

	if (div > 4096)
		div = 4096;

	bit = fls(div) - 1;
	switch(1 << bit) {
		case 16: 
			div_tmp = 2;
			if (div > (1<<bit)) {
				div_tmp++;
			}
			break;
		case 32:
			div_tmp = 3;
			if (div > (1<<bit)) {
				div_tmp += 2;
			}
			break;
		case 8:
			div_tmp = 4;
			if (div > (1<<bit)) {
				div_tmp -= 2;
			}
			break;
		default:
			div_tmp = bit - 1;
			if (div > (1<<bit)) {
				div_tmp++;
			}
			break;
	}

	return div_tmp;
}

static int ls1x_spi_setup(struct spi_device *spi)
{
	struct ls1x_spi *hw = ls1x_spi_to_hw(spi);
	u8 ret;

	/* 注意spi bit per word 控制器支持8bit */
//	bpw = t ? t->bits_per_word : spi->bits_per_word;

	if (spi->max_speed_hz != hw->speed_hz) {
		hw->speed_hz = spi->max_speed_hz;
		hw->div = ls1x_spi_div(spi, hw->speed_hz);
	}
	hw->mode = spi->mode & (SPI_CPOL | SPI_CPHA);

	ret = readb(hw->base + REG_SPCR);
	ret = ret & 0xf0;
	ret = ret | (hw->mode << 2) | (hw->div & 0x03);
	writeb(ret, hw->base + REG_SPCR);

	ret = readb(hw->base + REG_SPER);
	ret = ret & 0xfc;
	ret = ret | (hw->div >> 2);
	writeb(ret, hw->base + REG_SPER);

	return 0;
}

void ls1x_spi_chipselect(struct spi_device *spi, int is_active)
{
	struct ls1x_spi *hw = ls1x_spi_to_hw(spi);
	u8 ret;

	if (spi->chip_select != hw->chip_select) {
		ls1x_spi_setup(spi);
		hw->chip_select = spi->chip_select;
//		printf("clk = %ld div = %d speed_hz = %d mode = %d\n", 
//			hw->clk, hw->div, hw->speed_hz, hw->mode);
	}

	ret = readb(hw->base + REG_SOFTCS);
	ret = (ret & 0xf0) | (0x01 << spi->chip_select);
	
	if (is_active) {
		ret = ret & (~(0x10 << spi->chip_select));
		writeb(ret, hw->base + REG_SOFTCS);
	} else {
		ret = ret | (0x10 << spi->chip_select);
		writeb(ret, hw->base + REG_SOFTCS);
	}
}

static inline void ls1x_spi_wait_rxe(struct ls1x_spi *hw)
{
	u8 ret;

	ret = readb(hw->base + REG_SPSR);
	ret = ret | 0x80;
	writeb(ret, hw->base + REG_SPSR);	/* Int Clear */

	ret = readb(hw->base + REG_SPSR);
	if (ret & 0x40) {
		writeb(ret & 0xbf, hw->base + REG_SPSR);	/* Write-Collision Clear */
	}
}

static inline void ls1x_spi_wait_txe(struct ls1x_spi *hw)
{
	while (1) {
		if (readb(hw->base + REG_SPSR) & 0x80) {
			break;
		}
	}
}

void ls1x_spi_writeb(struct spi_device *spi, u8 value)
{
	struct ls1x_spi *hw = ls1x_spi_to_hw(spi);

	writeb(value, hw->base + REG_TXFIFO);
	ls1x_spi_wait_txe(hw);
	readb(hw->base + REG_RXFIFO);
	ls1x_spi_wait_rxe(hw);
}

u8 ls1x_spi_readb(struct spi_device *spi)
{
	struct ls1x_spi *hw = ls1x_spi_to_hw(spi);
	u8 value;

	writeb(0, hw->base + REG_TXFIFO);
	ls1x_spi_wait_txe(hw);
	value = readb(hw->base + REG_RXFIFO);
	ls1x_spi_wait_rxe(hw);
	return value;
}

/* spi flash 读使能 */
void ls1x_spi_flash_ren(struct spi_device *spi, int en)
{
	struct ls1x_spi *hw = ls1x_spi_to_hw(spi);
	u8 val, spcr;

	val = readb(hw->base + REG_PARAM);
	if (en) {
		val |= 0x01;
//		spcr = readb(hw->base + REG_SPCR);
//		spcr &= ~0x40;
//		writeb(spcr, hw->base + REG_SPCR);
	} else {
		val &= 0xfe;
//		spcr = readb(hw->base + REG_SPCR);
//		spcr |= 0x40;
//		writeb(spcr, hw->base + REG_SPCR);
	}
	writeb(val, hw->base + REG_PARAM);
}

static void ls1x_spi_hw_init(struct ls1x_spi *hw)
{
	u8 val;

	/* 使能SPI控制器，master模式，使能或关闭中断 */
	writeb(0x53, hw->base + REG_SPCR);
	/* 清空状态寄存器 */
	writeb(0xc0, hw->base + REG_SPSR);
	/* 1字节产生中断，采样(读)与发送(写)时机同时 */
	writeb(0x03, hw->base + REG_SPER);
	/* 片选设置 */
	writeb(0xff, hw->base + REG_SOFTCS);
	/* 关闭SPI flash */
	val = readb(hw->base + REG_PARAM);
	val &= 0xfe;
	writeb(val, hw->base + REG_PARAM);
	/* SPI flash时序控制寄存器 */
	writeb(0x05, hw->base + REG_TIMING);
}

int ls1x_spi_probe(void)
{
	spi0.base = LS1X_SPI0_BASE;
#if defined(LS1ASOC)
	spi0.clk = 133000000 / 2;
#else
	spi0.clk = tgt_apbfreq();
#endif
	ls1x_spi_hw_init(&spi0);

	spi_flash.hw = &spi0;
	spi_flash.max_speed_hz = 60000000;	/* Hz */
	spi_flash.mode = SPI_MODE_0;
	spi_flash.chip_select = 0;
	ls1x_spi_setup(&spi_flash);

	return 0;
}
