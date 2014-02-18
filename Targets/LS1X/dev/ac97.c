/* ac97 driver for loongson1 (alc203 alc655) */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <sys/malloc.h>
#include <pmon.h>
#include <machine/pio.h>

#include "include/ac97_codec.h"

#define RX_DMA_DESC_SIZE	64
#define TX_DMA_DESC_SIZE	64
#define RX_BUFF_SIZE		0x00040000	/* 256KB */
#define TX_BUFF_SIZE		0x00040000	/* 256KB */

#define RX_DMA_DESC		0x81400000
#define TX_DMA_DESC		0x81400080
#define RX_DATA_BUFF	0x81400100
#define TX_DATA_BUFF	(RX_DATA_BUFF + RX_BUFF_SIZE + 64)

//#define DMA_TX_ADDR		0xdfe72420	/* DMA发送操作的地址 */
//#define DMA_RX_ADDR		0x5fe74c4c	/* DMA接收操作的地址 */
#define DMA_TX_ADDR		0x8fe72420	/* DMA发送操作的地址 */
#define DMA_RX_ADDR		0x0fe74c4c	/* DMA接收操作的地址 */
#define ORDER_ADDR_IN	0xbfd01160	/* DMA配置寄存器 */
static unsigned int order_addr_in;
#define DMA_DESC_NUM	28	/* DMA描述符占用的字节数 7x4 */
/* DMA描述符 */
#define DMA_ORDERED		0x00
#define DMA_SADDR		0x04
#define DMA_DADDR		0x08
#define DMA_LENGTH		0x0c
#define DMA_STEP_LENGTH		0x10
#define DMA_STEP_TIMES		0x14
#define	DMA_CMD			0x18

        
#define DMA_BUF		0xa3b00000

#define LS1X_AC97_BASE	0xbfe74000
#define LS1X_AC97_REG(x)	(LS1X_AC97_BASE + (x))

/* Control Status Register (CSR) */
#define LS1X_AC97_CSR		LS1X_AC97_REG(0x00)
#define AC97_CSR_RESUME		(1 << 1)
#define AC97_CSR_RST_FORCE	(1 << 0)
/* Output Channel Configuration Registers (OCCn) */
#define LS1X_AC97_OCC0		LS1X_AC97_REG(0x04)
#define LS1X_AC97_OCC1		LS1X_AC97_REG(0x08)
#define LS1X_AC97_OCC2		LS1X_AC97_REG(0x0c)
#define OCH1_CFG_R_OFFSET	(8)
#define OCH0_CFG_L_OFFSET	(0)
#define OCH1_CFG_R_MASK		(0xFF)
#define OCH0_CFG_L_MASK		(0xFF)

/* Input Channel Configuration (ICC) */
#define LS1X_AC97_ICC		LS1X_AC97_REG(0x10)
#define ICH2_CFG_MIC_OFFSET	(16)
#define ICH1_CFG_R_OFFSET	(8)
#define ICH0_CFG_L_OFFSET	(0)
#define ICH2_CFG_MIC_MASK	(0xFF)
#define ICH0_CFG_R_MASK		(0xFF)
#define ICH1_CFG_L_MASK		(0xFF)

/* Channel Configurations (Sub-field) */
#define DMA_EN	(1 << 6)
#define FIFO_THRES_OFFSET	(4)
#define FIFO_THRES_MASK		(0x3)
#define SS_OFFSET		(2)	/* Sample Size 00:8bit 10:16bit */
#define SS_MASK		(0x3)
#define SR		(1 << 1)	/* Sample Rate 1:Variable 0:Fixed */
#define CH_EN	(1 << 0)


#define LS1X_AC97_CODECID	LS1X_AC97_REG(0x14)

/* Codec Register Access Command (CRAC) */
#define LS1X_AC97_CRAC		LS1X_AC97_REG(0x18)
#define CODEC_WR			(1 << 31)
#define CODEC_ADR_OFFSET	(16)
#define CODEC_DAT_OFFSET	(0)
#define CODEC_ADR_MASK		(0xFF)
#define CODEC_DAT_MASK		(0xFF)

