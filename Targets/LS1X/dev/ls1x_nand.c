/*
 *  Copyright (c) 2013 Tang, Haifeng <tanghaifeng-gz@loongson.cn>
 *
 *  This file is subject to the terms and conditions of the GNU General Public
 *  License. See the file COPYING in the main directory of this archive for
 *  more details.
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <linux/mtd/mtd.h>
#include <linux/mtd/nand.h>
#include <linux/mtd/partitions.h>
#include <sys/malloc.h>

#include <pmon.h>
#include <target/ls1x_nand.h>

static unsigned int order_addr_in;

struct ls1x_nand_info {
	struct nand_chip	nand_chip;

	/* MTD data control*/
	unsigned int	buf_start;
	unsigned int	buf_count;
	/* NAND registers*/
	unsigned int	mmio_base;

	unsigned int	dma_desc;
	unsigned int	dma_desc_phys;

	unsigned char	*data_buff;
	unsigned int	data_buff_phys;

	/* relate to the command */
//	int	use_ecc;	/* use HW ECC ? */
	unsigned int	cmd;

	unsigned int	seqin_column;
	unsigned int	seqin_page_addr;
};

struct mtd_info *ls1x_mtd = NULL;

static void nand_gpio_init(void)
{
#ifdef LS1ASOC
{
	int val;
#ifdef NAND_USE_LPC //NAND复用LPC PWM01
	val = __raw_readl(GPIO_MUX);
	val |= 0x2a000000;
	__raw_writel(val, GPIO_MUX);

	val = __raw_readl(GPIO_CONF2);
	val &= ~(0xffff<<6);			//nand_D0~D7 & nand_control pin
	__raw_writel(val, GPIO_CONF2);
#elif NAND_USE_SPI1 //NAND复用SPI1 PWM23
	val = __raw_readl(GPIO_MUX);
	val |= 0x14000000;
	__raw_writel(val, GPIO_MUX);

	val = __raw_readl(GPIO_CONF1);
	val &= ~(0xf<<12);				//nand_D0~D3
	__raw_writel(val, GPIO_CONF1);

	val = __raw_readl(GPIO_CONF2);
	val &= ~(0xfff<<12);			//nand_D4~D7 & nand_control pin
	__raw_writel(val, GPIO_CONF2);
#endif
}
#endif
}

static int ls1x_nand_ecc_calculate(struct mtd_info *mtd,
		const uint8_t *dat, uint8_t *ecc_code)
{
	return 0;
}

static int ls1x_nand_ecc_correct(struct mtd_info *mtd,
		uint8_t *dat, uint8_t *read_ecc, uint8_t *calc_ecc)
{
	return 0;
}

static void ls1x_nand_ecc_hwctl(struct mtd_info *mtd, int mode)
{
	return;
}

static int ls1x_nand_waitfunc(struct mtd_info *mtd, struct nand_chip *chip)
{
//	udelay(50);
	return 0;
}

static void ls1x_nand_select_chip(struct mtd_info *mtd, int chip)
{
	return;
}

static int ls1x_nand_dev_ready(struct mtd_info *mtd)
{
	struct ls1x_nand_info *info = mtd->priv;
	unsigned int ret;
	ret = nand_readl(info, NAND_CMD) & 0x000f0000;
	if (ret != 0x000f0000) {
		return 0;
	}
	return 1;
}

static void ls1x_nand_read_buf(struct mtd_info *mtd, uint8_t *buf, int len)
{
	struct ls1x_nand_info *info = mtd->priv;
	int real_len = min_t(size_t, len, info->buf_count - info->buf_start);

	memcpy(buf, info->data_buff + info->buf_start, real_len);
	info->buf_start += real_len;
}

static u16 ls1x_nand_read_word(struct mtd_info *mtd)
{
	struct ls1x_nand_info *info = mtd->priv;
	u16 retval = 0xFFFF;

	if (!(info->buf_start & 0x1) && info->buf_start < info->buf_count) {
		retval = *(u16 *)(info->data_buff + info->buf_start);
	}
	info->buf_start += 2;
	return retval;
}

