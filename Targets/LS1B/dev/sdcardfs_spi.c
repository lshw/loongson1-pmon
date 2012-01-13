#include <stdio.h>
#include <string.h>
#include <termio.h>
#include <fcntl.h>
#include <file.h>
#include <ctype.h>
#include <ramfile.h>
#include <sys/unistd.h>
#include <stdlib.h>
#undef _KERNEL
#include <errno.h>
#include <pmon.h>
#include <cpu.h>
#include <include/types.h>
#include <pmon/dev/loopdev.h>

#include <sys/buf.h>
#include <sys/uio.h>


//#define SDCARD_DBG
#define NMOD_SDCARD_STORAGE 1

#define SPI_REG_BASE 0x1fe80000
#define FCR_SPCR        0x00		//控制寄存器
#define FCR_SPSR        0x01		//状态寄存器
#define FCR_SPDR        0x02		//数据传输寄存器
#define FCR_SPER        0x03		//外部寄存器
#define FCR_PARAM			0x04		//SPI Flash参数控制寄存器
#define FCR_SOFTCS		0x05		//SPI Flash片选控制寄存器
#define FCR_TIMING		0x06		//SPI Flash时序控制寄存器

#define SET_SPI(idx,value) KSEG1_STORE8(SPI_REG_BASE+idx, value)
#define GET_SPI(idx)    KSEG1_LOAD8(SPI_REG_BASE+idx)


static unsigned char  flash_writeb_cmd(unsigned char value)
{
	unsigned char status, ret;

	SET_SPI(FCR_SPDR, value);
	ret = GET_SPI(FCR_SPSR);

	//sw: make sure the rf is not empty!      
	while ((*(volatile unsigned char *)(0xbfe80001))&0x01);

	ret = GET_SPI(FCR_SPSR);

	ret = GET_SPI(FCR_SPDR);
#ifdef SDCARD_DBG
	printf("==treg ret value: %08b\n",ret);
#endif		
	return ret;
}

static void spi_init0(void)
{ 
	SET_SPI(FCR_SPSR, 0xc0);
	//SPI Flash参数控制寄存器
	SET_SPI(FCR_PARAM, 0x00);
	//#define SPER      0x3	//外部寄存器
	//spre:01 [2]mode spi接口模式控制 1:采样与发送时机错开半周期  [1:0]spre 与Spr一起设定分频的比率
	SET_SPI(FCR_SPER, 0x05);
	//SPI Flash片选控制寄存器
	SET_SPI(FCR_SOFTCS, 0xbf);			//softcs
	SET_SPI(FCR_SPCR, 0x5c);
}


#if 0
LPM_MISC 0xbf004100 
GPIO_O_15_8  0x21
GPIO_O_23_16  0x22
GPIO_I_23_16 0x12
GPIO_OE_23_16 0x2
GPIO20 sclk
GPIO21 si
GPIO22 so
GPIO23 cs
LPB_MISC_CFG 0x40 //bit 0:spi_en
#endif
volatile unsigned char *GPIO_O_23_16 = (unsigned char *)0xbf004122;
volatile unsigned char *GPIO_I_23_16 = 0xbf004112;
volatile unsigned char *LPB_MISC_CFG = 0xbf004140;
volatile unsigned char *GPIO_OE_23_16 = 0xbf004102;
unsigned cached_gpio=0;

static inline void set_cs(int bit)
{
	if(bit) 
		SET_SPI(FCR_SOFTCS, 0xFF);		//cs high
	else 
		SET_SPI(FCR_SOFTCS, 0xBF);		//cs low
}


void PutPortraitChar(unsigned int ScreenX,unsigned int Line,unsigned char *StringPointer,unsigned int Overwrite_Back_Ground){
	printf("%s\n",StringPointer);
}

unsigned int BlockSize;

//*P_Watchdog_Clear=0x0001

//CLK_High()
//*P_IOB_Data|=0x0100;

//CLK_Low()
//*P_IOB_Data&=0xfeff;

//CS_High()
//*P_IOB_Data|=0x0800;

//CS_Low()
//*P_IOB_Data&=0xf7ff;

//DO_High()
//*P_IOB_Data|=0x4000;

//DO_Low()
//*P_IOB_Data&=0xbfff;
//********************************************
void SD_2Byte_Write(unsigned int IOData)//Simulating SPI IO
{
	flash_writeb_cmd((IOData>>8) & 0xff);
	flash_writeb_cmd(IOData & 0xff);
}

//********************************************
void SD_Write(unsigned int IOData)//Simulating SPI IO
{
	flash_writeb_cmd(IOData & 0xff);
}

