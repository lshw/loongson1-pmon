#include<pmon.h>
#include<asm.h>
#include<machine/types.h>
#include<linux/mtd/mtd.h>
#include<linux/mtd/nand.h>
#include<linux/mtd/partitions.h>
#include<sys/malloc.h>
#include <linux/mtd/nand_bch.h>
#include <nand_bch.h>

#ifndef __iomem
#define __iomem
#endif

#define LS1X_DMA_ACCESS_ADDR     0x1fe78040
#define LS1X_ORDER_REG_ADDR      (0xbfd01160)
#define LS1X_NAND_REG_BASE      (0xbfe78000)
#define MAX_BUFF_SIZE	4096
#define PAGE_SHIFT      12
#ifdef LS1GSOC
#define NO_SPARE_ADDRH(x)   ((x) >> (32 - (PAGE_SHIFT - 1 )))   
#define NO_SPARE_ADDRL(x)   ((x) << (PAGE_SHIFT - 1))
#define SPARE_ADDRH(x)      ((x) >> (32 - (PAGE_SHIFT )))   
#define SPARE_ADDRL(x)      ((x) << (PAGE_SHIFT ))
#elif (defined LS1FSOC|| defined LS1CSOC)
#define NO_SPARE_ADDRH(x)   (x)   
#define NO_SPARE_ADDRL(x)   0
#define SPARE_ADDRH(x)      (x)   
#define SPARE_ADDRL(x)      0
#endif

#define ALIGN_DMA(x)       (((x)+ 3)/4)
#define CHIP_DELAY_TIMEOUT (2*HZ/10)

#define STATUS_TIME_LOOP_R  100 
#define STATUS_TIME_LOOP_WS  300  
#define STATUS_TIME_LOOP_WM  60 
#define STATUS_TIME_LOOP_E   100 
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

#if defined LS1FSOC|| defined LS1CSOC
#define NAND_ECC_OFF    0
#define NAND_ECC_ON     1
#define RAM_OP_OFF      0
#define RAM_OP_ON       1
#endif

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
        uint32_t    cmd_valid:1;    //0
    	uint32_t    read:1;         //1
	uint32_t    write:1;        //2
	uint32_t    erase_one:1;    //3
    	uint32_t    erase_con:1;    //4
	uint32_t    read_id:1;      //5
	uint32_t    reset:1;        //6
	uint32_t    read_sr:1;      //7
	uint32_t    op_main:1;      //8
	uint32_t    op_spare:1;     //9
	uint32_t    done:1;         //10
//#ifdef  LS1FSOC||LS1CSOC
#if defined LS1FSOC|| defined LS1CSOC
        uint32_t    ecc_rd:1;         //11
	uint32_t    ecc_wr:1;         //12
	uint32_t    int_en:1;         //13
        uint32_t    resv14:1;         //14
	uint32_t    ram_op:1;         //15
#elif LS1GSOC
        uint32_t    resv1:5;//11-15 reserved
#endif        
        uint32_t    nand_rdy:4;//16-19
        uint32_t    nand_ce:4;//20-23
//#ifdef LS1FSOC||LS1CSOC
#if defined LS1FSOC|| defined LS1CSOC
	uint32_t    ecc_dma_req:1;         //24
	uint32_t    nul_dma_req:1;         //25