static uint8_t ls1x_nand_read_byte(struct mtd_info *mtd)
{
	struct ls1x_nand_info *info = mtd->priv;
	char retval = 0xFF;

	if (info->buf_start < info->buf_count)
		retval = info->data_buff[(info->buf_start)++];

	return retval;
}

static void ls1x_nand_write_buf(struct mtd_info *mtd, const uint8_t *buf, int len)
{
	struct ls1x_nand_info *info = mtd->priv;
	int real_len = min_t(size_t, len, info->buf_count - info->buf_start);

	memcpy(info->data_buff + info->buf_start, buf, real_len);
	info->buf_start += real_len;
}

static int ls1x_nand_verify_buf(struct mtd_info *mtd, const uint8_t *buf, int len)
{
	int i = 0;

	while (len--) {
		if (buf[i++] != ls1x_nand_read_byte(mtd) ) {
			printk("verify error..., i= %d !\n\n", i-1);
			return -1;
		}
	}
	return 0;
}

static void dma_cache_inv(unsigned long base, unsigned long num)
{
	CPU_IOFlushDCache((base & 0x1fffffff) | 0x80000000, num, 0);
}

static void dma_cache_wback(unsigned long base, unsigned long num)
{
	CPU_IOFlushDCache((base & 0x1fffffff) | 0x80000000, num, 1);
}

static void dma_setup(unsigned int flags, struct ls1x_nand_info *info)
{
	int timeout = 8000;

	writel(0, info->dma_desc + DMA_ORDERED);
	writel(info->data_buff_phys, info->dma_desc + DMA_SADDR);
	writel(DMA_ACCESS_ADDR, info->dma_desc + DMA_DADDR);
	writel((info->buf_count + 3) / 4, info->dma_desc + DMA_LENGTH);
	writel(0, info->dma_desc + DMA_STEP_LENGTH);
	writel(1, info->dma_desc + DMA_STEP_TIMES);

	if (flags) {
		writel(0x00001001, info->dma_desc + DMA_CMD);
	} else {
		writel(0x00000001, info->dma_desc + DMA_CMD);
	}
	dma_cache_wback((unsigned long)(info->dma_desc), DMA_DESC_NUM);

	writel((info->dma_desc_phys & ~0x1F) | 0x8, order_addr_in);
	while ((readl(order_addr_in) & 0x8) && (timeout-- > 0)) {
//		printf("%s. %x\n",__func__, readl(order_addr_in));
//		udelay(5);
	}
}

static int ls1x_nand_done(struct ls1x_nand_info *info)
{
	int ret, timeout = 200000;

	do {
		ret = nand_readl(info, NAND_CMD);
//		printf("NAND_CMD=0x%2X\n", nand_readl(info, NAND_CMD));
	} while (((ret & 0x400) != 0x400) && (timeout-- > 0));

	return timeout;
}

static void inline ls1x_nand_start(struct ls1x_nand_info *info)
{
	nand_writel(info, NAND_CMD, nand_readl(info, NAND_CMD) | 0x1);
}

static void inline ls1x_nand_stop(struct ls1x_nand_info *info)
{
}