//********************************************
unsigned int SD_2Byte_Read(void)//Simulating SPI IO
{
	unsigned int Buffer;
	Buffer = flash_writeb_cmd(0xff);	//Provide 8 extra clock同时读取总线返回到SPI控制器的数据
	Buffer = (Buffer<<8) | flash_writeb_cmd(0xff);
	
	return Buffer;
}

//********************************************
static int read_delay = 20;
#define DELAY_STEP 5
unsigned short SD_Read(void)//Simulating SPI IO
{
	unsigned short ret = 0xff00 | flash_writeb_cmd(0xff);
	if(read_delay) delay(read_delay); //need some delay
	return ret;
}

//********************************************
//ResType:Response Type, send 1 for R1; send 2 for R1b; send 3 for R2.
unsigned int SD_CMD_Write(unsigned int CMDIndex, unsigned long CMDArg, unsigned int ResType, unsigned int CSLowRSV)
{
	//There are 7 steps need to do.(marked by [1]-[7])
	unsigned int Response, Response2, CRC, MaximumTimes;
	unsigned int cont;
	Response2 = 0;	//响应
	MaximumTimes = 10;
	CRC = 0x0095;//0x0095 is only valid for CMD0
	if (CMDIndex != 0) CRC = 0x00ff;

	set_cs(0);//[1] CS Low
	
	SD_2Byte_Write(((CMDIndex|0x0040)<<8)+(CMDArg>>24));//[2] Transmit Command_Index & 1st Byte of Command_Argument.
	SD_2Byte_Write((CMDArg&0x00ffff00)>>8);				//[2] 2nd & 3rd Byte of Command_Argument
	SD_2Byte_Write(((CMDArg&0x000000ff)<<8)+CRC);		//[2] 4th Byte of Command_Argument & CRC only for CMD0

	//[3] Do High
	//[3] Restore Do to High Level
	//[4] Provide 8 extra clock after CMD
	flash_writeb_cmd(0xff);
	//[5] wait response
	switch (ResType)
	{
		case 1://R1
		{
			cont = 100;
			do{
				Response = SD_Read();
				cont--;
			}while ((Response == 0xffff) && (cont != 0));
			break;
		}
		case 2://R1b
		{
			cont = 100;
			do{
				Response = SD_Read();
				cont--;
			}while ((Response == 0xffff) && (cont != 0));//Read R1 firstly
			
			cont = 100;
			do{
				Response2 = SD_Read()-0xff00;
				cont--;
			}
			while ((Response2 != 0) && (cont != 0));//Wait until the Busy_Signal_Token is non-zero
			break;	
		}
		case 3: Response = SD_2Byte_Read();break;//R2
	}
	
	if (CSLowRSV == 0) set_cs(1);//[6] CS High (if the CMD has data block response CS should be kept low)
	flash_writeb_cmd(0xff);
	return Response;
}

//********************************************
unsigned int SD_Reset_Card(void)
{
	unsigned int i,Response;
	unsigned int cont;

	for (i=0;i<10;i++)//Send 74+ Clocks
	{
		flash_writeb_cmd(0xff);
	}
	
	cont = 20;
	while(cont--)
	{
		for(i=0;i<10;i++)//The max initiation times is 10.
		{
			Response = SD_CMD_Write(0x0000, 0x00000000, 1, 0);//Send CMD0 CMD0——0x01（SD卡处于in-idle-state）
			if (Response == 0xff01) break;
		}
		if (Response!=0xff01){
			read_delay += DELAY_STEP;
		}
		else break;
	}
	
	if (cont == 0){
//	#ifdef CONFIG_CHINESE
//		printf("错误: 没有SD卡或者命令发送失败\n");
//	#else
//		printf("ERROR: No SD Card or command send failure\n");
//	#endif
	}
	return Response;
}

//********************************************
//Polling轮询 the card after reset
unsigned int SD_Initiate_Card(void)
{
	unsigned int i,Response;
	unsigned int cont;
	
	cont = 100;
	while(cont){
		for(i=0; i<30; i++)//The max initiation times is 10.
		{
			Response = SD_CMD_Write(0x0037,0x00000000,1,0);//Send CMD55	 CMD55——0x01（SD卡处于in-idle-state）
			if (Response == 0xff01){
				Response = SD_CMD_Write(0x0029,0x00000000,1,0);//Send ACMD41 ACMD41——0x00（SD卡跳出in-idle-state，完成初始化准备接受下一条指令）
					if (Response == 0xff00) break;
			}
		}
		if (Response != 0xff00){
			read_delay += DELAY_STEP;
			cont --;
		}
		else break;
	}
	if (cont == 0){
//	#ifdef CONFIG_CHINESE
//		printf("错误: 没有SD卡或者命令发送失败\n");
//	#else
//		printf("ERROR: No SD Card or command send failure\n");
//	#endif
	}
	return Response;
}

