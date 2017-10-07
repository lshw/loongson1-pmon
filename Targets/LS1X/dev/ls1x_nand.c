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
//#include <sys/malloc.h>

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
	size_t			dma_desc_size;

	unsigned char	*data_buff;
	unsigned int	data_buff_phys;
	size_t			data_buff_size;

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
	return 0;
}

static void ls1x_nand_select_chip(struct mtd_info *mtd, int chip)
{
	return;
}

static int ls1x_nand_dev_ready(struct mtd_info *mtd)
{
	/* 多片flash的rdy信号如何判断？ */
	struct ls1x_nand_info *info = mtd->priv;
	unsigned int ret;
	ret = nand_readl(info, NAND_CMD) & 0x000f0000;
	if (ret) {
		return 1;
	}
	return 0;
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
		info->buf_start += 2;
	}
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
			printf("verify error..., i= %d !\n\n", i-1);
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

static int ls1x_nand_done(struct ls1x_nand_info *info)
{
	int ret, timeout = 40000;

	do {
		ret = nand_readl(info, NAND_CMD);
		timeout--;
//		printf("NAND_CMD=0x%2X\n", nand_readl(info, NAND_CMD));
	} while (((ret & 0x400) != 0x400) && timeout);

	return timeout;
}

static void inline ls1x_nand_start(struct ls1x_nand_info *info)
{
	nand_writel(info, NAND_CMD, nand_readl(info, NAND_CMD) | 0x1);
}

static void inline ls1x_nand_stop(struct ls1x_nand_info *info)
{
}

static void start_dma_nand(unsigned int flags, struct ls1x_nand_info *info)
{
	int timeout = 5000;
	int ret;

	writel(0, info->dma_desc + DMA_ORDERED);
	writel(info->data_buff_phys, info->dma_desc + DMA_SADDR);
	writel(DMA_ACCESS_ADDR, info->dma_desc + DMA_DADDR);
	writel((info->buf_count + 3) / 4, info->dma_desc + DMA_LENGTH);
	writel(0, info->dma_desc + DMA_STEP_LENGTH);
	writel(1, info->dma_desc + DMA_STEP_TIMES);

	if (flags) {
		writel(0x00003001, info->dma_desc + DMA_CMD);
	} else {
		writel(0x00000001, info->dma_desc + DMA_CMD);
	}

	ls1x_nand_start(info);	/* 使能nand命令 */
	writel((info->dma_desc_phys & ~0x1F) | 0x8, order_addr_in);	/* 启动DMA */
	while ((readl(order_addr_in) & 0x8)/* && (timeout-- > 0)*/) {
//		printf("%s. %x\n",__func__, readl(order_addr_in));
//		udelay(5);
	}

/* K9F5608U0D在读的时候 ls1x的nand flash控制器读取不到完成状态
   猜测是控制器对该型号flash兼容不好,目前解决办法是添加一段延时 */
#ifdef K9F5608U0D
	if (flags) {
		if (!ls1x_nand_done(info)) {
			printf("Wait time out!!!\n");
			ls1x_nand_stop(info);
		}
	} else {
		udelay(50);
	}
#else
	if (!ls1x_nand_done(info)) {
		printf("Wait time out!!!\n");
		ls1x_nand_stop(info);
	}
#endif

	while (timeout) {
		writel((info->dma_desc_phys & (~0x1F)) | 0x4, order_addr_in);
		do {
		} while (readl(order_addr_in) & 0x4);
		ret = readl(info->dma_desc + DMA_CMD);
//		if ((ret & 0x08) || flags) {
		if (ret & 0x08) {
			break;
		}
		timeout--;
	}
	if (!timeout) {
		printf("%s. %x\n",__func__, ret);
	}
}

