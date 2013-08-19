
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <linux/mtd/mtd.h>
#include <linux/mtd/nand.h>
#include <linux/mtd/partitions.h>
#include <sys/malloc.h>

#include <pmon.h>
#include <target/ls1x_nand.h>

#define EAE		1
#define EDC		2	/* Activate ECC/EDC */
#define EXTRA	4	/* Read/write spare area */

//#define USE_ECC
#undef USE_ECC

#undef OP_OOB
#define OP_MAIN
#undef OP_EAE

#define K9F1G08U0C
#ifdef K9F1G08U0C
// 2^PAGE_P_BLOCK = 32 / 64
#define PAGE_P_BLOCK (6) 
#define WRITE_SIZE (2048)
#define OOB_SIZE (64)
#define ERASE_SIZE (64*2048)
#ifdef USE_ECC
#define NAND_PARAM_V 0x7F85000
#endif
#endif

static unsigned int order_addr_in;

struct ls1x_nand_info {
	unsigned int 		buf_start;
	unsigned int		buf_count;
	/* NAND registers*/
	void 		*mmio_base;

	/* DMA information */
	int			dma_desc;	/*dma descriptor address;*/
	unsigned int 		dma_desc_phys;

	unsigned char		*data_buff;/*dma data buffer;*/
	unsigned int 		data_buff_phys;

	/* relate to the command */
/*	int			use_ecc;	*//* use HW ECC ? */
	unsigned int            cmd;
	unsigned int            page_addr;
	unsigned int            seqin_column;
	unsigned int            seqin_page_addr;
	unsigned int            timing_val;

	unsigned int            ori_data;
};

struct ls1x_nand_info ls1A_Nand_Info;

static void ls1g_nand_cmdfunc(unsigned command, int column, int page_addr, int flag);

static void ls1x_nand_init_hw(struct ls1x_nand_info *info)
{
	nand_writel(info, NAND_CMD, 0x00);
	nand_writel(info, NAND_ADDR_L, 0x00);
	nand_writel(info, NAND_ADDR_H, 0x00);
	nand_writel(info, NAND_TIMING, (HOLD_CYCLE << 8) | WAIT_CYCLE);
//#if defined(LS1ASOC) || defined(LS1BSOC)
//	nand_writel(info, NAND_PARAM, 0x00000100);	/* 设置外部颗粒大小，1A 2Gb? */
//#elif defined(LS1CSOC)
	nand_writel(info, NAND_PARAM, 0x08005000);
//#endif
	nand_writel(info, NAND_OPNUM, 0x00);
	nand_writel(info, NAND_CS_RDY, 0x88442211);	/* 重映射rdy1/2/3信号到rdy0 rdy用于判断是否忙 */
}

int nand_probe_boot(void)
{
	struct ls1x_nand_info *info = &ls1A_Nand_Info;
	printf("-----------------------------------------------------------\n");

	info->dma_desc = (unsigned int)(DMA_DESC | 0xa0000000);	/* DMA描述符地址 */
	if(info->dma_desc == NULL)
		return -1;
	info->dma_desc_phys = (unsigned int)(info->dma_desc) & 0x1fffffff;

	info->data_buff = (unsigned char *)(DATA_BUFF | 0xa0000000);	/* NAND的DMA数据缓存 */
	if(info->data_buff == NULL)
		return -1;
	info->data_buff_phys = (unsigned int)(info->data_buff) & 0x1fffffff;

	printf("info->dma_desc%x info->dma_desc_phys%x \n", info->dma_desc, info->dma_desc_phys);

	info->mmio_base = 0xbfe78000;
	order_addr_in = ORDER_ADDR_IN;

	printf("+++ cmd=%x\n", nand_readl(info, NAND_CMD));
	printf("+++ ADDR_L=%x\n", nand_readl(info, NAND_ADDR_L));
	printf("+++ ADDR_H=%x\n", nand_readl(info, NAND_ADDR_H));
	printf("+++ TIMING=%x\n", nand_readl(info, NAND_TIMING));
	printf("+++ IDL=%x\n", nand_readl(info, NAND_IDL));
	printf("+++ IDH=%x\n", nand_readl(info, NAND_IDH));
	printf("+++ STATUS=%x\n", nand_readl(info, NAND_STATUS));
	printf("+++ PARAM=%x\n", nand_readl(info, NAND_PARAM));
	printf("+++ OPNUM=%x\n", nand_readl(info, NAND_OPNUM));
	printf("+++ CS_RDY=%x\n", nand_readl(info, NAND_CS_RDY));

	ls1x_nand_init_hw(info);
	ls1g_nand_cmdfunc(NAND_CMD_RESET, 0, -1, 0);
	ls1g_nand_cmdfunc(NAND_CMD_READID, 0, -1, 0);
	printf("IDS=============================0x%x\n", *((int *)(info->data_buff)));

	printf("+++ cmd=%x\n", nand_readl(info, NAND_CMD));
	printf("+++ ADDR_L=%x\n", nand_readl(info, NAND_ADDR_L));
	printf("+++ ADDR_H=%x\n", nand_readl(info, NAND_ADDR_H));
	printf("+++ TIMING=%x\n", nand_readl(info, NAND_TIMING));
	printf("+++ IDL=%x\n", nand_readl(info, NAND_IDL));
	printf("+++ IDH=%x\n", nand_readl(info, NAND_IDH));
	printf("+++ STATUS=%x\n", nand_readl(info, NAND_STATUS));
	printf("+++ PARAM=%x\n", nand_readl(info, NAND_PARAM));
	printf("+++ OPNUM=%x\n", nand_readl(info, NAND_OPNUM));
	printf("+++ CS_RDY=%x\n", nand_readl(info, NAND_CS_RDY));

	return 0;
}