//********************************************
//Read CSD register
unsigned int SD_Get_CardInfo(unsigned char *ReadBuffer)
{
	unsigned int temp, Response, MaximumTimes;
	unsigned char temp2;
	MaximumTimes = 10;
	
	for(temp=0; temp<MaximumTimes; temp++)
	{
		//Response = SD_CMD_Write(9,0x00000000,3,1);//Send CMD9
		Response = SD_CMD_Write(9,0x00000000,1,1);//Send CMD9
		if (Response == 0xff00)
			temp=MaximumTimes;
	}
	
#ifdef CONFIG_CHINESE
	printf("获取CSD寄存器值...\n");
#else
	printf("get CSD register...\n");
#endif
	
	//Provide 8 clock to romove the first byte of data response (0x00fe)
	//sw: make sure the resp is 0xfe
	temp2 = flash_writeb_cmd(0xff);
	while(temp2 != 0xfe);
	
	for (temp=0; temp<8; temp++) 
	{
		unsigned short data;
		data = SD_2Byte_Read();
		ReadBuffer[2*temp] = (data>>8) & 0xff;
		ReadBuffer[2*temp+1] = data & 0xff;
	}
	
	//Provide 16 clock to remove the last 2 bytes of data response (CRC)
	flash_writeb_cmd(0xff);
	flash_writeb_cmd(0xff);
	
	set_cs(1);//CS_High()
	
	//Provide 8 extra clock after data response
	flash_writeb_cmd(0xff);

	return Response;
}

//********************************************
unsigned int SD_Overall_Initiation(void)
{
	unsigned int Response,Response_2;
	Response = 0x0000;
	Response_2 = 0xff00;
	
//	set_do(1);	//[1] Do High
				//[1] Do must be High when there is no transmition

	Response = SD_Reset_Card();//[2] Send CMD0
	if (Response != 0xff01){
	#ifdef CONFIG_CHINESE
		PutPortraitChar(0, 15, "没发现SD卡..", 1);//Print MSG
	#else
		PutPortraitChar(0, 15, "No SD Card..", 1);//Print MSG
	#endif
		Response_2 += 8;
		return Response_2;
	}

	Response = SD_Initiate_Card();//[3] Send CMD55+ACMD41 
	if (Response == 0xff00){
	#ifdef CONFIG_CHINESE
		PutPortraitChar(0,15,"初始化成功",1);//Print MSG
	#else
		PutPortraitChar(0,15,"Init Success",1);//Print MSG
	#endif
	}
	else{
		Response_2 += 4;
	#ifdef CONFIG_CHINESE
		PutPortraitChar(0,15,"没发现SD卡..",1);//Print MSG
	#else
		PutPortraitChar(0,15,"No SD Card..",1);//Print MSG
	#endif
	}

	return Response_2;
//	1111|1111||0000|0000 Response_2
//                  ||
//                  ||__CMD55+ACMD41 Fail
//                  |___CMD0 Fail
}

//********************************************
//Read CID register
unsigned int SD_Get_CardID(unsigned char *ReadBuffer)
{
	unsigned int temp, Response, MaximumTimes;
	MaximumTimes = 10;
	
	for(temp=0; temp<MaximumTimes; temp++)
	{
		Response = SD_CMD_Write(10, 0x00000000, 1, 1);//Send CMD10
		if (Response == 0xff00)
			//temp = MaximumTimes;
			break;
	}

#ifdef CONFIG_CHINESE
//	printf("获取CID寄存器值...\n");
#else
//	printf("get CID register...\n");
#endif

	//Provide 8 clock to romove the first byte of data response (0x00fe)
	//sw: make sure the resp is 0xfe
	//提供8个总线时钟读取SPI总线上的放回数据到SPI控制器
	while(flash_writeb_cmd(0xff) != 0xfe);
	
	//16*8=128bit
	for (temp=0; temp<8; temp++) //Get the CID data
	{
		unsigned short data;
		data = SD_2Byte_Read();
		ReadBuffer[2*temp] = (data>>8) & 0xff;
		ReadBuffer[2*temp+1] = data & 0xff;
	}

	//Provide 16 clock to remove the last 2 bytes of data response (CRC)
	flash_writeb_cmd(0xff);
	flash_writeb_cmd(0xff);

	set_cs(1);//CS_High()

	//Provide 8 extra clock after data response
	flash_writeb_cmd(0xff);

	return Response;
}

