#include<pmon.h>
#include<asm.h>
#include<machine/types.h>
#include<linux/mtd/mtd.h>
#include<linux/mtd/nand.h>
#include<linux/mtd/partitions.h>
#include<sys/malloc.h>

#ifndef __iomem
#define __iomem
#endif

#define DMA_ACCESS_ADDR     0x1fe78040
#define ORDER_REG_ADDR      (0xbfd01160)
#define MAX_BUFF_SIZE	4096
#define PAGE_SHIFT      12
#ifdef	LS1ASOC 
	#define NO_SPARE_ADDRH(x)   ((x))   
	#define NO_SPARE_ADDRL(x)   ((x) << (PAGE_SHIFT))
	#define SPARE_ADDRH(x)      ((x))   
	#define SPARE_ADDRL(x)      ((x) << (PAGE_SHIFT))
#else
	#define NO_SPARE_ADDRH(x)   ((x) >> (32 - (PAGE_SHIFT - 1 )))   
	#define NO_SPARE_ADDRL(x)   ((x) << (PAGE_SHIFT - 1))
	#define SPARE_ADDRH(x)      ((x) >> (32 - (PAGE_SHIFT )))   
	#define SPARE_ADDRL(x)      ((x) << (PAGE_SHIFT ))
#endif
#define ALIGN_DMA(x)       (((x)+ 3)/4)
#define CHIP_DELAY_TIMEOUT (2*HZ/10)

#define STATUS_TIME_LOOP_R  30 
#define STATUS_TIME_LOOP_WS  50  
#define STATUS_TIME_LOOP_WM  40  
#define STATUS_TIME_LOOP_E  40 
#define NAND_CMD        0x1
#define NAND_ADDRL      0x2
#define NAND_ADDRH      0x4
#define NAND_TIMING     0x8
#define NAND_IDL        0x10
#define NAND_STATUS_IDL 0x20
#define NAND_PARAM      0x40
#define NAND_OP_NUM     0X80
#define NAND_CS_RDY_MAP 0x100

#define DMA_ORDERAD     0x1
#define DMA_SADDR       0x2
#define DMA_DADDR       0x4
#define DMA_LENGTH      0x8
#define DMA_STEP_LENGTH 0x10
#define DMA_STEP_TIMES  0x20
#define DMA_CMD         0x40


enum{
    ERR_NONE        = 0,
    ERR_DMABUSERR   = -1,
    ERR_SENDCMD     = -2,
    ERR_DBERR       = -3,
    ERR_BBERR       = -4,
};

enum{
    STATE_READY = 0,
    STATE_BUSY  ,
};

struct ls1g_nand_platform_data{
        int enable_arbiter;
        struct mtd_partition *parts;
        unsigned int nr_parts;
};

struct ls1g_nand_cmdset {
        uint32_t    cmd_valid:1;
	uint32_t    read:1;
	uint32_t    write:1;
	uint32_t    erase_one:1;
	uint32_t    erase_con:1;
	uint32_t    read_id:1;
	uint32_t    reset:1;
	uint32_t    read_sr:1;
	uint32_t    op_main:1;
	uint32_t    op_spare:1;
	uint32_t    done:1;
        uint32_t    resv1:5;//11-15 reserved
        uint32_t    nand_rdy:4;//16-19
        uint32_t    nand_ce:4;//20-23
        uint32_t    resv2:8;//24-32 reserved
};

struct ls1g_nand_dma_desc{
        uint32_t    orderad;
        uint32_t    saddr;
        uint32_t    daddr;
        uint32_t    length;
        uint32_t    step_length;
        uint32_t    step_times;
        uint32_t    cmd;
};

struct ls1g_nand_dma_cmd{
        uint32_t    dma_int_mask:1;
        uint32_t    dma_int:1;
        uint32_t    dma_sl_tran_over:1;
        uint32_t    dma_tran_over:1;
        uint32_t    dma_r_state:4;
        uint32_t    dma_w_state:4;
        uint32_t    dma_r_w:1;
        uint32_t    dma_cmd:2;
        uint32_t    revl:17;
};

struct ls1g_nand_desc{
        uint32_t    cmd;
        uint32_t    addrl;
        uint32_t    addrh;
        uint32_t    timing;
        uint32_t    idl;//readonly
        uint32_t    status_idh;//readonly
        uint32_t    param;
        uint32_t    op_num;
        uint32_t    cs_rdy_map;
};

struct ls1g_nand_info {
	struct nand_chip	nand_chip;

//	struct platform_device	    *pdev;
        /* MTD data control*/
	unsigned int 		buf_start;
	unsigned int		buf_count;
        /* NAND registers*/
	void __iomem		*mmio_base;
        struct ls1g_nand_desc   nand_regs;
        unsigned int            nand_addrl;
        unsigned int            nand_addrh;
        unsigned int            nand_timing;
        unsigned int            nand_op_num;
        unsigned int            nand_cs_rdy_map;
        unsigned int            nand_cmd;

	/* DMA information */

        struct ls1g_nand_dma_desc  dma_regs;
        unsigned int            order_reg_addr;  
        unsigned int            dma_orderad;
        unsigned int            dma_saddr;
        unsigned int            dma_daddr;
        unsigned int            dma_length;
        unsigned int            dma_step_length;
        unsigned int            dma_step_times;
        unsigned int            dma_cmd;
        unsigned int		drcmr_dat;//dma descriptor address;
	unsigned int 		drcmr_dat_phys;
        size_t                  drcmr_dat_size;
	unsigned char		*data_buff;//dma data buffer;
	unsigned int 		data_buff_phys;
	size_t			data_buff_size;
        unsigned int            data_ask;
        unsigned int            data_ask_phys;
        unsigned int            data_length;
        unsigned int            cac_size;
        unsigned int            size;
        unsigned int            num;
        
	/* relate to the command */
	unsigned int		state;
//	int			use_ecc;	/* use HW ECC ? */
	size_t			data_size;	/* data size in FIFO */
        unsigned int            cmd;
        unsigned int            cmd_prev;
        unsigned int            page_addr;
//	struct completion 	cmd_complete;
        unsigned int            seqin_column;
        unsigned int            seqin_page_addr;
};

struct ls1g_nand_ask_regs{
        unsigned int dma_order_addr;
        unsigned int dma_mem_addr;
        unsigned int dma_dev_addr;
        unsigned int dma_length;
        unsigned int dma_step_length;
        unsigned int dma_step_times;
        unsigned int dma_state_tmp;
};

int ls1g_soc_nand_init(void);

static struct mtd_info *ls1g_soc_mtd = NULL;
#if 0
struct mtd_info *_soc_mtd = NULL;
#define KERNEL_AREA_SIZE 32*1024*1024
 const struct mtd_partition partition_info[] = {
//	{name ,size,offset,mask_flags }
        {"kernel",KERNEL_AREA_SIZE,0,0},
        {"os",0,KERNEL_AREA_SIZE,0},
        {(void *)0,0,0,0}
};
#endif
static struct nand_ecclayout hw_largepage_ecclayout = {
	.eccbytes = 24,
	.eccpos = {
		40, 41, 42, 43, 44, 45, 46, 47,
		48, 49, 50, 51, 52, 53, 54, 55,
		56, 57, 58, 59, 60, 61, 62, 63},
	.oobfree = { {2, 38} }
};

#define show_data_debug  0
#define show_debug(x,y)     show_debug_msk(x,y)
#define show_debug_msk(x,y)   do{ if(show_data_debug) {printk(KERN_ERR "%s:\n",__func__);show_data(x,y);} }while(0)

