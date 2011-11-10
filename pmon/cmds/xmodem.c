#include <pmon.h>
#include <stdio.h>
#include <include/stdarg.h>
#include "ramfiles.h"
#undef XMODEM_DEBUG
static uint16_t _crc_xmodem_update (uint16_t crc, uint8_t data)
{
    int i;
    crc = crc ^ ((uint16_t)data << 8);
    for (i=0; i<8; i++)
    {
        if (crc & 0x8000)
            crc = (crc << 1) ^ 0x1021;
        else
            crc <<= 1;
    }
    return crc;
}



//管脚定义
#define PIN_RXD            0   //PD0
#define PIN_TXD            1   //PD1

//常数定义
#define BLOCKSIZE       128            //M16的一个Flash页为128字节(64字)
#define DATA_BUFFER_SIZE    BLOCKSIZE   //定义接收缓冲区长度
//#define F_CPU            7372800         //系统时钟7.3728MHz

//定义Xmoden控制字符
#define XMODEM_NUL          0x00
#define XMODEM_SOH          0x01
#define XMODEM_STX          0x02
#define XMODEM_EOT          0x04
#define XMODEM_ACK          0x06
#define XMODEM_NAK          0x15
#define XMODEM_CAN          0x18
#define XMODEM_EOF          0x1A
#define XMODEM_WAIT_CHAR    'C'

//定义全局变量
struct str_XMODEM
{
    unsigned char SOH;                  //起始字节
    unsigned char BlockNo;               //数据块编号
    unsigned char nBlockNo;               //数据块编号反码
    unsigned char Xdata[BLOCKSIZE];            //数据128字节
    unsigned char CRC16hi;               //CRC16校验数据高位
    unsigned char CRC16lo;               //CRC16校验数据低位
}
strXMODEM;                           //XMODEM的接收数据结构

static unsigned long FlashAddress;               //FLASH地址
#define  BootAdd          0x3800         //Boot区的首地址(应用区的最高地址)
/*   GCC里面地址使用32位长度，适应所有AVR的容量*/


static unsigned char STATUS;                  //运行状态
#define ST_WAIT_START       0x00         //等待启动
#define ST_BLOCK_OK       0x01         //接收一个数据块成功
#define ST_BLOCK_FAIL       0x02         //接收一个数据块失败
#define ST_OK             0x03         //完成


#ifdef XMODEM_DEBUG
#define MYDBG dbg_printf("%d\n",__LINE__);
static char dbgbuf[2048];
static char *pmsg=dbgbuf;
static int dbg=0;
static int dbg_printf (const char *fmt, ...)
{
    int  len;
    va_list	    ap;

    if(!dbg)return 0;

    va_start(ap, fmt);
    len= vsprintf (pmsg, fmt, ap);
	pmsg+=len;
    if((pmsg-dbgbuf)>1800)pmsg=dbgbuf;
    va_end(ap);
    return (len);
}

static int dmsg(int argc,char *argv[])
{
if(pmsg!=dbgbuf)
printf("%s\n",dbgbuf);
return 0;
}
#else
#define MYDBG 
#define dbg_printf(...)
#endif

static int testchar()
{
	int count=2;
	int total, start;
	start = CPU_GetCOUNT();

	while(1)
	{
    if(tgt_testchar())
	return 100;
	if(!count)break;
	if((CPU_GetCOUNT()-start>0x3000000 )){
	start = CPU_GetCOUNT();
	count--;
	}
	}

	   return 0;
}

static int get_data(unsigned char *ptr,unsigned int len,unsigned int timeout)
{
	int i=0;
	volatile int count=1;
	while(i<len)
	{
		if(testchar()>0)
		ptr[i++]=tgt_getchar();
		else {
		if(!count)break;
		dbg_printf("count=%d\n",count);
		count--;
		}
	}
	dbg_printf("i=%d\n",i);
    return i;
}



//计算CRC16
static unsigned int calcrc(unsigned char *ptr, unsigned int count)
{
    unsigned int crc = 0;
    while (count--)
    {
        crc =_crc_xmodem_update(crc,*ptr++);
    }
    return crc;
}

