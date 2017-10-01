/* IIS driver for loongson 1c */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <sys/malloc.h>
#include <pmon.h>
#include <machine/pio.h>

#include <target/i2c-ls1x.h>

#define RX_DMA_DESC_SIZE	64
#define TX_DMA_DESC_SIZE	64
#define RX_BUFF_SIZE		0x00200000	/* 0.5MB */
#define TX_BUFF_SIZE		0x00200000	/* 1MB */

#define RX_DMA_DESC		0x80400000
#define TX_DMA_DESC		0x80400080
#define RX_DATA_BUFF	0x80400100
#define TX_DATA_BUFF	(RX_DATA_BUFF + RX_BUFF_SIZE + 64)

#define DMA_RX_ADDR		0x1fe6000c	/* DMA接收操作的地址 */
#define DMA_TX_ADDR		0x1fe60010	/* DMA发送操作的地址 */
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

#define LS1X_IIS_BASE	0xbfe60000

#define LS1X_IIS_VERSION	(LS1X_IIS_BASE + 0x00)
#define LS1X_IIS_CONFIG		(LS1X_IIS_BASE + 0x04)
#define LS1X_IIS_CONTROL	(LS1X_IIS_BASE + 0x08)
#define LS1X_IIS_RXDATA		(LS1X_IIS_BASE + 0x0c)
#define LS1X_IIS_TXDATA		(LS1X_IIS_BASE + 0x10)

#define DMA_BUF		0xa3b00000


struct ls1x_iis_info {
	/* IIS registers*/
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
static int ls1x_iis_reset = 1;

static void codec_init(void)
{
	struct i2c_client uda1342;
	u8 buf[3];

	uda1342.addr = 0x1a;

	/* 注意uda1342先发送高位再发送低位 */
	buf[0] = 0x00; buf[1] = 0x5c; buf[2] = 0x42;
	i2c_master_send(&uda1342, buf, 3);
	buf[0] = 0x01; buf[1] = 0x00; buf[2] = 0x04;	/* no mixer 输入不混合到输出 */
	i2c_master_send(&uda1342, buf, 3);
	buf[0] = 0x10; buf[1] = 0x00; buf[2] = 0x02;
	i2c_master_send(&uda1342, buf, 3);
	buf[0] = 0x11; buf[1] = 0x00; buf[2] = 0x00;
	i2c_master_send(&uda1342, buf, 3);
	buf[0] = 0x12; buf[1] = 0x00; buf[2] = 0x00;
	i2c_master_send(&uda1342, buf, 3);
	buf[0] = 0x20; buf[1] = 0x0f; buf[2] = 0x30;
	i2c_master_send(&uda1342, buf, 3);
	buf[0] = 0x21; buf[1] = 0x0f; buf[2] = 0x30;
	i2c_master_send(&uda1342, buf, 3);
}

static int iis_config(void)
{
	unsigned char sck_ratio;
	unsigned char bck_ratio;

#ifdef CONFIG_CHINESE
	printf("配置 iis 编解码器\n");
#else
	printf("config iis codec\n");
#endif

#define SAMP_RATE 44100
	sck_ratio = tgt_apbfreq()/(SAMP_RATE*2*2*2*16) - 1;
	bck_ratio = tgt_apbfreq()/(SAMP_RATE*2*2*512) - 1;
#else
	sck_ratio = 0xf;
	bck_ratio = 0x1;
#end
//	printf("sck_ratio=%x bck_ratio=%x\n", sck_ratio, bck_ratio);

	writel((16<<24) | (16<<16) | (sck_ratio<<8) | (bck_ratio<<0), LS1X_IIS_CONFIG);
	writel(0xc220, LS1X_IIS_CONTROL);

	codec_init();

#ifdef CONFIG_CHINESE
	printf("配置完毕\n");
#else
	printf("config done\n");
#endif
	return 0;
}

static int ls1x_iis_init_buff(struct ls1x_iis_info *info)
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
	info->mmio_base = LS1X_IIS_BASE;
	order_addr_in = ORDER_ADDR_IN;

	return 0;
}

static void ls1x_dma_init(struct ls1x_iis_info *info)
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

static void start_dma_rx(struct ls1x_iis_info *info)
{
	int timeout = 30000;
	int ret = 0;

	writel(readl(LS1X_IIS_CONTROL) | 0x2800, LS1X_IIS_CONTROL);

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

static void start_dma_rxs(struct ls1x_iis_info *info, unsigned int saddr, unsigned int length)
{
	int timeout = 30000;
	int ret = 0;

	writel(readl(LS1X_IIS_CONTROL) | 0x2800, LS1X_IIS_CONTROL);

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

static void start_dma_tx(struct ls1x_iis_info *info)
{
	int timeout = 30000;
	int ret = 0;

	writel(readl(LS1X_IIS_CONTROL) | 0x1080, LS1X_IIS_CONTROL);

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

static void start_dma_txs(struct ls1x_iis_info *info, unsigned int saddr, unsigned int length)
{
	int timeout = 30000;
	int ret = 0;

	writel(readl(LS1X_IIS_CONTROL) | 0x1080, LS1X_IIS_CONTROL);

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
int iis_test(int argc, char **argv)
{
	struct ls1x_iis_info *info;
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

	info = malloc(sizeof(struct ls1x_iis_info), M_DEVBUF, M_WAITOK);
	if (!info) {
		printf("Unable to allocate iis device structure.\n");
		return -1;
	}

	if (ls1x_iis_init_buff(info)) {
		printf("error: init buff have some error!\n");
		return -1;
	}

	ls1x_dma_init(info);
	iis_config();

#ifdef CONFIG_CHINESE
	printf("开始放音：注意听是否跟所录声音一致\n");
#else
	printf("begin test\n");
#endif
	
	/* 需要配置DMA */
	start_dma_rx(info);
/*	for (i = 0; i < RX_BUFF_SIZE; i += 512) {
		start_dma_rxs(info, info->rx_data_buff_phys + i, 128);
	}*/

#if 0
	rx_buff = (unsigned short *)RX_DATA_BUFF;
	tx_buff = (unsigned int *)TX_DATA_BUFF;

	for (i=0; i<(RX_BUFF_SIZE/2); i++) {
		j = 0x0000ffff & (unsigned int)rx_buff[i];
		tx_buff[i] = (j<<16) | j;
	}
#else
	rx_buff = (unsigned int *)RX_DATA_BUFF;
	tx_buff = (unsigned int *)TX_DATA_BUFF;

	for (i=0; i<(RX_BUFF_SIZE); i++) {
		j = 0x0000ffff & *(rx_buff + i);
		*(tx_buff + i) = (j<<16) | j;
	}
#endif

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
	{"iis_test", "file", 0, "iis_test file", iis_test, 0, 99, CMD_REPEAT},
	{0, 0}
};

static void init_cmd __P((void)) __attribute__ ((constructor));

static void init_cmd(void)
{
	cmdlist_expand(Cmds, 1);
}
#endif