static void show_data(void * base,int num)
{
    int i=0;
    unsigned char *arry=( unsigned char *) base;
    printk(KERN_ERR "base==0x%08x \n",arry);
    for(i=0;i<num;i++){
        if(!(i % 32)){
            printk(KERN_ERR "\n");
        }
        if(!(i % 16)){
            printk("  ");
        }
        printk("%02x ",arry[i]);
    }
    printk(KERN_ERR "\n");
    
}



static int ls1g_nand_ecc_calculate(struct mtd_info *mtd,
		const uint8_t *dat, uint8_t *ecc_code)
{
	return 0;
}
static int ls1g_nand_ecc_correct(struct mtd_info *mtd,
		uint8_t *dat, uint8_t *read_ecc, uint8_t *calc_ecc)
{
	struct ls1g_nand_info *info = mtd->priv;
	/*
	 * Any error include ERR_SEND_CMD, ERR_DBERR, ERR_BUSERR, we
	 * consider it as a ecc error which will tell the caller the
	 * read fail We have distinguish all the errors, but the
	 * nand_read_ecc only check this function return value
	 */
	return 0;
}

static void ls1g_nand_ecc_hwctl(struct mtd_info *mtd, int mode)
{
	return;
}

static int ls1g_nand_waitfunc(struct mtd_info *mtd, struct nand_chip *this)
{
    udelay(50);
    return 0;
}

static void ls1g_nand_select_chip(struct mtd_info *mtd, int chip)
{
	return;
}

static int ls1g_nand_dev_ready(struct mtd_info *mtd)
{
	return 1;
}

static void ls1g_nand_read_buf(struct mtd_info *mtd, uint8_t *buf, int len)
{
	struct ls1g_nand_info *info = mtd->priv;
	int i,real_len = min_t(size_t, len, info->buf_count - info->buf_start);

	memcpy(buf, info->data_buff + info->buf_start, real_len);
	show_debug(info->data_buff,0x40);
	info->buf_start += real_len;
}

static u16 ls1g_nand_read_word(struct mtd_info *mtd)
{
	struct ls1g_nand_info *info = mtd->priv;
	u16 retval = 0xFFFF;
	if(!(info->buf_start & 0x1) && info->buf_start < info->buf_count){
		retval = *(u16 *)(info->data_buff + info->buf_start);
	}
	info->buf_start += 2;
	return retval;
}

static uint8_t ls1g_nand_read_byte(struct mtd_info *mtd)
{
	struct ls1g_nand_info *info = mtd->priv;
	char retval = 0xFF;

	if (info->buf_start < info->buf_count)
	/* Has just send a new command? */
		retval = info->data_buff[(info->buf_start)++];
	show_debug(info->data_buff,6);
	return retval;
}

static void ls1g_nand_write_buf(struct mtd_info *mtd,const uint8_t *buf, int len)
{
	int i;
	struct ls1g_nand_info *info = mtd->priv;
	int real_len = min_t(size_t, len, info->buf_count - info->buf_start);
//	info->buf_count = real_len;

	memcpy(info->data_buff + info->buf_start, buf, real_len);
	show_debug(info->data_buff,0x20);
	info->buf_start += real_len;
}

static int ls1g_nand_verify_buf(struct mtd_info *mtd,const uint8_t *buf, int len)
{
	int i=0; 
	struct ls1g_nand_info *info = mtd->priv;
	show_debug(info->data_buff,0x20);
	while(len--){
		if(buf[i++] != ls1g_nand_read_byte(mtd) ){
			printk("?????????????verify error..., i= %d !\n\n", i-1);
			return -1;
		}
	}
	return 0;
}

static void ls1g_nand_cmdfunc(struct mtd_info *mtd, unsigned command,int column, int page_addr);
static void ls1g_nand_init_mtd(struct mtd_info *mtd,struct ls1g_nand_info *info);

int ls1g_nand_init(struct mtd_info *mtd)
{
	int ret=0;
	ret = ls1g_nand_pmon_info_init(mtd->priv, mtd);
	ls1g_nand_init_mtd(mtd, (struct ls1g_nand_info *)(mtd->priv));
	return ret;
}

static void ls1g_nand_init_mtd(struct mtd_info *mtd, struct ls1g_nand_info *info)
{
	struct nand_chip *this = &info->nand_chip;
	
	//(各种芯片选项)在一定程度上设置用于告诉nand_scan函数有关特殊的函数操作.
	this->options = 8;//(f->flash_width == 16) ? NAND_BUSWIDTH_16: 0;

	this->waitfunc		= ls1g_nand_waitfunc;	/*等待设备准备好 硬件相关函数*/
	this->select_chip		= ls1g_nand_select_chip;	/*控制CE信号*/
	this->dev_ready		= ls1g_nand_dev_ready;	/*板特定的设备ready/busy信息*/
	this->cmdfunc			= ls1g_nand_cmdfunc;		/*命令处理函数*/
	this->read_word		= ls1g_nand_read_word;	/*从芯片读一个字*/
	this->read_byte		= ls1g_nand_read_byte;	/*从芯片读一个字节*/
	this->read_buf		= ls1g_nand_read_buf;	/*将芯片数据读到缓冲区*/
	this->write_buf		= ls1g_nand_write_buf;	/*将缓冲区内容写入芯片*/
	this->verify_buf		= ls1g_nand_verify_buf;	/*验证芯片和写入缓冲区中的数据*/

//	this->ecc.mode		= NAND_ECC_NONE;
	this->ecc.mode		= NAND_ECC_SOFT;	/*ECC模式 这里是软件模式*/
//	this->ecc.hwctl		= ls1g_nand_ecc_hwctl;
//	this->ecc.calculate	= ls1g_nand_ecc_calculate;
//	this->ecc.correct		= ls1g_nand_ecc_correct;
//	this->ecc.size		= 2048;
//	this->ecc.bytes		= 24;

//	this->ecc.layout		= &hw_largepage_ecclayout;
//	mtd->owner				= THIS_MODULE;
}

#define write_z_cmd  do{                                    \
            *((volatile unsigned int *)(0xbfe78000)) = 0;   \
            *((volatile unsigned int *)(0xbfe78000)) = 0;   \
            *((volatile unsigned int *)(0xbfe78000)) = 400; \
    }while(0)


static unsigned ls1g_nand_status(struct ls1g_nand_info *info)
{
    struct ls1g_nand_desc *nand_regs = (volatile struct ls1g_nand_desc *)(info->mmio_base);
    struct ls1g_nand_cmdset *nand_cmd = (struct ls1g_nand_cmdset *)(&(nand_regs->cmd));
    udelay(100);
    return(nand_cmd->done);
}

/*
 *  flags & 0x1   orderad
 *  flags & 0x2   saddr
 *  flags & 0x4   daddr
 *  flags & 0x8   length
 *  flags & 0x10  step_length
 *  flags & 0x20  step_times
 *  flags & 0x40  cmd
 ***/