/* OCHn and ICHn Registers
   OCHn are the output fifos (data that will be send to the codec), ICHn are the 
   input fifos (data received from the codec).
 */
#define LS1X_AC97_OCH0		LS1X_AC97_REG(0x20)
#define LS1X_AC97_OCH1		LS1X_AC97_REG(0x24)
#define LS1X_AC97_OCH2		LS1X_AC97_REG(0x28)
#define LS1X_AC97_OCH3		LS1X_AC97_REG(0x2c)
#define LS1X_AC97_OCH4		LS1X_AC97_REG(0x30)
#define LS1X_AC97_OCH5		LS1X_AC97_REG(0x34)
#define LS1X_AC97_OCH6		LS1X_AC97_REG(0x38)
#define LS1X_AC97_OCH7		LS1X_AC97_REG(0x3c)
#define LS1X_AC97_OCH8		LS1X_AC97_REG(0x40)
#define LS1X_AC97_ICH0		LS1X_AC97_REG(0x44)
#define LS1X_AC97_ICH1		LS1X_AC97_REG(0x48)
#define LS1X_AC97_ICH2		LS1X_AC97_REG(0x4c)

/* Interrupt Status Register (INTS) */
#define LS1X_AC97_INTRAW	LS1X_AC97_REG(0x54)
#define ICH_FULL		(1 << 31)
#define ICH_TH_INT		(1 << 30)
#define OCH1_FULL		(1 << 7)
#define OCH1_EMPTY		(1 << 6)
#define OCH1_TH_INT		(1 << 5)
#define OCH0_FULL		(1 << 4)
#define OCH0_EMPTY		(1 << 3)
#define OCH0_TH_INT		(1 << 2)
#define CW_DONE		(1 << 1)
#define CR_DONE		(1 << 0)

#define LS1X_AC97_INTM		LS1X_AC97_REG(0x58)

/* 中断状态/清除寄存器 
   屏蔽后的中断状态寄存器，对本寄存器的读操作将清除寄存器0x54中的所有中断状态 */
#define LS1X_AC97_INT_CLR	LS1X_AC97_REG(0x5c)
/* OCH中断清除寄存器
   对本寄存器的读操作将清除寄存器0x54中的所有output channel的中断状态对应的 bit[7:2] */
#define LS1X_AC97_INT_OCCLR	LS1X_AC97_REG(0x60)
/* ICH中断清除寄存器 
   对本寄存器的读操作将清除寄存器0x54中的所有input channel的中断状态对应的 bit[31:30] */
#define LS1X_AC97_INT_ICCLR	LS1X_AC97_REG(0x64)
/* CODEC WRITE 中断清除寄存器 
   对本寄存器的读操作将清除寄存器0x54中的中bit[1] */
#define LS1X_AC97_INT_CWCLR	LS1X_AC97_REG(0x68)
/* CODEC READ 中断清除寄存器
   对本寄存器的读操作将清除寄存器0x54中的中bit[0] */
#define LS1X_AC97_INT_CRCLR	LS1X_AC97_REG(0x6c)

#define ALC655 0x414c4760
#define ALC203 0x414c4770

struct ls1x_ac97_info {
	/* AC97 registers*/
	unsigned int	mmio_base;

	unsigned int	rx_dma_desc;
	unsigned int	rx_dma_desc_phys;
	unsigned int	tx_dma_desc;
	unsigned int	tx_dma_desc_phys;

	unsigned char	*rx_data_buff;
	unsigned int	rx_data_buff_phys;
	unsigned char	*tx_data_buff;
	unsigned int	tx_data_buff_phys;
};

static unsigned short sample_rate = 0xac44;
static int codec_reset = 1;
static int ls1x_ac97_reset = 1;

static unsigned short ls1x_ac97_read(unsigned short reg)
{
	int i = 1000;
	u32 data = 0;
	
	data |= CODEC_WR;
	data |= ((u32)reg << CODEC_ADR_OFFSET);
	writel(data, LS1X_AC97_CRAC);

	/* now wait for the data */
	while (i-- > 0) {
		if ((readl(LS1X_AC97_INTRAW) & CR_DONE) != 0)
			break;
		delay(500);
	}
	if (i > 0) {
		readl(LS1X_AC97_INT_CRCLR);
		return readl(LS1X_AC97_CRAC) & 0xffff;
	}
	printf("AC97 command read timeout\n");
	return 0;
}

