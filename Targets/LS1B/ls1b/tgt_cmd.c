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



//----------------------------------------

static int syscall_i2c_type,syscall_i2c_addrlen;
static char syscall_i2c_addr[3];

static int i2c_read_syscall(int type,long long addr,union commondata *mydata)
{
char c;
switch(type)
{
case 1:
syscall_i2c_addr[syscall_i2c_addrlen] = addr;
tgt_i2cread(syscall_i2c_type,syscall_i2c_addr,syscall_i2c_addrlen+1,&mydata->data1,1);

break;

default: return -1;break;
}
return 0;
}

static int i2c_write_syscall(int type,long long addr,union commondata *mydata)
{
char c;
switch(type)
{
case 1:
syscall_i2c_addr[syscall_i2c_addrlen] = addr;
tgt_i2cwrite(syscall_i2c_type,syscall_i2c_addr,syscall_i2c_addrlen+1,&mydata->data1,1);

break;

default: return -1;break;
}
return 0;
return -1;
}
//----------------------------------------

static int i2cs(int argc,char **argv)
{
	volatile int i;
	if(argc<2) 
		return -1;

	syscall_i2c_type=strtoul(argv[1],0,0);
	syscall_i2c_addrlen=argc-2;
	for(i=2;i<argc;i++)syscall_i2c_addr[i-2]=strtoul(argv[i],0,0);
	syscall1=(void*)i2c_read_syscall;
	syscall2=(void*)i2c_write_syscall;

	syscall_addrtype=0;

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

static int ehcitest(int argc,char **argv)
{
char str[100];
*(volatile int *)0xbfd00424 |= 0x80000000;
*(volatile int *)0xbfe00010 = 2;
*(volatile int *)0xbfe00054 = 0x1000;
*(volatile int *)0xbfe00010 = 1;
*(volatile int *)0xbfe00050 = 1;
delay(1000000);
strcpy(str,"pcs 0;d4 0xbfe00000 40;");
do_cmd(str);
	return 0;
}

extern void spi_read_w25x_id();
static const Cmd Cmds[] =
{
	{"MyCmds"},
	{"i2cs","0 for rtc,1 for ics950220", 0, "test i2c", i2cs, 0, 99, CMD_REPEAT},
	{"fcrtest","", 0, "fcrtest", fcrtest, 0, 99, CMD_REPEAT},
	{"ehcitest","", 0, "ehcitest",ehcitest, 0, 99, CMD_REPEAT},
	{"spi_read_w25x_id","",0,"spi_read_w25x_id",spi_read_w25x_id,0,99,CMD_REPEAT},
	{0, 0}
};

static void init_cmd __P((void)) __attribute__ ((constructor));

static void
init_cmd()
{
	cmdlist_expand(Cmds, 1);
}