int
nand_erase_boot(void *base, int size, int verbose)
{
	int nand_pagePtr;
	unsigned int command;
	unsigned int erase_num = 0;
	int cnt=0;


	printf("erase_addr:0x%x, erase_size:0x%x Byte\n", base, size);

	nand_pagePtr = (int)base;
	nand_pagePtr <<= PAGE_P_BLOCK;

#ifdef TEST_MOD
    size = ERASE_SIZE * (TEST_PAGES/64); 
	erase_num = size/(ERASE_SIZE);
#else
	erase_num = size/(ERASE_SIZE);
#endif
    erase_num++;

	printf("nand erase pagePtr is 0x%x\n", nand_pagePtr);
	printf("nand erase blocks num is %d\n", erase_num);

	command = NAND_CMD_ERASE1;

	while(erase_num > 0) {
		ls1g_nand_cmdfunc(command, 0, nand_pagePtr, 0);
		erase_num--;
		nand_pagePtr += (1 << PAGE_P_BLOCK);
	}

	printf("after erase_nand\n");
	return 0;
}


int nand_program_boot(void *fl_base, void *data_base, int data_size, int flag)
{

#ifdef OP_OOB   
    nand_program_op(fl_base, data_base, data_size, EXTRA);
#endif
#ifdef OP_MAIN
    nand_program_op(fl_base, data_base, data_size, EDC);
#endif
#ifdef OP_EAE
    nand_program_op(fl_base, data_base, data_size, EAE);

#endif    

}

int nand_program_op(void *flash_base, void *data_base, int data_size, int flag)
{
	struct ls1x_nand_info *info = &ls1A_Nand_Info;
	unsigned char *data_buf = (unsigned char *)data_base;
	int page_addr = (int)flash_base;

	unsigned int program_num;

	int op_size = 0;
	int buf_p_size = 0;

	printf("in nand_program_boot...........\n");
	printf("data_base is %x page_addr=%x\n", data_base, page_addr);

	if(flag == EDC)
		op_size = WRITE_SIZE;
	else if(flag == EXTRA)
		op_size = OOB_SIZE;
	else if(flag == EAE)
		op_size = (WRITE_SIZE + OOB_SIZE);

//#if 0
#ifdef USE_ECC
	op_size = (op_size/204)*204;
	buf_p_size = (op_size/204)*188;
#endif
	printf("op_size is %d buf_p_size=%d\n", op_size, buf_p_size);


//#if 0
#ifdef	USE_ECC
   	program_num = data_size / buf_p_size;
    if(data_size % buf_p_size != 0)
#else
   	program_num = data_size / op_size;
    if(data_size % op_size != 0)
#endif
    	program_num++;
 
	printf("program_num = %d, flag=%d\n", program_num, flag);

	while (program_num > 0) {
		ls1g_nand_cmdfunc(NAND_CMD_SEQIN, 0x00, page_addr, flag);
		memcpy(info->data_buff, data_buf, op_size);
		info->buf_count = op_size;
		ls1g_nand_cmdfunc(NAND_CMD_PAGEPROG, 0, -1, flag);

		program_num--;
		page_addr++;

//#if 0
#ifdef USE_ECC
		data_buf += buf_p_size;
#else
		data_buf += op_size;
#endif
	}
	
	return 0;
}

