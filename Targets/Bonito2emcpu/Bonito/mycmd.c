#include "target/sbd.h"
#include <include/pmon.h>
#define nr_printf printf
#define nr_gets gets
#define nr_strtol strtoul
#define nr_strtoll strtoull
unsigned long long
strtoull(const char *nptr,char **endptr,int base);

//-------------------------------------------PNP------------------------------------------
// MB PnP configuration register
#define PNP_KEY_ADDR    0xbfd003f0
#define PNP_DATA_ADDR   0xbfd003f1

// Device LDN
#define LDN_SERIAL1     0x01
#define LDN_SERIAL2     0x02
#define LDN_PARALLEL    0x03
#define LDN_KEYBOARD    0x05
#define LDN_MOUSE       0x06



void PNPSetConfig(char LdnNumber, char Index, char data);
char PNPGetConfig(char LdnNumber, char Index);



void EnterMBPnP(void)
{

                outb(PNP_KEY_ADDR,0x87);
                outb(PNP_KEY_ADDR,0x87);

}

void ExitMBPnP(void)
{
        outb(PNP_KEY_ADDR,0xaa);
}

void PNPSetConfig(char LdnNumber, char Index, char data)
{
        EnterMBPnP();                                // Enter IT8712 MB PnP mode
        outb(PNP_KEY_ADDR,0x07);
        outb(PNP_DATA_ADDR,LdnNumber);
        outb(PNP_KEY_ADDR,Index);
        outb(PNP_DATA_ADDR,data);
        ExitMBPnP();
}

char PNPGetConfig(char LdnNumber, char Index)
{
        char rtn;

        EnterMBPnP();                                // Enter IT8712 MB PnP mode
        outb(PNP_KEY_ADDR,0x07);
        outb(PNP_DATA_ADDR,LdnNumber);
        outb(PNP_KEY_ADDR,Index);
        rtn = inb(PNP_DATA_ADDR);
        ExitMBPnP();
        return rtn;
}


static int PnpRead(int argc,char **argv)
{
	unsigned char LdnNumber,Index,data;
		if(argc!=3){nr_printf("pnpr LdnNumber Index\n");return -1;}
		
		LdnNumber=nr_strtol(argv[1],0,0);
		Index=nr_strtol(argv[2],0,0);
data=PNPGetConfig(LdnNumber,Index);
nr_printf("pnpread logic device %d ,index=0x%02x,value=0x%02x\n",LdnNumber,Index,data);
return 0;
}

static int PnpWrite(int argc,char **argv)
{
        unsigned char LdnNumber,Index,data;
                if(argc!=4){nr_printf("pnpw LdnNumber Index data\n");return -1;}
		LdnNumber=nr_strtol(argv[1],0,0);
		Index=nr_strtol(argv[2],0,0);
		data=nr_strtol(argv[3],0,0);

PNPSetConfig(LdnNumber,Index,data);
nr_printf("pnpwrite logic device %d ,index=0x%02x,value=0x%02x,",LdnNumber,Index,data);
data=PNPGetConfig(LdnNumber,Index);
nr_printf("result=0x%02x\n",data);
return 0;
}

#define WATCHDOG_REG BONITO(0x0160)
static int watchdog(int argc,char **argv)
{
char c;
long l;
if(argc>2)return -1;
if(argc==1)
{
	printf("value=%08x\n",WATCHDOG_REG);
}
else
{
l=strtoul(argv[1],0,0);
if(l==-1){WATCHDOG_REG =0;}
else {WATCHDOG_REG =0;WATCHDOG_REG=(3<<24)|l;}
}
return 0;
}
//-------------------------------------------------------------------------------------------------
static int cmd_reset(int argc,char **argv)
{
volatile int *p=0xbfe00170;
int i;
if(argc!=2)return -1;
i=strtoul(argv[1],0,0);
p[0]=0;
p[0]=i;
/*
 *必须要延迟，使cpu不发请求，否则因为不是同时启动会有问题
 */
for(i=0;i<3;i++)
delay(1000000UL);
//while(1);
return 0;
}