static void ls1x_ac97_write(unsigned short reg, unsigned short val)
{
	int i = 1000;
	u32 data = 0;
	
	data &= ~(CODEC_WR);
	data |= ((u32)reg << CODEC_ADR_OFFSET) | ((u32)val << CODEC_DAT_OFFSET);
	writel(data, LS1X_AC97_CRAC);

	while (i-- > 0) {
		if ((readl(LS1X_AC97_INTRAW) & CW_DONE) != 0)
			break;
		delay(500);
	}
	if (i > 0) {
		readl(LS1X_AC97_INT_CWCLR);
	}
}

static void codec_init(void)
{
	int codec_id;
	u32 x, conf;

	/* codec reset */
	if (codec_reset) {
		ls1x_ac97_write(AC97_RESET, 0x0000);
		delay(500000);
		codec_reset = 0;
	}

	codec_id = ls1x_ac97_read(AC97_VENDOR_ID1);
	codec_id = (codec_id << 16) | ls1x_ac97_read(AC97_VENDOR_ID2);
#ifdef CONFIG_CHINESE
	printf("编解码器ID： %x \n", codec_id);
#else
	printf("codec ID :%x \n", codec_id);
#endif

	if (codec_id == ALC655) {
		/* 输出通道配置寄存器 */
		x = readl(LS1X_AC97_OCC0);
		conf = x & ~(FIFO_THRES_MASK << FIFO_THRES_OFFSET) & ~(SS_MASK << SS_OFFSET);
		conf = (conf | (3 << FIFO_THRES_OFFSET) | (0 << SS_OFFSET) | DMA_EN | CH_EN) & (~SR);
		conf |= (conf << OCH1_CFG_R_OFFSET) | (conf << OCH0_CFG_L_OFFSET);
		writel(conf, LS1X_AC97_OCC0);

		/* 输入通道配置寄存器 */
		x = readl(LS1X_AC97_ICC);
		conf = x & ~(FIFO_THRES_MASK << FIFO_THRES_OFFSET) & ~(SS_MASK << SS_OFFSET);
		conf = (conf | (3 << FIFO_THRES_OFFSET) | (0 << SS_OFFSET) | DMA_EN | CH_EN) & (~SR);
		conf |= (conf << ICH2_CFG_MIC_OFFSET) | (conf << ICH1_CFG_R_OFFSET) | (conf << ICH0_CFG_L_OFFSET);
		writel(conf, LS1X_AC97_ICC);

		ls1x_ac97_write(AC97_ALC650_MULTICH, 0x201);
	}
	else if (codec_id == ALC203) {
		/* 输出通道配置寄存器 */
		x = readl(LS1X_AC97_OCC0);
		conf = x & ~(FIFO_THRES_MASK << FIFO_THRES_OFFSET) & ~(SS_MASK << SS_OFFSET);
		conf |= (2 << FIFO_THRES_OFFSET) | (2 << SS_OFFSET) | DMA_EN | CH_EN | SR;
		conf |= (conf << OCH1_CFG_R_OFFSET) | (conf << OCH0_CFG_L_OFFSET);
		writel(conf, LS1X_AC97_OCC0);

		/* 输入通道配置寄存器 */
		x = readl(LS1X_AC97_ICC);
		conf = x & ~(FIFO_THRES_MASK << FIFO_THRES_OFFSET) & ~(SS_MASK << SS_OFFSET);
		conf |= (2 << FIFO_THRES_OFFSET) | (2 << SS_OFFSET) | DMA_EN | CH_EN | SR;
		conf |= (conf << ICH2_CFG_MIC_OFFSET) | (conf << ICH1_CFG_R_OFFSET) | (conf << ICH0_CFG_L_OFFSET);
		writel(conf, LS1X_AC97_ICC);
		
		ls1x_ac97_write(AC97_PCM_FRONT_DAC_RATE, sample_rate);
		ls1x_ac97_write(AC97_PCM_SURR_DAC_RATE, sample_rate);
		ls1x_ac97_write(AC97_PCM_LFE_DAC_RATE, sample_rate);
		ls1x_ac97_write(AC97_PCM_LR_ADC_RATE, sample_rate);
		ls1x_ac97_write(AC97_PCM_MIC_ADC_RATE, sample_rate);
	}

	ls1x_ac97_write(AC97_POWERDOWN, 0x0000);

	/* 设置音量 */
	ls1x_ac97_write(AC97_MASTER, 0x0808);
	ls1x_ac97_write(AC97_HEADPHONE, 0x0808);
	ls1x_ac97_write(AC97_MASTER_MONO, 0x0008);
//	ls1x_ac97_write(AC97_MASTER_TONE, 0x0808);
//	ls1x_ac97_write(AC97_PC_BEEP, 0x0808);
	ls1x_ac97_write(AC97_PHONE, 0x0004);
	ls1x_ac97_write(AC97_MIC, 0x035f);
	ls1x_ac97_write(AC97_LINE, 0x0808);
//	ls1x_ac97_write(AC97_CD, 0x0808);
//	ls1x_ac97_write(AC97_VIDEO, 0x0808);
	ls1x_ac97_write(AC97_AUX, 0x0808);
	ls1x_ac97_write(AC97_PCM, 0x0808);
//	ls1x_ac97_write(AC97_CENTER_LFE_MASTER, 0x0808);
//	ls1x_ac97_write(AC97_SURROUND_MASTER, 0x0808);

	/* record设置录音寄存器 */
	ls1x_ac97_write(AC97_REC_SEL, 0x0000);	/* MIC */
//	ls1x_ac97_write(AC97_REC_SEL, 0x0404);	/* Linein */
//	ls1x_ac97_write(AC97_REC_SEL, 0x0303);	/* AUX */
	ls1x_ac97_write(AC97_REC_GAIN, 0x0f0f);
	ls1x_ac97_write(AC97_REC_GAIN_MIC, 0x0f0f);
	ls1x_ac97_write(AC97_GENERAL_PURPOSE, (1<<13)|(1<<10));
}