static void show_dma_regs(void *dma_regs,int flag)
{
    unsigned int *regs=dma_regs;
    printf("\n");
    printf("0x%08x:0x%08x\n",regs,*regs);
    printf("0x%08x:0x%08x\n",++regs,*regs);
    printf("0x%08x:0x%08x\n",++regs,*regs);
    printf("0x%08x:0x%08x\n",++regs,*regs);
    printf("0x%08x:0x%08x\n",++regs,*regs);
    printf("0x%08x:0x%08x\n",++regs,*regs);
    printf("0x%08x:0x%08x\n",++regs,*regs);
    if(flag)
    printf("0xbfd01160:0x%08x\n",*(volatile unsigned int *)0xbfd01160);
}
/*before DMA rw=1(hit writeback and invalidated)  ;after DMA rw=0(invalidated)*/
#define  __dma_write_ddr2(cmd,bit) (~((cmd)&(1<<bit)))
static void dma_cache_nand(struct ls1g_nand_info *info,unsigned char rw )
{
    struct ls1g_nand_dma_desc *dma_base = (volatile struct ls1g_nand_dma_desc *)(info->drcmr_dat);

    if(__dma_write_ddr2(dma_base->cmd,12))
    {
//        CPU_IOFlushDCache(((dma_base->saddr)&0x1fffffff)|0x80000000,(dma_base->length)*4,rw);
    }
    
}

static void nand_cache_inv(unsigned long base,unsigned long num)
{
    
    CPU_IOFlushDCache((base & 0x1fffffff)|0x80000000,num,0);
}

static void nand_cache_wb(unsigned long base,unsigned long num)
{
    CPU_IOFlushDCache((base & 0x1fffffff)|0x80000000,num,1);
}


unsigned int data_bak[2112];
static void dma_setup(unsigned int flags,struct ls1g_nand_info *info)
{
	int order;
//	int k;
//	unsigned int *data_uncache;

	struct ls1g_nand_dma_desc *dma_base = (volatile struct ls1g_nand_dma_desc *)(info->drcmr_dat);
	dma_base->orderad = (flags & DMA_ORDERAD)== DMA_ORDERAD ? info->dma_regs.orderad : info->dma_orderad;
	dma_base->saddr = (flags & DMA_SADDR)== DMA_SADDR ? info->dma_regs.saddr : info->dma_saddr;
	dma_base->daddr = (flags & DMA_DADDR)== DMA_DADDR ? info->dma_regs.daddr : info->dma_daddr;
	dma_base->length = (flags & DMA_LENGTH)== DMA_LENGTH ? info->dma_regs.length: info->dma_length;
	info->data_length = info->dma_regs.length;
	dma_base->step_length = (flags & DMA_STEP_LENGTH)== DMA_STEP_LENGTH ? info->dma_regs.step_length: info->dma_step_length;
	dma_base->step_times = (flags & DMA_STEP_TIMES)== DMA_STEP_TIMES ? info->dma_regs.step_times: info->dma_step_times;
	dma_base->cmd = (flags & DMA_CMD)== DMA_CMD ? info->dma_regs.cmd: info->dma_cmd;
	/*flush cache before DMA operation*/
	if((dma_base->cmd)&(0x1 << 12)){
		nand_cache_wb((unsigned long)(info->data_buff),info->cac_size);
#if 0
		memcpy((unsigned char *)data_bak, info->data_buff, info->cac_size);
		data_uncache = (unsigned int *)(((unsigned int)info->data_buff) | 0xa0000000);
		printk ("data_uncache's addr= 0x%x ,data_buff's addr= 0x%x !\n", data_uncache, info->data_buff);
		for (k=0; k<info->cac_size/4; k++)	//lxy
		{
			if (data_uncache[k] != data_bak[k])
				printk ("lxy: cache flush fail......, 0x%x --> 0x%x !\n", data_uncache[k], data_bak[k]);
		}
#endif
	}
	nand_cache_wb((unsigned long)(info->drcmr_dat),0x20);

	*(volatile unsigned int *)(info->order_reg_addr) = ((unsigned int )info->drcmr_dat_phys) | 0x1<<3;
//	memset(&(info->dma_regs),0,sizeof(struct ls1g_nand_dma_desc));
}

static void dma_ask(struct ls1g_nand_info *info)
{
	memset((char *)info->data_ask,0,sizeof(struct ls1g_nand_ask_regs));
	*(volatile unsigned int *)info->order_reg_addr = 0x1<<2|(info->data_ask_phys)& 0xfffffff0;
//	show_dma_regs((void *)(info->data_ask),1);
}

/**
 *  flags & 0x1     cmd
 *  flags & 0x2     addrl
 *  flags & 0x4     addrh
 *  flags & 0x8     timing
 *  flags & 0x10    idl
 *  flags & 0x20    status_idh
 *  flags & 0x40    param
 *  flags & 0x80    op_num
 *  flags & 0x100   cs_rdy_map
 ****/
static void nand_setup(unsigned int flags ,struct ls1g_nand_info *info)
{
//	printk("addrl+++++++++++++++++++++==%x\n\n",info->nand_regs.addrl);
	int i; 
	struct ls1g_nand_desc *nand_base = (struct ls1g_nand_desc *)(info->mmio_base);
	nand_base->cmd = 0;
	nand_base->addrl = (flags & NAND_ADDRL)==NAND_ADDRL ? info->nand_regs.addrl: info->nand_addrl;
	nand_base->addrh = (flags & NAND_ADDRH)==NAND_ADDRH ? info->nand_regs.addrh: info->nand_addrh;
	nand_base->timing = (flags & NAND_TIMING)==NAND_TIMING ? info->nand_regs.timing: info->nand_timing;
	nand_base->op_num = (flags & NAND_OP_NUM)==NAND_OP_NUM ? info->nand_regs.op_num: info->nand_op_num;	//NAND读写操作Byte数；擦除为块数
	nand_base->cs_rdy_map = (flags & NAND_CS_RDY_MAP)==NAND_CS_RDY_MAP ? info->nand_regs.cs_rdy_map: info->nand_cs_rdy_map;
	if(flags & NAND_CMD){
		nand_base->cmd = (info->nand_regs.cmd) &(~0xff);
		nand_base->cmd = info->nand_regs.cmd;
		if(info->nand_regs.cmd & 0x20){
			i = 100;
			//判断done操作完成 nand_cmd 0xbfe78000
			while(!ls1g_nand_status(info)){
				if(!(i--)){
					write_z_cmd; //写操作
					break;
				}
				udelay(30);
			}
			*(int *)(info->data_buff) = nand_base->idl;
		}
	}
	else
		nand_base->cmd = info->nand_cmd;
}

static  int sync_dma(struct ls1g_nand_info *info)
{
	int *end ;
	struct ls1g_nand_ask_regs *ask = info->data_ask;
	end = ((unsigned int)(info->data_buff_phys)&0x1fffffff) + info->data_length*4;
	while(1){
		dma_ask(info);
		udelay(100);
		if(ask->dma_mem_addr == end)
			break;
	}
	/*flush cache after DMA operation*/
//	dma_cache_nand(info,0);
	return 0;
}

