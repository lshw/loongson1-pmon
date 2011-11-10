#include "target/sbd.h"
#define nr_printf printf
#define nr_gets gets
#define nr_strtol strtoul

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


static tpp(int argc,char **argv)
{
unsigned char c;
volatile unsigned char *p=0xbfd00378;
if(argc>2)return -1;
if(argc==2)
{
*p=strtoul(argv[1],0,0);
}
	c=*p;
	printf("pin 2-9 value=0x%02x(%08bb)\n",c,c);
return 0;
}
//------------------------------------------------------------------------------------------------
#include "serial.c"
//------------------------------------------------------------------------------------------------

static const Cmd Cmds[] =
{
	{"MyCmds"},
	{"pnpr",	"LDN index", 0, "pnpr LDN(logic device NO) index", PnpRead, 0, 99, CMD_REPEAT},
	{"pnpw",	"LDN index value", 0, "pnpw LDN(logic device NO) index value", PnpWrite, 0, 99, CMD_REPEAT},
	{"tpp","",0,"tpp [value]",tpp,0,99,CMD_REPEAT},
	{"serial","[i|r|w] com [c]",0,"serial test",cmd_serial,0,99,CMD_REPEAT},
	{0, 0}
};


static void init_cmd __P((void)) __attribute__ ((constructor));

static void
init_cmd()
{
	cmdlist_expand(Cmds, 1);
}