static unsigned char data_buff[4096];
//unsigned char *data_buff = 0xa1e00000;
int nand_read_boot(int pagePtr, void *data_buf, int flag)
{
	int nand_pagePtr;
	int i;
	struct ls1x_nand_info *info = &ls1A_Nand_Info;

	nand_pagePtr = pagePtr;

	*(volatile unsigned int *)0xbfd01160 = 0x10;        //DMA STOP
//	*(volatile unsigned int *)(0xbfe78018) = NAND_PARAM_V;// 0x00405100;

	if(flag == EXTRA)
		ls1g_nand_cmdfunc(NAND_CMD_READOOB, 0, (unsigned int)nand_pagePtr, EXTRA);
	else if(flag == EDC)
		ls1g_nand_cmdfunc(NAND_CMD_READ0, 0, (unsigned int)nand_pagePtr, EDC);
	else if(flag == EAE)
		ls1g_nand_cmdfunc(NAND_CMD_READ0, 0, (unsigned int)nand_pagePtr, EAE);

	return 0;
}


int
nand_verify_boot(void *fl_base, void *data_base, int data_size, int verbose)
{
#ifdef OP_OOB
 //   nand_verify_op(fl_base, data_base, data_size, EXTRA);
#endif 
#ifdef OP_MAIN
//    nand_verify_op(fl_base, data_base, data_size, EDC);
#endif
#ifdef OP_EAE
//    nand_verify_op(fl_base, data_base, data_size, EAE);
   
#endif    
}