static void ls1g_nand_cmdfunc(struct mtd_info *mtd, unsigned command,int column, int page_addr)
{
	struct ls1g_nand_info *info = mtd->priv;
	int ret,i,nandcmd;
	unsigned cmd_prev;
	int status_time,page_prev;
//        int timeout = CHIP_DELAY_TIMEOUT;
	unsigned int base;
//        init_completion(&info->cmd_complete);
	static int ccc=0;
	
	cmd_prev = info->cmd;
	page_prev = info->page_addr;
	info->cmd = command;
	info->page_addr = page_addr;
	//show_dma_regs((void *)(info->mmio_base),0); 
	switch(command){
		case NAND_CMD_READOOB:
//			printf ("in NAND_CMD_READOOB! \n");
			if(info->state == STATE_BUSY){
				printk("nandflash chip if busy...\n");
				return;
			}
			info->state = STATE_BUSY;
			info->buf_count = mtd->oobsize;
			info->buf_start = 0;
			info->cac_size = info->buf_count;
			if(info->buf_count <=0 )
				break;
			/*nand regs set*/
			info->dma_regs.cmd = 0;		//下一个描述符地址寄存器
			info->nand_regs.cmd = 0;		//
			info->nand_regs.addrh = SPARE_ADDRH(page_addr);	//读、写、擦除操作起始地址高8位
			info->nand_regs.addrl = SPARE_ADDRL(page_addr) + mtd->writesize;	//读、写、擦除操作起始地址低32位
//			printf ("addrh = 0x%x , addrl = 0x%x !\n", info->nand_regs.addrh, info->nand_regs.addrl);
			info->nand_regs.op_num = info->buf_count;	//NAND读写操作Byte数；擦除为块数
			/*nand cmd set */ 
			((struct ls1g_nand_cmdset*)&(info->nand_regs.cmd))->read = 1;
			((struct ls1g_nand_cmdset*)&(info->nand_regs.cmd))->op_spare = 1;
			((struct ls1g_nand_cmdset*)&(info->nand_regs.cmd))->cmd_valid = 1;
			/*dma regs config*/
			info->dma_regs.length = ALIGN_DMA(info->buf_count);	//传输数据长度寄存器 代表一块被搬运内容的长度，单位是字
			((struct ls1g_nand_dma_cmd *)&(info->dma_regs.cmd))->dma_int_mask = 0;
			/*dma GO set*/       
			dma_setup(DMA_LENGTH|DMA_CMD, info);
			nand_setup(NAND_ADDRL|NAND_ADDRH|NAND_OP_NUM|NAND_CMD, info);
			sync_dma(info);
		break;
	#if 0            
		case NAND_CMD_READOOB:
		info->state = STATE_BUSY; 
		info->buf_count = mtd->oobsize - column;
		info->buf_start = 0;
		if(info->buf_count <=0 )
		break;
		/*nand regs set*/
		info->nand_regs.addrh =  page_addr >> (32 - PAGE_SHIFT);

		info->nand_regs.addrl = (page_addr << PAGE_SHIFT) + column + 2048;
		info->nand_regs.op_num = info->buf_count;
		/*nand cmd set */
		info->nand_regs.cmd=0;
		((struct ls1g_nand_cmdset*)&(info->nand_regs.cmd))->read = 1;
		((struct ls1g_nand_cmdset*)&(info->nand_regs.cmd))->op_spare = 1;
		((struct ls1g_nand_cmdset*)&(info->nand_regs.cmd))->cmd_valid = 1;
		/*dma regs config*/
		info->dma_regs.length =0;
		info->dma_regs.cmd = 0;
		info->dma_regs.length = (info->buf_count + 3)/4;
		((struct ls1g_nand_dma_cmd *)&(info->dma_regs.cmd))->dma_int_mask = 0;
		/*dma GO set*/       
		dma_setup(DMA_LENGTH|DMA_CMD,info);
		nand_setup(NAND_ADDRL|NAND_ADDRH|NAND_OP_NUM|NAND_CMD,info);
		//                printf("\ncmdfunc\n");
		sync_dma(info);
		break;
	#endif     
		case NAND_CMD_READ0:
			if(info->state == STATE_BUSY){
				printk("nandflash chip if busy...\n");
				return;
			}
			info->state = STATE_BUSY;
			info->buf_count = mtd->oobsize + mtd->writesize ;
			info->buf_start =  0 ;
			info->cac_size = info->buf_count;
			if(info->buf_count <=0 )
				break;
			info->nand_regs.addrh = SPARE_ADDRH(page_addr);
			info->nand_regs.addrl = SPARE_ADDRL(page_addr);
			info->nand_regs.op_num = info->buf_count;
			/*nand cmd set */ 
			info->nand_regs.cmd = 0; 
			info->dma_regs.cmd = 0;
			((struct ls1g_nand_cmdset*)&(info->nand_regs.cmd))->read = 1;
			((struct ls1g_nand_cmdset*)&(info->nand_regs.cmd))->op_spare = 1;
			((struct ls1g_nand_cmdset*)&(info->nand_regs.cmd))->op_main = 1;
			((struct ls1g_nand_cmdset*)&(info->nand_regs.cmd))->cmd_valid = 1; 
			/*dma regs config*/
			info->dma_regs.length = ALIGN_DMA(info->buf_count);
			((struct ls1g_nand_dma_cmd *)&(info->dma_regs.cmd))->dma_int_mask = 0;
			dma_setup(DMA_LENGTH|DMA_CMD,info);
			nand_setup(NAND_ADDRL|NAND_ADDRH|NAND_OP_NUM|NAND_CMD,info);
			sync_dma(info);
		break;
		case NAND_CMD_SEQIN:
			if(info->state == STATE_BUSY){
				printk("nandflash chip if busy...\n");
				return;
			}
			info->state = STATE_BUSY;
			info->buf_count = mtd->oobsize + mtd->writesize - column;
			info->buf_start = 0;
			info->seqin_column = column;

			info->seqin_page_addr = page_addr;
			//                complete(&info->cmd_complete);
		break;
		case NAND_CMD_PAGEPROG:
			if(info->state == STATE_BUSY){
				printf("nandflash chip if busy...\n");
				return;
			}
			info->state = STATE_BUSY;
			if(cmd_prev != NAND_CMD_SEQIN){
				printf("Prev cmd don't complete...\n");
				break;
			}
			if(info->buf_count <= 0 )
				break;

			if(((info->num)++) % 512 == 0){
				printk("nand have write : %d M\n",(info->size)++); 
			}

			/*nand regs set*/
			info->nand_regs.addrh =  SPARE_ADDRH(info->seqin_page_addr);
			info->nand_regs.addrl =  SPARE_ADDRL(info->seqin_page_addr) + info->seqin_column;
			info->nand_regs.op_num = info->buf_start;//info->buf_count;//	lxy

			/*nand cmd set */ 
			info->nand_regs.cmd = 0; 
			info->dma_regs.cmd = 0;
			((struct ls1g_nand_cmdset*)&(info->nand_regs.cmd))->write = 1;
			if(info->seqin_column < mtd->writesize)
				((struct ls1g_nand_cmdset*)&(info->nand_regs.cmd))->op_main = 1;
			((struct ls1g_nand_cmdset*)&(info->nand_regs.cmd))->op_spare = 1;
			((struct ls1g_nand_cmdset*)&(info->nand_regs.cmd))->cmd_valid = 1; 

			/*dma regs config*/
			info->dma_regs.length = ALIGN_DMA(info->buf_start);
			((struct ls1g_nand_dma_cmd *)&(info->dma_regs.cmd))->dma_int_mask = 0;
			((struct ls1g_nand_dma_cmd *)&(info->dma_regs.cmd))->dma_r_w = 1;
			nand_setup(NAND_ADDRL|NAND_ADDRH|NAND_OP_NUM|NAND_CMD,info);
			dma_setup(DMA_LENGTH|DMA_CMD,info);
			sync_dma(info);
			while(!ls1g_nand_status(info)){		//lxy
//				if(!(status_time--)){
//					write_z_cmd;
//					break;
//				}
				udelay(50);
			}
		break;
		case NAND_CMD_RESET:
			info->state = STATE_BUSY;
			/*nand cmd set */ 
			((struct ls1g_nand_cmdset*)&(info->nand_regs.cmd))->reset = 1;
			((struct ls1g_nand_cmdset*)&(info->nand_regs.cmd))->cmd_valid = 1; 
			nand_setup(NAND_CMD,info);
			status_time = STATUS_TIME_LOOP_R;
			while(!ls1g_nand_status(info)){
				if(!(status_time--)){
					write_z_cmd;
					break;
				}
				udelay(50);
			}

			info->state = STATE_READY;
			//                complete(&info->cmd_complete);
		break;
		case NAND_CMD_ERASE1:
			info->state = STATE_BUSY;
			/*nand regs set*/
			info->nand_regs.addrh =  NO_SPARE_ADDRH(page_addr);
			info->nand_regs.addrl =  NO_SPARE_ADDRL(page_addr) ;
			/*nand cmd set */ 
			info->nand_regs.cmd = 0; 
			((struct ls1g_nand_cmdset*)&(info->nand_regs.cmd))->erase_one = 1;
			((struct ls1g_nand_cmdset*)&(info->nand_regs.cmd))->cmd_valid = 1; 
			nand_setup(NAND_ADDRL|NAND_ADDRH|NAND_OP_NUM|NAND_CMD,info);
			status_time = STATUS_TIME_LOOP_E;
			udelay(3000);    
			while(!ls1g_nand_status(info)){		//lxy
				if(!(status_time--)){
					write_z_cmd;
					break;
				}
				udelay(50);
			}

			info->state = STATE_READY;
			//                complete(&info->cmd_complete);
		break;
		case NAND_CMD_STATUS:
			info->buf_count = 0x1;
			info->buf_start = 0x0;
			*(unsigned char *)info->data_buff=ls1g_nand_status(info) | 0x80;
			//                complete(&info->cmd_complete);
		break;
		case NAND_CMD_READID:
			if(info->state == STATE_BUSY){
				printf("nandflash chip if busy...\n");
				return;
			}
			info->state = STATE_BUSY;
			info->buf_count = 0x4;
			info->buf_start = 0;
			{
			#define  _NAND_IDL      ( *((volatile unsigned int*)(0xbfe78010)))
			#define  _NAND_IDH       (*((volatile unsigned int*)(0xbfe78014)))
			#define  _NAND_BASE      0xbfe78000
			#define  _NAND_SET_REG(x,y)   do{*((volatile unsigned int*)(_NAND_BASE+x)) = (y);}while(0)                           
			#define  _NAND_READ_REG(x,y)  do{(y) =  *((volatile unsigned int*)(_NAND_BASE+x));}while(0) 

			unsigned int id_val_l=0,id_val_h=0;
			unsigned int timing = 0;
			unsigned char *data = (unsigned char *)(info->data_buff);
			_NAND_READ_REG(0xc,timing);
			_NAND_SET_REG(0xc,0x30f0); 
			_NAND_SET_REG(0x0,0x21); 

			while(((id_val_l |= _NAND_IDL) & 0xff)  == 0){
				id_val_h = _NAND_IDH;
			}

			while (id_val_h == 0)	//lxy
			{
				id_val_h = _NAND_IDH;
			}

			//printk("id_val_l=0x%08x\nid_val_h=0x%08x\n",id_val_l,id_val_h);
			_NAND_SET_REG(0xc,timing);
			data[0]  = (id_val_h & 0xff);
			data[1]  = (id_val_l & 0xff000000)>>24;
			data[2]  = (id_val_l & 0x00ff0000)>>16;
			data[3]  = (id_val_l & 0x0000ff00)>>8;
			//printk(KERN_ERR "IDS=============================0x%x\n",*((int *)(info->data_buff)));

			}

		#if 0 
			((struct ls1g_nand_cmdset*)&(info->nand_regs.cmd))->read_id = 1;
			((struct ls1g_nand_cmdset*)&(info->nand_regs.cmd))->cmd_valid = 1;             
			nand_setup(NAND_CMD,info);
			status_time = STATUS_TIME_LOOP_R;
			while(!ls1g_nand_status(info)){
			if(!(status_time--)){
			write_z_cmd;
			break;
			}
			udelay(30);
			}
		#endif
		break;
		case NAND_CMD_ERASE2:
		case NAND_CMD_READ1:
		break;
		default :
			printf(KERN_ERR "non-supported command.\n");
		break;
	}

	if(info->cmd == NAND_CMD_READ0 || info->cmd == NAND_CMD_READOOB ){
		nand_cache_inv((unsigned long)(info->data_buff),info->cac_size);
	}

	info->state = STATE_READY;
}

