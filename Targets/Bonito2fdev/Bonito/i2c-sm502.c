#ifdef DEVBD2F_SM502
static volatile char *mmio = 0;

#define GPIO_DIR_REG 		(volatile unsigned int *)(mmio + 0x1000c)
#define GPIO_DATA_REG		(volatile unsigned int *)(mmio + 0x10004)
#define G_OUTPUT		1
#define G_INPUT			0
#define GPIO_SDA_DIR_SHIFT	15
#define	GPIO_SCL_DIR_SHIFT	14
#define GPIO_SDA_DATA_SHIFT	15
#define GPIO_SCL_DATA_SHIFT	14

#elif defined(DEVBD2F_FIREWALL)

#define GPIO_DIR_REG 		(volatile unsigned int *)(0xbfe00120)
#define GPIO_DATA_REG		(volatile unsigned int *)(0xbfe0011c)

#define G_OUTPUT		0
#define G_INPUT			1
#define GPIO_SDA_DIR_SHIFT	2
#define	GPIO_SCL_DIR_SHIFT	3
#define GPIO_SDA_DATA_SHIFT	2
#define GPIO_SCL_DATA_SHIFT	3
//extern int word_addr = 0;

#endif

static void i2c_sleep(int ntime)
{
	int i,j=0;
	for(i=0; i<300*ntime; i++)
	{
		j=i;
		j+=i;
	}
}

void sda_dir(int ivalue)
{
	int tmp;
	tmp = *GPIO_DIR_REG;
	if(ivalue == 1)
		*GPIO_DIR_REG = tmp|(0x1<<GPIO_SDA_DIR_SHIFT);
	else
		*GPIO_DIR_REG = tmp&(~(0x1<<GPIO_SDA_DIR_SHIFT));
}
void scl_dir(int ivalue)
{
	int tmp;
	tmp = *GPIO_DIR_REG;
	if(ivalue == 1)
		*GPIO_DIR_REG = tmp|(0x1<<GPIO_SCL_DIR_SHIFT);
	else
		*GPIO_DIR_REG = tmp&(~(0x1<<GPIO_SCL_DIR_SHIFT));
}

void sda_bit(int ivalue)
{
	int tmp;
	tmp = *GPIO_DATA_REG;
	if(ivalue == 1)
		*GPIO_DATA_REG = tmp|(0x1<<GPIO_SDA_DATA_SHIFT);
	else
		*GPIO_DATA_REG = tmp&(~(0x1<<GPIO_SDA_DATA_SHIFT));
}
void scl_bit(int ivalue)
{
	int tmp;
	tmp = *GPIO_DATA_REG;
	if(ivalue == 1)
		*GPIO_DATA_REG = tmp|(0x1<<GPIO_SCL_DATA_SHIFT);
	else
		*GPIO_DATA_REG = tmp&(~(0x1<<GPIO_SCL_DATA_SHIFT));
}


static void i2c_start(void)
{
	sda_dir(G_OUTPUT);
	scl_dir(G_OUTPUT);
	scl_bit(0);
	i2c_sleep(1);
	sda_bit(1);
	i2c_sleep(1);
	scl_bit(1);
	i2c_sleep(5);
	sda_bit(0);
	i2c_sleep(5);
	scl_bit(0);
	i2c_sleep(2);
	
}
static void i2c_stop(void)
{
	sda_dir(G_OUTPUT);
	scl_dir(G_OUTPUT);
	scl_bit(0);
	i2c_sleep(1);
	sda_bit(0);
	i2c_sleep(1);
	scl_bit(1);
	i2c_sleep(5);
	sda_bit(1);
	i2c_sleep(5);
	scl_bit(0);
	i2c_sleep(2);
	
		
}

static void i2c_send_ack(int ack)
{
	sda_dir(G_OUTPUT);
	sda_bit(ack);
	i2c_sleep(3);
	scl_bit(1);
	i2c_sleep(5);
	scl_bit(0);
	i2c_sleep(2);
}

static char i2c_rec_ack()
{
        char res = 1;
        int num=10;
	int tmp;
        sda_dir(G_INPUT);
        i2c_sleep(3);
        scl_bit(1);
        i2c_sleep(5);
#ifdef DEVBD2F_SM502 
	tmp = ((*GPIO_DATA_REG)&(0x1<<GPIO_SDA_DATA_SHIFT));
#elif DEVBD2F_FIREWALL
	tmp = ((*GPIO_DATA_REG)&(0x1<<(GPIO_SDA_DATA_SHIFT+16)));
#endif
        //wait for a ack signal from slave

        while(tmp)
        {
                i2c_sleep(1);
                num--;
                if(!num)
                {
                        res = 0;
                        break;
                }
#ifdef DEVBD2F_SM502	
		tmp = ((*GPIO_DATA_REG)&(0x1<<GPIO_SDA_DATA_SHIFT));
#elif DEVBD2F_FIREWALL
		tmp = ((*GPIO_DATA_REG)&(0x1<<(GPIO_SDA_DATA_SHIFT+16)));
#endif
        }
        scl_bit(0);
        i2c_sleep(3);
        return res;
}


static unsigned char i2c_rec()
{
	int i;
	int tmp;
	unsigned char or_char;
	unsigned char value = 0x00;
	sda_dir(G_INPUT);
	for(i=7;i>=0;i--)
	{
		i2c_sleep(5);
		scl_bit(1);
		i2c_sleep(3);
#ifdef DEVBD2F_SM502
		tmp = ((*GPIO_DATA_REG)&(0x1<<GPIO_SDA_DATA_SHIFT));
#elif DEVBD2F_FIREWALL
		tmp = ((*GPIO_DATA_REG)&(0x1<<(GPIO_SDA_DATA_SHIFT+16)));
#endif
		if(tmp)
			or_char=0x1;
		else
			or_char=0x0;
		or_char<<=i;
		value|=or_char;
		i2c_sleep(3);
		scl_bit(0);
	}
	return value;
}

