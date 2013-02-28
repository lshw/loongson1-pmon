/* ac97 driver for loongson1 (alc203 alc655) */

#include <pmon.h>
#include <cpu.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <linux/types.h>
#include "include/ac97_codec.h"

#define CONFREG_BASE	0xbfd00000

#define DMA_BASE	0xbfe74080

#define dma_reg_write(addr, val) do{ *(volatile u32 *)(DMA_BASE+(addr))=val; }while(0)
//#define dma_reg_read(addr) *(volatile u32 * )(DMA_BASE+(addr))
#define dma_reg_read(addr) *(volatile u32 *)(addr)
        
#define DMA_BUF		0xa3b00000
#define BUF_SIZE	0x00200000	/* 2MB */
#define REC_DMA_BUF		(DMA_BUF + BUF_SIZE)
#define REC_BUF_SIZE	BUF_SIZE
#define SYNC_W		1    /* sync cache for writing data */
#define CPU2FIFO	1

#define AC97_RECORD	0
#define AC97_PLAY	1

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

static unsigned short sample_rate = 0xac44;
static int ac97_rw = 0;
static int codec_reset = 1;
static int ls1x_ac97_reset = 1;

/* DMA描述符 */
struct desc {
	u32 ordered;		//下一个描述符地址寄存器
	u32 saddr;			//内存地址寄存器
	u32 daddr;			//设备地址寄存器
	u32 length;		//长度寄存器
	u32 step_length;	//间隔长度寄存器
	u32 step_times;	//循环次数寄存器
	u32 cmd;			//控制寄存器
};
static struct desc *dma_desc_base;

static u32 play_desc1[7] = {
	0x1,
	DMA_BUF & 0x1fffffff,
	0xdfe72420,
	0x8,
	0x0,
	(BUF_SIZE/8/4),
	0x00001001
};

static u32 rec_desc1[7] = {
	0x1,
	REC_DMA_BUF & 0x1fffffff,
	0xdfe74c4c,
	0x8,
	0x0,
	(BUF_SIZE/8/4),
	0x00000001
};

static u32 readl(u32 addr)
{
	return *(volatile u32 *)addr;
}

static void writel(u32 val, u32 addr)
{
	*(volatile u32 *)addr = val;
}

static void init_audio_data(void)
{
	unsigned int *data = (unsigned int*)DMA_BUF;
	int i;

	for (i=0; i<(BUF_SIZE>>3); i++) {
		data[i*2] = 0;//0x7fffe000;
		data[i*2+1] = 0;//0x1f2e3d4c;
	}
}

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
		conf = (conf | (2 << FIFO_THRES_OFFSET) | (2 << SS_OFFSET) | DMA_EN | CH_EN) & (~SR);
		conf |= (conf << OCH1_CFG_R_OFFSET) | (conf << OCH0_CFG_L_OFFSET);
		writel(conf, LS1X_AC97_OCC0);

		/* 输入通道配置寄存器 */
		x = readl(LS1X_AC97_ICC);
		conf = x & ~(FIFO_THRES_MASK << FIFO_THRES_OFFSET) & ~(SS_MASK << SS_OFFSET);
		conf = (conf | (2 << FIFO_THRES_OFFSET) | (2 << SS_OFFSET) | DMA_EN | CH_EN) & (~SR);
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

