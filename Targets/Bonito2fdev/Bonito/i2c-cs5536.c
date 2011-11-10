int	SMB_BASE_ADDR=0;
#define	SMB_SDA				0x00
#define	SMB_STS				0x01
#define	SMB_STS_SLVSTP		(1 << 7)
#define	SMB_STS_SDAST		(1 << 6)
#define	SMB_STS_BER		(1 << 5)
#define	SMB_STS_NEGACK		(1 << 4)
#define	SMB_STS_STASTR		(1 << 3)
#define	SMB_STS_NMATCH		(1 << 2)
#define	SMB_STS_MASTER		(1 << 1)
#define	SMB_STS_XMIT		(1 << 0)
#define	SMB_CTRL_STS			0x02
#define	SMB_CSTS_TGSTL		(1 << 5)
#define	SMB_CSTS_TSDA		(1 << 4)
#define	SMB_CSTS_GCMTCH		(1 << 3)
#define	SMB_CSTS_MATCH		(1 << 2)
#define	SMB_CSTS_BB		(1 << 1)
#define	SMB_CSTS_BUSY		(1 << 0)
#define	SMB_CTRL1			0x03
#define	SMB_CTRL1_STASTRE	(1 << 7)
#define	SMB_CTRL1_NMINTE	(1 << 6)
#define	SMB_CTRL1_GCMEN		(1 << 5)
#define	SMB_CTRL1_ACK		(1 << 4)
#define	SMB_CTRL1_RSVD		(1 << 3)
#define	SMB_CTRL1_INTEN		(1 << 2)
#define	SMB_CTRL1_STOP		(1 << 1)
#define	SMB_CTRL1_START		(1 << 0)
#define	SMB_ADDR			0x04
#define	SMB_ADDR_SAEN		(1 << 7)
#define	SMB_CTRL2			0x05
#define	SMB_ENABLE		(1 << 0)
#define	SMB_CTRL3			0x06


//p395
static int i2c_wait()
{
	char c;
	int i;
	delay(1000);
	for(i=0;i<20;i++)
	{
	c = linux_inb(SMB_BASE_ADDR|SMB_STS);
	if(c&SMB_STS_BER)return -1;
	if(c&SMB_STS_SDAST)return 0;
	delay(100);
	}
	return -2;
}




static int i2c_read_single(int addr, int regNo,char *value)
{
	int i;
	unsigned char c;
	int j;
	linux_outb(SMB_CTRL1_START,SMB_BASE_ADDR|SMB_CTRL1);
	i2c_wait();
	linux_outb(addr&0xfe,SMB_BASE_ADDR);
	i2c_wait();
	linux_outb(regNo,SMB_BASE_ADDR);
	i2c_wait();
	linux_outb(SMB_CTRL1_ACK|SMB_CTRL1_START,SMB_BASE_ADDR|SMB_CTRL1);
	i2c_wait();
	linux_outb(addr|1,SMB_BASE_ADDR);
	i2c_wait();
	*value=linux_inb(SMB_BASE_ADDR);
	linux_outb(2,SMB_BASE_ADDR|SMB_CTRL1);
	i2c_wait();
	//clear error
	//linux_outb(0x10,SMB_BASE_ADDR|SMB_STS);
	c=linux_inb(SMB_BASE_ADDR|SMB_STS);
	linux_outb(0x30,SMB_BASE_ADDR|SMB_STS);
	return 0;
}

static int i2c_write_single(int addr, int regNo,char *value)
{
	int i;
	unsigned char c;
	int j;
	linux_outb(SMB_CTRL1_START,SMB_BASE_ADDR|SMB_CTRL1);
	i2c_wait();
	linux_outb(addr&0xfe,SMB_BASE_ADDR);
	i2c_wait();
	linux_outb(regNo,SMB_BASE_ADDR);
	i2c_wait();
	linux_outb(*value,SMB_BASE_ADDR);
	i2c_wait();
	linux_outb(2,SMB_BASE_ADDR|SMB_CTRL1);
	i2c_wait();
	//clear error
	//linux_outb(0x10,SMB_BASE_ADDR|SMB_STS);
	c=linux_inb(SMB_BASE_ADDR|SMB_STS);
	linux_outb(0x30,SMB_BASE_ADDR|SMB_STS);
	return 0;
}