int ls1g_nand_detect(struct mtd_info *mtd)
{
        printf("NANDFlash info:\nerasesize\t%d B\nwritesize\t%d B\noobsize  \t%d B\n",mtd->erasesize, mtd->writesize,mtd->oobsize );
        return (mtd->erasesize != 1<<17 || mtd->writesize != 1<<11 || mtd->oobsize != 1<<6);

}
static void ls1g_nand_init_info(struct ls1g_nand_info *info)
{
//    *((volatile unsigned int *)0xbfe78018) = 0x30000;	//外部颗粒容量大小
    info->num=0;
    info->size=0;
    info->cac_size = 0; 
    info->state = STATE_READY;

    info->cmd_prev = -1;
    info->page_addr = -1;
    info->nand_addrl = 0x0;
    info->nand_addrh = 0x0;
    info->nand_timing = 0x4<<8 | 0x12;
    info->nand_op_num = 0x0;
    info->nand_cs_rdy_map = 0x00000000;
    info->nand_cmd = 0;

    info->dma_orderad = 0xa0811000;
    info->dma_saddr = info->data_buff_phys;
    info->dma_daddr = DMA_ACCESS_ADDR;
    info->dma_length = 0x0;
    info->dma_step_length = 0x0;
    info->dma_step_times = 0x1;
    info->dma_cmd = 0x0;

    info->order_reg_addr = ORDER_REG_ADDR;
}

int ls1g_nand_pmon_info_init(struct ls1g_nand_info *info, struct mtd_info *mtd)
{
	info->drcmr_dat = 0xa0400000;	//DMA描述符地址
//	info->drcmr_dat = (unsigned int)(malloc(sizeof(struct ls1g_nand_dma_desc)+32, M_DMAMAP, M_WAITOK));
//	info->drcmr_dat = ((info->drcmr_dat+0x1f)&(~0x1f)) & 0xfffffff | 0xa0000000;
	if(info->drcmr_dat == NULL)
		return -1;
	info->drcmr_dat_phys = (info->drcmr_dat) & 0x1fffffff;	//DMA描述符物理地址

	info->mmio_base = 0x1fe78000 | 0xa0000000; //NAND寄存器基地址

	info->data_buff = (unsigned char *)0xa0500000;//malloc(MAX_BUFF_SIZE,M_DMAMAP,M_WAITOK);	//DMA数据缓存
//	info->data_buff = ((unsigned int)(malloc(MAX_BUFF_SIZE+32, M_DMAMAP, M_WAITOK)) + 0x1f) & (~0x1f);
//	info->data_buff = (unsigned char *)((((unsigned int)info->data_buff+0x1f)&(~0x1f)) & 0xfffffff | 0xa0000000);
	if(info->data_buff == NULL)
		return -1;

	info->data_buff_phys = (unsigned int)(info->data_buff) & 0x1fffffff;	//DAM数据缓存物理地址
	printf("data_buff = 0x%08x\ndata_buff_phys = 0x%08x\n", info->data_buff, info->data_buff_phys);

	info->data_ask = 0xa0600000;
//	info->data_ask = (unsigned int)(malloc(sizeof(struct ls1g_nand_ask_regs)+32, M_DMAMAP, M_WAITOK));
//	info->data_ask = (((info->data_ask+0x1f)&(~0x1f)) & 0xfffffff | 0xa0000000);
	if(info->data_ask == NULL)
		return -1;
	info->data_ask_phys = info->data_ask & 0x1fffffff;

	ls1g_nand_init_info(info);
	/*
	if(ls1g_nand_detect(mtd)){
		printk(KERN_ERR "PMON driver don't support the NANDFlash!\n");
		return -1;
	}
	*/
	return 0;
}