static int cmd_reset1(int argc,char **argv)
{
volatile int *p=0xbfe00170;
int i;
if(argc!=4)return -1;
i=strtoul(argv[1],0,0);
tgt_delay(strtoul(argv[2],0,0));
p[0]=0;
p[0]=i;
tgt_delay(strtoul(argv[3],0,0));
return 0;
}

static int cmd_test(int argc,char **argv)
{
	char c='0';
	int delay;
	int i;
	delay=strtoul(argv[1],0,0);
	for(i=0;;i++)
	{
	if(tgt_testchar()&& tgt_getchar()=='q')break;

	tgt_delay(delay);
		tgt_putchar(c+i%10);
	
	tgt_delay(delay);
		tgt_putchar('\r');
	
	tgt_delay(delay);
		tgt_putchar('\n');
	}
return 0;	
}
int highmemcpy(long long dst,long long src,long long count);
int highmemset(long long addr,unsigned char c,unsigned long long count);
void mycacheflush(unsigned long long addr,unsigned int size,unsigned int rw);
static int cmd_cpumap(int argc,char **argv)
{
unsigned long long mapaddr;
if(argc!=2)return -1;
mapaddr=nr_strtoll(argv[1],0,0);
//unsigned long long cpureg=0x900000001ff00000ULL;
asm("li $2,0x08000000;
	 sd $2,0x18(%0);
	 sd $2,0x28(%0);
	 sd %1,0x20(%0);
	 "
	 ::"r"(0x900000001ff00000ULL),"r"(mapaddr)
	 :"$2"
   );
return 0;
}

/*
 * 1个cache行地址对齐的地址只能归一个cpu所有，因为cache行刷新是按照行来做的。
 * 保证两个cpu不会同时写
 * 结构
 * 输出enable,输入enable,输入数据
 * 输 
 */

#include "serial.c"
#define SHAREBASEADDRESS 0x98000000c0000000ULL
#define COMBASEADDRESS 0x98000000c0100000ULL
static void serial_init()
{
unsigned long long base;
int line;
	for(line=0;line<3;line++)
	{
	base=COMBASEADDRESS+line*64;
	initserial(line);
	highmemset(base,0,64);
	mycacheflush(base,64,1);
	}
}



struct serial_struc{
char wp; //写指针,主cpu将串口数据写到fifo中
char rp; //读指针,主cpu读从cpu发来的数据
char data[30];
} ;
#define MYDBG printf("%s:%d\n",__FUNCTION__,__LINE__);

#define nextp(x) ((x+1)%30)
int serial_poll(int unuseded)
{
unsigned long long base;
struct serial_struc ibuf;
struct serial_struc obuf;
int commited;
int line;
char state;
static unsigned int count=0;
static int busy=0;
if(busy)return 0;
busy=1;
count++;

	for(line=0;line<3;line++)
	{
		commited=0;
		base=COMBASEADDRESS+line*64;
				mycacheflush(base+32,32,0);
				highmemcpy(&obuf,base,32);
				highmemcpy(&ibuf,base+32,32);
		if(count%0x1000==0)
		{
			while(getDebugstate(line)&0x01)
			{
				if(nextp(obuf.wp)!=ibuf.rp) {
					obuf.data[obuf.wp]=readDebugChar(line);
					obuf.wp=nextp(obuf.wp);
					commited=1;
				}
				else break;
			}
		}

			   if(ibuf.wp!=obuf.rp)
			   {
			while(getDebugstate(line)&0x20)
			{
			   if(ibuf.wp!=obuf.rp)
			   {
				   writeDebugChar(line,ibuf.data[obuf.rp]);
				   obuf.rp=nextp(obuf.rp);
				   commited=1;
			   }
			   else break;
			}
			   }

			if(commited){
				highmemcpy(base,&obuf,32);
				mycacheflush(base,32,1);
						}
	}
busy=0;
return 0;
}

#define CMDBASEADDRESS 0x98000000c0120000ULL
struct mycmd_struc{
char wp;
char rp;
char data[30];
};

static int inidx[4][3]={{3,6,9},
                {0,7,10},
                {1,4,11},
                {2,5,8}
                    };