static int ac97_config(void)
{
#ifdef CONFIG_CHINESE
	printf("配置 ac97 编解码器\n");
#else
	printf("config ac97 codec\n");
#endif
	
	/* AC97 codec冷启动 */
	if (ls1x_ac97_reset) {
		writel(AC97_CSR_RST_FORCE, LS1X_AC97_CSR);
		writel(AC97_CSR_RESUME, LS1X_AC97_CSR);
		delay(1000);
		writel(AC97_CSR_RST_FORCE, LS1X_AC97_CSR);
		delay(500000);
		ls1x_ac97_reset = 0;
	}

	/* disables the generation of an interrupt */
	writel(0, LS1X_AC97_INTM);
//	writel(0xffffffff, LS1X_AC97_INTM);
	readl(LS1X_AC97_INT_CLR);
	readl(LS1X_AC97_INT_OCCLR);
	readl(LS1X_AC97_INT_ICCLR);
	readl(LS1X_AC97_INT_CWCLR);
	readl(LS1X_AC97_INT_CRCLR);

	codec_init();

#ifdef CONFIG_CHINESE
	printf("配置完毕\n");
#else
	printf("config done\n");
#endif
	return 0;
}

static int ls1x_ac97_init_buff(struct ls1x_ac97_info *info)
{
	/* dma描述符 */
	info->rx_dma_desc = (unsigned int)(RX_DMA_DESC | 0xa0000000);	/* DMA描述符地址 */
	/* PMON中使用该malloc(xx,xx)函数需要#include <sys/malloc.h> */
//	info->rx_dma_desc = (unsigned int)(malloc(RX_DMA_DESC_SIZE, M_DEVBUF, M_WAITOK)) | 0xa0000000;
	if(info->rx_dma_desc == NULL)
		return -1;
	info->rx_dma_desc_phys = (unsigned int)(info->rx_dma_desc) & 0x1fffffff;

	info->tx_dma_desc = (unsigned int)(TX_DMA_DESC | 0xa0000000);	/* DMA描述符地址 */
//	info->tx_dma_desc = (unsigned int)(malloc(TX_DMA_DESC_SIZE, M_DEVBUF, M_WAITOK)) | 0xa0000000;
	if(info->tx_dma_desc == NULL)
		return -1;
	info->tx_dma_desc_phys = (unsigned int)(info->tx_dma_desc) & 0x1fffffff;

	/* 数据缓存 */
	info->rx_data_buff = (unsigned char *)(RX_DATA_BUFF | 0xa0000000);
	if(info->rx_data_buff == NULL)
		return -1;
	info->rx_data_buff_phys = (unsigned int)(info->rx_data_buff) & 0x1fffffff;

	info->tx_data_buff = (unsigned char *)(TX_DATA_BUFF | 0xa0000000);
	if(info->tx_data_buff == NULL)
		return -1;
	info->tx_data_buff_phys = (unsigned int)(info->tx_data_buff) & 0x1fffffff;

	/*  */
	info->mmio_base = LS1X_AC97_BASE;
	order_addr_in = ORDER_ADDR_IN;

	return 0;
}