#endif        
        uint32_t    resv26:8;//26-31 reserved
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
//        info->buf_count = real_len;

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
                printk("?????????????????????????????????????????????????????verify error...\n\n");
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
    ret = ls1g_nand_pmon_info_init(mtd->priv,mtd);
    ls1g_nand_init_mtd(mtd,(struct ls1g_nand_info *)(mtd->priv));
    return ret;
}
static void ls1g_nand_init_mtd(struct mtd_info *mtd,struct ls1g_nand_info *info)
{
	struct nand_chip *this = &info->nand_chip;

	//this->options = NAND_CACHEPRG|NAND_SKIP_BBTSCAN;//(f->flash_width == 16) ? NAND_BUSWIDTH_16: 0;
	this->options = NAND_CACHEPRG;//(f->flash_width == 16) ? NAND_BUSWIDTH_16: 0;

	this->waitfunc		= ls1g_nand_waitfunc;
	this->select_chip	= ls1g_nand_select_chip;
	this->dev_ready		= ls1g_nand_dev_ready;
	this->cmdfunc		= ls1g_nand_cmdfunc;
	this->read_word		= ls1g_nand_read_word;
	this->read_byte		= ls1g_nand_read_byte;
	this->read_buf		= ls1g_nand_read_buf;
	this->write_buf		= ls1g_nand_write_buf;
	this->verify_buf	= ls1g_nand_verify_buf;


#if NNAND_BCH
#define BCH_BUG(a...) printf(a);while(1);
	{
		int bch = 4;
		int writesize = 2048;
		int oobsize = 64;
		unsigned int eccsteps, eccbytes;
		if (!mtd_nand_has_bch()) {
			BCH_BUG("BCH ECC support is disabled\n");
		}
		/* use 512-byte ecc blocks */
		eccsteps = writesize/512;
		eccbytes = (bch*13+7)/8;
		/* do not bother supporting small page devices */
		if ((oobsize < 64) || !eccsteps) {
			BCH_BUG("bch not available on small page devices\n");
		}
		if ((eccbytes*eccsteps+2) > oobsize) {
			BCH_BUG("invalid bch value \n", bch);
		}
		this->ecc.mode = NAND_ECC_SOFT_BCH;
		this->ecc.size = 512;
		this->ecc.strength = bch;
		this->ecc.bytes = eccbytes;
		printk("using %u-bit/%u bytes BCH ECC\n", bch, this->ecc.size);
	}
#else
	this->ecc.mode		= NAND_ECC_NONE;
	//this->ecc.mode		= NAND_ECC_SOFT;
	this->ecc.size		= 2048;
	this->ecc.bytes		= 24;
	this->ecc.layout = &hw_largepage_ecclayout;
#endif
	this->ecc.hwctl		= ls1g_nand_ecc_hwctl;
	this->ecc.calculate	= ls1g_nand_ecc_calculate;
	this->ecc.correct	= ls1g_nand_ecc_correct;


//        mtd->owner = THIS_MODULE;
}
#define write_z_cmd  do{                                    \
            *((volatile unsigned int *)(LS1X_NAND_REG_BASE)) = 0;   \
            *((volatile unsigned int *)(LS1X_NAND_REG_BASE)) = 0;   \
            *((volatile unsigned int *)(LS1X_NAND_REG_BASE)) = 400; \
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

static void nand_cache_wb_inv(unsigned long base,unsigned long num)
{
    CPU_IOFlushDCache((base & 0x1fffffff)|0x80000000,num,0);
}

static void nand_cache_wb(unsigned long base,unsigned long num)
{
    CPU_IOFlushDCache((base & 0x1fffffff)|0x80000000,num,1);
}

static void dma_setup(unsigned int flags,struct ls1g_nand_info *info)
{
    int order;
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
        }
        nand_cache_wb((unsigned long)(info->drcmr_dat),0x20);
        
        *(volatile unsigned int *)(info->order_reg_addr) = ((unsigned int )info->drcmr_dat_phys) | 0x1<<3;