static int xmodem_transfer(char *base)
{
    unsigned char c;
    unsigned int i;
    unsigned int crc;
    unsigned int filesize=0;
    unsigned char BlockCount=1;               //数据块累计(仅8位，无须考虑溢出)

    //向PC机发送开始提示信息
        STATUS=ST_WAIT_START;               //并且数据='d'或'D',进入XMODEM
    c=0;
	while(1)
	{
		tgt_putchar(XMODEM_WAIT_CHAR);
		if(testchar()>0)break;

	}
    while(STATUS!=ST_OK)                  //循环接收，直到全部发完
    {
	
        i=get_data(&strXMODEM.SOH,BLOCKSIZE+5,1000);   //限时1秒，接收133字节数据
        if(i)
        {
            //分析数据包的第一个数据 SOH/EOT/CAN
            switch(strXMODEM.SOH)
            {
            case XMODEM_SOH:               //收到开始符SOH
                if (i>=(BLOCKSIZE+5))
                {
                    STATUS=ST_BLOCK_OK;
                }
                else
                {
                    STATUS=ST_BLOCK_FAIL;      //如果数据不足，要求重发当前数据块
                    tgt_putchar(XMODEM_NAK);
                }
                break;
            case XMODEM_EOT:               //收到结束符EOT
                tgt_putchar(XMODEM_ACK);            //通知PC机全部收到
                STATUS=ST_OK;
            printf("transfer succeed!\n");
                break;
            case XMODEM_CAN:               //收到取消符CAN
                tgt_putchar(XMODEM_ACK);            //回应PC机
                STATUS=ST_OK;
            printf("Warning:user cancelled!\n");
                break;
            default:                     //起始字节错误
                tgt_putchar(XMODEM_NAK);            //要求重发当前数据块
                STATUS=ST_BLOCK_FAIL;
                break;
            }
        }
		else 
		{
		dbg_printf("time out!\n");
			break;
		}
        if (STATUS==ST_BLOCK_OK)            //接收133字节OK，且起始字节正确
        {
			dbg_printf("BlockCount=%d,strXMODEM.BlockNo=%d\n",BlockCount,strXMODEM.BlockNo);
            if (BlockCount != strXMODEM.BlockNo)//核对数据块编号正确
            {
                tgt_putchar(XMODEM_NAK);            //数据块编号错误，要求重发当前数据块
                continue;
            }
            if (BlockCount !=(unsigned char)(~strXMODEM.nBlockNo))
            {
                tgt_putchar(XMODEM_NAK);            //数据块编号反码错误，要求重发当前数据块
                continue;
            }
            crc=strXMODEM.CRC16hi<<8;
            crc+=strXMODEM.CRC16lo;
            //AVR的16位整数是低位在先，XMODEM的CRC16是高位在先
            if(calcrc(&strXMODEM.Xdata[0],BLOCKSIZE)!=crc)
            {
                tgt_putchar(XMODEM_NAK);              //CRC错误，要求重发当前数据块
				dbg_printf("crc error\n");
                continue;
            }
            //正确接收128个字节数据，刚好是M16的一页
            memcpy(base+filesize,strXMODEM.Xdata,128);
            filesize+=128;
            tgt_putchar(XMODEM_ACK);                 //回应已正确收到一个数据块
            BlockCount++;                       //数据块累计加1
        }
    }

    //退出Bootloader程序，从0x0000处执行应用程序
    printf("xmodem finished\n");
    return filesize;
}

#if NRAMFILES > 0
struct Ramfile;
struct Ramfile *addRamFile(char *filename, unsigned long base, unsigned long size, int flags);
int deleteRamFile(char *filename);
#endif

static int xmodem(int argc,char *argv[])
{
    int i = 0;
    char buf_2[100];
    char tmp[20];
    int base_flash = 0xbfc00000;
    int start_ram = 0x80300000;
    char boot[20]="elf";
    void *base = NULL;
	char *file;
	int file_size;
#ifdef XMODEM_DEBUG
    pmsg=dbgbuf;
	dbg=0;
#endif
	base = 0x84000000;
	file="xmodem";

	for(i=1;i<argc;i++)
	{
	 if(!strncmp(argv[i],"base=",5))
	 {
	  base=strtoul(&argv[i][5],0,0);
	 }
	 else if(!strncmp(argv[i],"file=",5))
	 {
	  file=&argv[i][5];
	 }
#ifdef XMODEM_DEBUG
	 else if(!strncmp(argv[i],"dbg=",5))
	 {
	  dbg=strtoul(&argv[i][4],0,0);
	 }
#endif
	}


    printf("Waiting for serial transmitting datas\n");
    file_size = xmodem_transfer(base);
    printf("Load successfully! Start at 0x%x, size 0x%x\n", base, file_size);
#if NRAMFILES > 0
	deleteRamFile(file);
	addRamFile(file,base,file_size,0);
#endif
    return 0; 
}


static const Cmd Cmds[] =
{
	{"MyCmds"},
	{"xmodem","",0,"xmodem serial",xmodem,0,99,CMD_REPEAT},
#ifdef XMODEM_DEBUG
	{"dmsg","",0,"xmodem serial",dmsg,0,99,CMD_REPEAT},
#endif
	{0, 0}
};


static void init_cmd __P((void)) __attribute__ ((constructor));

static void
init_cmd()
{
	cmdlist_expand(Cmds, 1);
}