//********************************************
unsigned int Read_Single_Block(unsigned long int ByteAddress,unsigned char *ReadBuffer)
{
	unsigned int temp, Response, MaximumTimes;
	MaximumTimes = 10;
	
	for(temp=0; temp<MaximumTimes; temp++)
	{
		Response = SD_CMD_Write(17,ByteAddress,1,1);//Send CMD17
		if (Response == 0xff00)
			temp=MaximumTimes;
	}
	
	if (Response!=0xff00) return Response;
		
	while (SD_Read()!=0xfffe) {;}
	
	for (temp=0; temp<256; temp++) //Get the readed data
	{
		unsigned short data;
		data=SD_2Byte_Read();
		ReadBuffer[2*temp]=(data>>8)&0xff;
		ReadBuffer[2*temp+1]=data&0xff;
	}
	
	//Provide 16 clock to remove the last 2 bytes of data response (CRC)
	flash_writeb_cmd(0xff);
	flash_writeb_cmd(0xff);
	
	set_cs(1);//CS_High()
	
	//Provide 8 extra clock after data response
	flash_writeb_cmd(0xff);
	return Response;
}

//********************************************
unsigned int Write_Single_Block(unsigned long int ByteAddress,unsigned char *WriteBuffer)
{
	unsigned int temp, Response, MaximumTimes;
	MaximumTimes = 10;
	
	for(temp=0; temp<MaximumTimes; temp++)
	{
		Response=SD_CMD_Write(24,ByteAddress,1,1);//Send CMD24
		if (Response==0xff00)
			temp=MaximumTimes;
	}
	
	//Provide 8 extra clock after CMD response
	flash_writeb_cmd(0xff);
	
	SD_Write(0x00fe);//Send Start Block Token
	for (temp=0;temp<256;temp++) SD_2Byte_Write(((WriteBuffer[2*temp])<<8)|(WriteBuffer[2*temp+1]));//Data Block
	SD_2Byte_Write(0xffff);//Send 2 Bytes CRC
	
	Response = SD_Read();
	while (SD_Read()!=0xffff) {;}
	
	set_cs(1);//CS_High()
	
	//Provide 8 extra clock after data response
	flash_writeb_cmd(0xff);
	
	return Response;
}
//----------------------------------

static int sdcard_init(void)
{
//	static int inited=0;
	int inited = 0;
	int ret;

	if(inited)
		return 0;
//sw
//	*LPB_MISC_CFG |=1;
/*
GPIO20 sclk
GPIO21 si
GPIO22 so
GPIO23 cs
*/

/*
 * sw: do it in spi_init
 * 
	SET_SPI(FCR_SPCR, 0x5e);
	SET_SPI(FCR_SPER, 0x00);
*/
	
	spi_init0();

	set_cs(1);
	ret = SD_Overall_Initiation();
	inited = 1;
	return ret;
}

int test_sdcard(int argc, char **argv)
{
	char str[100];
	unsigned char ReadBuffer[16];
	int ret;
	
	ret = sdcard_init();
	if (ret != 0xff00)
		goto error1;
	SD_Get_CardID(ReadBuffer);
//	sprintf(str,"pcs 0;d1 0x%x 16 ", ReadBuffer);
//	do_cmd(str);
#ifdef CONFIG_CHINESE
	printf("制造商 ID (MID) : 0x%x\n", ReadBuffer[0]);
	printf("OEM/应用 ID (OID) : %c%c\n", ReadBuffer[1], ReadBuffer[2]);
	printf("产品名称 (PNM) : %c%c%c%c%c\n", ReadBuffer[3], ReadBuffer[4], ReadBuffer[5], ReadBuffer[6], ReadBuffer[7]);
#else
	printf("Manufacturer ID (MID) : 0x%x\n", ReadBuffer[0]);
	printf("OEM/Application ID (OID) : %c%c\n", ReadBuffer[1], ReadBuffer[2]);
	printf("Product Name (PNM) : %c%c%c%c%c\n", ReadBuffer[3], ReadBuffer[4], ReadBuffer[5], ReadBuffer[6], ReadBuffer[7]);
#endif
	
	SD_Get_CardInfo(ReadBuffer);
	sprintf(str,"pcs 0;d1 0x%x 16 ",ReadBuffer);
	do_cmd(str);
	return 0;
error1:
#ifdef CONFIG_CHINESE
	printf("SD卡初始化失败\n");
#else
	printf("SDcard Initialization failure\n");
#endif
//	return -1;
	return 0;
}