static void ls1x_dma_init(struct ls1x_ac97_info *info)
{
	writel(0, info->rx_dma_desc + DMA_ORDERED);
	writel(info->rx_data_buff_phys, info->rx_dma_desc + DMA_SADDR);
	writel(DMA_RX_ADDR, info->rx_dma_desc + DMA_DADDR);
//	writel((info->buf_count + 3) / 4, info->rx_dma_desc + DMA_LENGTH);
	writel(0, info->rx_dma_desc + DMA_STEP_LENGTH);
	writel(1, info->rx_dma_desc + DMA_STEP_TIMES);
	writel(0, info->rx_dma_desc + DMA_CMD);

	writel(0, info->tx_dma_desc + DMA_ORDERED);
	writel(info->tx_data_buff_phys, info->tx_dma_desc + DMA_SADDR);
	writel(DMA_TX_ADDR, info->tx_dma_desc + DMA_DADDR);
//	writel((info->buf_count + 3) / 4, info->tx_dma_desc + DMA_LENGTH);
	writel(0, info->tx_dma_desc + DMA_STEP_LENGTH);
	writel(1, info->tx_dma_desc + DMA_STEP_TIMES);
	writel(0, info->tx_dma_desc + DMA_CMD);
}

static void start_dma_rx(struct ls1x_ac97_info *info)
{
	int timeout = 30000;
	int ret = 0;

//	writel(0, info->rx_dma_desc + DMA_ORDERED);
//	writel(info->rx_data_buff_phys, info->rx_dma_desc + DMA_SADDR);
//	writel(DMA_RX_ADDR, info->rx_dma_desc + DMA_DADDR);
	writel((RX_BUFF_SIZE + 3) / 4, info->rx_dma_desc + DMA_LENGTH);
//	writel(0, info->rx_dma_desc + DMA_STEP_LENGTH);
//	writel(1, info->rx_dma_desc + DMA_STEP_TIMES);
//	writel(0, info->rx_dma_desc + DMA_CMD);

//	writel(0x00000000, info->rx_dma_desc + DMA_CMD);
	writel(0x00000001, info->rx_dma_desc + DMA_CMD);	/* 关中断方式 */

	writel((info->rx_dma_desc_phys & (~0x1F)) | 0xa, order_addr_in);	/* 启动DMA */
	while ((readl(order_addr_in) & 0x8) && (timeout-- > 0)) {
//		printf("%s. %x\n",__func__, readl(order_addr_in));
//		udelay(5);
	}

	while (1) {
		writel((info->rx_dma_desc_phys & (~0x1F)) | 0x6, order_addr_in);
		do {
		} while (readl(order_addr_in) & 0x4);
//		printf("%s. %x\n",__func__, readl(info->rx_dma_desc + DMA_CMD));
		ret = readl(info->rx_dma_desc + DMA_CMD);
		if (ret & 0x08) {
			break;
		}
//		udelay(5);
	}
	writel((info->rx_dma_desc_phys & (~0x1F)) | 0x12, order_addr_in);	/* 结束DMA */
	printf("%s. %x\n",__func__, readl(info->rx_dma_desc + DMA_CMD));
}

