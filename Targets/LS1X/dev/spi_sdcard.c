#include <stdio.h>
#include <string.h>
#include <termio.h>
#include <fcntl.h>
#include <file.h>
#include <ctype.h>
#include <ramfile.h>
#include <stdlib.h>
#undef _KERNEL
#include <errno.h>
#include <pmon.h>
#include <cpu.h>

#include <pmon/dev/loopdev.h>

#include <sys/unistd.h>
#include <sys/buf.h>
#include <sys/uio.h>

//#define SDCARD_DBG

#ifdef SDCARD_USE_SPI0
#define SPI_REG_BASE 0x1fe80000
#elif SDCARD_USE_SPI1
#define SPI_REG_BASE 0x1fec0000
#endif

#define FCR_SPCR        0x00		//控制寄存器
#define FCR_SPSR        0x01		//状态寄存器
#define FCR_SPDR        0x02		//数据传输寄存器
#define FCR_SPER        0x03		//外部寄存器
#define FCR_PARAM		0x04		//SPI Flash参数控制寄存器
#define FCR_SOFTCS		0x05		//SPI Flash片选控制寄存器
#define FCR_TIMING		0x06		//SPI Flash时序控制寄存器

#define SET_SPI(idx, value)	KSEG1_STORE8(SPI_REG_BASE+idx, value)
#define GET_SPI(idx)		KSEG1_LOAD8(SPI_REG_BASE+idx)

#define MAX_TRY 200

unsigned char sdhc_flag = 0;
unsigned short resp[5];

static unsigned char flash_writeb_cmd(unsigned char value)
{
	unsigned char ret;

	SET_SPI(FCR_SPDR, value);

	/* sw: make sure the rf is not empty! */
	while ((GET_SPI(FCR_SPSR)) & 0x01);
	
	ret = GET_SPI(FCR_SPDR);
#ifdef SDCARD_DBG
	printf("==treg ret value: %08b\n",ret);
#endif		
	return ret;
}

static void ls1x_spi_init(void)
{
	unsigned int div, div_tmp;
	unsigned int bit;
	unsigned int clk;
#ifdef SDCARD_USE_SPI1
	#if LS1ASOC
		/* NAND复用SPI1 */
		*(volatile unsigned int*)0xbfd00420 &= ~(1 << 26);
	#elif LS1BSOC
		/* 使能SPI1控制器，与CAN0 CAN1 GPIO38-GPIO41复用,同时占用PWM0 PWM1用于片选. */
		/* 编程需要注意 */
		*(volatile unsigned int *)0xbfd00424 |= (0x3 << 23);
		/* disable gpio38-41 */
		*(volatile unsigned int *)0xbfd010c4 &= ~(0xf << 6);
	#endif
#endif
#ifdef SDCARD_USE_SPI0
	#if LS1ASOC
	#elif LS1BSOC
	/* disable gpio24-27 */
	*(volatile unsigned int *)0xbfd010c0 &= ~(0xf << 24);
	#endif
#endif

#define DIV_ROUND_UP(n,d) (((n) + (d) - 1) / (d))

	clk = tgt_cpufreq() / 2;
	div = DIV_ROUND_UP(clk, SDCARD_CLK);
	
	if (div < 2)
		div = 2;

	if (div > 4096)
		div = 4096;

	bit = fls(div) - 1;
	switch(1 << bit) {
		case 16: 
			div_tmp = 2;
			if (div > (1<<bit)) {
				div_tmp++;
			}
			break;
		case 32:
			div_tmp = 3;
			if (div > (1<<bit)) {
				div_tmp += 2;
			}
			break;
		case 8:
			div_tmp = 4;
			if (div > (1<<bit)) {
				div_tmp -= 2;
			}
			break;
		default:
			div_tmp = bit-1;
			if (div > (1<<bit)) {
				div_tmp++;
			}
			break;
	}
//	printf("clk = %ld div_tmp = %d bit = %d\n", clk, div_tmp, bit);

	/* 设置分频并使能spi */
	SET_SPI(FCR_SPCR, (div_tmp & 3) | 0x50);
	SET_SPI(FCR_SPER, (div_tmp >> 2) & 3);

	SET_SPI(FCR_SPSR, 0xc0);
	SET_SPI(FCR_PARAM, 0x00);
	SET_SPI(FCR_SOFTCS, ~((0x1<<SDCARD_CS)<<4));
}

