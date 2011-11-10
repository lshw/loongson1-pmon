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
#include <include/types.h>

#define SPI_REG_BASE 0x1f004000
#define FCR_SPCR        0x00
#define FCR_SPSR        0x01
#define FCR_SPDR        0x02
#define FCR_SPER        0x03

#define SET_SPI(idx,value) KSEG1_STORE8(SPI_REG_BASE+idx, value)
#define GET_SPI(idx)    KSEG1_LOAD8(SPI_REG_BASE+idx)
static unsigned char  flash_writeb_cmd(unsigned char value)
{
      unsigned char status, ret;
      SET_SPI(FCR_SPDR, value);
      while ((*(volatile unsigned char *)(0xbf004001))&0x01);
      return GET_SPI(FCR_SPDR);
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
volatile unsigned char *GPIO_O_23_16=0xbf004122;
volatile unsigned char *GPIO_I_23_16=0xbf004112;
volatile unsigned char *LPB_MISC_CFG=0xbf004140;
volatile unsigned char *GPIO_OE_23_16=0xbf004102;
unsigned cached_gpio=0;

inline void set_cs(int bit)
{
if(bit) cached_gpio|=(1<<(23-16));
else cached_gpio &=~(1<<(23-16));
*GPIO_O_23_16=cached_gpio;
cached_gpio=*GPIO_O_23_16; //flush write to pin
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
	flash_writeb_cmd((IOData>>8)&0xff);
	flash_writeb_cmd(IOData&0xff);
}
//********************************************
void SD_Write(unsigned int IOData)//Simulating SPI IO
{
	flash_writeb_cmd(IOData&0xff);
}
//********************************************
unsigned int SD_2Byte_Read()//Simulating SPI IO
{
	unsigned int BitCounter,Buffer;
	Buffer=flash_writeb_cmd(0xff);
	Buffer=(Buffer<<8)|flash_writeb_cmd(0xff);
	
	return Buffer;
}
//********************************************
static int read_delay=20;
#define DELAY_STEP 5
unsigned short SD_Read()//Simulating SPI IO
{
	unsigned short ret= 0xff00|flash_writeb_cmd(0xff);
    if(read_delay) delay(read_delay); //need some delay
	return ret;
}
//********************************************
unsigned int SD_CMD_Write(unsigned int CMDIndex,unsigned long CMDArg,unsigned int ResType,unsigned int CSLowRSV)//ResType:Response Type, send 1 for R1; send 2 for R1b; send 3 for R2.
{	//There are 7 steps need to do.(marked by [1]-[7])
	unsigned int temp,Response,Response2,CRC,MaximumTimes;
	Response2=0;
	MaximumTimes=10;
	CRC=0x0095;//0x0095 is only valid for CMD0
	if (CMDIndex!=0) CRC=0x00ff;
	
	set_cs(0);//[1] CS Low
	
	SD_2Byte_Write(((CMDIndex|0x0040)<<8)+(CMDArg>>24));//[2] Transmit Command_Index & 1st Byte of Command_Argument.
	SD_2Byte_Write((CMDArg&0x00ffff00)>>8);				//[2] 2nd & 3rd Byte of Command_Argument
	SD_2Byte_Write(((CMDArg&0x000000ff)<<8)+CRC);		//[2] 4th Byte of Command_Argument & CRC only for CMD0

	
	//[3] Do High
	//[3] Restore Do to High Level
	//[4] Provide 8 extra clock after CMD
	flash_writeb_cmd(0xff);
	
	switch (ResType)//[5] wait response
	{
		case 1://R1
				{
					do
						Response=SD_Read();
					while (Response==0xffff);
					break;
				}
		case 2://R1b
				{
					do
						Response=SD_Read();
					while (Response==0xffff);//Read R1 firstly
					
					do
						Response2=SD_Read()-0xff00;
					while (Response2!=0);//Wait until the Busy_Signal_Token is non-zero
					break;	
				}
		case 3: Response=SD_2Byte_Read();break;//R2
	}
	
	if (CSLowRSV==0) set_cs(1);//[6] CS High (if the CMD has data block response CS should be kept low)
	 
	flash_writeb_cmd(0xff);
	return Response;
}
//********************************************
unsigned int SD_Reset_Card()
{
	unsigned int i,Response;
	
	for (i=0;i<10;i++)//Send 74+ Clocks
	{
	flash_writeb_cmd(0xff);
	}
	
	while(1)
	{
	for(i=0;i<10;i++)//The max initiation times is 10.
	{
		Response=SD_CMD_Write(0x0000,0x00000000,1,0);//Send CMD0
		if (Response==0xff01) i=10;
	}
		if (Response!=0xff01) read_delay +=DELAY_STEP;
		else break;
		printf("Response=%x read_delay=%d\n",Response,read_delay);
	}

	return Response;
}
//********************************************
unsigned int SD_Initiate_Card()//Polling the card after reset
{
	unsigned int i,Response;
	
	while(1)
	{
	for(i=0;i<30;i++)//The max initiation times is 10.
	{
		Response=SD_CMD_Write(0x0037,0x00000000,1,0);//Send CMD55
		Response=SD_CMD_Write(0x0029,0x00000000,1,0);//Send ACMD41
		if (Response==0xff00) i=30;
	}
		if (Response!=0xff00) read_delay +=DELAY_STEP;
		else break;
		printf("Response=%x read_delay=%d\n",Response,read_delay);
	}

	return Response;
}
//********************************************
unsigned int SD_Get_CardInfo(unsigned char *ReadBuffer)//Read CSD register
{
	unsigned int temp,Response,MaximumTimes;
	MaximumTimes=10;
	
	for(temp=0;temp<MaximumTimes;temp++)
	{
		Response=SD_CMD_Write(9,0x00000000,3,1);//Send CMD9
		if (Response==0xff00)
			temp=MaximumTimes;
	}
	
	 //Provide 8 clock to romove the first byte of data response (0x00fe)
	 flash_writeb_cmd(0xff);
	
	for (temp=0;temp<8;temp++) 
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
unsigned int SD_Overall_Initiation()
{
	unsigned int Response,Response_2;
	Response=0x0000;
	Response_2=0xff00;
	
//	set_do(1);//[1] Do High
						//[1] Do must be High when there is no transmition

	Response=SD_Reset_Card();//[2] Send CMD0
	if (Response!=0xff01)
	{
		PutPortraitChar(0,15,"No SD Card..",1);//Print MSG
		Response_2+=8;
		return Response_2;
	}
		
	Response=SD_Initiate_Card();//[3] Send CMD55+ACMD41 
	if (Response==0xff00)
		PutPortraitChar(0,15,"Init Success",1);//Print MSG
	else
		{
		Response_2+=4;
		PutPortraitChar(0,15,"No SD Card..",1);//Print MSG
		}
	
	return Response_2;
//	1111|1111||0000|0000 Response_2
//                  ||
//                  ||__CMD55+ACMD41 Fail
//                  |___CMD0 Fail
}
//********************************************
unsigned int SD_Get_CardID(unsigned char *ReadBuffer)//Read CID register
{
	unsigned int temp,Response,MaximumTimes;
	MaximumTimes=10;
	
	for(temp=0;temp<MaximumTimes;temp++)
	{
		Response=SD_CMD_Write(10,0x00000000,1,1);//Send CMD9
		if (Response==0xff00)
			temp=MaximumTimes;
	}
	
	//Provide 8 clock to romove the first byte of data response (0x00fe)
	flash_writeb_cmd(0xff);
	
	for (temp=0;temp<8;temp++) //Get the CID data
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
unsigned int Read_Single_Block(unsigned long int ByteAddress,unsigned char *ReadBuffer)
{
	unsigned int temp,Response,MaximumTimes;
	MaximumTimes=10;
	
	for(temp=0;temp<MaximumTimes;temp++)
	{
		Response=SD_CMD_Write(17,ByteAddress,1,1);//Send CMD17
		if (Response==0xff00)
			temp=MaximumTimes;
	}
	
	if (Response!=0xff00) return Response;
		
	while (SD_Read()!=0xfffe) {;}
	
	for (temp=0;temp<256;temp++) //Get the readed data
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
	unsigned int temp,Response,MaximumTimes;
	MaximumTimes=10;
	
	for(temp=0;temp<MaximumTimes;temp++)
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
	
	Response=SD_Read();
	while (SD_Read()!=0xffff) {;}
	
	set_cs(1);//CS_High()
	
	//Provide 8 extra clock after data response
	flash_writeb_cmd(0xff);
	
	return Response;
}
//----------------------------------

static void sdcard_init()
{
	unsigned char tmp;
	static int inited=0;

	if(inited)return;
	*LPB_MISC_CFG |=1;
/*
GPIO20 sclk
GPIO21 si
GPIO22 so
GPIO23 cs
*/
  SET_SPI(FCR_SPCR, 0x5e);
  SET_SPI(FCR_SPER, 0x00);
	tmp=*GPIO_OE_23_16;
	tmp|=(3<<(20-16))|(1<<(23-16));
	tmp&=~(1<<(22-16));
	*GPIO_OE_23_16=tmp;
	cached_gpio=*GPIO_O_23_16; 
	set_cs(1);
	//set_do(1);
	SD_Overall_Initiation();
	inited=1;
}

int test_sdcard(int argc,char **argv)
{
char str[100];
unsigned char ReadBuffer[16];
	sdcard_init();
	SD_Get_CardID(ReadBuffer);
	sprintf(str,"pcs 0;d1 0x%x 16 ",ReadBuffer);
	do_cmd(str);
	SD_Get_CardInfo(ReadBuffer);
	sprintf(str,"pcs 0;d1 0x%x 16 ",ReadBuffer);
	do_cmd(str);
	return 0;
}

static const Cmd Cmds[] =
{
	{"MyCmds"},
	{"test_sdcard","",0,"test_sdcard",test_sdcard,0,99,CMD_REPEAT},
	{0, 0}
};

static void init_cmd __P((void)) __attribute__ ((constructor));

static void
init_cmd()
{
	cmdlist_expand(Cmds, 1);
}
//----------------------------------------

#if 1
/*
 * Supported paths:
 *	/dev/sdcard
 */
#define CACHEDSECTORS 16
unsigned char cachedbuffer[512*CACHEDSECTORS];
unsigned int cachedaddr[CACHEDSECTORS];
static int
   sdcard_open (fd, fname, mode, perms)
   int	       fd;
   const char *fname;
   int         mode;
   int	       perms;
{

	_file[fd].posn = 0;
	_file[fd].data = 0;
	sdcard_init();

	memset(cachedaddr,-1,sizeof(cachedaddr));
	
	return (fd);
}

/** close(fd) close fd */
static int
   sdcard_close (fd)
   int             fd;
{
	return(0);
}

/** read(fd,buf,n) read n bytes into buf from fd */
static int
   sdcard_read (fd, buf, n)
   int fd;
   void *buf;
   size_t n;
{
	unsigned int pos=_file[fd].posn;
	unsigned int left=n,once;

        while(left)
        {
		unsigned int index=(pos>>9)&(CACHEDSECTORS-1);
		unsigned int indexaddr=(pos>>9);
		unsigned char *indexbuf;

		indexbuf=&cachedbuffer[index*512];	
		if(cachedaddr[index]!=indexaddr)
          Read_Single_Block(pos&~511,indexbuf);
		  cachedaddr[index]=indexaddr;

        once=min(512 - (pos&511),left);
        memcpy(buf,indexbuf+(pos&511),once);
        buf+=once;
        pos+=once;
        left-=once;
        }
        _file[fd].posn=pos;

	return (n);
}


static int
   sdcard_write (fd, buf, n)
   int fd;
   const void *buf;
   size_t n;
{
		unsigned int pos=_file[fd].posn;
        unsigned long left = n;
        unsigned long once;

        while(left)
        {
		unsigned int index=(pos>>9)&(CACHEDSECTORS-1);
		unsigned int indexaddr=(pos>>9);
		unsigned char *indexbuf;
		indexbuf=&cachedbuffer[index*512];	
		if((cachedaddr[index]!=indexaddr)&&(pos&511))
          Read_Single_Block(pos&~511,indexbuf);
		  cachedaddr[index]=indexaddr;

        once=min(512 - (pos&511),left);
        memcpy(indexbuf+(pos&511),buf,once);
        Write_Single_Block(pos&~511,indexbuf);
        buf+=once;
        pos+=once;
        left-=once;
        }
        _file[fd].posn=pos;
	return (n);
}

/*************************************************************
 *  sdcard_lseek(fd,offset,whence)
 */
static off_t
sdcard_lseek (fd, offset, whence)
	int             fd, whence;
	off_t            offset;
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

static void
   init_fs()
{
	/*
	 * Install ram based file system.
	 */
	filefs_init(&sdcardfs);
}
#endif