static void start_dma_rxs(struct ls1x_ac97_info *info, unsigned int saddr, unsigned int length)
{
	int timeout = 30000;
	int ret = 0;

	writel(saddr, info->rx_dma_desc + DMA_SADDR);
	writel(length, info->rx_dma_desc + DMA_LENGTH);

//	writel(0x00000000, info->rx_dma_desc + DMA_CMD);
	writel(0x00000001, info->rx_dma_desc + DMA_CMD);	/* 关中断方式 */

	writel((info->rx_dma_desc_phys & (~0x1F)) | 0xa, order_addr_in);	/* 启动DMA */
	do {
	} while ((readl(order_addr_in) & 0x8) && (timeout-- > 0));

	do {
		writel((info->rx_dma_desc_phys & (~0x1F)) | 0x6, order_addr_in);
		do {
		} while (readl(order_addr_in) & 0x4);
//		printf("%s. %x\n",__func__, readl(info->rx_dma_desc + DMA_CMD));
		ret = readl(info->rx_dma_desc + DMA_CMD);
//		udelay(5);
	} while(!(ret & 0x08));

	writel((info->rx_dma_desc_phys & (~0x1F)) | 0x12, order_addr_in);	/* 结束DMA */
//	printf("%s. %x\n",__func__, readl(info->rx_dma_desc + DMA_CMD));
}

static void start_dma_tx(struct ls1x_ac97_info *info)
{
	int timeout = 30000;
	int ret = 0;

//	writel(0, info->tx_dma_desc + DMA_ORDERED);
//	writel(info->tx_data_buff_phys, info->tx_dma_desc + DMA_SADDR);
//	writel(DMA_TX_ADDR, info->tx_dma_desc + DMA_DADDR);
	writel((TX_BUFF_SIZE + 3) / 4, info->tx_dma_desc + DMA_LENGTH);
//	writel(0, info->tx_dma_desc + DMA_STEP_LENGTH);
//	writel(1, info->tx_dma_desc + DMA_STEP_TIMES);
//	writel(0, info->tx_dma_desc + DMA_CMD);

//	writel(0x00001000, info->tx_dma_desc + DMA_CMD);
	writel(0x00003001, info->tx_dma_desc + DMA_CMD);	/* 关中断方式 */

	writel((info->tx_dma_desc_phys & (~0x1F)) | 0x9, order_addr_in);	/* 启动DMA */
	while ((readl(order_addr_in) & 0x8) && (timeout-- > 0)) {
//		printf("%s. %x\n",__func__, readl(order_addr_in));
//		udelay(5);
	}

	while (1) {
//		writel((info->tx_dma_desc_phys & (~0x1F)) | 0x5, order_addr_in);
		writel((info->tx_dma_desc_phys & (~0x1F)) | 0x5, order_addr_in);	/* 避免读取的值不正确 */
		do {
		} while (readl(order_addr_in) & 0x4);
		delay(3);
		ret = readl(info->tx_dma_desc + DMA_CMD);
//		printf("%s. %x\n",__func__, ret);
		if ((ret & 0x08) || (!(ret & 0xf0))) {
			break;
		}
	}
	writel((info->tx_dma_desc_phys & (~0x1F)) | 0x11, order_addr_in);	/* 结束DMA */
	printf("%s. %x\n",__func__, readl(info->tx_dma_desc + DMA_CMD));
}

