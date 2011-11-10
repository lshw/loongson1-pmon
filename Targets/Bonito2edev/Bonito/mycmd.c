#define nr_printf printf
#define nr_gets gets
#define nr_strtol strtoul
//-------------------------------------------PNP------------------------------------------
// MB PnP configuration register

#define PNP_KEY_ADDR (0xbfd00000+0x3f0)
#define PNP_DATA_ADDR (0xbfd00000+0x3f1)


void PNPSetConfig(char Index, char data);
char PNPGetConfig(char Index);

#define SUPERIO_CFG_REG 0x85

void EnterMBPnP(void)
{
	pcitag_t tag;
	char confval;
	tag=_pci_make_tag(VTSB_BUS,VTSB_DEV, VTSB_ISA_FUNC);
	confval=_pci_conf_readn(tag,SUPERIO_CFG_REG,1);
	_pci_conf_writen(tag,SUPERIO_CFG_REG,confval|2,1);	
}

void ExitMBPnP(void)
{
	pcitag_t tag;
	char confval,val;
	tag=_pci_make_tag(VTSB_BUS,VTSB_DEV, VTSB_ISA_FUNC);
	confval=_pci_conf_readn(tag,SUPERIO_CFG_REG,1);
	_pci_conf_writen(tag,SUPERIO_CFG_REG,confval&~2,1);	
}

void PNPSetConfig(char Index, char data)
{
        EnterMBPnP();                                // Enter IT8712 MB PnP mode
        outb(PNP_KEY_ADDR,Index);
        outb(PNP_DATA_ADDR,data);
        ExitMBPnP();
}

char PNPGetConfig(char Index)
{
        char rtn;

        EnterMBPnP();                                // Enter IT8712 MB PnP mode
        outb(PNP_KEY_ADDR,Index);
        rtn = inb(PNP_DATA_ADDR);
        ExitMBPnP();
        return rtn;
}



int dumpsis(int argc,char **argv)
{
int i;
volatile unsigned char *p=0xbfd003c4;
unsigned char c;
for(i=0;i<0x15;i++)
{
p[0]=i;
c=p[1];
printf("sr%x=0x%02x\n",i,c);
}
p[0]=5;
p[1]=0x86;
printf("after set 0x86 to sr5\n");
for(i=0;i<0x15;i++)
{
p[0]=i;
c=p[1];
printf("sr%x=0x%02x\n",i,c);
}
return 0;
}

unsigned char i2cread(char slot,char offset);


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

static int PnpRead(int type,long long addr,union commondata *mydata)
{
switch(type)
{
case 1:mydata->data1=PNPGetConfig(addr);break;
default: return -1;break;
}
return 0;
}

static int PnpWrite(int type,long long addr,union commondata *mydata)
{
switch(type)
{
case 1:PNPSetConfig(addr,mydata->data1);break;
default: return -1;break;
}
return 0;
}

static int pnps(int argc,char **argv)
{
syscall1=(void*)PnpRead;
syscall2=(void*)PnpWrite;
syscall_addrtype=0;
return 0;
}

#include "target/via686b.h"
static int i2cslot=0;

static int DimmRead(int type,long long addr,union commondata *mydata)
{
char c;
switch(type)
{
case 1:
linux_outb((i2cslot<<1)+0xa1,SMBUS_HOST_ADDRESS);
linux_outb(addr,SMBUS_HOST_COMMAND);
linux_outb(0x8,SMBUS_HOST_CONTROL); 
if((c=linux_inb(SMBUS_HOST_STATUS))&0x1f)
{
linux_outb(c,SMBUS_HOST_STATUS);
}

linux_outb(linux_inb(SMBUS_HOST_CONTROL)|0x40,SMBUS_HOST_CONTROL);

while(linux_inb(SMBUS_HOST_STATUS)&SMBUS_HOST_STATUS_BUSY);

if((c=linux_inb(SMBUS_HOST_STATUS))&0x1f)
{
linux_outb(c,SMBUS_HOST_STATUS);
}

mydata->data1=linux_inb(SMBUS_HOST_DATA0);
break;

default: return -1;break;
}
return 0;
}

static int DimmWrite(int type,long long addr,union commondata *mydata)
{
return -1;
}


static int Ics950220Read(int type,long long addr,union commondata *mydata)
{
char c;
switch(type)
{
case 1:
linux_outb(0xd3,SMBUS_HOST_ADDRESS); //0xd3
linux_outb(addr,SMBUS_HOST_COMMAND);
linux_outb(1,SMBUS_HOST_DATA0);
linux_outb(0x14,SMBUS_HOST_CONTROL); //0x14
if((c=linux_inb(SMBUS_HOST_STATUS))&0x1f)
{
linux_outb(c,SMBUS_HOST_STATUS);
}

linux_outb(linux_inb(SMBUS_HOST_CONTROL)|0x40,SMBUS_HOST_CONTROL);

while(linux_inb(SMBUS_HOST_STATUS)&SMBUS_HOST_STATUS_BUSY);

if((c=linux_inb(SMBUS_HOST_STATUS))&0x1f)
{
linux_outb(c,SMBUS_HOST_STATUS);
}

mydata->data1=linux_inb(SMBUS_HOST_DATA1+1);
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
linux_outb(0xd2,SMBUS_HOST_ADDRESS); //0xd3
linux_outb(addr,SMBUS_HOST_COMMAND);
linux_outb(1,SMBUS_HOST_DATA0);
linux_outb(0x14,SMBUS_HOST_CONTROL); //0x14
if((c=linux_inb(SMBUS_HOST_STATUS))&0x1f)
{
linux_outb(c,SMBUS_HOST_STATUS);
}

c=linux_inb(SMBUS_HOST_CONTROL);
linux_outb(mydata->data1,SMBUS_HOST_DATA1+1);
linux_outb(c|0x40,SMBUS_HOST_CONTROL);

while(linux_inb(SMBUS_HOST_STATUS)&SMBUS_HOST_STATUS_BUSY);

if((c=linux_inb(SMBUS_HOST_STATUS))&0x1f)
{
linux_outb(c,SMBUS_HOST_STATUS);
}

break;

default: return -1;break;
}
return 0;
return -1;
}

static int i2cs(int argc,char **argv)
{
if(argc!=2) return -1;

i2cslot=strtoul(argv[1],0,0);

switch(i2cslot)
{
case 0:
case 1:
 syscall1=(void*)DimmRead;
 syscall2=(void*)DimmWrite;
break;
case 2:
 syscall1=(void*)Ics950220Read;
 syscall2=(void*)Ics950220Write;
break;
default:
 return -1;
break;
}

syscall_addrtype=0;

return 0;
}

static const Cmd Cmds[] =
{
	{"MyCmds"},
	{"pnps",	"", 0, "select pnp ops for d1,m1 ", pnps, 0, 99, CMD_REPEAT},
	{"dumpsis",	"", 0, "dump sis registers", dumpsis, 0, 99, CMD_REPEAT},
	{"i2cs","slotno #slot 0-1 for dimm,slot 2 for ics95220", 0, "select i2c ops for d1,m1", i2cs, 0, 99, CMD_REPEAT},
	{0, 0}
};

static void init_cmd __P((void)) __attribute__ ((constructor));

static void
init_cmd()
{
	cmdlist_expand(Cmds, 1);
}