/* 片选 需要根据具体使能那该引脚作为片选来修改 */
static inline void set_cs(int bit)
{
	if (bit) {
		/* CS拉高 */
		SET_SPI(FCR_SOFTCS, 0xFF);
	}
	else {
		/* CS拉低 */
		SET_SPI(FCR_SOFTCS, ~((0x1<<SDCARD_CS)<<4));
	}
}

void SD_2Byte_Write(unsigned int IOData)
{
	flash_writeb_cmd((IOData>>8) & 0xff);
	flash_writeb_cmd(IOData & 0xff);
}

void SD_Write(unsigned int IOData)
{
	flash_writeb_cmd(IOData & 0xff);
}

unsigned int SD_2Byte_Read(void)
{
	unsigned int Buffer;
	/* Provide 8 extra clock同时读取总线返回到SPI控制器的数据 */
	Buffer = flash_writeb_cmd(0xff);
	Buffer = (Buffer<<8) | flash_writeb_cmd(0xff);
	
	return Buffer;
}

static int read_delay = 20;
#define DELAY_STEP 5

unsigned short SD_Read(void)
{
	unsigned short ret = 0xff00 | flash_writeb_cmd(0xff);
/*
	if(read_delay) {
		int delay_t = read_delay;
		while(delay_t)
			delay_t--; //need some delay
	}
//	printk("===sd_read ret = %x\n",ret);
*/
	return ret;
}

/* ResType:Response Type, send 1 for R1; send 2 for R1b; send 3 for R2. */
unsigned int SD_CMD_Write(unsigned int CMDIndex, unsigned long CMDArg, unsigned int ResType, unsigned int CSLowRSV)
{
	/* There are 7 steps need to do.(marked by [1]-[7]) */
	unsigned int Response, Response2, CRC;
	unsigned int cont;
	Response2 = 0;
	CRC = 0x0095;	/* 0x0095 is only valid for CMD0 */

	if (CMDIndex != 0) CRC = 0x00ff;

	set_cs(0);
	
	SD_2Byte_Write(((CMDIndex|0x0040)<<8)+(CMDArg>>24));//[2] Transmit Command_Index & 1st Byte of Command_Argument.
	SD_2Byte_Write((CMDArg&0x00ffff00)>>8);				//[2] 2nd & 3rd Byte of Command_Argument
	SD_2Byte_Write(((CMDArg&0x000000ff)<<8)+CRC);		//[2] 4th Byte of Command_Argument & CRC only for CMD0

	//[3] Do High
	//[3] Restore Do to High Level
	//[4] Provide 8 extra clock after CMD
	flash_writeb_cmd(0xff);
	//[5] wait response
	switch (ResType) {
		case 1://R1
			cont = MAX_TRY;
			do {
				Response = SD_Read();
				cont--;
			} while ((Response == 0xffff) && (cont != 0));
			break;
		case 2://R1b
			cont = MAX_TRY;
			do {
				Response = SD_Read();
				cont--;
			} while ((Response == 0xffff) && (cont != 0));//Read R1 firstly
			
			cont = MAX_TRY;
			do {
				Response2 = SD_Read()-0xff00;
				cont--;
			} while ((Response2 != 0) && (cont != 0));//Wait until the Busy_Signal_Token is non-zero
			break;
		case 3:
			Response = SD_2Byte_Read();
			break;//R2
	}
	
	if (CSLowRSV == 0) set_cs(1);//[6] CS High (if the CMD has data block response CS should be kept low)
	flash_writeb_cmd(0xff);
	return Response;
}

