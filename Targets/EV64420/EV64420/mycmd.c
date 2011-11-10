#define nr_printf printf
#define nr_gets gets
#define nr_strtol strtoul
//-------------------------------------------PNP------------------------------------------
// MB PnP configuration register

#define PNP_KEY_ADDR (PCI_IO_SPACE_BASE+0x3f0)
#define PNP_DATA_ADDR (PCI_IO_SPACE_BASE+0x3f1)


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


static int PnpRead(int argc,char **argv)
{
	unsigned char Index,data;
		if(argc!=2){return -1;}
		
		Index=nr_strtol(argv[1],0,0);
data=PNPGetConfig(Index);
nr_printf("pnpread index=0x%02x,value=0x%02x\n",Index,data);
return 0;
}

static int PnpWrite(int argc,char **argv)
{
        unsigned char Index,data;
        if(argc!=3){return -1;}
		Index=nr_strtol(argv[1],0,0);
		data=nr_strtol(argv[2],0,0);
PNPSetConfig(Index,data);
nr_printf("pnpwrite index=0x%02x,value=0x%02x,",Index,data);
data=PNPGetConfig(Index);
nr_printf("result=0x%02x\n",data);
return 0;
}

int dumpmv(int argc,char **argv)
{
	unsigned int readdata;
	int func;
	unsigned int data;
	nr_printf("pci configure register\n");
	for(func=0;func<5;func++)
	{
	unsigned int reg;
	pcitag_t pcidev=pci_make_tag(0,0,func);
	
	for (reg=0;reg<256;reg+=4)
	{
	readdata=_pci_conf_read(pcidev,reg);
	nr_printf("%01x.%02x: %08x\n",func,reg,readdata);
	}
	}
	printf("io rgisters\n");
	{unsigned int i,data;
	for(i=0;i<0x10000;i=i+4)
	{
	data=GT_READ(i);
	printf("%04x: %08x\n",i,data);
	}
	}
	
}

static const Cmd Cmds[] =
{
	{"MyCmds"},
	{"pnpr",	"LDN index", 0, "pnpr LDN(logic device NO) index", PnpRead, 0, 99, CMD_REPEAT},
	{"pnpw",	"LDN index value", 0, "pnpw index value", PnpWrite, 0, 99, CMD_REPEAT},
	{"dumpmv",	"", 0, "dump ev64420 registers", dumpmv, 0, 99, CMD_REPEAT},
	{0, 0}
};

static void init_cmd __P((void)) __attribute__ ((constructor));

static void
init_cmd()
{
	cmdlist_expand(Cmds, 1);
}