static void find_good_part(struct mtd_info *ls1g_soc_mtd)
{
	int offs;
	int start=-1;
	char name[20];
	int idx=0;
	
	for(offs=0;offs< ls1g_soc_mtd->size;offs+=ls1g_soc_mtd->erasesize){
		if(ls1g_soc_mtd->block_isbad(ls1g_soc_mtd,offs)&& start>=0){
			sprintf(name,"g%d",idx++);
			add_mtd_device(ls1g_soc_mtd,start,offs-start,name);
			start=-1;
		}
		else if(start<0){
			start=offs;
		}
	}

	if(start>=0){
		sprintf(name,"g%d",idx++);
		add_mtd_device(ls1g_soc_mtd,start,offs-start,name);
	}
}

#define __ww(addr,val)  *((volatile unsigned int*)(addr)) = (val)
#define __rw(addr,val)  val =  *((volatile unsigned int*)(addr))
#define __display(addr,num) do{for(i=0;i<num;i++){printf("0x%08x:0x%08x\n",(addr+i*sizeof(int)),*((volatile unsigned int*)(addr+i*sizeof(int))));}}while(0)
#define NAND_REG_BASE   0xbfe78000
#define NAND_DEV        0x1fe78040
#define DMA_DESP        0xa0800000
#define DMA_DESP_ORDER  0x00800008
#define DMA_ASK_ORDER   0x00800004
#define DDR_PHY         0x02400000
#define DDR_ADDR        0xa2400000

#define STATUS_TIME 100

#define cmd_to_zero  do{            \
            __ww(NAND_REG_BASE,0);  \
            __ww(NAND_REG_BASE,0);  \
            __ww(NAND_REG_BASE,0x400);  \
}while(0)

//验证擦除
unsigned int verify_erase(unsigned int addr,int all)
{
	int i=0,flag=0;
	volatile unsigned int *base = (unsigned int*)DDR_ADDR;
	unsigned int val=0;
	
	//#define DMA_DESP        0xa0800000
	__ww(DMA_DESP+0, 0);				//下一个描述符地址寄存器 这里设置为无效
	__ww(DMA_DESP+0x4, DDR_PHY);	//内存地址寄存器
	__ww(DMA_DESP+0x8, NAND_DEV);	//设备地址寄存器
	__ww(DMA_DESP+0xc, 0x8400);		//长度寄存器 代表一块被搬运内容的长度，单位是字
	__ww(DMA_DESP+0x10, 0x0);		//间隔长度寄存器 间隔长度说明两块被搬运内存数据块之间的长度，前一个step的结束地址与后一个step的开始地址之间的间隔
	__ww(DMA_DESP+0x14, 0x1);		//循环次数寄存器 循环次数说明在一次DMA操作中需要搬运的块的数目
	__ww(DMA_DESP+0x18, 0x0);		//控制寄存器
	__ww(0xbfd01160, DMA_DESP_ORDER);	//DMA模块控制寄存器位 在confreg模块，存放第一个DMA描述符地址寄存器
											//#define DMA_DESP_ORDER  0x00800008
											//可以开始读描述符链的第一个DMA描述符

	__ww(NAND_REG_BASE+0x0, 0x0);
	__ww(NAND_REG_BASE+0x4, addr<<(12+6));//读、写、擦除操作起始地址低32位
	__ww(NAND_REG_BASE+0x1c, 0x21000);//main + spare NAND读写操作Byte数；擦除为块数
	__ww(NAND_REG_BASE+0x0, 0x300);	//操作发生在NAND的SPARE区 操作发生在NAND的MAIN区
	__ww(NAND_REG_BASE+0x0, 0x303);	//读操作 命令有效

	udelay(5000);
	while(1){
		//#define DMA_ASK_ORDER   0x00800004
		__ww(0xbfd01160, DMA_ASK_ORDER);//用户请求将当前DMA操作的相关信息写回到指定的内存地址
		__rw(DMA_DESP+0x4, val);	//读取DMA内存的值到val中
		if(val == (0x21000+DDR_PHY)){break;}
		udelay(400);
	}
	for(;i<0x8400;i++){
		if(*(base+i) != 0xffffffff){printf("\naddr 0x%08x: 0x%08x isn't 0xFFFFFFFF",(base+i),*(base+i));flag=1;if(all){return flag;}}
	}
	return flag;
}

void nanderase_verify(int argc, char ** argv)
{
	int i=0, flag=0;
	int status_time;
	char *detail;
//	all =strtoul(argv[1],0,0);
	detail = argv[1];
	flag = strncmp(detail, "detail", 6);
	__ww(NAND_REG_BASE+0x0, 0x0);
	__ww(NAND_REG_BASE+0x0, 0x41);	//NAND复位，命令有效
	__ww(NAND_REG_BASE+0x4, 0x00);	//读、写、擦除操作起始地址低32位
	status_time = STATUS_TIME;

	printf("erase blockaddr: 0x%08x", i);
	for(i=0; i<1024; i++){
		printf("\b\b\b\b\b\b\b\b");
		printf("%08x", i<<(11+6));
		__ww(NAND_REG_BASE+0x4, i<<(11+6));	//128K？
		__ww(NAND_REG_BASE+0x0, 0x8);	//擦除操作
		__ww(NAND_REG_BASE+0x0, 0x9);	//擦除操作 命令有效
		udelay(2000);
		//外部NAND芯片RDY情况
		while((*((volatile unsigned int *)(NAND_REG_BASE)) & 0x1<<16) == 0){
			if(!(status_time--)){
				cmd_to_zero;	//操作完成
				status_time = STATUS_TIME;
				break;
			}
			udelay(80);
		}
		//验证擦除
		if(verify_erase(i,flag)){printf("BLOCK:%d,addr:0x%08x,some error or bad block\n", i, i<<(11+6));}
	}
	printf("\nerase all nandflash ok...\n");

#if 0  
//	__ww(NAND_REG_BASE+0x0,0x9);
//	udelay(1000000);
//	__display(NAND_REG_BASE,0x10);
//    return 0;  

//    __ww(NAND_REG_BASE+0x0,0x0);
//    __display(0xbfd00420,0x10);
//        udelay(1000000);
//    __display(NAND_REG_BASE,0x1);  
//    __ww(NAND_REG_BASE+0x0,0x0);
//    __display(NAND_REG_BASE,0x1);

__ww(DMA_DESP,0xa0081100); 
__ww(DMA_DESP+0x4,DDR_PHY); 
__ww(DMA_DESP+0x8,NAND_DEV); 
__ww(DMA_DESP+0xc,0x10);
__ww(DMA_DESP+0x10,0x0); 
__ww(DMA_DESP+0x14,0x1); 
__ww(DMA_DESP+0x18,0x0);
//    udelay(1000000);
__ww(0xbfd01160,DMA_DESP_ORDER);
//    udelay(1000000);
__display(DMA_DESP,0x10);
__display(0xbfd01160,0x1);
//    udelay(1000000);
__ww(NAND_REG_BASE+0x4,0x800); 
__ww(NAND_REG_BASE+0x1c,0x40); 
//    udelay(1000000);
__display(NAND_REG_BASE,0X4); 
__ww(NAND_REG_BASE,0x203);
//    udelay(1000000);
__display(NAND_REG_BASE,0X4); 
__ww(0xbfd01160,0x8000004);
__display(0xa8000000,0x8); 
__display(DDR_ADDR,0x10);

//   while(1);
#endif
}

