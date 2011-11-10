#define cpu_to_le32(x) (x)
#define le32_to_cpu(x) (x)
#define BONITO_PCICMD_MABORT PCI_STATUS_MASTER_ABORT
#define BONITO_PCICMD_MTABORT PCI_STATUS_MASTER_TARGET_ABORT
#define KSEG1ADDR(x) ((x)|0xa0000000)
#define PCIBIOS_SUCCESSFUL              0x00
#define PCIBIOS_FUNC_NOT_SUPPORTED      0x81
#define PCIBIOS_BAD_VENDOR_ID           0x83
#define PCIBIOS_DEVICE_NOT_FOUND        0x86
#define PCIBIOS_BAD_REGISTER_NUMBER     0x87
#define PCIBIOS_SET_FAILED              0x88
#define PCIBIOS_BUFFER_TOO_SMALL        0x89

#define PCI_ACCESS_READ  0
#define PCI_ACCESS_WRITE 1
	
    typedef unsigned char u8;
    typedef unsigned short u16;
    typedef unsigned int u32;


static inline void
bflush (void)
{
	/* flush Bonito register writes */
	(void) BONITO_PCICMD;
}

static int
p6032_pcibios_config_access(unsigned char access_type,pcitag_t tag,
			    unsigned char where, u32 *data)
{
	int bus;
	
	u_int32_t addr, type;
	void *addrp;
	int device;
	int function;
	int reg = where & ~3;
	_pci_break_tag (tag, &bus, &device, &function);

	if (bus == 0) 
	{
	        /* Type 0 configuration on onboard PCI bus */
		if (device > 20 || function > 7) {
			*data = -1;	/* device out of range */
			return PCIBIOS_DEVICE_NOT_FOUND;
		}
		addr = (1 << (device+11)) | (function << 8) | reg;
		type = 0;
	} else {
	        /* Type 1 configuration on offboard PCI bus */
	        if (bus > 255 || device > 31 || function > 7) {
			*data = -1;	/* device out of range */
		        return PCIBIOS_DEVICE_NOT_FOUND;
		}
		addr = (bus << 16) | (device << 11) | (function << 8) | reg;
		type = 0x10000;
	}

	/* clear aborts */
	BONITO_PCICMD |= BONITO_PCICMD_MABORT | BONITO_PCICMD_MTABORT;

	BONITO_PCIMAP_CFG = (addr >> 16) | type;
	bflush ();

	addrp = (void *)KSEG1ADDR(BONITO_PCICFG_BASE | (addr & 0xffff));
	if (access_type == PCI_ACCESS_WRITE)
		*(volatile unsigned int *)addrp = cpu_to_le32(*data);
	else
		*data = le32_to_cpu(*(volatile unsigned int *)addrp);

#if 0
prom_printf ("pci_config: (%d,%d,%d)/%x 0x%02x %s 0x%x\n", bus, device, function, addr,
	     reg,
	     access_type == PCI_ACCESS_WRITE ? "<-" : "->",
	     *data);
#endif

	if (BONITO_PCICMD & (BONITO_PCICMD_MABORT | BONITO_PCICMD_MTABORT)) {
	    BONITO_PCICMD |= BONITO_PCICMD_MABORT | BONITO_PCICMD_MTABORT;
	    *data = -1;
	    return PCIBIOS_DEVICE_NOT_FOUND;
	}

	return PCIBIOS_SUCCESSFUL;
}


static int
p6032_pcibios_read_config_byte(pcitag_t dev, int where, u8 *val)
{
        u32 data;
	int status;
	status = p6032_pcibios_config_access(PCI_ACCESS_READ, dev, where, &data);
	*val = (data >> ((where & 3) << 3)) & 0xff;

	return status;
}


static int
p6032_pcibios_read_config_word (pcitag_t dev, int where, u16 *val)
{
        u32 data;
	int status;

	if (where & 1)
		return PCIBIOS_BAD_REGISTER_NUMBER;

	status = p6032_pcibios_config_access(PCI_ACCESS_READ, dev, where, &data);
	*val = (data >> ((where & 3) << 3)) & 0xffff;

	return status;
}

static int
p6032_pcibios_read_config_dword (pcitag_t dev, int where, u32 *val)
{
	u32 data;
	int status;

	if (where & 3)
		return PCIBIOS_BAD_REGISTER_NUMBER;
	
	status = p6032_pcibios_config_access(PCI_ACCESS_READ, dev, where, &data);
	*val = data;

	return status;
}


static int
p6032_pcibios_write_config_byte (pcitag_t dev, int where, u8 val)
{
	u32 data;
	int status;
	status = p6032_pcibios_config_access(PCI_ACCESS_READ, dev, where, &data);

	if (status != PCIBIOS_SUCCESSFUL)
		return status;

	data = (data & ~(0xff << ((where & 3) << 3))) |
	       (val << ((where & 3) << 3));

	status = p6032_pcibios_config_access(PCI_ACCESS_WRITE, dev, where, &data);
	return status;
}

static int
p6032_pcibios_write_config_word (pcitag_t dev, int where, u16 val)
{
        u32 data;
	int status;

	if (where & 1)
		return PCIBIOS_BAD_REGISTER_NUMBER;
       
        status = p6032_pcibios_config_access(PCI_ACCESS_READ, dev, where, &data);

	if (status != PCIBIOS_SUCCESSFUL)
		return status;

	data = (data & ~(0xffff << ((where & 3) << 3))) | 
	       (val << ((where & 3) << 3));

	status = p6032_pcibios_config_access(PCI_ACCESS_WRITE, dev, where, &data);
	return status;
}

static int
p6032_pcibios_write_config_dword( pcitag_t dev, int where, u32 val)
{
	if (where & 3)
		return PCIBIOS_BAD_REGISTER_NUMBER;

	return p6032_pcibios_config_access(PCI_ACCESS_WRITE, dev, where, &val);
}

pcireg_t
_pci_conf_readn(pcitag_t tag, int reg, int width)
{
unsigned char b8;
unsigned short b16;
unsigned int b32;
    switch(width)
    {
	    case 1:p6032_pcibios_read_config_byte(tag,reg,&b8);return b8;
	    case 2:p6032_pcibios_read_config_word(tag,reg,&b16);return b16;
	    case 4:p6032_pcibios_read_config_dword(tag,reg,&b32);return b32;
    }
    return 0;
}

void
_pci_conf_writen(pcitag_t tag, int reg, pcireg_t data,int width)
{

       switch(width)
       {
	case 1:  p6032_pcibios_write_config_byte(tag,reg,data);return;
	case 2:  p6032_pcibios_write_config_word(tag,reg,data);return;
	case 4:  p6032_pcibios_write_config_dword(tag,reg,data);return;
       }
       return ;
}