static void ls1x_nand_cmdfunc(struct mtd_info *mtd, unsigned command, int column, int page_addr)
{
	struct ls1x_nand_info *info = mtd->priv;
	unsigned cmd_prev;
	
	cmd_prev = info->cmd;
	info->cmd = command;

	switch (command) {
	case NAND_CMD_READOOB:
		info->buf_count = mtd->oobsize;
		info->buf_start = 0;
/*		if (info->buf_count <= 0) {
			printf("oobsize error!!!\n");
			break;
		}*/
		nand_writel(info, NAND_CMD, SPARE | READ);
		nand_writel(info, NAND_ADDR_L, MAIN_SPARE_ADDRL(page_addr) + mtd->writesize);
		nand_writel(info, NAND_ADDR_H, MAIN_SPARE_ADDRH(page_addr));
		nand_writel(info, NAND_OPNUM, info->buf_count);
		nand_writel(info, NAND_PARAM, (nand_readl(info, NAND_PARAM) & 0xc000ffff) | (info->buf_count << 16));
		ls1x_nand_start(info);
		dma_setup(0, info);
		if (!ls1x_nand_done(info)) {
			printf("Wait time out!!!\n");
			/* Stop State Machine for next command cycle */
			ls1x_nand_stop(info);
			break;
		}
		dma_cache_inv((unsigned long)(info->data_buff), info->buf_count);
		break;
	case NAND_CMD_READ0:
		info->buf_count = mtd->oobsize + mtd->writesize;
		info->buf_start = 0;
/*		if (info->buf_count <= 0) {
			printf("oobsize+writesize error!!!\n");
			break;
		}*/
		nand_writel(info, NAND_CMD, SPARE | MAIN | READ);
		nand_writel(info, NAND_ADDR_L, MAIN_SPARE_ADDRL(page_addr));
		nand_writel(info, NAND_ADDR_H, MAIN_SPARE_ADDRH(page_addr));
		nand_writel(info, NAND_OPNUM, info->buf_count);
		nand_writel(info, NAND_PARAM, (nand_readl(info, NAND_PARAM) & 0xc000ffff) | (info->buf_count << 16)); /* 1C注意 */
		ls1x_nand_start(info);
		dma_setup(0, info);
		if (!ls1x_nand_done(info)) {
			printf("Wait time out!!!\n");
			/* Stop State Machine for next command cycle */
			ls1x_nand_stop(info);
			break;
		}
		dma_cache_inv((unsigned long)(info->data_buff), info->buf_count);
		break;
	case NAND_CMD_SEQIN:
		info->buf_count = mtd->oobsize + mtd->writesize - column;
		info->buf_start = 0;
		info->seqin_column = column;
		info->seqin_page_addr = page_addr;
		break;
	case NAND_CMD_PAGEPROG:
		if (cmd_prev != NAND_CMD_SEQIN) {
			printf("Prev cmd don't complete...\n");
			break;
		}
/*		if (info->buf_count <= 0) {
			printf("write oobsize+writesize error!!!\n");
			break;
		}*/

		if (info->seqin_column < mtd->writesize)
			nand_writel(info, NAND_CMD, SPARE | MAIN | WRITE);
		else
			nand_writel(info, NAND_CMD, SPARE | WRITE);
		nand_writel(info, NAND_ADDR_L, MAIN_SPARE_ADDRL(info->seqin_page_addr) + info->seqin_column);
		nand_writel(info, NAND_ADDR_H, MAIN_SPARE_ADDRH(info->seqin_page_addr));
		nand_writel(info, NAND_OPNUM, info->buf_count);
		nand_writel(info, NAND_PARAM, (nand_readl(info, NAND_PARAM) & 0xc000ffff) | (info->buf_count << 16)); /* 1C注意 */
		dma_cache_wback((unsigned long)(info->data_buff), info->buf_count);
		ls1x_nand_start(info);
		dma_setup(1, info);
		if (!ls1x_nand_done(info)) {
			printf("Wait time out!!!\n");
			/* Stop State Machine for next command cycle */
			ls1x_nand_stop(info);
			break;
		}
		break;
	case NAND_CMD_RESET:
		nand_writel(info, NAND_CMD, RESET);
		ls1x_nand_start(info);
		if (!ls1x_nand_done(info)) {
			printf("Wait time out!!!\n");
			/* Stop State Machine for next command cycle */
			ls1x_nand_stop(info);
		}
		break;
	case NAND_CMD_ERASE1:
		nand_writel(info, NAND_ADDR_L, MAIN_ADDRL(page_addr));
		nand_writel(info, NAND_ADDR_H, MAIN_ADDRH(page_addr));
		nand_writel(info, NAND_OPNUM, 0x01);
		nand_writel(info, NAND_PARAM, (nand_readl(info, NAND_PARAM) & 0xc000ffff) | (0x1 << 16)); /* 1C注意 */
		nand_writel(info, NAND_CMD, ERASE);
		ls1x_nand_start(info);
		if (!ls1x_nand_done(info)) {
			printk(KERN_ERR "Wait time out!!!\n");
			ls1x_nand_stop(info);
		}
		break;
	case NAND_CMD_STATUS:
		info->buf_count = 0x1;
		info->buf_start = 0x0;
		nand_writel(info, NAND_CMD, READ_STATUS);
		ls1x_nand_start(info);
		if (!ls1x_nand_done(info)) {
			printf("Wait time out!!!\n");
			/* Stop State Machine for next command cycle */
			ls1x_nand_stop(info);
		}
		*(info->data_buff) = (nand_readl(info, NAND_IDH) >> 8) & 0xff;
		*(info->data_buff) |= 0x80;
		break;
	case NAND_CMD_READID:
		info->buf_count = 0x5;
		info->buf_start = 0;
		nand_writel(info, NAND_CMD, READ_ID);
		ls1x_nand_start(info);
		if (!ls1x_nand_done(info)) {
			printf("Wait time out!!!\n");
			/* Stop State Machine for next command cycle */
			ls1x_nand_stop(info);
			break;
		}
		*(info->data_buff) = nand_readl(info, NAND_IDH);
		*(info->data_buff + 1) = (nand_readl(info, NAND_IDL) >> 24) & 0xff;
		*(info->data_buff + 2) = (nand_readl(info, NAND_IDL) >> 16) & 0xff;
		*(info->data_buff + 3) = (nand_readl(info, NAND_IDL) >> 8) & 0xff;
		*(info->data_buff + 4) = nand_readl(info, NAND_IDL) & 0xff;
		break;
	case NAND_CMD_ERASE2:
	case NAND_CMD_READ1:
		break;
	default :
		printf("non-supported command.\n");
		break;
	}

	nand_writel(info, NAND_CMD, 0x00);
}

