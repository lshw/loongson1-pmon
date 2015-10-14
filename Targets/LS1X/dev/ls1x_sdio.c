/* sdio test for loongson 1c */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <pmon.h>
#include <machine/pio.h>

#define MISC_CTRL 0xbfd00424

#define LS1X_SDIO_BASE  0xbfe6c000
#define SDI_CON     0x00
#define SDI_PRE     0x04
#define SDI_CMDARG  0x08
#define SDI_CMDCON  0x0c
#define SDI_CMDSTA  0x10
#define SDI_RSP0    0x14
#define SDI_RSP1    0x18
#define SDI_RSP2    0x1C
#define SDI_RSP3    0x20
#define SDI_DTIMER  0x24
#define SDI_BSIZE   0x28
#define SDI_DATCON  0x2C
#define SDI_DATCNT  0x30
#define SDI_DATSTA  0x34
#define SDI_FIFOSTA 0x38
#define SDI_INTMSK  0x3C
#define SDI_WRDAT   0x40
#define SDI_STAADD0 0x44
#define SDI_STAADD1 0x48
#define SDI_STAADD2 0x4c
#define SDI_STAADD3 0x50
#define SDI_STAADD4 0x54
#define SDI_STAADD5 0x58
#define SDI_STAADD6 0x5c
#define SDI_STAADD7 0x60
#define SDI_INTEN   0x64

#define SDIO_DES_ADDR   0x1fe6c040
#define CONFREG_BASE    0xbfd00000
#define SDIO_RD_MEM_ADDR 0x900000
#define SDIO_WR_MEM_ADDR 0x800000

#define DAT_4_WIRE 0
#define ERASE_START_ADDR 0x5000
#define ERASE_END_ADDR   0x5000

static unsigned int	dma_desc;
static unsigned int	dma_desc_phys;
static size_t		dma_desc_size;

#define DMA_ACCESS_ADDR	0x1fe6c040	/* DMA对NAND操作的地址 */
#define ORDER_ADDR_IN	0xbfd01160	/* DMA配置寄存器 */
#define DMA_DESC_NUM	64	/* DMA描述符占用的字节数 7x4 */
/* DMA描述符 */
#define DMA_ORDERED		0x00
#define DMA_SADDR		0x04
#define DMA_DADDR		0x08
#define DMA_LENGTH		0x0c
#define DMA_STEP_LENGTH		0x10
#define DMA_STEP_TIMES		0x14
#define	DMA_CMD			0x18

static unsigned int order_addr_in;

unsigned int ls1x_sdio_base;

static void cmd_send(int cmd_index, int wait_rsp, int long_rsp, int check_crc, int cmd_arg)
{
	int sdicmdcon;

	sdicmdcon = (cmd_index & 0x3f) | 0x140 | (wait_rsp << 9) | (long_rsp << 10) | (check_crc << 13);
	writel(cmd_arg, ls1x_sdio_base + SDI_CMDARG);
	writel(sdicmdcon, ls1x_sdio_base + SDI_CMDCON);
	printf("sending cmd %d ...\n", cmd_index);
}

static void cmd_check(int cmd_index)
{
	int cmd_fin = 0;
	volatile unsigned int rsp_cmdindex = 0;
	volatile unsigned int sdiintmsk = 0;

	while (cmd_fin == 0) {
		sdiintmsk = readl(ls1x_sdio_base + SDI_INTMSK);
		cmd_fin   = sdiintmsk & 0x1c0;
		printf("sdiintmsk = %0x \n",sdiintmsk);
	}
//	writel(0xffffffff, ls1x_sdio_base + SDI_INTMSK);
	rsp_cmdindex = readl(ls1x_sdio_base + SDI_CMDSTA);
	rsp_cmdindex = rsp_cmdindex & 0x3f;
}