extern int cpuid;
int mycmd_poll(int unuseded)
{
unsigned long long ibase;
unsigned long long obase;
struct mycmd_struc ibuf;
struct mycmd_struc obuf;
int commited;
int cpu;
char state;
static unsigned int count=0;
static char cmdbuf[3][0x100];
static int cmdp[3]={0,0,0};
int idx;
static int  busy=0;
count++;
if(count%0x1000!=0||busy)return 0;
busy=1;

	for(cpu=0;cpu<3;cpu++)
	{
		commited=0;
		ibase=CMDBASEADDRESS+(cpuid*3+cpu)*sizeof(struct mycmd_struc);
		obase=CMDBASEADDRESS+inidx[cpuid][cpu]*sizeof(struct mycmd_struc);
				mycacheflush(ibase,32,0);
				highmemcpy(&ibuf,ibase,32);
				highmemcpy(&obuf,obase,32);
				if(ibuf.wp==obuf.rp)continue;
				idx=cmdp[cpu];
				while(ibuf.wp!=obuf.rp)
				{
						cmdbuf[cpu][idx++]=ibuf.data[obuf.rp];
						obuf.rp=nextp(obuf.rp);
					if((ibuf.data[obuf.rp]==0)||(idx==0x100))
					{
						highmemcpy(obase,&obuf,32);
						mycacheflush(obase,32,1);
						do_cmd(cmdbuf[cpu]);
						idx=0;
					}
				}
				cmdp[cpu]=idx;
				highmemcpy(obase,&obuf,32);
				mycacheflush(obase,32,1);
	}

busy=0;
return 0;
}

static int cpu_runcmd(int cpu,char *str)
{
int cpu;
unsigned long long ibase;
unsigned long long obase;
struct mycmd_struc ibuf;
struct mycmd_struc obuf;
char *p=str;
int quit=0;
if(!*p)return -1;
		ibase=CMDBASEADDRESS+(cpuid*3+cpu)*sizeof(struct mycmd_struc);
		obase=CMDBASEADDRESS+inidx[cpuid][cpu]*sizeof(struct mycmd_struc);
		while(!quit)
		{
				mycacheflush(ibase,32,0);
				highmemcpy(&ibuf,ibase,32);
				highmemcpy(&obuf,obase,32);
				while(nextp(obuf.wp)!=ibuf.rp)
				{
					obuf.data[obuf.wp]=*p;
					obuf.wp=nextp(obuf.wp);
					if(!*p){quit=1;break;}
					p++;
				}
				highmemcpy(obase,&obuf,32);
				mycacheflush(obase,32,1);
				idle();
		}
return 0;
}

static int cmd_cpurun(int argc,char **argv)
{
int cpu;
int i;
char myline[0x200];
if(argc<3)return -1;

cpu=strtoul(argv[1],0,0);
myline[0]=0;
for(i=2;i<argc;i++)
{
strcat(myline,argv[i]);
strcat(myline," ");
}

 cpu_runcmd(cpu,myline);
	return 0;
}

int
fake_serialdrv (int op, struct DevEntry *dev, unsigned long param, int data)
{
struct serial_struc ibuf;
struct serial_struc obuf;
int commited=0;
char ret=0;
int	line;
unsigned long long base;
char str[100];

line= (int) dev->sio -1;
base=COMBASEADDRESS+line*64;

if((op==OP_TXRDY)||(op==OP_TX)||(op==OP_RXRDY)||(op==OP_RX))
{
		mycacheflush(base,32,0);
		highmemcpy(&obuf,base+32,32);
		highmemcpy(&ibuf,base,32);
}
	switch (op) {
		case OP_INIT:
		mycacheflush(base,64,0);
			return 0;

		case OP_XBAUD:
		case OP_BAUD:
			return 0;

		case OP_TXRDY:
		if(nextp(obuf.wp)!=ibuf.rp) {
			return 1;
		}
		break;

		case OP_TX:
		 obuf.data[obuf.wp]=data;
		 obuf.wp=nextp(obuf.wp);
		 commited=1;
			break;

		case OP_RXRDY:
	   if(ibuf.wp!=obuf.rp) return 1;
	   break;

		case OP_RX:
		   data=ibuf.data[obuf.rp];
		   obuf.rp=nextp(obuf.rp);
		   ret=data;
		   commited=1;
		   break;

		case OP_RXSTOP:
			break;
	}
	if(commited)
	{
		highmemcpy(base+32,&obuf,32);
		mycacheflush(base+32,32,1);
	}
	return ret;
}