static int i2c_read_smbblock(int addr,int regNo,unsigned char *buf,char len) {
	int i;
	unsigned char c,count;
	int j;

	linux_outb(SMB_CTRL1_START,SMB_BASE_ADDR|SMB_CTRL1);
	i2c_wait();
	linux_outb(addr&0xfe,SMB_BASE_ADDR);
	i2c_wait();
	linux_outb(regNo,SMB_BASE_ADDR);
	i2c_wait();
	linux_outb(SMB_CTRL1_START,SMB_BASE_ADDR|SMB_CTRL1);
	i2c_wait();
	linux_outb(addr|1,SMB_BASE_ADDR);
	i2c_wait();
	count = linux_inb((SMB_BASE_ADDR));
	delay(4000);
	i2c_wait();
	for(i = 0; i < count+20; i++)
	{
	if(i2c_wait()<0){printf("i=%d\n",i);break;}
	c=linux_inb((SMB_BASE_ADDR));
	if(i<len)buf[i]=c;
	}
	i2c_wait();
	c=linux_inb((SMB_BASE_ADDR));

	linux_outb(2,SMB_BASE_ADDR|SMB_CTRL1);

	i2c_wait();
	//clear error
	c=linux_inb(SMB_BASE_ADDR|SMB_STS);
	linux_outb(0x30,SMB_BASE_ADDR|SMB_STS);
	return count;
}

static int i2c_write_smbblock(int addr,int regNo, char *buf,int len) {
	int i, j;
	unsigned char c;


	linux_outb(SMB_CTRL1_START,SMB_BASE_ADDR|SMB_CTRL1);
	i2c_wait();
	linux_outb(addr&0xfe,SMB_BASE_ADDR);
	i2c_wait();
	linux_outb(regNo,SMB_BASE_ADDR);
	i2c_wait();

	linux_outb(len,SMB_BASE_ADDR);
	i2c_wait();

	for(i=0;i<len;i++)
	{
	linux_outb(buf[i],SMB_BASE_ADDR);
	i2c_wait();
	}

	linux_outb(2,SMB_BASE_ADDR|SMB_CTRL1);

	i2c_wait();
	//clear error
	c=linux_inb(SMB_BASE_ADDR|SMB_STS);
	linux_outb(0x30,SMB_BASE_ADDR|SMB_STS);


	return len;
}


int tgt_i2cread(int type,unsigned char *addr,int addrlen,unsigned char reg,unsigned char *buf,int count)
{
int i;
if(!SMB_BASE_ADDR)SMB_BASE_ADDR=(_pci_conf_read(_pci_make_tag(0,14,0),0x10))&~3;
memset(buf,-1,count);
switch(type)
{
case I2C_SINGLE:
	for(i=0;i<count;i++)
	{
	i2c_read_single(addr[0],reg+i,buf+i);
	}
break;
case I2C_SMB_BLOCK:
i2c_read_smbblock(addr[0],reg,buf,count);
default: return 0;break;
}
return count;
}

int tgt_i2cwrite(int type,unsigned char *addr,int addrlen,unsigned char reg,unsigned char *buf,int count)
{
int i;
if(!SMB_BASE_ADDR)SMB_BASE_ADDR=(_pci_conf_read(_pci_make_tag(0,14,0),0x10))&~3;
switch(type)
{
case I2C_SINGLE:
	for(i=0;i<count;i++)
	{
	i2c_write_single(addr[0],reg,buf+i);
	}
break;
case I2C_SMB_BLOCK:
i2c_write_smbblock(addr[0],reg,buf,count);
break;
default:return -1;break;
}
return count;
}