//        memset(&(info->dma_regs),0,sizeof(struct ls1g_nand_dma_desc));
}
static void dma_ask(struct ls1g_nand_info *info)
{
    memset((char *)info->data_ask,0,sizeof(struct ls1g_nand_ask_regs));
    *(volatile unsigned int *)info->order_reg_addr = 0x1<<2|(info->data_ask_phys)& 0xfffffff0;
       //show_dma_regs((void *)(info->data_ask),1);
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
//    printk("addrl+++++++++++++++++++++==%x\n\n",info->nand_regs.addrl);
   int i; 
    struct ls1g_nand_desc *nand_base = (struct ls1g_nand_desc *)(info->mmio_base);
    nand_base->cmd = 0;
    nand_base->addrl = (flags & NAND_ADDRL)==NAND_ADDRL ? info->nand_regs.addrl: info->nand_addrl;
    nand_base->addrh = (flags & NAND_ADDRH)==NAND_ADDRH ? info->nand_regs.addrh: info->nand_addrh;
    nand_base->timing = (flags & NAND_TIMING)==NAND_TIMING ? info->nand_regs.timing: info->nand_timing;
    nand_base->op_num = (flags & NAND_OP_NUM)==NAND_OP_NUM ? info->nand_regs.op_num: info->nand_op_num;
    nand_base->cs_rdy_map = (flags & NAND_CS_RDY_MAP)==NAND_CS_RDY_MAP ? info->nand_regs.cs_rdy_map: info->nand_cs_rdy_map;
    nand_base->param = ((nand_base->param)& 0xc000ffff)|(nand_base->op_num << 16); 
    if(flags & NAND_CMD){
//        nand_base->cmd = (info->nand_regs.cmd) &(~0xff);
        nand_base->cmd = (info->nand_regs.cmd & (~1));
        nand_base->cmd = info->nand_regs.cmd ;
/*        if(info->nand_regs.cmd & 0x20){
                        i = 100;
            while(!ls1g_nand_status(info)){
                if(!(i--)){
                    write_z_cmd;
                    break;
                }
                udelay(30);
            }
            *(int *)(info->data_buff) = nand_base->idl;
        }
*/        
    }
    else
        nand_base->cmd = info->nand_cmd;
}
static  int sync_dma(struct ls1g_nand_info *info)
{
    int status_time;
    status_time=STATUS_TIME_LOOP_WS;
    while(!(*((volatile unsigned int*)(LS1X_NAND_REG_BASE)) & (0x1<<10))){
        if(!(status_time--)){
			/*time out,so clear cmd,fixme */
            write_z_cmd;
            break;
        }
        udelay(2);
    }
#if 0
    int *end ;
    struct ls1g_nand_ask_regs *ask = info->data_ask;
        end = ( (unsigned int )(info->data_buff_phys)&0x1fffffff) + info->data_length*4 ;
    while(1){
        dma_ask(info);
        udelay(100);
        if(ask->dma_mem_addr == end)
            break;
    }
    /*flush cache after DMA operation*/
    //dma_cache_nand(info,0); 
        return 0;
#endif
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
                if(info->state == STATE_BUSY){
                    printk("nandflash chip if busy...\n");
                    return;
                }
                info->state = STATE_BUSY; 
                info->buf_count = mtd->oobsize;
                info->buf_start = 0;
                info->cac_size = info->buf_count;
                nand_cache_wb_inv((unsigned long)(info->data_buff),info->cac_size);
                if(info->buf_count <=0 )
                    break;
                /*nand regs set*/
                info->dma_regs.cmd = 0;
                info->nand_regs.cmd = 0; 
                info->nand_regs.addrh =  SPARE_ADDRH(page_addr);
                info->nand_regs.addrl = SPARE_ADDRL(page_addr) + mtd->writesize;
                info->nand_regs.op_num = info->buf_count;
               /*nand cmd set */
//#ifdef LS1FSOC||LS1CSOC
#if defined LS1FSOC|| defined LS1CSOC
                ((struct ls1g_nand_cmdset*)&(info->nand_regs.cmd))->int_en = 1;
                ((struct ls1g_nand_cmdset*)&(info->nand_regs.cmd))->ram_op = RAM_OP_OFF;
                ((struct ls1g_nand_cmdset*)&(info->nand_regs.cmd))->ecc_rd = NAND_ECC_OFF;
                ((struct ls1g_nand_cmdset*)&(info->nand_regs.cmd))->ecc_wr = NAND_ECC_OFF;
#endif
                ((struct ls1g_nand_cmdset*)&(info->nand_regs.cmd))->read = 1;
                ((struct ls1g_nand_cmdset*)&(info->nand_regs.cmd))->op_spare = 1;
                ((struct ls1g_nand_cmdset*)&(info->nand_regs.cmd))->cmd_valid = 1;
                /*dma regs config*/
                info->dma_regs.length =ALIGN_DMA(info->buf_count);
                ((struct ls1g_nand_dma_cmd *)&(info->dma_regs.cmd))->dma_int_mask = 0;
                /*dma GO set*/       
                nand_setup(NAND_ADDRL|NAND_ADDRH|NAND_OP_NUM|NAND_CMD,info);
                dma_setup(DMA_LENGTH|DMA_CMD,info);
                sync_dma(info);
                break;
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
                nand_cache_wb_inv((unsigned long)(info->data_buff),info->cac_size);
                info->nand_regs.addrh = SPARE_ADDRH(page_addr);
                info->nand_regs.addrl = SPARE_ADDRL(page_addr);
                info->nand_regs.op_num = info->buf_count;
                 /*nand cmd set */ 
                info->nand_regs.cmd = 0; 
                info->dma_regs.cmd = 0;
//#ifdef LS1FSOC||LS1CSOC                
#if defined LS1FSOC|| defined LS1CSOC
                ((struct ls1g_nand_cmdset*)&(info->nand_regs.cmd))->int_en = 1;
                ((struct ls1g_nand_cmdset*)&(info->nand_regs.cmd))->ram_op = RAM_OP_OFF;
                ((struct ls1g_nand_cmdset*)&(info->nand_regs.cmd))->ecc_rd = NAND_ECC_OFF;
                ((struct ls1g_nand_cmdset*)&(info->nand_regs.cmd))->ecc_wr = NAND_ECC_OFF;
#endif
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
                info->cac_size = info->buf_count;

                   /*nand regs set*/
                info->nand_regs.addrh =  SPARE_ADDRH(info->seqin_page_addr);
                info->nand_regs.addrl =  SPARE_ADDRL(info->seqin_page_addr) + info->seqin_column;
                info->nand_regs.op_num = info->buf_start;
 
               /*nand cmd set */ 
                info->nand_regs.cmd = 0; 
                info->dma_regs.cmd = 0;
//#ifdef LS1FSOC||LS1CSOC
#if defined LS1FSOC|| defined LS1CSOC
                ((struct ls1g_nand_cmdset*)&(info->nand_regs.cmd))->int_en = 1;
                ((struct ls1g_nand_cmdset*)&(info->nand_regs.cmd))->ram_op = RAM_OP_OFF;
                ((struct ls1g_nand_cmdset*)&(info->nand_regs.cmd))->ecc_rd = NAND_ECC_OFF;
                ((struct ls1g_nand_cmdset*)&(info->nand_regs.cmd))->ecc_wr = NAND_ECC_OFF;
#endif                
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
                break;
            case NAND_CMD_RESET:
                info->state = STATE_BUSY;
               /*nand cmd set */ 
//#ifdef LS1FSOC||LS1CSOC
#if defined LS1FSOC|| defined LS1CSOC
                ((struct ls1g_nand_cmdset*)&(info->nand_regs.cmd))->int_en = 0;
                ((struct ls1g_nand_cmdset*)&(info->nand_regs.cmd))->ram_op = RAM_OP_OFF;
#endif
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
//#ifdef LS1FSOC||LS1CSOC
#if defined LS1FSOC|| defined LS1CSOC
                ((struct ls1g_nand_cmdset*)&(info->nand_regs.cmd))->int_en = 0;
                ((struct ls1g_nand_cmdset*)&(info->nand_regs.cmd))->ram_op = RAM_OP_OFF;
#endif                
                ((struct ls1g_nand_cmdset*)&(info->nand_regs.cmd))->erase_one = 1;
                ((struct ls1g_nand_cmdset*)&(info->nand_regs.cmd))->cmd_valid = 1; 
                nand_setup(NAND_ADDRL|NAND_ADDRH|NAND_OP_NUM|NAND_CMD,info);
                status_time = STATUS_TIME_LOOP_E;
                udelay(2000);    
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
                info->buf_count = 0x5;
                info->buf_start = 0;
//                info->cac_size = info->buf_count;
                {
#define  _NAND_IDL      ( *((volatile unsigned int*)(LS1X_NAND_REG_BASE+0x10)))
#define  _NAND_IDH       (*((volatile unsigned int*)(LS1X_NAND_REG_BASE+0x14)))
#define  _NAND_BASE      LS1X_NAND_REG_BASE
#define  _NAND_SET_REG(x,y)   do{*((volatile unsigned int*)(_NAND_BASE+x)) = (y);}while(0)                           
#define  _NAND_READ_REG(x,y)  do{(y) =  *((volatile unsigned int*)(_NAND_BASE+x));}while(0) 


                    unsigned int id_val_l=0,id_val_h=0;
                    unsigned int timing = 0;
                    unsigned char *data = (unsigned char *)(info->data_buff);
//                    _NAND_READ_REG(0xc,timing);
  //                  _NAND_SET_REG(0xc,0x30f0); 
//                    _NAND_SET_REG(0x18,0x5000); 
                    _NAND_SET_REG(0x0,0x21); 

                    while(!(id_val_h = _NAND_IDH));
                    while(!((id_val_l |= _NAND_IDL) & 0xff));
//                    _NAND_SET_REG(0xc,timing);
                    data[0]  = (id_val_h & 0xff);
                    data[1]  = (id_val_l & 0xff000000)>>24;
                    data[2]  = (id_val_l & 0x00ff0000)>>16;
                    data[3]  = (id_val_l & 0x0000ff00)>>8;
                    data[4]  = (id_val_l & 0x000000ff);
                }
                break;
            case NAND_CMD_ERASE2:
            case NAND_CMD_READ1:
                break;
            default :
                printf(KERN_ERR "non-supported command.\n");
		break;
        }

        if(info->cmd == NAND_CMD_READ0 || info->cmd == NAND_CMD_READOOB  ){
            nand_cache_wb_inv((unsigned long)(info->data_buff),info->cac_size);
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
#ifdef LS1CSOC   
    unsigned int chipsize=0,pagesize=0,regsize=0,id_num=0;;
    struct nand_flash_dev *type = NULL;
    int i, dev_id, maf_id;
#define  ID_NUM     5
    _NAND_SET_REG(0x18,0x5000); 
    _NAND_SET_REG(0x0,0x21);
    udelay(2); 
    id_num = ID_NUM; 
    maf_id = (_NAND_IDH  & 0xff);  
    dev_id = (_NAND_IDL >>24 & 0xff); 
    for (i = 0; nand_flash_ids[i].name != NULL; i++) {
        if (dev_id == nand_flash_ids[i].id) {
            type =  &nand_flash_ids[i];
            break;
        }
    }
    //chipsize = type->chipsize << 20;
    chipsize = type->chipsize * 8;//Mb
    if (!type->pagesize) {
        i = (_NAND_IDL >>8 & 0xff); 
       	pagesize = 1024 << (i & 0x3);
    
    }
    switch(pagesize){
        case 2048:
            switch(chipsize){
                case 1024: 
                    regsize = 0;break;
                case 2048:
                    regsize = 1;break;
                case 4096:
                    regsize = 2;break;
                default:
                    regsize = 3;break;
            }
            break ;
        case 4096:
            regsize = 4;break;
        case 8192:
            switch(chipsize){
                case 32768:
                    regsize = 5;break;
                case 65536:
                    regsize = 6;break;
                default:
                    regsize = 7;break;
            }
        case 512:
            switch(chipsize){
                case 64:
                    regsize = 9;break;
                case 128:
                    regsize = 0xa;break;
                case 256:
                    regsize = 0xb;break;
                case 512:
                    regsize = 0xc;break;
                default:
                    regsize = 0xd;break;
            }
        default:
            printk(KERN_ERR "nand error(pagesize)\n");
            break;
    }
//    printf("pagesize == 0x%08x,chipsize==0x%08x\n",pagesize,chipsize);
    *((volatile unsigned int *)(LS1X_NAND_REG_BASE+0x18)) = (regsize & 0xf) << 8 | (pagesize & 0x3fff) << 16|(id_num & 0x7) << 12;
#else    
    //*((volatile unsigned int *)0xbfe78018) = 0x400;
    *((volatile unsigned int *)(LS1X_NAND_REG_BASE+0x18)) = 0x400;
#endif
    info->num=0;
    info->size=0;
    info->cac_size = 0; 
    info->state = STATE_READY;

    info->cmd_prev = -1;
    info->page_addr = -1;
    info->nand_addrl = 0x0;
    info->nand_addrh = 0x0;
#if defined LS1GSOC || defined LS1CSOC     
    info->nand_timing = 0x209;
#else    
    info->nand_timing = 0x4<<8 | 0xb;
#endif
    info->nand_op_num = 0x0;
    info->nand_cs_rdy_map = 0x88442200;
    info->nand_cmd = 0;

    info->dma_orderad = 0xa0811000;
    info->dma_saddr = info->data_buff_phys;
    info->dma_daddr = LS1X_DMA_ACCESS_ADDR;
    info->dma_length = 0x0;
    info->dma_step_length = 0x0;
    info->dma_step_times = 0x1;
    info->dma_cmd = 0x0;

    info->order_reg_addr = LS1X_ORDER_REG_ADDR;
}
int ls1g_nand_pmon_info_init(struct ls1g_nand_info *info,struct mtd_info *mtd)
{	
        
        info->drcmr_dat =  (unsigned int)(malloc(sizeof(struct ls1g_nand_dma_desc)+32,M_DMAMAP,M_WAITOK));
        info->drcmr_dat = ((info->drcmr_dat+0x1f)&(~0x1f))&0xfffffff|0x80000000;
        if(info->drcmr_dat == NULL)
            return -1;
	info->drcmr_dat_phys = (info->drcmr_dat) & 0x1fffffff;

//        printf("drcmr_dat==0x%08x,drcmr_phy==0x%08x\n",info->drcmr_dat,info->drcmr_dat_phys);
        //info->mmio_base = 0x1fe78000 | 0xa0000000;
        info->mmio_base = LS1X_NAND_REG_BASE;
    	
        info->data_buff = ((unsigned int)(malloc(MAX_BUFF_SIZE + 32,M_DMAMAP,M_WAITOK)) + 0x1f)&(~0x1f);
        info->data_buff = (unsigned char *)((((unsigned int)info->data_buff+0x1f) & (~0x1f))&0xfffffff|0x80000000);
        if(info->data_buff == NULL)
            return -1;
  //      printf("data_buff==0x%08x\n",info->data_buff);
        info->data_buff_phys = (unsigned int)(info->data_buff) & 0x1fffffff;
    //    printf("data_buff_phys==0x%08x\n",info->data_buff_phys);
        
        info->data_ask = (unsigned int)(malloc(sizeof(struct ls1g_nand_ask_regs)+32,M_DMAMAP,M_WAITOK));
        info->data_ask = (((info->data_ask+0x1f)&(~0x1f))&0xfffffff|0x80000000);
        if(info->data_ask ==NULL)
            return -1;
        info->data_ask_phys = info->data_ask & 0x1fffffff;
      //  printf("ask_dat==0x%08x,ask_phy==0x%08x\n",info->data_ask,info->data_ask_phys);

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
    for(offs=0;offs< ls1g_soc_mtd->size;offs+=ls1g_soc_mtd->erasesize)
    {
        if(ls1g_soc_mtd->block_isbad(ls1g_soc_mtd,offs)&& start>=0)
        {
            sprintf(name,"g%d",idx++);
            add_mtd_device(ls1g_soc_mtd,start,offs-start,name);
            start=-1;
        }
        else if(start<0)
        {
            start=offs;
        }

    }

    if(start>=0)
    {
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
#define DDR_PHY         0x08400000
#define DDR_ADDR        0xa8400000

#define STATUS_TIME 100

#define cmd_to_zero  do{            \
            __ww(NAND_REG_BASE,0);  \
            __ww(NAND_REG_BASE,0);  \
            __ww(NAND_REG_BASE,0x400);  \
}while(0)

unsigned int verify_erase(unsigned int addr,int all)
{
    int i=0,flag=0;
    volatile unsigned int *base = (unsigned int*)DDR_ADDR;
    unsigned int val=0;
    __ww(DMA_DESP+0,0);
    __ww(DMA_DESP+0x4,DDR_PHY); 
    __ww(DMA_DESP+0x8,NAND_DEV); 
    __ww(DMA_DESP+0xc,0x8000);  //0x8400
    __ww(DMA_DESP+0x10,0x0); 
    __ww(DMA_DESP+0x14,0x1); 
    __ww(DMA_DESP+0x18,0x0);
    __ww(0xbfd01160,DMA_DESP_ORDER);

	__ww(NAND_REG_BASE+0x0, 0x0);
//	__ww(NAND_REG_BASE+0x4, 0); //页内起始地址
	__ww(NAND_REG_BASE+0x8, addr<<(6));
	__ww(NAND_REG_BASE+0x1c, 0x20000);		// 0x21000 ??? 128k+4k
	__ww(NAND_REG_BASE+0x0, 0x100); //main area //300
	__ww(NAND_REG_BASE+0x0, 0x103); //main area  start //303
/*  ????????
    __ww(NAND_REG_BASE+0x0,0x0);
    __ww(NAND_REG_BASE+0x4,addr<<(12+6)); //????????
    __ww(NAND_REG_BASE+0x1c,0x21000);//main + spare
    __ww(NAND_REG_BASE+0x0,0x300);
    __ww(NAND_REG_BASE+0x0,0x303);
  */  
    udelay(5000);
    while(1){
        __ww(0xbfd01160,DMA_ASK_ORDER);
        __rw(DMA_DESP+0x4,val);
        if(val == (0x20000+DDR_PHY)){break;}  //0x21000
        udelay(400);
    }
    for(;i<0x8000;i++){ //0x8400
        if(*(base+i) != 0xffffffff){printf("\naddr 0x%08x: 0x%08x isn't 0xFFFFFFFF",(base+i),*(base+i));flag=1;if(all){return flag;}}
    }
    return flag;
}

void nanderase_verify(int argc,char ** argv)
{
    int i=0,flag=0;
    int status_time ;
    char *detail;
//    all =strtoul(argv[1],0,0);
    detail = argv[1];
    flag=strncmp(detail,"detail",6);
    __ww(NAND_REG_BASE+0x0,0x0);   
    __ww(NAND_REG_BASE+0x0,0x41);
    __ww(NAND_REG_BASE+0x4,0x00);
    status_time = STATUS_TIME;

       printf("erase blockaddr: 0x%08x",i); 
    for(i=0;i<1024;i++){
        printf("\b\b\b\b\b\b\b\b");
        printf("%08x",i<<(11+6)); 
        __ww(NAND_REG_BASE+0x4,i<<(11+6));   
        __ww(NAND_REG_BASE+0x0,0x8);
        __ww(NAND_REG_BASE+0x0,0x9);
            udelay(2000);
        while((*((volatile unsigned int *)(NAND_REG_BASE)) & 0x1<<16) == 0)
        {
            if(!(status_time--)){
                cmd_to_zero;
                status_time = STATUS_TIME;
                break;
            }
            udelay(80);
        }
        if(verify_erase(i,flag)){printf("BLOCK:%d,addr:0x%08x,some error or bad block\n",i,i<<(11+6));}
    }
    printf("\nerase all nandflash ok...\n");




#if 0  
    //  __ww(NAND_REG_BASE+0x0,0x9);
//    udelay(1000000);
//    __display(NAND_REG_BASE,0x10);
//    return 0;  

//    __ww(NAND_REG_BASE+0x0,0x0);
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
    __ww(NAND_REG_BASE+0x0,0x0);   
    __ww(NAND_REG_BASE+0x0,0x41);
    __ww(NAND_REG_BASE+0x4,0x00);

    status_time = STATUS_TIME;

       printf("erase blockaddr: 0x%08x",i); 
    for(i=0;i<1024;i++){
        printf("\b\b\b\b\b\b\b\b");
       printf("%08x",i<<(11+6)); 
        __ww(NAND_REG_BASE+0x4,i<<(11+6));   
        __ww(NAND_REG_BASE+0x0,0x8);
        __ww(NAND_REG_BASE+0x0,0x9);
            udelay(2000);
        while((*((volatile unsigned int *)(NAND_REG_BASE)) & 0x1<<16) == 0)
        {
            if(!(status_time--)){
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

static void nand_read_id(void)
{
    unsigned int cmd_id = 0x21;
    unsigned int cmd_reg = 0xbfe78000;
    unsigned int id_reg =  0xbfe78010;
    unsigned int id_val=0;
    unsigned int id_val_h=0;
    *((volatile unsigned int *)(0xbfe7800c))=0x30f0;
    *((volatile unsigned int *)(0xbfe78000))=0x21;
#define    IDL  *((volatile unsigned int*)(0xbfe78010))  
#define    IDH  *((volatile unsigned int*)(0xbfe78014))  
    while(1){
        while(((id_val |= IDL) & 0xff)  == 0){
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
            printf("read_id_l:0x%08x\nread_id_h:0x%08x\n",id_val,id_val_h);

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
    if(argc != 5)
    {
        printf("\nnandwrite_test : cmd  addr(start write page_num)  timing  op_num(write pages)\n");
        printf("EXAMPLE:nandwrite_test : 0x203  0x10 0x412 0x5\n\n");
        return;
    }
       nand_num=0; 
     cmd = strtoul(argv[1],0,0);
     addr = strtoul(argv[2],0,0);
     timing = strtoul(argv[3],0,0);
     op_num = strtoul(argv[4],0,0);
//     dma_num = strtoul(argv[5],0,0);
     if(cmd&0x9){addr <<= 12;}
     else{addr <<= 11;}
//    pages = (op_num>>11)+1;
    pages = op_num;
for(i=0;i<pages;i++){
     __ww(NAND_REG_BASE+TIMING,timing); 
     __ww(NAND_REG_BASE+OP_NUM,0x840); 
     __ww(NAND_REG_BASE+ADDR,addr);
     __ww(DDR_ADDR,0xffffffff);
/*dma configure*/
    __ww(DMA_DESP,0xa0081100); 
    __ww(DMA_DESP+0x4,DDR_PHY); 
    __ww(DMA_DESP+0x8,NAND_DEV); 
    __ww(DMA_DESP+0xc,0x210);
    __ww(DMA_DESP+0x10,0x0); 
    __ww(DMA_DESP+0x14,0x1); 
    __ww(DMA_DESP+0x18,0x1000);
    __ww(0xbfd01160,DMA_DESP_ORDER);
/*send cmd*/ 
     __ww(NAND_REG_BASE+CMD,cmd&0x200);
     __ww(NAND_REG_BASE+CMD,cmd);
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
    printf("nand_num===0x%08x\n",nand_num);
    
}

}
static const Cmd Cmds[] =
{
	{"MyCmds"},
	{"nanderase_verify","val",0,"hardware test",nanderase_verify,0,99,CMD_REPEAT},
	{"nanderase","val",0,"hardware test",nanderase,0,99,CMD_REPEAT},
	{"nandreadid_gpio","val",0,"hardware test",nand_gpio_read_id,0,99,CMD_REPEAT},
	{"nandreadid","val",0,"hardware test",nand_read_id,0,99,CMD_REPEAT},
	{"nandwrite_test","val",0,"hardware test",nandwrite_test,0,99,CMD_REPEAT},
	{0, 0}
};

static void init_cmd __P((void)) __attribute__ ((constructor));
static void init_cmd()
{
//	cmdlist_expand(Cmds, 1);
}
#if 0
int nand_flash_add_parts(struct mtd_info * soc_mtd ,int parts)
{
    return 0;
}
#endif 
static char *mtd_id="nand-flash";
int ls1g_soc_nand_init(void)
{
 //       nand_gpio_read_id();
        int val,i;
	struct nand_chip *this;
        printf("\nNAND DETE\n");
        /* Allocate memory for MTD device structure and private data */
	ls1g_soc_mtd = malloc(sizeof(struct mtd_info) + sizeof(struct ls1g_nand_info),M_DEVBUF,M_WAITOK);
	if (!ls1g_soc_mtd) {
		printk("Unable to allocate fcr_soc NAND MTD device structure.\n");
		return -ENOMEM;
	}
	/* Get pointer to private data */
	this = (struct nand_chip *)(&ls1g_soc_mtd[1]);

	/* Initialize structures */
	memset(ls1g_soc_mtd, 0, sizeof(struct mtd_info));
	memset(this, 0, sizeof(struct ls1g_nand_info));

	/* Link the private data with the MTD structure */
	ls1g_soc_mtd->priv = this;
	/* 15 us command delay time */
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
        ls1g_soc_mtd->name=mtd_id;
  
        if(!nand_flash_add_parts(ls1g_soc_mtd,0)){
            add_mtd_device(ls1g_soc_mtd,0,0x00e00000,"kernel");
            add_mtd_device(ls1g_soc_mtd,0x00e00000,0x06700000,"os");
            add_mtd_device(ls1g_soc_mtd,0x07500000,0x00b00000,"data");
        }
	/* Return happy */
	return 0;
}