static int cmd_or(int ac,char **av)
{
int i;
char myline[0x200];
if(ac<2)return -1;
myline[0]=0;
for(i=1;i<ac;i++)
{
strcat(myline,av[i]);
strcat(myline," ");
}
do_cmd(myline);
return 0;
}

int dma_poll(void *unused)
{
    int int_status = BONITO_INTISR ;
	static unsigned  int i=0;
	static int busy=0;
	if(busy)return 0;
	busy=1;
    if(int_status&(1<<10)){
		printf("dma timeout %d.\n",i++);
      while(int_status&(1<<10)){
        delay(1000);
        int_status = BONITO_INTISR;
      }
    }
	busy=0;
	return 0;
}

static int cmd_count(int argc,char **argv)
{
	unsigned long long count;
	asm("dmfc0 %0,$9":"=r"(count));
	printf("%08lx%08lx\n",(count>>32),count&0xffffffff);
	return 0;
}

static int cmd_testw(int argc,char **argv)
{
	unsigned long count;
	unsigned long long addr;
	char i=0;
	if(argc!=3)return -1;
	addr=strtoul(argv[1],0,0);
	addr|=0x9000000000000000ULL;
	count=strtoul(argv[2],0,0);
	while(count--)
	{
	  asm(" sb %1,(%0)
		   "
		  ::"r"(addr),"r"(i)
		  :"$2"
		 );
	}
	return 0;
}
//------------------------------------------------------------------------------------------------

static const Cmd Cmds[] =
{
	{"MyCmds"},
	{"pnpr",	"LDN index", 0, "pnpr LDN(logic device NO) index", PnpRead, 0, 99, CMD_REPEAT},
	{"pnpw",	"LDN index value", 0, "pnpw LDN(logic device NO) index value", PnpWrite, 0, 99, CMD_REPEAT},
	{"watchdog","",0,"watchdog [counter|-1]",watchdog,0,99,CMD_REPEAT},
	{"count","",0,"count",cmd_count,0,99,CMD_REPEAT},
	{"reset","",0,"reset",cmd_reset,0,99,CMD_REPEAT},
	{"reset1","",0,"reset1",cmd_reset1,0,99,CMD_REPEAT},
	{"test","",0,"test",cmd_test,0,99,CMD_REPEAT},
	{"cpumap","addr",0,"cpumap addr",cmd_cpumap,0,99,CMD_REPEAT},
	{"cpurun","cpu args",0,"run cmds on other cpu",cmd_cpurun,0,99,CMD_REPEAT},
	{"serial","[i|r|w|t] line",0,"serial test",cmd_serial,0,99,CMD_REPEAT},
	{"testw","addr count",0,"loop write test",cmd_testw,0,99,CMD_REPEAT},
	{"|","cmd",0,"run cmd and return 0",cmd_or,0,99,CMD_REPEAT},
	{0, 0}
};


static void init_cmd __P((void)) __attribute__ ((constructor));

static void
init_cmd()
{
	cmdlist_expand(Cmds, 1);
if(maincpu)
	tgt_poll_register(1, dma_poll, 0);
if(maincpu)
{
#ifdef SLAVECPU_MEM_SERIAL
	serial_init();
	tgt_poll_register(1, serial_poll, 0);
#endif
#if 0
	highmemset(CMDBASEADDRESS,0,32*12);
	mycacheflush(CMDBASEADDRESS,32*12,1);
#else
	if(maincpu)
	{
	highmemset(SHAREBASEADDRESS,0,0x200000);
	mycacheflush(SHAREBASEADDRESS,0x200000,1);
	}
	else
	{
	mycacheflush(SHAREBASEADDRESS,0x200000,0);
	}
#endif
}
/*if(getenv("cmdpoll"))*/	tgt_poll_register(1, mycmd_poll, 0);
}