unsigned int SD_Reset_Card(void)
{
	unsigned int i,Response;
	unsigned int cont = 20;

	/* Send 74+ Clocks */
	for (i=0; i<10; i++) {
		flash_writeb_cmd(0xff);
	}

	while (cont--) {
		for (i=0; i<MAX_TRY; i++) {
			/* Send CMD0 CMD0——0x01（SD卡处于in-idle-state） */
			Response = SD_CMD_Write(0x0000, 0x00000000, 1, 0);
			if (Response == 0xff01)
				break;
		}
		if (Response!=0xff01)
			read_delay += DELAY_STEP;
		else
			break;
	}
	
	if (cont == 0) {
//	#ifdef CONFIG_CHINESE
//		printf("错误: 没有SD卡或者命令发送失败\n");
//	#else
//		printf("ERROR: No SD Card or command send failure\n");
//	#endif
	}
	return Response;
}

unsigned int SD_CMD_Write_Crc(unsigned int CMDIndex, unsigned long CMDArg, unsigned char Crc, unsigned int ResType, unsigned int CSLowRSV)
{
	/* There are 7 steps need to do.(marked by [1]-[7]) */
	unsigned int Response, Response2, CRC, MaximumTimes;
	unsigned int cont;

	Response2 = 0;
	MaximumTimes = 10;
	CRC = 0x0095;//0x0095 is only valid for CMD0
	if (CMDIndex != 0)
		CRC = 0x00ff;

	set_cs(0);//[1] CS Low
	
	SD_2Byte_Write(((CMDIndex|0x0040)<<8)+(CMDArg>>24));//[2] Transmit Command_Index & 1st Byte of Command_Argument.
	SD_2Byte_Write((CMDArg&0x00ffff00)>>8);				//[2] 2nd & 3rd Byte of Command_Argument
	SD_2Byte_Write(((CMDArg&0x000000ff)<<8)+Crc);		//[2] 4th Byte of Command_Argument & CRC only for CMD0

	/* [3] Do High
	   [3] Restore Do to High Level
	   [4] Provide 8 extra clock after CMD */
	flash_writeb_cmd(0xff);

	/* [5] wait response */
	switch (ResType) {
		case 1://R1
			cont = MAX_TRY;
			do {
				Response = SD_Read();
				cont--;
			} while ((Response == 0xffff) && (cont != 0));
			break;
		case 2://R1b
			cont = MAX_TRY;
			do {
				Response = SD_Read();
				cont--;
			} while ((Response == 0xffff) && (cont != 0));//Read R1 firstly
			
			cont = MAX_TRY;
			do {
				Response2 = SD_Read()-0xff00;
				cont--;
			} while ((Response2 != 0) && (cont != 0));//Wait until the Busy_Signal_Token is non-zero
			break;
		case 3:
			Response = SD_2Byte_Read();
			break;//R2
		case 4://R7
			resp[0] = SD_Read();
			resp[1] = SD_Read();
			resp[2] = SD_Read();
			resp[3] = SD_Read();
			resp[4] = SD_Read();
			Response = resp[0];
			break;
	}

	/* [6] CS High (if the CMD has data block response CS should be kept low) */
	if (CSLowRSV == 0)
		set_cs(1);
	flash_writeb_cmd(0xff);
	return Response;
}