static void data_check(void)
{
	int data_fin = 0;
	int sdiintmsk = 0;

	while (data_fin == 0) {
//		printf("sdiintmsk = %0x \n", sdiintmsk);
		sdiintmsk = readl(ls1x_sdio_base + SDI_INTMSK);
		data_fin  = sdiintmsk & 0x1f;
	}

//	printf("sdiintmsk = %0x \n", sdiintmsk);

	if (sdiintmsk & 0x8) {
		printf("crc state err \n");
	}
	if (sdiintmsk & 0x4) {
		printf("data crc err \n");
	}
	if (sdiintmsk & 0x10) {
		printf("program err \n");
	}
	if (sdiintmsk & 0x2) {
		printf("data time out \n");
	}
	if (sdiintmsk & 0x1) {
		printf("data finished \n");
	}
}

static void sdio_cfg_dma(int rd_wr_flag, int block_num)
{
	writel(0, dma_desc + DMA_ORDERED);
	writel(DMA_ACCESS_ADDR, dma_desc + DMA_DADDR);
	writel(block_num*128, dma_desc + DMA_LENGTH);
	writel(0, dma_desc + DMA_STEP_LENGTH);
	writel(1, dma_desc + DMA_STEP_TIMES);

	if (rd_wr_flag) {
		writel(0x00000001, dma_desc + DMA_CMD);
		writel(SDIO_RD_MEM_ADDR, dma_desc + DMA_SADDR);
	} 
	else {
		writel(0x00001001, dma_desc + DMA_CMD);
		writel(SDIO_WR_MEM_ADDR, dma_desc + DMA_SADDR);
	}
	writel((dma_desc_phys & ~0x1F) | 0x8, order_addr_in);	/* 启动DMA */
	while ((readl(order_addr_in) & 0x8)/* && (timeout-- > 0)*/) {
//		printf("%s. %x\n",__func__, readl(order_addr_in));
//		udelay(5);
	}
}

static void sdio_rd_wr(int rd_wr_flag, int single_block, int addr)
{
	int block_num, cmd_index, wait_rsp, longrsp, check_crc, cmd_arg;
	int misc_ctrl;

	writel(0x000200, ls1x_sdio_base + SDI_BSIZE);
	writel(0x7fffff, ls1x_sdio_base + SDI_DTIMER);

	writel(readl(MISC_CTRL) | (1<<23), MISC_CTRL);

	if (rd_wr_flag) {
		if (single_block) {
			if(DAT_4_WIRE) {
				writel(0x1c001, ls1x_sdio_base + SDI_DATCON);
			}  // 4 wire
			else {
				writel(0x0c001, ls1x_sdio_base + SDI_DATCON);
			}  // single wire
			block_num = 0x1;
			cmd_index = 0x11; // send cmd17 to read single block
		}
		else {
			if (DAT_4_WIRE) {
				writel(0x1c004, ls1x_sdio_base + SDI_DATCON);
			} // 4 blocks
			else {
				writel(0x0c004, ls1x_sdio_base + SDI_DATCON);
			}  // single wire
			block_num = 0x4;
			cmd_index = 0x12; // send cmd18 to read single block
		}
		sdio_cfg_dma(rd_wr_flag, block_num);
		wait_rsp  = 1;
		longrsp   = 0;
		check_crc = 1;
		cmd_arg   = addr;
		cmd_send(cmd_index, wait_rsp, longrsp, check_crc, cmd_arg);
		cmd_check(cmd_index);
		data_check();
		if (single_block) {
			printf("sdio single block read done \n");
		} else {
			printf("sdio multi block read done \n");
		}
	}
	else {
		if (single_block) {
			if (DAT_4_WIRE) {
				writel(0x1c001, ls1x_sdio_base + SDI_DATCON);
			}  // 0x1c001 for 4 wire / 0x0c001 for single wire
			else {
				writel(0x0c001, ls1x_sdio_base + SDI_DATCON);
			}  // single wire single block
			block_num = 0x1;
			cmd_index = 0x18; // send cmd24 to write single block
		}
		else {
			if (DAT_4_WIRE) {
				writel(0x1c004, ls1x_sdio_base + SDI_DATCON);
			} // 4 wires 4 blocks
			else {
				writel(0x0c004, ls1x_sdio_base + SDI_DATCON);
			} // single wire 4 blocks
			block_num = 0x4;
			cmd_index = 0x19; // send cmd25 to read single block
		}
		sdio_cfg_dma(rd_wr_flag, block_num);
		wait_rsp  = 1;
		longrsp   = 0;
		check_crc = 1;
		cmd_arg   = addr;
		cmd_send(cmd_index, wait_rsp, longrsp, check_crc, cmd_arg);
		cmd_check(cmd_index);
		printf("=======wait======== \n");
		data_check();
		if (single_block) {
			printf("sdio single block write done \n");
		}
		else {
			printf("sdio multi block write done \n");
		}
	}
}

