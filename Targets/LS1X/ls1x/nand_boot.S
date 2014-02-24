
#include <target/ls1x_nand.h>

#define	DATA_LENGTH		0x80000	/* 512KB 预计使用512KB存放PMON */

#undef BOOT_ECCNAND

#ifdef BOOT_ECCNAND
#define DMA_READ_WORDS	(((NAND_PAGE_SIZE / 204) * 188) >> 2)
#define NAND_READ_BYTES	((NAND_PAGE_SIZE / 204) * 204)
#define NAND_READ_CMD	0x4903
#define TRANS_TIME		(DATA_LENGTH / DMA_READ_WORDS)
#else
#define DMA_READ_WORDS	((DATA_LENGTH >> 2))
#define NAND_READ_BYTES 4*DMA_READ_WORDS	
#define NAND_READ_CMD	0x103
#define TRANS_TIME		0x1
#endif

boot:
	/* config pll div for cpu and sdram */
	#define SDRAM_DIV_2	0x0
	#define SDRAM_DIV_3	0x2
	#define SDRAM_DIV_4	0x1
	#define SDRAM_PARAM_DIV_NUM		((1 << (SDRAM_DIV+1)) % 5)

	li		t0, 0xbfe78030

/*	li		v0, 0xbfc00000+(NVRAM_POS+PLL_OFFS)
	lw		t2, 0(v0)
	lw		t3, 4(v0)

	li		v0, 0x00000000
	beq		t2, v0, 1f
	nop
	li		v0, 0xffffffff
	beq		t2, v0, 1f
	nop

	li		v0, 0x00000000
	beq		t3, v0, 1f
	nop
	li		v0, 0xffffffff
	beq		t3, v0, 1f
	nop
	nop
	b		2f
	nop*/
1:
	/* 设置PLL倍频 及SDRAM分频 */
	li		t2, (0x80000008 | (PLL_MULT << 8) | (0x3 << 2) | SDRAM_DIV)
	/* 设置CPU分频 */
	li		t3, (0x00008003 | (CPU_DIV << 8))
2:
	/* 注意：首先需要把分频使能位清零 */
	li		t1, 0x2
	sw		t1, 0x4(t0)
	sw		t2, 0x0(t0)
	sw		t3, 0x4(t0)
	DELAY(2000)

	/* 初始化sdram */
#include "sdram_cfg.S"

	li		t1, 0xbfd00410
	li		a1, SD_PARA0
	sw		a1, 0x0(t1)
	li		a1, SD_PARA1
	sw		a1, 0x4(t1)

dma_nand_cfg_read:
	#DMA read ddr operation
	li		a1, DMA_DESC	/* dma desc地址 自定义的一段内存地址 */
	li		t0, 0x00
	sw		t0, 0x0(a1)	/* dma_order_addr */
	li		t0, DATA_BUFF & 0x1fffffff
	sw		t0, 0x4(a1)	/* dma_mem_addr */

	li		t0, DMA_ACCESS_ADDR
	li		t1, DMA_ACCESS_ADDR
	or		t0, t0, t1
	or		t0, t0, t1
	and		t0, t0, 0x1fffffff
	sw		t0, 0x8(a1)	/* dma_dev_addr */

	li		t0, DMA_READ_WORDS
	sw		t0, 0xc(a1)	/* dma_length */
	li		t0, 0x0
	sw		t0, 0x10(a1)	/* dma_step_length */
	li		t0, TRANS_TIME
	sw		t0, 0x14(a1)	/* step times */
	li		t0, 0x1 
	sw		t0, 0x18(a1)	/* dma cmd */
	nop

	/* state_reg [12] =1 read  ddr and write devices, 0 write ddr and read devices */
	li		a1, ORDER_ADDR_IN
	li		t0, (DMA_DESC & 0x1fffffff) | 0x8	/* which dma channel to be configured */
	sw		t0, 0x0(a1)
	nop

#ifdef	BOOT_ECCNAND	
	li		t4, TRANS_TIME
	li		t3, 0x0
#endif
nand_rd_cmd:
	li		t2, LS1X_NAND_BASE
	li		t1, NAND_PARAMETER | 0x8005000
	sw		t1, 0x18(t2)
	li		t1, NAND_READ_BYTES
	sw		t1, 0x1c(t2)
#ifdef	BOOT_ECCNAND	
ecc_rd:
	sw		t3, 0x8(t2)	/* page addr */
#endif
	li		t1, NAND_READ_CMD
	sw		t1, 0x0(t2)
	nop

1:
	lw		t0, 0x0(t2)
	and		t0, t0, 0x400
	beqz	t0, 1b	/* 等待nand命令完成 */
	nop
#ifdef	BOOT_ECCNAND	
	sw		zero, 0x0(t2)	/* clear nand_control's cmd */
	sub		t4, 1
	add		t3,	1			/* increase page addr */
	bnez	t4, ecc_rd
	nop
#endif

	li		t1, 0x10	/* stop DMA */
	sw		t1, 0x0(a1)