static unsigned char i2c_send(unsigned char value)
{//we assume that now scl is 0
	int i;
	unsigned char and_char;
	sda_dir(G_OUTPUT);
	for(i=7;i>=0;i--)
	{
		and_char = value;
		and_char>>=i;
		and_char&=0x1;
		if(and_char)
			sda_bit(1);
		else
			sda_bit(0);
		i2c_sleep(1);
		scl_bit(1);
		i2c_sleep(5);
		scl_bit(0);
		i2c_sleep(1);
	}
	sda_bit(1);	
	return 1;
}

unsigned char i2c_rec_s(unsigned char *addr,int addrlen,unsigned char reg,unsigned char* buf ,int count)
{
int i;
int j;

	
	unsigned char value;
		//start signal
		for(i=0;i<count;i++)
	{
		i2c_start();
		for(j=0;j<addrlen;j++)
		{
		//write slave_addr
		i2c_send(addr[j]);
		if(!i2c_rec_ack())
			return 0;
		}

		i2c_send(reg);
		if(!i2c_rec_ack())
			return 0;

		//repeat start
		i2c_start();
		//write slave_addr+1
		i2c_send(addr[0]|0x1);
		if(!i2c_rec_ack())
			return 0;
		//read data
		buf[i]=i2c_rec();	
//		i2c_send_ack(1);//***add in***//
		i2c_stop();
		
		reg++;
	}

	return count;
}

unsigned char i2c_send_s(unsigned char *addr,int addrlen,unsigned char reg,unsigned char * buf ,int count)
{
int i;
int j;
	for(i=0;i<count;i++)
	{	
		i2c_start();	
		for(j=0;j<addrlen;j++)
		{
		//write slave_addr
		i2c_send(addr[j]);
		if(!i2c_rec_ack())
			return 0;
		}

		i2c_send(reg);
		if(!i2c_rec_ack())
			return 0;

		i2c_send(buf[i]);
		if(!i2c_rec_ack())
			return 0;
		i2c_stop();
		reg++;
	}
	return 1;
}


unsigned char i2c_rec_b(unsigned char *addr,int addrlen,unsigned char reg,unsigned char* buf ,int count)
{
int i;
int j;

	
	unsigned char value;
		//start signal
		i2c_start();
		for(j=0;j<addrlen;j++)
		{
		//write slave_addr
		i2c_send(addr[j]);
		if(!i2c_rec_ack())
			return 0;
		}

		i2c_send(reg);
		if(!i2c_rec_ack())
			return 0;

		//repeat start
		i2c_start();
		//write slave_addr+1
		i2c_send(addr[0]|0x1);
		if(!i2c_rec_ack())
			return 0;

		for(i=0;i<count;i++)
		{
		//read data
		buf[i]=i2c_rec();	
//		i2c_send_ack(1);//***add in***//
		
		}
		i2c_stop();

	return count;
}
unsigned char i2c_send_b(unsigned char *addr,int addrlen,unsigned char reg,unsigned char * buf ,int count)
{
int i;
int j;
		i2c_start();	
		for(j=0;j<addrlen;j++)
		{
		//write slave_addr
		i2c_send(addr[j]);
		if(!i2c_rec_ack())
			return 0;
		}

		i2c_send(reg);
		if(!i2c_rec_ack())
			return 0;

	for(i=0;i<count;i++)
	{	

		i2c_send(buf[i]);
		if(!i2c_rec_ack())
			return 0;
	}
		i2c_stop();
	return count;
}
//----------------------
/*
 * 0 single: 每次读一个
 * 1 smb block
 */
int tgt_i2cread(int type,unsigned char *addr,int addrlen,unsigned char reg,unsigned char *buf,int count)
{
int i;
tgt_i2cinit();
memset(buf,-1,count);
switch(type)
{
case I2C_SINGLE:
return i2c_rec_s(addr,addrlen,reg,buf,count);
break;
case I2C_BLOCK:
return i2c_rec_b(addr,addrlen,reg,buf,count);
break;

default: return 0;break;
}
return 0;
}

int tgt_i2cwrite(int type,unsigned char *addr,int addrlen,unsigned char reg,unsigned char *buf,int count)
{
tgt_i2cinit();
switch(type&0xff)
{
case I2C_SINGLE:
i2c_send_s(addr,addrlen,reg,buf,count);
break;
case I2C_BLOCK:
return i2c_send_b(addr,addrlen,reg,buf,count);
break;
case I2C_SMB_BLOCK:
break;
default:return -1;break;
}
return -1;
}


int tgt_i2cinit()
{
#if defined(DEVBD2F_SM502)
pcitag_t tag;
static int inited=0;
int tmp;
		if(!inited)
		{
		tag=_pci_make_tag(0,14,0);
	
		mmio = _pci_conf_readn(tag,0x14,4);
		mmio =(int)mmio|(0xb0000000);
		tmp = *(volatile int *)(mmio + 0x40);
		*(volatile int *)(mmio + 0x40) =tmp|0x40;
		
//		tgt_printf("clock enable bit 40 = %x\n", *(volatile int *)(mmio + 0x40));
		inited=1;
		}
#endif
}