static void sdio_sgl_rd(void)
{
	sdio_rd_wr(1,1,0x5000);
}

static void sdio_sgl_wr(void)
{
	int i;
	for (i=0; i<256; i=i+4) {
		*(volatile unsigned int *)(0xa0000000 + SDIO_WR_MEM_ADDR + i) = (i+3)| ((i+2)<<8) | ((i+1)<<16) | (i<<24);
	}
	for(i=0; i<256; i=i+4) {
		*(volatile unsigned int *)(0xa0000000 + SDIO_WR_MEM_ADDR + 0x100 + i) = i | ((i+1)<<8) | ((i+2)<<16) | ((i+3)<<24);
	}
	printf("write ram done \n");
	sdio_rd_wr(0,1,0x5000);
}

static void sdio_erase(void)
{
	int cmd_index, wait_rsp, longrsp, check_crc, cmd_arg;

	cmd_arg   = ERASE_START_ADDR;
	cmd_index = 32;
	wait_rsp  = 1;
	longrsp   = 0;
	check_crc = 1;
	cmd_send(cmd_index, wait_rsp, longrsp, check_crc, cmd_arg);  // send cmd32 to send acmd 
	cmd_check(cmd_index);

	cmd_arg   = ERASE_END_ADDR;
	cmd_index = 33;
	wait_rsp  = 1;
	longrsp   = 0;
	check_crc = 1;
	cmd_send(cmd_index, wait_rsp, longrsp, check_crc, cmd_arg);  // send cmd32 to send acmd 
	cmd_check(cmd_index);

	cmd_arg   = 0;
	cmd_index = 38;
	wait_rsp  = 1;
	longrsp   = 0;
	check_crc = 1;
	cmd_send(cmd_index, wait_rsp, longrsp, check_crc, cmd_arg);  // send cmd38 to send acmd 
	cmd_check(cmd_index);
}

#define ALIGN(x,a)		__ALIGN_MASK((x),(typeof(x))(a)-1)
#define __ALIGN_MASK(x,mask)	(((x)+(mask))&~(mask))