/* Polling轮询 the card after reset */
unsigned int SD_Initiate_Card(void)
{
	unsigned int i,Response;
	unsigned int cont = 200;
	sdhc_flag = 0;

	Response = SD_CMD_Write_Crc(0x0008, 0x000001aa, 0x87, 4, 0);	//lxy: check for voltage and partion

	while (cont) {
		for (i=0; i<MAX_TRY; i++) {
			/* Send CMD55	 CMD55——0x01（SD卡处于in-idle-state） */
			Response = SD_CMD_Write(0x0037, 0x00000000, 1, 0);
			if (Response == 0xff01) {
				/* Send ACMD41 ACMD41——0x00（SD卡跳出in-idle-state，完成初始化准备接受下一条指令）check HCS supported */
				Response = SD_CMD_Write(0x0029, 0x40000000, 1, 0);
				if (Response == 0xff00)
					break;
			}
		}
		if (Response != 0xff00) {
			read_delay += DELAY_STEP;
			cont --;
		}
		else
			break;
	}
	if (cont == 0) {
//	#ifdef CONFIG_CHINESE
//		printf("错误: 没有SD卡或者命令发送失败\n");
//	#else
//		printf("ERROR: No SD Card or command send failure\n");
//	#endif
	}
	return Response;
}

/* Read CSD register */
unsigned int SD_Get_CardInfo(unsigned char *ReadBuffer)
{
	unsigned int temp, Response, MaximumTimes;
	unsigned char temp2;
	MaximumTimes = 10;
	
	for (temp=0; temp<MaximumTimes; temp++) {
		/* Send CMD9 */
//		Response = SD_CMD_Write(9,0x00000000,3,1);
		Response = SD_CMD_Write(9,0x00000000,1,1);
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
	while (temp2 != 0xfe);
	
	for (temp=0; temp<8; temp++) {
		unsigned short data;
		data = SD_2Byte_Read();
		ReadBuffer[2*temp] = (data>>8) & 0xff;
		ReadBuffer[2*temp+1] = data & 0xff;
	}
	
	/* Provide 16 clock to remove the last 2 bytes of data response (CRC) */
	flash_writeb_cmd(0xff);
	flash_writeb_cmd(0xff);
	
	set_cs(1);
	
	/* Provide 8 extra clock after data response */
	flash_writeb_cmd(0xff);

	return Response;
}

unsigned int SD_Overall_Initiation(void)
{
	unsigned int Response,Response_2;
	Response = 0x0000;
	Response_2 = 0xff00;

	/* [1] Do High
	   [1] Do must be High when there is no transmition */
//	set_do(1);

	/* [2] Send CMD0 */
	Response = SD_Reset_Card();
	if (Response != 0xff01) {
	#ifdef CONFIG_CHINESE
		printf("没发现SD卡..\n");
	#else
		printf("No SD Card..\n");
	#endif
		Response_2 += 8;
		return Response_2;
	}

	/* [3] Send CMD55+ACMD41 */
	Response = SD_Initiate_Card();
	if (Response == 0xff00) {
	#ifdef CONFIG_CHINESE
		printf("初始化成功\n");
	#else
		printf("SDcard Init Success\n");
	#endif
	}
	else {
		Response_2 += 4;
	#ifdef CONFIG_CHINESE
		printf("没发现SD卡..\n");
	#else
		printf("No SD Card..\n");
	#endif
	}

	/* get ccs */
	Response = SD_CMD_Write_Crc(0x003a, 0x00000000, 0xfd, 4, 0);
	if ((resp[1] & 0x40) == 0x40)
		sdhc_flag = 1;

	return Response_2;
}

/* Read CID register */
unsigned int SD_Get_CardID(unsigned char *ReadBuffer)
{
	unsigned int temp, Response, MaximumTimes;
	MaximumTimes = 10;
	
	for (temp=0; temp<MaximumTimes; temp++) {
		/* Send CMD10 */
		Response = SD_CMD_Write(10, 0x00000000, 1, 1);
		if (Response == 0xff00)
			//temp = MaximumTimes;
			break;
	}

#ifdef CONFIG_CHINESE
//	printf("获取CID寄存器值...\n");
#else
//	printf("get CID register...\n");
#endif

	/* Provide 8 clock to romove the first byte of data response (0x00fe)
	   sw: make sure the resp is 0xfe
	   提供8个总线时钟读取SPI总线上的放回数据到SPI控制器 */
	while (flash_writeb_cmd(0xff) != 0xfe);
	
	/* 16*8=128bit Get the CID data */
	for (temp=0; temp<8; temp++) {
		unsigned short data;
		data = SD_2Byte_Read();
		ReadBuffer[2*temp] = (data>>8) & 0xff;
		ReadBuffer[2*temp+1] = data & 0xff;
	}

	/* Provide 16 clock to remove the last 2 bytes of data response (CRC) */
	flash_writeb_cmd(0xff);
	flash_writeb_cmd(0xff);

	set_cs(1);

	/* Provide 8 extra clock after data response */
	flash_writeb_cmd(0xff);

	return Response;
}

unsigned int Read_Single_Block(unsigned long int ByteAddress,unsigned char *ReadBuffer)
{
	unsigned int temp, Response, MaximumTimes;
	unsigned short data;
	MaximumTimes = 10;
	
	for (temp=0; temp<MaximumTimes; temp++) {
		/* Send CMD17 */
		Response = SD_CMD_Write(17, ByteAddress, 1, 1);
		if (Response == 0xff00)
			break;
	}
	
	if (Response != 0xff00)
		return Response;
		
	while (SD_Read() != 0xfffe) {
	}
	/* Get the readed data */
	for (temp=0; temp<256; temp++) {
		data = SD_2Byte_Read();
		ReadBuffer[2*temp] = (data>>8) & 0xff;
		ReadBuffer[2*temp+1] = data & 0xff;
	}
	
	/* Provide 16 clock to remove the last 2 bytes of data response (CRC) */
	flash_writeb_cmd(0xff);
	flash_writeb_cmd(0xff);
	
	set_cs(1);
	
	/* Provide 8 extra clock after data response */
	flash_writeb_cmd(0xff);
	return Response;
}

unsigned int Write_Single_Block(unsigned long int ByteAddress,unsigned char *WriteBuffer)
{
	unsigned int temp, Response, MaximumTimes;
	MaximumTimes = 10;
	
	for (temp=0; temp<MaximumTimes; temp++) {
		/* Send CMD24 */
		Response = SD_CMD_Write(24, ByteAddress, 1, 1);
		if (Response == 0xff00)
			break;
	}
	
	/* Provide 8 extra clock after CMD response */
	flash_writeb_cmd(0xff);

	/* Send Start Block Token */
	SD_Write(0x00fe);
	for (temp=0; temp<256; temp++) {
		/* Data Block */
		SD_2Byte_Write(((WriteBuffer[2*temp])<<8) | (WriteBuffer[2*temp+1]));
	}
	/* Send 2 Bytes CRC */
	SD_2Byte_Write(0xffff);
	
	Response = SD_Read();
	while (SD_Read()!=0xffff) {;}
	
	set_cs(1);
	
	/* Provide 8 extra clock after data response */
	flash_writeb_cmd(0xff);
	
	return Response;
}

static int sdcard_init(void)
{
//	static int inited=0;
	int ret;

//	if(inited)
//		return 0;
		
	ls1x_spi_init();
	set_cs(1);
	ret = SD_Overall_Initiation();
//	inited = 1;
	return ret;
}


#if 1
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
	sprintf(str, "pcs 0;d1 0x%x 16 ", ReadBuffer);
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

/* 用于测试SPI接口,不断发送0或1数据，用示波器观察clk波形和SPI0_MOSI波形是否正确 */
void test_spi(void)
{
	unsigned int i = 0;
	unsigned char val = 0xFF;
	
	ls1x_spi_init();
	set_cs(0);
	while (1) {
		flash_writeb_cmd(val);
		if (i == 0x1000000) {
			break;
		}
		if ((i%10) == 0) {
			val = ~val;
		}
		i++;
	}
}

static const Cmd Cmds[] = {
	{"MyCmds"},
	{"test_sdcard", "", 0, "test_sdcard", test_sdcard, 0, 99, CMD_REPEAT},
	{"test_spi", "", 0, "test spi controller", test_spi, 0, 99, CMD_REPEAT},
	{0, 0}
};

static void init_cmd __P((void)) __attribute__ ((constructor));

static void init_cmd(void)
{
	cmdlist_expand(Cmds, 1);
}
#endif

/* Supported paths: /dev/sdcard */
#define CACHEDSECTORS 16
unsigned char cachedbuffer[512 * CACHEDSECTORS];
unsigned int cachedaddr[CACHEDSECTORS];

static int sdcard_open (int fd, const char *fname, int mode, int perms)
{
	_file[fd].posn = 0;
	_file[fd].data = 0;
	sdcard_init();

	memset(cachedaddr, -1, sizeof(cachedaddr));
	
	return (fd);
}

/* close(fd) close fd */
static int sdcard_close(int fd)
{
	return(0);
}

/* read(fd,buf,n) read n bytes into buf from fd */
static int sdcard_read(int fd, void *buf, size_t n)
{
	unsigned int pos = _file[fd].posn;
	unsigned int left = n;
	unsigned int once;
	unsigned int index = (pos>>9)&(CACHEDSECTORS-1);
	unsigned int indexaddr = (pos>>9);
	unsigned char *indexbuf;

	while (left) {
		index = (pos>>9) & (CACHEDSECTORS-1);
		indexaddr = (pos>>9);

		indexbuf = &cachedbuffer[index*512];	
		if (cachedaddr[index] != indexaddr) {
			if (sdhc_flag == 1)
				Read_Single_Block(pos>>9, indexbuf);
			else
				Read_Single_Block(pos&~511, indexbuf);
		}
		cachedaddr[index] = indexaddr;

		once = min(512 - (pos&511), left);
		memcpy(buf, indexbuf + (pos&511), once);
		buf += once;
		pos += once;
		left -= once;
	}
	_file[fd].posn = pos;

	return (n);
}


static int sdcard_write(int fd, const void *buf, size_t n)
{
	unsigned int pos=_file[fd].posn;
	unsigned long left = n;
	unsigned long once;
	unsigned int index = (pos>>9)&(CACHEDSECTORS-1);
	unsigned int indexaddr = (pos>>9);
	unsigned char *indexbuf;

	while (left) {
		index = (pos>>9) & (CACHEDSECTORS-1);
		indexaddr = (pos>>9);
		indexbuf = &cachedbuffer[index*512];
		if ((cachedaddr[index] != indexaddr) && (pos&511)) {
			if (sdhc_flag == 1)
				Read_Single_Block(pos>>9, indexbuf);
			else
				Read_Single_Block(pos&~511, indexbuf);
		}
		cachedaddr[index] = indexaddr;

		once = min(512 - (pos&511),left);
		memcpy(indexbuf+(pos&511), buf, once);

		if (sdhc_flag == 1)
			Write_Single_Block(pos>>9, indexbuf);
		else
			Write_Single_Block(pos&~511, indexbuf);

		buf += once;
		pos += once;
		left -= once;
	}
	_file[fd].posn = pos;
	return (n);
}

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
			_file[fd].posn = offset;
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

static void init_fs(void)
{
	/* Install ram based file system. */
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

int sdcardmatch(struct device *parent, void *match, void *aux)
{
//	struct confargs *ca = aux;
//	if (!strncmp(ca->ca_name, "sdcard",6))
//		return 1;
//	else
//		return 0;
	return 1;
}


void sdcardattach(struct device *parent, struct device *self, void *aux)
{
	struct loopdev_softc *priv = (void *)self;
	strncpy(priv->dev, "/dev/sdcard0", 63);
	priv->bs = DEV_BSIZE;
	priv->seek = 0;
	priv->count = -1;
	priv->access = O_RDWR;
#if NGZIP > 0
	priv->unzip = 0;
#endif
}