static void ls1x_nand_init_hw(struct ls1x_nand_info *info)
{
	nand_writel(info, NAND_CMD, 0x00);
	nand_writel(info, NAND_ADDR_L, 0x00);
	nand_writel(info, NAND_ADDR_H, 0x00);
	nand_writel(info, NAND_TIMING, (HOLD_CYCLE << 8) | WAIT_CYCLE);
#if defined(LS1ASOC) || defined(LS1BSOC)
	nand_writel(info, NAND_PARAM, 0x00000100);	/* 设置外部颗粒大小，1A 2Gb? */
#elif defined(LS1CSOC)
	nand_writel(info, NAND_PARAM, 0x08005000);
#endif
	nand_writel(info, NAND_OPNUM, 0x00);
	nand_writel(info, NAND_CS_RDY, 0x88442211);	/* 重映射rdy1/2/3信号到rdy0 rdy用于判断是否忙 */
}

/*
int ls1x_nand_detect(struct mtd_info *mtd)
{
	printf("NANDFlash info:\nerasesize\t%d B\nwritesize\t%d B\noobsize  \t%d B\n", mtd->erasesize, mtd->writesize, mtd->oobsize);
	return (mtd->erasesize != 1<<17 || mtd->writesize != 1<<11 || mtd->oobsize != 1<<6);
}*/