static void ls1x_nand_cmdfunc(struct mtd_info *mtd, unsigned command, int column, int page_addr)
{
	struct ls1x_nand_info *info = mtd->priv;

	switch (command) {
	case NAND_CMD_READOOB:
		info->buf_count = mtd->oobsize;
		info->buf_start = column;
		nand_writel(info, NAND_CMD, SPARE | READ);
		nand_writel(info, NAND_ADDR_L, MAIN_SPARE_ADDRL(page_addr) + mtd->writesize);
		nand_writel(info, NAND_ADDR_H, MAIN_SPARE_ADDRH(page_addr));
		nand_writel(info, NAND_OPNUM, info->buf_count);
		nand_writel(info, NAND_PARAM, (nand_readl(info, NAND_PARAM) & 0xc000ffff) | (info->buf_count << 16));
		start_dma_nand(0, info);
		break;
	case NAND_CMD_READ0:
		info->buf_count = mtd->writesize + mtd->oobsize;
		info->buf_start = column;
		nand_writel(info, NAND_CMD, SPARE | MAIN | READ);
		nand_writel(info, NAND_ADDR_L, MAIN_SPARE_ADDRL(page_addr));
		nand_writel(info, NAND_ADDR_H, MAIN_SPARE_ADDRH(page_addr));
		nand_writel(info, NAND_OPNUM, info->buf_count);
		nand_writel(info, NAND_PARAM, (nand_readl(info, NAND_PARAM) & 0xc000ffff) | (info->buf_count << 16)); /* 1C注意 */
		start_dma_nand(0, info);
		break;
	case NAND_CMD_SEQIN:
		info->buf_count = mtd->writesize + mtd->oobsize - column;
		info->buf_start = 0;
		info->seqin_column = column;
		info->seqin_page_addr = page_addr;
		break;
	case NAND_CMD_PAGEPROG:
		if (info->seqin_column < mtd->writesize)
			nand_writel(info, NAND_CMD, SPARE | MAIN | WRITE);
		else
			nand_writel(info, NAND_CMD, SPARE | WRITE);
		nand_writel(info, NAND_ADDR_L, MAIN_SPARE_ADDRL(info->seqin_page_addr) + info->seqin_column);
		nand_writel(info, NAND_ADDR_H, MAIN_SPARE_ADDRH(info->seqin_page_addr));
		nand_writel(info, NAND_OPNUM, info->buf_count);
		nand_writel(info, NAND_PARAM, (nand_readl(info, NAND_PARAM) & 0xc000ffff) | (info->buf_count << 16)); /* 1C注意 */
		start_dma_nand(1, info);
		break;
	case NAND_CMD_RESET:
		info->buf_count = 0x0;
		info->buf_start = 0x0;
		nand_writel(info, NAND_CMD, RESET);
		ls1x_nand_start(info);
		if (!ls1x_nand_done(info)) {
			printf("Wait time out!!!\n");
			ls1x_nand_stop(info);
		}
		break;
	case NAND_CMD_ERASE1:
		info->buf_count = 0x0;
		info->buf_start = 0x0;
		nand_writel(info, NAND_ADDR_L, MAIN_ADDRL(page_addr));
		nand_writel(info, NAND_ADDR_H, MAIN_ADDRH(page_addr));
		nand_writel(info, NAND_OPNUM, 0x01);
		nand_writel(info, NAND_PARAM, (nand_readl(info, NAND_PARAM) & 0xc000ffff) | (0x1 << 16)); /* 1C注意 */
		nand_writel(info, NAND_CMD, ERASE);
		ls1x_nand_start(info);
		if (!ls1x_nand_done(info)) {
			printf("Wait time out!!!\n");
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
//	case NAND_CMD_READ1:
		info->buf_count = 0x0;
		info->buf_start = 0x0;
		break;
	default :
		info->buf_count = 0x0;
		info->buf_start = 0x0;
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

static int ls1x_nand_scan(struct mtd_info *mtd)
{
	struct ls1x_nand_info *info = mtd->priv;
	struct nand_chip *chip = (struct nand_chip *)info;
	uint64_t chipsize;
	int exit_nand_size;

	if (nand_scan_ident(mtd, 1))
		return -ENODEV;

	chipsize = (chip->chipsize << 3) >> 20;	/* Mbit */

	switch (mtd->writesize) {
	case 2048:
		switch (chipsize) {
		case 1024:
		#if defined(LS1ASOC)
			exit_nand_size = 0x1;
		#else
			exit_nand_size = 0x0;
		#endif
			break;
		case 2048:
			exit_nand_size = 0x1; break;
		case 4096:
			exit_nand_size = 0x2; break;
		case 8192:
			exit_nand_size = 0x3; break;
		default:
			exit_nand_size = 0x3; break;
		}
		break;
	case 4096:
		exit_nand_size = 0x4; break;
	case 8192:
		switch (chipsize) {
		case 32768:
			exit_nand_size = 0x5; break;
		case 65536:
			exit_nand_size = 0x6; break;
		case 131072:
			exit_nand_size = 0x7; break;
		default:
			exit_nand_size = 0x8; break;
		}
		break;
	case 512:
		switch (chipsize) {
		case 64:
			exit_nand_size = 0x9; break;
		case 128:
			exit_nand_size = 0xa; break;
		case 256:
			exit_nand_size = 0xb; break;
		case 512:
			exit_nand_size = 0xc;break;
		default:
			exit_nand_size = 0xd; break;
		}
		break;
	default:
		printf("exit nand size error!\n");
		return -ENODEV;
	}
	nand_writel(info, NAND_PARAM, (nand_readl(info, NAND_PARAM) & 0xfffff0ff) | (exit_nand_size << 8));
	chip->cmdfunc(mtd, NAND_CMD_RESET, 0, 0);

	return nand_scan_tail(mtd);
}

#define ALIGN(x,a)		__ALIGN_MASK((x),(typeof(x))(a)-1)
#define __ALIGN_MASK(x,mask)	(((x)+(mask))&~(mask))

int ls1x_nand_init_buff(struct ls1x_nand_info *info)
{
	/* DMA描述符地址 */
	info->dma_desc_size = ALIGN(DMA_DESC_NUM, PAGE_SIZE);	/* 申请内存大小，页对齐 */
//	info->dma_desc = (unsigned int)(DMA_DESC | 0xa0000000);
	info->dma_desc = ((unsigned int)malloc(info->dma_desc_size) & 0x0fffffff) | 0xa0000000;
	info->dma_desc = (unsigned int)ALIGN((unsigned int)info->dma_desc, 32);	/* 地址32字节对齐 */
	if(info->dma_desc == NULL)
		return -1;
	info->dma_desc_phys = (unsigned int)(info->dma_desc) & 0x1fffffff;

	/* NAND的DMA数据缓存 */
	info->data_buff_size = ALIGN(MAX_BUFF_SIZE, PAGE_SIZE);	/* 申请内存大小，页对齐 */
//	info->data_buff = (unsigned char *)(DATA_BUFF | 0xa0000000);
	info->data_buff = (unsigned char *)(((unsigned int)malloc(info->data_buff_size) & 0x0fffffff) | 0xa0000000);
	info->data_buff = (unsigned char *)ALIGN((unsigned int)info->data_buff, 32);	/* 地址32字节对齐 */
	if(info->data_buff == NULL)
		return -1;
	info->data_buff_phys = (unsigned int)(info->data_buff) & 0x1fffffff;

	order_addr_in = ORDER_ADDR_IN;

	//printf("data_buff_addr:0x%08x, dma_addr:0x%08x\n", info->data_buff, info->dma_desc);
	
	return 0;
}

extern unsigned int output_mode;

int ls1x_nand_init(void)
{
	struct ls1x_nand_info *info;
	struct nand_chip *chip;

	printf("\nNAND detect\n");

	/* Allocate memory for MTD device structure and private data */
	ls1x_mtd = malloc(sizeof(struct mtd_info) + sizeof(struct ls1x_nand_info));
	if (!ls1x_mtd) {
		printf("Unable to allocate NAND MTD device structure.\n");
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
//	chip->cmdfunc(ls1x_mtd, NAND_CMD_RESET, 0, 0);
	ls1x_nand_init_hw(info);

	/* Scan to find existence of the device */
	if (ls1x_nand_scan(ls1x_mtd)) {
		free(ls1x_mtd);
		return -ENXIO;
	}

	/* Register the partitions */
	ls1x_mtd->name = "ls1x-nand";

//如果设置文件中定义MTDPARTS ， mtd分区设置将从env mtdparts里获取
#ifndef MTDPARTS
	add_mtd_device(ls1x_mtd, 2*1024*1024, 20*1024*1024, "kernel");
	add_mtd_device(ls1x_mtd, 22*1024*1024, 106*1024*1024, "rootfs");
	add_mtd_device(ls1x_mtd, 0, 2*1024*1024, "pmon(nand)");
#endif
	return 0;
}

/******************************************************************************/
/* 用于nand flash 环境变量读写的函数 */
//#if defined(NAND_BOOT_EN) && defined(LS1CSOC)
int nand_probe_boot(void)
{
	return 0;
}

int nand_erase_boot(void *base, int size)
{
	int page_addr;
	int page_per_block = ls1x_mtd->erasesize / ls1x_mtd->writesize;
	int erase_num = 0;

	page_addr = (unsigned int)base / ls1x_mtd->writesize;

	erase_num = size / ls1x_mtd->erasesize;
	if (size % ls1x_mtd->erasesize != 0)
	    erase_num++;

	while (erase_num > 0) {
		ls1x_nand_cmdfunc(ls1x_mtd, NAND_CMD_ERASE1, 0, page_addr);
		erase_num--;
		page_addr += page_per_block;
	}

	return 0;
}

int nand_write_boot(void *base, void *buffer, int size)
{
	struct ls1x_nand_info *info = ls1x_mtd->priv;
	unsigned char *data_buf = (unsigned char *)buffer;
	unsigned int page_addr = (unsigned int)base / ls1x_mtd->writesize;
	unsigned int buffer_size = size;

	while (buffer_size > ls1x_mtd->writesize) {
		ls1x_nand_cmdfunc(ls1x_mtd, NAND_CMD_SEQIN, 0x00, page_addr);
		memcpy(info->data_buff, data_buf, ls1x_mtd->writesize);
		ls1x_nand_cmdfunc(ls1x_mtd, NAND_CMD_PAGEPROG, 0, -1);

		buffer_size -= ls1x_mtd->writesize;
		data_buf += ls1x_mtd->writesize;
		page_addr += 1;
	}
	if (buffer_size) {
		ls1x_nand_cmdfunc(ls1x_mtd, NAND_CMD_SEQIN, 0x00, page_addr);
		memcpy(info->data_buff, data_buf, buffer_size);
		ls1x_nand_cmdfunc(ls1x_mtd, NAND_CMD_PAGEPROG, 0, -1);
	}
	return 0;
}

int nand_read_boot(void *base, void *buffer, int size)
{
	struct ls1x_nand_info *info = ls1x_mtd->priv;
	unsigned char *data_buf = (unsigned char *)buffer;
	unsigned int page_addr = (unsigned int)base / ls1x_mtd->writesize;
	unsigned int buffer_size = size;

	while (buffer_size > ls1x_mtd->writesize) {
		ls1x_nand_cmdfunc(ls1x_mtd, NAND_CMD_READ0, 0x00, page_addr);
		memcpy(data_buf, info->data_buff, ls1x_mtd->writesize);

		buffer_size -= ls1x_mtd->writesize;
		data_buf += ls1x_mtd->writesize;
		page_addr += 1;
	}
	if (buffer_size) {
		ls1x_nand_cmdfunc(ls1x_mtd, NAND_CMD_READ0, 0x00, page_addr);
		memcpy(data_buf, info->data_buff, buffer_size);
	}

	return 0;
}

int nand_verify_boot(void *base, void *buffer, int size)
{
	unsigned char *r_buf = NULL;
	int i = 0;

	r_buf = malloc(size);
	if (!r_buf) {
		printf("Unable to allocate NAND MTD device structure.\n");
		return -ENOMEM;
	}

	nand_read_boot(base, r_buf, size);

	while (size--) {
		if (*((unsigned char *)buffer + i) != r_buf[i]) {
			printf("verify error..., i= %d !\n\n", i-1);
			free(r_buf);
			return -1;
		}
		i++;
	}

	free(r_buf);
	
	return 0;
}

/* 只检测nand flash 0地址第0页 */
int ls1x_nand_test(void)
{
	unsigned char *buffer = NULL;
	int *nand_addr = 0;
	int i, ret = 0;

	if (nand_scan_ident(ls1x_mtd, 1)) {
		return -ENODEV;
	}

	buffer = malloc(ls1x_mtd->writesize);
	if (!buffer) {
		printf("Unable to allocate NAND MTD device structure.\n");
		return -ENOMEM;
	}

	for (i=0; i<ls1x_mtd->writesize; i++) {
		*(buffer + i) = i;
	}

	nand_erase_boot(nand_addr, ls1x_mtd->writesize);
	nand_write_boot(nand_addr, buffer, ls1x_mtd->writesize);
	ret = nand_verify_boot(nand_addr, buffer, ls1x_mtd->writesize);

	free(buffer);

	return ret;
}
//#endif
