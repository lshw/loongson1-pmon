#include <pmon.h>
#include <target/i2c.h>
union commondata{
		unsigned char data1;
		unsigned short data2;
		unsigned int data4;
		unsigned int data8[2];
		unsigned char c[8];
};
extern unsigned int syscall_addrtype;
extern int (*syscall1)(int type,long long addr,union commondata *mydata);
extern int (*syscall2)(int type,long long addr,union commondata *mydata);
unsigned char i2c_read(unsigned char addr,int v);
int i2c_write(unsigned char data, unsigned char addr,int v);
static int rtc_verbose=0;

static int __rtcsyscall1(int type,long long addr,union commondata *mydata)
{
switch(type)
{
case 1:mydata->data1=i2c_read(addr,rtc_verbose);break;
default:break;
}
return 0;
}

static int __rtcsyscall2(int type,long long addr,union commondata *mydata)
{
switch(type)
{
case 1: i2c_write(addr,mydata->data1,rtc_verbose);return 0;
default:break;
}
}


static int Ics950220Read(int type,long long addr,union commondata *mydata)
{
unsigned char c;
unsigned char count;
switch(type)
{
case 1:
i2c_send(I2C_START|I2C_WRITE,0xd2);
i2c_send(I2C_WRITE,addr);
i2c_send(I2C_START|I2C_WRITE,0xd3);
count=i2c_recv();
i2c_send(I2C_WACK,0);
c=i2c_recv();
mydata->data1=c;

i2c_send(I2C_STOP|I2C_WRITE,0);
break;
default: return -1;break;
}
return 0;
}

static int Ics950220Write(int type,long long addr,union commondata *mydata)
{
char c;
switch(type)
{
case 1:
i2c_send(I2C_START|I2C_WRITE,0xd2);
i2c_send(I2C_WRITE,addr);
i2c_send(I2C_WRITE,1);
i2c_send(I2C_WRITE,mydata->data1);
i2c_send(I2C_STOP|I2C_WRITE,0);
break;

default: return -1;break;
}
return 0;
return -1;
}

static int i2cs(int argc,char **argv)
{
    unsigned int tmp;
	if(argc<2)return -1;
	if(!strcmp(argv[1],"init")){
	if(argc!=3)return -1;
	tmp=strtoul(argv[2],0,0);

 * FCR_SOC_I2C_PRER_LO = tmp&0xff;
 * FCR_SOC_I2C_PRER_HI = (tmp>>8)&0xff;
 * FCR_SOC_I2C_CTR = 0x80;
	return 0;
	}
	switch(strtoul(argv[1],0,0))
	{
	case 0:
	syscall1=__rtcsyscall1;
	syscall2=__rtcsyscall2;
	rtc_verbose=(argc==2)?0:1;
    printf("select rtc\n");
	break;
	case 1:
	syscall1=Ics950220Read;
	syscall2=Ics950220Write;
    printf("select Ics950220\n");
	break;
	default:
	return -1;
	break;
	}
	return 0;
}
static void fcrtest(int argc,char *argv[])
{
int i;
char buf[100];
printf("test spi");
sprintf(buf,"spi_read_w25x_id");
do_cmd(buf);
sprintf(buf,"spi_read_w25x_id");
do_cmd(buf);
printf("test i2c");
sprintf(buf,"i2cs 0");
do_cmd(buf);
for(i=0;i<5;i++)
{
delay(1000*1000);
sprintf(buf,"d1 0 10");
do_cmd(buf);
}
}

static unsigned char phy_addr=0x13;

static int phys_read(int type,long long addr,union commondata *mydata)
{
unsigned char c;
unsigned char count;
switch(type)
{
case 2:
mydata->data2=phy_read((unsigned char)phy_addr,(unsigned char)addr);
break;
default: return -1;break;
}
return 0;
}

static int phys_write(int type,long long addr,union commondata *mydata)
{
char c;
switch(type)
{
case 2:
phy_write((unsigned char)phy_addr,(unsigned char)addr,mydata->data2);
break;
default: return -1;break;
}
return 0;
return -1;
}

static int cmd_phys(int argc,char **argv)
{
	syscall1=phys_read;
	syscall2=phys_write;
	syscall_addrtype=0x10;
if(argc>1)phy_addr=strtoul(argv[1],0,0);
 return 0;
}

extern void spi_read_w25x_id();
static const Cmd Cmds[] =
{
	{"MyCmds"},
	{"phys","phys phyaddr", 0, "read phy", cmd_phys, 0, 99, CMD_REPEAT},
	{"i2cs","0 for rtc,1 for ics950220", 0, "test i2c", i2cs, 0, 99, CMD_REPEAT},
	{"fcrtest","", 0, "fcrtest", fcrtest, 0, 99, CMD_REPEAT},
	{"spi_read_w25x_id","",0,"spi_read_w25x_id",spi_read_w25x_id,0,99,CMD_REPEAT},
	{0, 0}
};

static void init_cmd __P((void)) __attribute__ ((constructor));

static void
init_cmd()
{
	cmdlist_expand(Cmds, 1);
}