static int ls1x_sdio_init(void)
{
	int cmd_index,wait_rsp,longrsp,check_crc;
	int cmd_arg;
	unsigned int resp;
	int card_info;
	int sdiintmsk = 0;
	int rca;
	int trans_state;

	int i, ret;
	ls1x_sdio_base = LS1X_SDIO_BASE;

	/* 设置复用 */
	ret = readl(MISC_CTRL);
	ret |= (1 << 16);
	writel(ret, MISC_CTRL);

	writel(0x01, ls1x_sdio_base + SDI_CON);	// enable clk
	writel(0x00000007, ls1x_sdio_base + SDI_PRE);
	writel(0xffffffff, ls1x_sdio_base + SDI_INTMSK);
	writel(0xffffffff, ls1x_sdio_base + SDI_INTEN);

	delay(1000);	// wait at least 74 clk for sd memory card init      
	cmd_index = 0;
	wait_rsp  = 0;
	longrsp   = 0;
	check_crc = 0;
	cmd_arg   = 0;
	cmd_send(cmd_index, wait_rsp, longrsp, check_crc, cmd_arg);	// send cmd0 for reset
	cmd_check(cmd_index);
	delay(300000);

	while (1) {
		cmd_index = 8;
		wait_rsp  = 1;
		longrsp   = 0;
		check_crc = 1;
		cmd_arg   = 0x1aa;
		cmd_send(cmd_index, wait_rsp, longrsp, check_crc, cmd_arg);	// send cmd8 to get sd memory card support voltage(VHS) 
		cmd_check(cmd_index);
		/* 命令超时？ */
		sdiintmsk = readl(ls1x_sdio_base + SDI_INTMSK);
		if ((sdiintmsk & 0x80) == 0x80) {
			writel(0x80, ls1x_sdio_base + SDI_INTMSK);
		} else {
			break;
		}
	}
	resp = 0;
	delay(1000);

	while ((resp & 0x80000000) == 0) {
		cmd_index = 55;
		wait_rsp  = 1;
		longrsp   = 0;
		check_crc = 1;
		cmd_arg   = 0;
		cmd_send(cmd_index, wait_rsp, longrsp, check_crc, cmd_arg);  // send cmd55 to send acmd 
		cmd_check(cmd_index);

		cmd_index = 41;
		wait_rsp  = 1;
		longrsp   = 0;
		check_crc = 0;
		cmd_arg   = 0x40ff8000;
		cmd_send(cmd_index, wait_rsp, longrsp, check_crc, cmd_arg);  // send acmd41 to get the card's HCS, and power up.
		cmd_check(cmd_index);
		/* 命令超时？ */
		sdiintmsk = readl(ls1x_sdio_base + SDI_INTMSK);
		if ((sdiintmsk & 0x80) == 0x80) {
			writel(0x80, ls1x_sdio_base + SDI_INTMSK);
		}
		resp  = readl(ls1x_sdio_base + SDI_RSP0);    // read response
		printf("ocr = %0x \n", resp);
	}
	printf("sd card is in ready state \n"); // wait for cmd 0 finished

	resp = 0;
	cmd_index = 2;
	wait_rsp  = 1;
	longrsp   = 1;
	check_crc = 0;
	cmd_arg   = 0;
	cmd_send(cmd_index, wait_rsp, longrsp, check_crc, cmd_arg);  // send cmd2 to get the card's CID.
	cmd_check(cmd_index);
	resp = readl(ls1x_sdio_base + SDI_RSP0);
	card_info = resp;
	printf("sd card's CID[127:96] is 0x%08x \n",card_info);
	printf("Manufacturer ID (MID) : 0x%2x\n", (resp >> 24));
	printf("OEM/Application ID (OID) : %c%c\n", (resp >> 16), (resp >> 8));
	printf("Product Name (PNM) : %c", resp);
	resp = readl(ls1x_sdio_base + SDI_RSP1);
	printf("%c%c%c%c\n", (resp >> 24), (resp >> 16), (resp >> 8));
	card_info = resp;
	printf("sd card's CID[ 95:64] is 0x%08x \n",card_info);
	resp = readl(ls1x_sdio_base + SDI_RSP2);
	card_info = resp;
	printf("sd card's CID[ 63:32] is 0x%08x \n",card_info);
	resp = readl(ls1x_sdio_base + SDI_RSP3);
	card_info = resp;
	printf("sd card's CID[ 31: 0] is 0x%08x \n",card_info);

	resp = 0;
	while ((resp & 0x1e00) != 0x600) {
		cmd_index = 3;
		wait_rsp  = 1;
		longrsp   = 0;
		check_crc = 1;
		cmd_arg   = 0;
		cmd_send(cmd_index, wait_rsp, longrsp, check_crc, cmd_arg);  // send cmd3 to get the card's RCA
		cmd_check(cmd_index);

		resp = readl(ls1x_sdio_base + SDI_RSP0);
	}
	printf("sd card is in stby state \n");

	rca = resp & 0xffff0000;
	printf("sd card's rca is %0x \n",rca);

	resp = 0;
	cmd_index = 9;
	wait_rsp  = 1;
	longrsp   = 1;
	check_crc = 0;
	cmd_arg   = rca;
	cmd_send(cmd_index, wait_rsp, longrsp, check_crc, cmd_arg);  // send cmd9 to get the card's CSD
	cmd_check(cmd_index);
//	resp = readl(ls1x_sdio_base + SDI_RSP0); // 
//	card_info = resp;
//	printf("sd card's csd[127:96] is 0x%08x \n",card_info); // 
	resp = readl(ls1x_sdio_base + SDI_RSP1);
	card_info = resp;
//	printf("sd card's csd[ 95:64] is 0x%08x \n",card_info); // 
	resp = readl(ls1x_sdio_base + SDI_RSP2);
//	card_info = resp;
//	printf("sd card's csd[ 63:32] is 0x%08x \n",card_info); // 
//	resp = readl(ls1x_sdio_base + SDI_RSP3); 
//	card_info = resp;
//	printf("sd card's csd[ 31: 0] is 0x%08x \n",card_info); // 
	i = ((card_info >> 16) & 0xf);	//READ_BL_LEN
	i = 1 << i;
	card_info = ((card_info & 0x3ff) << 10) | ((resp >> 30) & 0x3);	//C_SIZE
	card_info++;	//C_SIZE + 1
	i = i * card_info;
	card_info = (resp >> 15) & 0x7;		//C_SIZE_MULT
	card_info = 1 << (card_info + 2);
	i = (i * card_info) >> 10;
	printf ("sd card's capacity = %d MB!\n", i);

	cmd_index = 7;
	wait_rsp  = 1;
	longrsp   = 0;
	check_crc = 1;
	cmd_arg   = rca;
	cmd_send(cmd_index, wait_rsp, longrsp, check_crc, cmd_arg);  // send cmd7 to set card into select state.
	cmd_check(cmd_index);

	resp = 0;
	cmd_index = 13;
	wait_rsp  = 1;
	longrsp   = 0;
	check_crc = 1;
	cmd_arg   = rca;
	cmd_send(cmd_index, wait_rsp, longrsp, check_crc, cmd_arg);  // send cmd13 to get the card's state
	cmd_check(cmd_index);

	resp = readl(ls1x_sdio_base + SDI_RSP0); // 
	trans_state = resp & 0x1e00;
	if (trans_state == 0x800) {
		printf("sd card is in tran state \n");
	}
	else {
		printf("sd card stays in stby state \n");
	} 

	if (DAT_4_WIRE) {
		cmd_index = 55;
		wait_rsp  = 1;
		longrsp   = 0;
		check_crc = 1;
		cmd_arg   = rca;
		cmd_send(cmd_index, wait_rsp, longrsp, check_crc, cmd_arg);  // send cmd55 to send acmd 
		cmd_check(cmd_index);

		cmd_index = 6;
		wait_rsp  = 1;
		longrsp   = 0;
		check_crc = 1;
		cmd_arg   = 0x2;
		cmd_send(cmd_index, wait_rsp, longrsp, check_crc, cmd_arg);  // send acmd6 to config 4 wire mode
		cmd_check(cmd_index);
		printf("change bus width as 4 wires \n");
	}

	dma_desc_size = ALIGN(DMA_DESC_NUM, PAGE_SIZE);	/* 申请内存大小，页对齐 */
	dma_desc = ((unsigned int)malloc(dma_desc_size) & 0x0fffffff) | 0xa0000000;
	dma_desc = (unsigned int)ALIGN((unsigned int)dma_desc, 32);	/* 地址32字节对齐 */
	dma_desc_phys = (unsigned int)(dma_desc) & 0x1fffffff;

	order_addr_in = ORDER_ADDR_IN;

/*	ret = readl(MISC_CTRL);
	ret &= ~(1 << 16);
	writel(ret, MISC_CTRL);*/

	return 0;
}

#if 1
static const Cmd Cmds[] = {
	{"MyCmds"},
	{"ls1x_sdio_init", "", 0, "ls1x sdio sd card init", ls1x_sdio_init, 0, 99, CMD_REPEAT},
	{"sdio_sgl_rd", "", 0, "sdio_sgl_rd", sdio_sgl_rd, 0, 99, CMD_REPEAT},
	{"sdio_sgl_wr", "", 0, "sdio_sgl_wr", sdio_sgl_wr, 0, 99, CMD_REPEAT},
	{"sdio_erase", "", 0, "sdio_erase", sdio_erase, 0, 99, CMD_REPEAT},
	{0, 0}
};

static void init_cmd __P((void)) __attribute__ ((constructor));

static void init_cmd(void)
{
	cmdlist_expand(Cmds, 1);
}
#endif