int ac97_config(void)
{
	int i = 0;

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
//	writel(0, LS1X_AC97_INTM);
	writel(0xffffffff, LS1X_AC97_INTM);
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

void dma_config(void)
{
	u32 addr;

	dma_desc_base = (struct desc*)malloc(sizeof(struct desc) + 32);
	addr = dma_desc_base = (u32)dma_desc_base & ~31;
	
	/* play */
	if (AC97_PLAY == ac97_rw) {
		dma_desc_base->ordered = play_desc1[0] | (addr & 0x1fffffff);
		dma_desc_base->saddr = play_desc1[1];
		dma_desc_base->daddr = play_desc1[2];
		dma_desc_base->length = play_desc1[3];
		dma_desc_base->step_length = play_desc1[4];
		dma_desc_base->step_times = play_desc1[5];
		dma_desc_base->cmd = play_desc1[6];
		pci_sync_cache(0, (unsigned long)addr, 32*7, SYNC_W);

		addr = (u32)(addr & 0x1fffffff);

		*(volatile u32*)(CONFREG_BASE + 0x1160) = addr | 0x00000009;
	}
	/* record */
	else {
		dma_desc_base->ordered = rec_desc1[0] | (addr & 0x1fffffff);
		dma_desc_base->saddr = rec_desc1[1];
		dma_desc_base->daddr = rec_desc1[2];
		dma_desc_base->length = rec_desc1[3];
		dma_desc_base->step_length = rec_desc1[4];
		dma_desc_base->step_times = rec_desc1[5];
		dma_desc_base->cmd = rec_desc1[6];
		pci_sync_cache(0, (unsigned long)addr, 32*7, SYNC_W);

		addr = (u32)(addr & 0x1fffffff);

		*(volatile u32*)(CONFREG_BASE + 0x1160) = addr | 0x0000000a;
	}
}

/* 测试录音结果 */
int ac97_test(int argc,char **argv)
{
	char cmdbuf[100];
	int i;

	if ((argc!=2) && (argc!=1))
		return -1;
	if (argc == 2) {
		sprintf(cmdbuf, "load -o 0x%x -r %s", DMA_BUF, argv[1]);
		do_cmd(cmdbuf);
	}
	ac97_rw = AC97_PLAY;

#ifdef CONFIG_CHINESE
	printf("开始放音：注意听是否跟所录声音一致\n");
#else
	printf("begin test\n");
#endif
	
	/* 需要配置DMA */
	dma_config();

	for (i=0; i<8; i++) {
		delay(1000000);
		printf(".");
	}
	*(volatile u32*)(CONFREG_BASE + 0x1160) = 0x00000011;

	/* codec reset */
//	ls1x_ac97_write(AC97_RESET, 0x0000);

#ifdef CONFIG_CHINESE
	printf("放音结束\n");
#else
	printf("test done\n");
#endif

//	free(dma_desc_base);

	return 0;
}

/* 把codec 输入的数据通过DMA传输到内存 */
int ac97_read(int argc,char **argv)
{
	unsigned int i;
	unsigned int j;
	unsigned short *rec_buff;
	unsigned int *ply_buff;
	ac97_rw = AC97_RECORD;

	init_audio_data();
	
	/*1.ac97 config read*/
	ac97_config();

	/*2.dma_config read*/
	dma_config();

	/*3.set dma desc*/
#ifdef CONFIG_CHINESE
	printf("请用麦克风录一段音 ");
#else
	printf("please speck to microphone ");
#endif
	for (i=0; i<8; i++) {
		delay(1000000);
		printf("."); 
	}
	*(volatile u32*)(CONFREG_BASE + 0x1160) = 0x00000012;
#ifdef CONFIG_CHINESE
	printf("录音结束\n");
#else
	printf("rec done\n");
#endif

	/*4.wait dma done,return*/ 
//	while(((dma_reg_read(0x2c))&0x8)==0)
//		delay(1000);//1 ms 
//	dma_reg_write(0x2c,0x8);
	
	/*5.transform single channel to double channel*/
	rec_buff = (unsigned short *)REC_DMA_BUF;
	ply_buff = (unsigned int *)DMA_BUF;

	for (i=0; i<(REC_BUF_SIZE/4); i++) {
		j = 0x0000ffff & (unsigned int)rec_buff[i];
		ply_buff[i] = (j<<16) | j;
	}

	return 0;
}

static const Cmd Cmds[] =
{
	{"MyCmds"},
	{"ac97_test", "file", 0, "ac97_test file", ac97_test, 0, 99, CMD_REPEAT},
	{"ac97_read", "", 0, "ac97_read", ac97_read, 0, 99, CMD_REPEAT},
	{"ac97_config", "", 0, "ac97_config", ac97_config, 0, 99, CMD_REPEAT},
	{0, 0}
};

static void init_cmd __P((void)) __attribute__ ((constructor));

static void
init_cmd()
{
	cmdlist_expand(Cmds, 1);
}