int ls1x_nand_init_buff(struct ls1x_nand_info *info)
{
	info->dma_desc = (unsigned int)(DMA_DESC | 0xa0000000);	/* DMA描述符地址 */
	if(info->dma_desc == NULL)
		return -1;
	info->dma_desc_phys = (unsigned int)(info->dma_desc) & 0x1fffffff;

	info->data_buff = (unsigned char *)(DATA_BUFF | 0xa0000000);	/* NAND的DMA数据缓存 */
	if(info->data_buff == NULL)
		return -1;
	info->data_buff_phys = (unsigned int)(info->data_buff) & 0x1fffffff;

	order_addr_in = ORDER_ADDR_IN;

	printf("data_buff_addr:0x%08x, dma_addr:0x%08x\n", info->data_buff, info->dma_desc);
	
	return 0;
}

extern unsigned int output_mode;

int ls1x_nand_init(void)
{
	struct ls1x_nand_info *info;
	struct nand_chip *chip;

	printf("\nNAND DETE\n");

	/* Allocate memory for MTD device structure and private data */
	ls1x_mtd = malloc(sizeof(struct mtd_info) + sizeof(struct ls1x_nand_info), M_DEVBUF, M_WAITOK);
	if (!ls1x_mtd) {
		printk("Unable to allocate fcr_soc NAND MTD device structure.\n");
		return -ENOMEM;
	}

	info = (struct ls1x_nand_info *)(&ls1x_mtd[1]);
	chip = (struct nand_chip *)(&ls1x_mtd[1]);
	memset(ls1x_mtd, 0, sizeof(struct mtd_info));
	memset(info, 0, sizeof(struct ls1x_nand_info));

	ls1x_mtd->priv = info;
//	chip->chip_delay = 15;	/* 15 us command delay time 从数据手册获知命令延迟时间 */

	chip->options		= NAND_CACHEPRG;
//	chip->ecc.mode		= NAND_ECC_NONE;
	chip->ecc.mode		= NAND_ECC_SOFT;
//	chip->controller	= &info->controller;
	chip->waitfunc		= ls1x_nand_waitfunc;
	chip->select_chip	= ls1x_nand_select_chip;
	chip->dev_ready		= ls1x_nand_dev_ready;
	chip->cmdfunc		= ls1x_nand_cmdfunc;
	chip->read_word		= ls1x_nand_read_word;
	chip->read_byte		= ls1x_nand_read_byte;
	chip->read_buf		= ls1x_nand_read_buf;
	chip->write_buf		= ls1x_nand_write_buf;
	chip->verify_buf	= ls1x_nand_verify_buf;

	info->mmio_base = LS1X_NAND_BASE;

	if (ls1x_nand_init_buff(info)) {
		printf("\n\nerror: PMON nandflash driver have some error!\n\n");
		return -ENXIO;
	}

	nand_gpio_init();
	chip->cmdfunc(ls1x_mtd, NAND_CMD_RESET, 0, 0);
	ls1x_nand_init_hw(info);

	/* Scan to find existence of the device */
	if (nand_scan(ls1x_mtd, 1)) {
		free(ls1x_mtd, M_DEVBUF);
		return -ENXIO;
	}

/*	if (ls1x_nand_detect(ls1x_mtd)) {
		printf("error: PMON driver don't support the NANDFlash!\n ");
		return -ENXIO;
	}*/

	/* Register the partitions */
	ls1x_mtd->name = "ls1x-nand";
#ifdef FAST_STARTUP
	if (output_mode == 0) {
		add_mtd_device(ls1x_mtd, 0, 0x400000, "kernel");
		return 0;
	}
#endif

#if defined(NAND_BOOT) && defined(LS1CSOC)
//	add_mtd_device(ls1x_mtd, 0, 512*1024, "pmon");
	add_mtd_device(ls1x_mtd, 512*1024, (14*1024-512)*1024, "kernel");
#else
	add_mtd_device(ls1x_mtd, 0, 14*1024*1024, "kernel");				//14MB
#endif
	add_mtd_device(ls1x_mtd, 14*1024*1024, 100*1024*1024, "os");		//100MB
	add_mtd_device(ls1x_mtd, (100+14)*1024*1024, 14*1024*1024, "data");	//14MB

	return 0;
}