void nanderase(void)
{
	int i=0;
	int status_time ;

	__ww(NAND_REG_BASE+0x0, 0x0);   
	__ww(NAND_REG_BASE+0x0, 0x41);
	__ww(NAND_REG_BASE+0x4, 0x00);

	status_time = STATUS_TIME;

	printf("erase blockaddr: 0x%08x", i);
	for (i=0; i<1024; i++) {
		printf("\b\b\b\b\b\b\b\b");
		printf("%08x", i<<(11+6));
		__ww(NAND_REG_BASE+0x4, i<<(11+6));
		__ww(NAND_REG_BASE+0x0, 0x8);
		__ww(NAND_REG_BASE+0x0, 0x9);
		udelay(2000);

		while ((*((volatile unsigned int *)(NAND_REG_BASE)) & 0x1<<16) == 0) {
			if (!(status_time--)) {
				cmd_to_zero;
				status_time = STATUS_TIME;
				break;
			}
			udelay(80);
		}
	}
	printf("\nerase all nandflash ok...\n");
}

#include "nand_gpio.c"

static int nand_read_id(void)
{
	unsigned int cmd_id = 0x21;
	unsigned int cmd_reg = 0xbfe78000;
	unsigned int id_reg =  0xbfe78010;
	unsigned int id_val = 0;
	unsigned int id_val_h = 0;
	int i, dev_id, maf_idx, busw;
	struct mtd_info *mtd = ls1g_soc_mtd;
	struct nand_chip *chip = mtd->priv;
	struct nand_flash_dev *type = NULL;

	*((volatile unsigned int *)(0xbfe7800c)) = 0x30f0;	//NAND命令有效需等待的周期数 NAND一次读写所需总时钟周期数
	*((volatile unsigned int *)(0xbfe78000)) = 0x21;		//读ID操作 命令有效
#define    IDL  *((volatile unsigned int*)(0xbfe78010))  
#define    IDH  *((volatile unsigned int*)(0xbfe78014))  
	while(1){
		while(((id_val |= IDL) & 0xff) == 0){
			id_val_h = IDH;
		}
		break;
	}
	/*
	__asm__ __volatile__(
	".set mips3\n"
	"\tlw $4,%1\n"
	"\tlw $5,%2\n"
	"\tsw $4,0($5)\n"
	//            "\tla,a0,%0i"
	"1:\tlw $4,0(%3)\n"
	"\tbeqz $4,1f\n"
	"\tmove %0,$4\n"
	:"=r"(id_val)
	:"r"(cmd_id),"r"(cmd_reg),"r"(id_reg)
	);
	*/
	
	dev_id = (id_val >> 24);
	
	/* Lookup the flash id */
	for (i = 0; nand_flash_ids[i].name != NULL; i++) {
		if (dev_id == nand_flash_ids[i].id) {
			type =  &nand_flash_ids[i];
			break;
		}
	}
	
	if (!type){
		printf("No NAND device found!!!\n");
		return 1;
	}
	
	/* Try to identify manufacturer */
	for (maf_idx = 0; nand_manuf_ids[maf_idx].id != 0x0; maf_idx++) {
		if (nand_manuf_ids[maf_idx].id == (unsigned char)id_val_h)
			break;
	}
	
	busw = type->options & NAND_BUSWIDTH_16;
	
//	printf("read_id_l:0x%08x\nread_id_h:0x%08x\n", id_val, id_val_h);
	printf("NAND device: Manufacturer ID:"
		       " 0x%02x, Chip ID: 0x%02x (%s %s)\n", id_val_h,
		       (id_val >> 24), nand_manuf_ids[maf_idx].name, mtd->name);
	printf("NAND bus width %d instead %d bit\n",
		       (chip->options & NAND_BUSWIDTH_16) ? 16 : 8,
		       busw ? 16 : 8);
	return 0;
}

#define TIMING 0xc
#define OP_NUM  0x1c
#define ADDR    0x4
#define CMD     0x0
static unsigned int nand_num=0;

static void nandwrite_test(int argc,char **argv)/*cmd addr(L,page num) timing op_num(byte) */
{
	unsigned int cmd = 0,addr=0,timing=0x412,op_num=0,dma_num=0;
	unsigned int pages=0,i,val,timeout;
	
	if(argc != 5){
		printf("\nnandwrite_test : cmd  addr(start write page_num)  timing  op_num(write pages)\n");
		printf("EXAMPLE:nandwrite_test : 0x203  0x10 0x412 0x5\n\n");
		return;
	}
	nand_num = 0;
	cmd = strtoul(argv[1],0,0);
	addr = strtoul(argv[2],0,0);
	timing = strtoul(argv[3],0,0);
	op_num = strtoul(argv[4],0,0);
//	dma_num = strtoul(argv[5],0,0);
	if(cmd & 0x9){addr <<= 12;}
	else{addr <<= 11;}
//	pages = (op_num>>11)+1;
	pages = op_num;
	for(i=0;i<pages;i++){
		__ww(NAND_REG_BASE+TIMING, timing);
		__ww(NAND_REG_BASE+OP_NUM, 0x840);
		__ww(NAND_REG_BASE+ADDR, addr);
		__ww(DDR_ADDR, 0xffffffff);
		/*dma configure*/
		__ww(DMA_DESP, 0xa0081100);
		__ww(DMA_DESP+0x4, DDR_PHY);
		__ww(DMA_DESP+0x8, NAND_DEV);
		__ww(DMA_DESP+0xc, 0x210);
		__ww(DMA_DESP+0x10, 0x0);
		__ww(DMA_DESP+0x14, 0x1);
		__ww(DMA_DESP+0x18, 0x1000);
		__ww(0xbfd01160, DMA_DESP_ORDER);

		/*send cmd*/ 
		__ww(NAND_REG_BASE+CMD, cmd&0x200);
		__ww(NAND_REG_BASE+CMD, cmd);	
		udelay(100);
		while(1){
			__ww(0xbfd01160,DMA_ASK_ORDER);
			__rw(DMA_DESP+0x4,val);
			if(val == (0x210+DDR_PHY)){break;}
			udelay(20);
		}
		timeout=30;
		udelay(300);
		__rw(NAND_REG_BASE+CMD,val);
		while(!(val&0x1<<20)){
			udelay(20);
			if(!(timeout--)){nand_num++;break;}
			__rw(NAND_REG_BASE+CMD,val);
		}
		printf("nand_num===0x%08x\n", nand_num);
	}
}