static void start_dma_txs(struct ls1x_ac97_info *info, unsigned int saddr, unsigned int length)
{
	int timeout = 30000;
	int ret = 0;

	writel(saddr, info->tx_dma_desc + DMA_SADDR);
	writel(length, info->tx_dma_desc + DMA_LENGTH);

//	writel(0x00001000, info->tx_dma_desc + DMA_CMD);
	writel(0x00003001, info->tx_dma_desc + DMA_CMD);	/* 关中断方式 */

	writel((info->tx_dma_desc_phys & (~0x1F)) | 0x9, order_addr_in);	/* 启动DMA */
	do {
	} while ((readl(order_addr_in) & 0x8) && (timeout-- > 0));

	do {
		writel((info->tx_dma_desc_phys & (~0x1F)) | 0x5, order_addr_in);
		do {
		} while (readl(order_addr_in) & 0x4);
//		writel((info->tx_dma_desc_phys & (~0x1F)) | 0x5, order_addr_in);	/* 避免读取的值不正确 */
//		delay(3);
		ret = readl(info->tx_dma_desc + DMA_CMD);
		delay(300);
//		printf("%s. %x\n",__func__, ret);
		if ((ret & 0x08) || (!(ret & 0xf0))) {
			break;
		}
	} while (1);
//	printf("%s. %x\n",__func__, ret);
//	printf("%s. %x\n",__func__, readl(info->tx_dma_desc + DMA_CMD));
	writel((info->tx_dma_desc_phys & (~0x1F)) | 0x11, order_addr_in);	/* 结束DMA */
}

/* 测试录音放音 */
int ac97_test(int argc, char **argv)
{
	struct ls1x_ac97_info *info;
	unsigned int i, j;
	unsigned int *rx_buff;
	unsigned int *tx_buff;
	char cmdbuf[100];

	if ((argc!=2) && (argc!=1))
		return -1;
	if (argc == 2) {
		sprintf(cmdbuf, "load -o 0x%x -r %s", DMA_BUF, argv[1]);
		do_cmd(cmdbuf);
	}

	info = malloc(sizeof(struct ls1x_ac97_info), M_DEVBUF, M_WAITOK);
	if (!info) {
		printf("Unable to allocate ac97 device structure.\n");
		return -1;
	}

	if (ls1x_ac97_init_buff(info)) {
		printf("error: init buff have some error!\n");
		return -1;
	}

	ls1x_dma_init(info);
	ac97_config();

#ifdef CONFIG_CHINESE
	printf("录音中\n");
#else
	printf("Recording\n");
#endif

	/* 需要配置DMA */
	start_dma_rx(info);
/*	for (i = 0; i < RX_BUFF_SIZE; i += 512) {
		start_dma_rxs(info, info->rx_data_buff_phys + i, 128);
	}*/

#ifdef CONFIG_CHINESE
	printf("放音中：注意听是否跟所录声音一致\n");
#else
	printf("Playback\n");
#endif

	rx_buff = (unsigned int *)RX_DATA_BUFF;
	tx_buff = (unsigned int *)TX_DATA_BUFF;

/*	for (i=0; i<(RX_BUFF_SIZE/2); i++) {
		j = 0x0000ffff & (unsigned int)rx_buff[i];
		tx_buff[i] = (j<<16) | j;
	}*/

	for (i=0; i<(RX_BUFF_SIZE); i++) {
//		j = 0x0000ffff & *(rx_buff + i);
		*(tx_buff + i) = *(rx_buff + i);
	}

//	start_dma_tx(info);
//	start_dma_txs(info, info->tx_data_buff_phys + 0, TX_BUFF_SIZE/4);
	for (i = 0; i < TX_BUFF_SIZE; i += 2048) {
		start_dma_txs(info, info->tx_data_buff_phys + i, 512);
	}

#ifdef CONFIG_CHINESE
	printf("放音结束\n");
#else
	printf("test done\n");
#endif

//	free((unsigned int *)info->rx_dma_desc, M_DEVBUF);
//	free((unsigned int *)info->tx_dma_desc, M_DEVBUF);
	free(info, M_DEVBUF);

	return 0;
}

#if 0
static const Cmd Cmds[] = {
	{"MyCmds"},
	{"ac97_test", "file", 0, "ac97_test file", ac97_test, 0, 99, CMD_REPEAT},
	{0, 0}
};

static void init_cmd __P((void)) __attribute__ ((constructor));

static void init_cmd(void)
{
	cmdlist_expand(Cmds, 1);
}
#endif