static const Cmd Cmds[] =
{
	{"MyCmds"},
	{"test_sdcard", "", 0, "test_sdcard", test_sdcard, 0, 99, CMD_REPEAT},
	{0, 0}
};

static void init_cmd __P((void)) __attribute__ ((constructor));

static void
init_cmd()
{
	cmdlist_expand(Cmds, 1);
}
//----------------------------------------

/*
 * Supported paths:
 *	/dev/sdcard
 */
#define CACHEDSECTORS 16
unsigned char cachedbuffer[512*CACHEDSECTORS];
unsigned int cachedaddr[CACHEDSECTORS];

static int sdcard_open (int fd, const char *fname, int mode, int perms)
{
	_file[fd].posn = 0;
	_file[fd].data = 0;
	sdcard_init();

	memset(cachedaddr, -1, sizeof(cachedaddr));
	
	return (fd);
}

/** close(fd) close fd */
static int sdcard_close (int fd)
{
	return(0);
}

/** read(fd,buf,n) read n bytes into buf from fd */
static int sdcard_read (int fd, void *buf, size_t n)
{
	unsigned int pos = _file[fd].posn;
	unsigned int left = n, once;

	while(left)
	{
		unsigned int index=(pos>>9)&(CACHEDSECTORS-1);
		unsigned int indexaddr=(pos>>9);
		unsigned char *indexbuf;

		indexbuf = &cachedbuffer[index*512];	
		if(cachedaddr[index] != indexaddr)
		Read_Single_Block(pos&~511, indexbuf);
		cachedaddr[index] = indexaddr;

		once = min(512 - (pos&511),left);
		memcpy(buf, indexbuf + (pos&511), once);
		buf += once;
		pos += once;
		left -= once;
	}
	_file[fd].posn = pos;

	return (n);
}


static int sdcard_write (int fd, const void *buf, size_t n)
{
	unsigned int pos=_file[fd].posn;
	unsigned long left = n;
	unsigned long once;

	while(left)
	{
		unsigned int index = (pos>>9)&(CACHEDSECTORS-1);
		unsigned int indexaddr = (pos>>9);
		unsigned char *indexbuf;
		indexbuf = &cachedbuffer[index*512];	
		if((cachedaddr[index] != indexaddr) && (pos&511))
		Read_Single_Block(pos&~511, indexbuf);
		cachedaddr[index] = indexaddr;

		once = min(512 - (pos&511),left);
		memcpy(indexbuf+(pos&511), buf, once);
		Write_Single_Block(pos&~511, indexbuf);
		buf += once;
		pos += once;
		left -= once;
	}
	_file[fd].posn = pos;
	return (n);
}

/*************************************************************
 *  sdcard_lseek(fd,offset,whence)
 */
static off_t sdcard_lseek(int fd, off_t offset, int whence)
{
	switch (whence) {
		case SEEK_SET:
			_file[fd].posn = offset;
		break;
		case SEEK_CUR:
			_file[fd].posn += offset;
		break;
		case SEEK_END:
			_file[fd].posn =  offset;
		break;
		default:
			errno = EINVAL;
		return (-1);
	}
	return (_file[fd].posn);
}

static FileSystem sdcardfs =
{
	"sdcard", FS_MEM,
	sdcard_open,
	sdcard_read,
	sdcard_write,
	sdcard_lseek,
	sdcard_close,
	NULL
};

static void init_fs __P((void)) __attribute__ ((constructor));

static void init_fs()
{
	/*
	 * Install ram based file system.
	 */
	filefs_init(&sdcardfs);
}

int sdcardmatch( struct device *parent, void *match, void *aux);
void sdcardattach(struct device *parent, struct device *self, void *aux);

struct cfattach sdcard_ca = {
	.ca_devsize = sizeof(struct loopdev_softc),
	.ca_match = sdcardmatch,
	.ca_attach = sdcardattach,
};

struct cfdriver sdcard_cd = {
	.cd_devs = NULL,
	.cd_name = "sdcard",
	.cd_class = DV_DISK,
};

int
sdcardmatch(parent, match, aux)
    struct device *parent;
    void *match, *aux;
{
//	struct confargs *ca = aux;
//	if (!strncmp(ca->ca_name, "sdcard",6))
//		return 1;
//	else
//		return 0;
	return 1;
}


void
sdcardattach(parent, self, aux)
    struct device *parent, *self;
    void *aux;
{
	struct loopdev_softc *priv = (void *)self;
	strncpy(priv->dev,"/dev/sdcard0",63);
	priv->bs = DEV_BSIZE;
	priv->seek = 0;
	priv->count = -1;
	priv->access = O_RDWR;
#if NGZIP > 0
	priv->unzip = 0;
#endif
}