void nand_oob_test()
{
#define	READ_LEN	(2048+64)

	int k;
//	int start_addr = 0x6c54000;
	int start_addr = 0x0;
	unsigned char spi_buf[READ_LEN], flash_buf[READ_LEN], flash_buf1[READ_LEN];
	volatile unsigned char *spi_p = ((volatile unsigned char *)(0xbf000000));
	struct ls1g_nand_info *info = ls1g_soc_mtd->priv;
	struct nand_chip *this = &info->nand_chip;
	int err_flag=0;

	for (k=0; k<READ_LEN; k++)
	{
		spi_buf[k] = *spi_p++;
	}

	printf ("lxy: spi_flash's data : \n");
	show_data(spi_buf, READ_LEN);

	printf ("oob's data: \n");
	this->cmdfunc(ls1g_soc_mtd, NAND_CMD_READOOB, 2048, start_addr>>12);
	this->read_buf(ls1g_soc_mtd, flash_buf, 64);
	show_data(flash_buf, 64);

//	this->cmdfunc(ls1g_soc_mtd, NAND_CMD_READ0, 0, start_addr>>12);
//	this->read_buf(ls1g_soc_mtd, flash_buf, READ_LEN);
//	show_data(flash_buf, READ_LEN);

	this->cmdfunc(ls1g_soc_mtd, NAND_CMD_ERASE1, 0, start_addr>>12);

//	this->cmdfunc(ls1g_soc_mtd, NAND_CMD_READ0, 0, start_addr>>12);
//	this->read_buf(ls1g_soc_mtd, flash_buf, READ_LEN);
//	show_data(flash_buf, READ_LEN);

	this->cmdfunc(ls1g_soc_mtd, NAND_CMD_SEQIN, 0, start_addr>>12);
	this->write_buf(ls1g_soc_mtd, spi_buf, READ_LEN);
	this->cmdfunc(ls1g_soc_mtd, NAND_CMD_PAGEPROG, -1, -1);

	printf ("flash_buf's data: \n");
	this->cmdfunc(ls1g_soc_mtd, NAND_CMD_READ0, 0, start_addr>>12);
	this->read_buf(ls1g_soc_mtd, flash_buf, READ_LEN);
	show_data(flash_buf, READ_LEN);

	return 0;

	this->cmdfunc(ls1g_soc_mtd, NAND_CMD_READOOB, 2048, start_addr>>12);
	this->read_buf(ls1g_soc_mtd, flash_buf, 64);
	show_data(flash_buf, 64);

//static void show_data(void * base,int num)

	for (k=2048; k<64; k++)
	{
		if (spi_buf[k] != flash_buf[k-2048])
		{
			err_flag = 1;
			printf ("\n%d: 0x%x --> 0x%x !\n", k, spi_buf[k], flash_buf[k]);
		}
	}
	if (err_flag == 0)
		printf("nandflash test ok !\n");

	this->cmdfunc(ls1g_soc_mtd, NAND_CMD_READ0, 0, start_addr>>12);
	this->read_buf(ls1g_soc_mtd, flash_buf1, READ_LEN);

	for (k=0; k<READ_LEN; k++)
	{
		if (spi_buf[k] != flash_buf1[k])
		{
			err_flag = 1;
			printf ("\n%d: 0x%x --> 0x%x !\n", k, spi_buf[k], flash_buf1[k]);
		}
	}
	if (err_flag == 0)
		printf("nandflash test ok !\n");
}

static const Cmd Cmds[] =
{
	{"MyCmds"},
	{"nanderase_verify","val",0,"hardware test",nanderase_verify,0,99,CMD_REPEAT},
	{"nanderase","val",0,"hardware test",nanderase,0,99,CMD_REPEAT},
	{"nandreadid_gpio","val",0,"hardware test",nand_gpio_read_id,0,99,CMD_REPEAT},
	{"nandreadid","val",0,"hardware test",nand_read_id,0,99,CMD_REPEAT},
	{"nandwrite_test","val",0,"hardware test",nandwrite_test,0,99,CMD_REPEAT},
	{"nand_oob_test","val",0,"nand_oob_test",nand_oob_test,0,99,CMD_REPEAT},
	{"nand_init","val",0,"ls1g_soc_nand_init",ls1g_soc_nand_init,0,99,CMD_REPEAT},
	{0, 0}
};

static void init_cmd __P((void)) __attribute__ ((constructor));
static void init_cmd()
{
	cmdlist_expand(Cmds, 1);
}

extern unsigned int output_mode;

int ls1g_soc_nand_init(void)
{
	unsigned int val;
	struct nand_chip *this;
	printf("\nNAND DETE\n");

#ifdef	LS1ASOC
#ifdef	NAND_USE_LPC
//#if 1	//NAND复用LPC
	__rw(0xbfd00420, val);
	val |= 0x2a000000;
	__ww(0xbfd00420, val);

	__rw(0xbfd010c8, val);
	val &= ~(0xffff<<6);	//nand_D0~D7 & nand_control pin
	__ww(0xbfd010c8, val);
#elif	NAND_USE_SPI1
//#else //NAND复用SPI1
	__rw(0xbfd00420, val);
	val |= 0x14000000;
	__ww(0xbfd00420, val);

	__rw(0xbfd010c4, val);
	val &= ~(0xf<<12);		//nand_D0~D3
	__ww(0xbfd010c4, val);

	__rw(0xbfd010c8, val);
	val &= ~(0xfff<<12);	//nand_D4~D7 & nand_control pin
	__ww(0xbfd010c8, val);
#endif
#endif	//LS1ASOC

	/* Allocate memory for MTD device structure and private data */
	ls1g_soc_mtd = malloc(sizeof(struct mtd_info) + sizeof(struct nand_chip), M_DEVBUF, M_WAITOK);
	if (!ls1g_soc_mtd) {
		printk("Unable to allocate fcr_soc NAND MTD device structure.\n");
		return -ENOMEM;
	}
	/* Get pointer to private data */
	this = (struct nand_chip *)(&ls1g_soc_mtd[1]);

	/* Initialize structures */
	memset(ls1g_soc_mtd, 0, sizeof(struct mtd_info));
	memset(this, 0, sizeof(struct nand_chip));

	/* Link the private data with the MTD structure */
	ls1g_soc_mtd->priv = this;
	/* 15 us command delay time 从数据手册获知命令延迟时间 */
	this->chip_delay = 15;
	if(ls1g_nand_init(ls1g_soc_mtd)){
		printf("\n\nerror: PMON nandflash driver have some error!\n\n");
		return -ENXIO;
	}

	/* Scan to find existence of the device */
	if (nand_scan(ls1g_soc_mtd, 1)) {
		free(ls1g_soc_mtd,M_DEVBUF);
		return -ENXIO;
	}
	if(ls1g_nand_detect(ls1g_soc_mtd)){
		printf("error: PMON driver don't support the NANDFlash!\n ");
		return -ENXIO;
	}

	/* Register the partitions */
#ifdef FAST_STARTUP
	if (output_mode == 0)
	{
		add_mtd_device(ls1g_soc_mtd,0,0x400000,"kernel");
		return 0;
	}
#endif
	add_mtd_device(ls1g_soc_mtd, 0, 14*1024*1024, "kernel");				//14MB
	add_mtd_device(ls1g_soc_mtd, 14*1024*1024, 100*1024*1024, "os");		//100MB
	add_mtd_device(ls1g_soc_mtd, (100+14)*1024*1024, 14*1024*1024, "data");	//14MB

//	find_good_part(ls1g_soc_mtd);
	/* Return happy */
	return 0;
}