int
nand_verify_op(void *fl_base, void *data_base, int data_size, int flag)
{
	unsigned char *ori_data_buff;
	unsigned char *data_buf_tmp;
	int cmp_size;
    int nand_pagePtr;
    unsigned  int data_addr;
    int i, j;
    int op_size;
    struct ls1x_nand_info *info = &ls1A_Nand_Info;
	int buf_p_size = 0;


    if(flag == EXTRA)
        op_size = OOB_SIZE;
    else if(flag == EDC)
        op_size = WRITE_SIZE;
    else
        op_size = (WRITE_SIZE + OOB_SIZE);

#ifdef USE_ECC
        op_size = (op_size/204)*204;
		buf_p_size = (op_size/204)*188;
#endif

    ori_data_buff = (unsigned char *)info->ori_data;
#ifdef TEST_MOD 
    data_addr = 0xa1380000;
    data_size = op_size * TEST_PAGES;
#else
	data_addr = (unsigned int)(&data_buff[0]);
#endif

#ifdef PAGES_P_TIME
    op_size *= PAGES_P_TIME;
#endif    

// printf("data_addr is:%x\n", data_addr);

    data_addr |= 0xa0000000;
//	info->data_buff = (unsigned char *)((unsigned int)(&data_buff[0]) | 0xa0000000);
    
//	info->data_buff_phys =(unsigned char*)( (unsigned int)(data_buf_tmp) & 0x1fffffff);
	info->data_buff_phys =(unsigned char*)( (unsigned int)(data_addr) & 0x1fffffff);

    nand_pagePtr  = (int)((int)fl_base << PAGE_P_BLOCK);
	
    printf("flag=%d\n", flag);        
	while(1)
	{
		if(data_size <= 0)
			break;	

    //    printf("data_size=%d\n", data_size);        
        if(nand_pagePtr % 128 == 0)
            printf("rd page 0x%x\n", nand_pagePtr);
        memset((unsigned char *)data_addr, 0, 16);

#ifdef	USE_ECC
   		cmp_size = data_size>buf_p_size ? buf_p_size : data_size;
#else
   		cmp_size = data_size>op_size?op_size:data_size;
#endif

		nand_read_boot(nand_pagePtr, NULL, flag);

        data_buf_tmp = data_addr;

                    //printf("&data_buf_tmp:%x\n", &data_buf_tmp[0]);
//                    printf("&ori_data_buff:%x\n", &ori_data_buff[0]);
//        info->data_buff = (unsigned char *)0xa1980000;
           for(i=0; i<cmp_size; i++)
                if(data_buf_tmp[i] != ori_data_buff[i])
                {
                    j = i;          
           
                    printf("rd error page 0x%x, i= %d !\n", nand_pagePtr, i);
                    printf("&data_buf_tmp:%x\n", &data_buf_tmp[i]);

                    for(; i<j+5; i++)
                        printf("%x ", data_buf_tmp[i]);

                    printf("\n");

                    i = j;
                    printf("ori_data_buff:%x\n", &ori_data_buff[i]);
            
                    for(; i<j+5; i++)
                        printf("%x ", ori_data_buff[i]);
    
                    printf("\n");

                    return -1;
                } 

        nand_pagePtr ++;
#ifdef	USE_ECC
        data_base += buf_p_size;
        ori_data_buff += buf_p_size;
		data_size -= buf_p_size;
#else
        data_base += cmp_size;
        ori_data_buff += cmp_size;
		data_size -= cmp_size;
#endif
        
        if(cmp_size < op_size) 
    		break;
#ifdef TEST_MOD
        if(((unsigned int)ori_data_buff - 0xa1a80000) >= (8*WRITE_SIZE))
            ori_data_buff = ((unsigned int)0xa1a80000);
#endif            
	}

	printf("nand_verify, OK\n");
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
//	writel(((info->buf_count * 188) + 3) / 4, info->dma_desc + DMA_LENGTH);
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

static void ls1g_nand_cmdfunc(unsigned command, int column, int page_addr, int flag)
{
	struct ls1x_nand_info *info = &ls1A_Nand_Info;
	unsigned cmd_prev;
	int page_prev;

	cmd_prev = info->cmd;
	page_prev = info->page_addr;

	info->cmd = command;
	info->page_addr = page_addr;

	switch(command){
	case NAND_CMD_READOOB:
	#if 0
		if (info->state == STATE_BUSY) {
			printf("nandflash chip if busy...\n");
			return;
		}
		info->state = STATE_BUSY; 
		info->buf_count = OOB_SIZE;
		info->buf_start = 0;
		info->cac_size = info->buf_count;
		if(info->buf_count <=0 )
			break;
		info->nand_regs.addrh = SPARE_ADDRH(page_addr);
		info->nand_regs.addrl = SPARE_ADDRL(page_addr)+0x800;/*0x800  ->    mtd->writesize*/
		info->nand_regs.op_num = info->buf_count;
		info->nand_regs.cmd = 0;

		((struct ls1g_nand_cmdset*)(&(info->nand_regs.cmd)))->read = 1;
		((struct ls1g_nand_cmdset*)&(info->nand_regs.cmd))->op_spare = 1;
		((struct ls1g_nand_cmdset*)&(info->nand_regs.cmd))->cmd_valid = 1;

		dma_setup(0, info);
		nand_setup(F_NAND_ADDRL|F_NAND_ADDRH|F_NAND_OP_NUM|F_NAND_CMD,info);
		break;
		#endif

	case NAND_CMD_READ0:
		if(flag == EAE)
			info->buf_count = (WRITE_SIZE + OOB_SIZE);
		else if(flag == EDC)
			info->buf_count = WRITE_SIZE;
	#ifdef USE_ECC
		info->buf_count = (info->buf_count / 204) * 204;
	#endif
	info->buf_start =  0;
		if (info->buf_count <= 0)
			break;
	#ifdef USE_ECC
	#endif
	#ifdef USE_ECC
//		info->dma_regs.length = ALIGN_DMA((WRITE_SIZE/204)*188);
		writel(((WRITE_SIZE/204)*188 + 3) / 4, info->dma_desc + DMA_LENGTH);
	#else
//		info->dma_regs.length = ALIGN_DMA(info->buf_count);
	#endif
		nand_writel(info, NAND_CMD, ECC_DMA_REQ | RS_WR | RS_RD | WAIT_RSRD_DONE | MAIN | READ);
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
		info->buf_count = OOB_SIZE + WRITE_SIZE - column;	/*mtd->oobsize + mtd->writesize - column;*/
		info->buf_start = 0;
		info->seqin_column = column;
		info->seqin_page_addr = page_addr;
		break;

	case NAND_CMD_PAGEPROG:
	#ifdef USE_ECC
		nand_writel(info, NAND_CMD, ECC_DMA_REQ | RS_WR | RS_RD | MAIN | WRITE);
	#else
		nand_writel(info, NAND_CMD, MAIN | WRITE);
	#endif
		nand_writel(info, NAND_ADDR_L, MAIN_SPARE_ADDRL(info->seqin_page_addr) + info->seqin_column);
		nand_writel(info, NAND_ADDR_H, MAIN_SPARE_ADDRH(info->seqin_page_addr));
		nand_writel(info, NAND_OPNUM, info->buf_count);
		nand_writel(info, NAND_PARAM, (nand_readl(info, NAND_PARAM) & 0xc000ffff) | (info->buf_count << 16)); /* 1C注意 */

	//#if 0
	//#if 0
	#ifdef USE_ECC
//		info->dma_regs.length = ALIGN_DMA((WRITE_SIZE/204)*188);
		writel(((WRITE_SIZE/204)*188 + 3) / 4, info->dma_desc + DMA_LENGTH);
	#else
		writel((info->buf_count + 3) / 4, info->dma_desc + DMA_LENGTH);
	#endif
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
		delay(50);
		break;
	default :
		printf("non-supported command.\n");
		delay(50);
		break;
	}
}

