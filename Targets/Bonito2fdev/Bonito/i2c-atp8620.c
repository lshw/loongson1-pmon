#define I2C_NACK	0x08
#define	I2C_RD		0x20
#define	I2C_WR		0x10
#define	I2C_START	0x80
#define	I2C_STOP	0x40
#if 1
unsigned char atp8620_i2c_read(unsigned char slave_addr,unsigned char sub_addr,unsigned char* buf ,int count)
{
	pcitag_t tag;	
	int i;
	volatile unsigned char * iog;
	volatile unsigned char tmp;
	tag = _pci_make_tag(0,9,0);
	iog = _pci_conf_readn(tag,0x10,4);
#define CTR  (volatile unsigned char *)(iog + 0x0)
#define TXR  (volatile unsigned char *)(iog + 0x1)
#define RXR  (volatile unsigned char *)(iog + 0x2)
//	printf("iog %x \n",iog);
	iog =((unsigned int )iog|0xbfd00000|0xb0)&0xfffffffe;
	for(i=0;i<count;i++)
	{
		tmp = *CTR;
		while(tmp&0x1)
			tmp=*CTR;
		delay(10);
		
		*TXR = slave_addr;
		*CTR = I2C_WR|I2C_START;

		if(word_addr){
		
			tmp = *CTR;
			while(tmp&0x1)
				tmp=*CTR;
			delay(10);
			
			*TXR = 0;
			*CTR = I2C_WR;
		}
		tmp = *CTR;
		while(tmp&0x1)
			tmp=*CTR;
		delay(10);
		*TXR = sub_addr;
		*CTR = I2C_WR;

		tmp = *CTR;
		while(tmp&0x1)
			tmp=*CTR;
		delay(10);
		*TXR = slave_addr+1;
		*CTR = I2C_WR|I2C_START;
		
		delay(1);
///	
		*CTR = I2C_RD;
		tmp = *CTR;
		while(tmp&0x1)
			tmp=*CTR;
		delay(10);

		tmp = *RXR;
		buf[i] = tmp;

		*CTR = I2C_STOP;

	}
}

#else
unsigned char atp8620_i2c_read(unsigned char slave_addr,unsigned char sub_addr,unsigned char* buf ,int count)
{
	pcitag_t tag;	
	int i;
	volatile unsigned int * iog;
	volatile unsigned int tmp;
	tag = _pci_make_tag(0,9,0);
	iog = _pci_conf_readn(tag,0x10,4);
//	printf("iog %x \n",iog);
	iog =((unsigned int )iog|0xbfd00000|0xb0)&0xfffffffe;
//	printf("iog %x \n",iog);
	for(i=0;i<count;i++)
	{
		tmp = *iog;
//	printf("tmp %x \n",tmp);
		while(tmp&0x1)
			{tmp = *iog;
			}
		delay(10);

		tmp = (slave_addr&0xff)<<8;
		tmp = tmp|I2C_WR|I2C_START|I2C_NACK;
		*iog = tmp;
		
		if(word_addr)
		{	
			tmp = *iog;
			while(tmp&0x1)
				tmp = *iog;
		delay(10);
		
			tmp = 0;
			tmp = tmp|I2C_WR|I2C_NACK;
			*iog = tmp;

		}

		tmp = *iog;
		while(tmp&0x1)
			tmp = *iog;
		delay(10);

		tmp = (sub_addr&0xff)<<8;
		tmp = tmp|I2C_WR|I2C_NACK;
		*iog = tmp;

		tmp = *iog;
		while(tmp&0x1)
			tmp = *iog;	
		delay(10);

		tmp = ((slave_addr&0xff)+1)<<8;
		tmp = tmp|I2C_WR|I2C_START|I2C_NACK;
		*iog = tmp;

		tmp = *iog;
		while(tmp&0x1)
			tmp = *iog;	
		delay(10);

		tmp = I2C_RD|I2C_STOP;
		*iog = tmp;

		tmp = *iog;
		while(tmp&0x1)
			tmp = *iog;	
		delay(10);

		tmp = *iog;
		tmp = tmp&0xffff0000;
		tmp = tmp>>16;
		buf[i] = tmp&0xff;

/*
		tmp = I2C_STOP;
		*iog = tmp;
		
		tmp = *iog;
		while(tmp&0x1)
			tmp = *iog;	
		delay(10);
*/
	}
	return 0;


}

#endif

unsigned char atp8620_i2c_write(unsigned char slave_addr,unsigned char sub_addr,unsigned char * buf ,int count)
{
	pcitag_t tag;	
	int i;
	volatile unsigned int * iog;
	volatile unsigned int tmp;
	tag = _pci_make_tag(0,9,0);
	iog = _pci_conf_readn(tag,0x10,4);
	iog = ((unsigned int )iog|0xbfd00000|0xb0)&0xfffffffe;

	for(i=0;i<count;i++)
	{
		tmp = *iog;
		while(tmp&0x1)
			tmp = *iog;

		tmp = (slave_addr&0xff)<<8;
		tmp = tmp|I2C_WR|I2C_START|I2C_NACK;
		*iog = tmp;
		
		if(word_addr)
		{	
			tmp = *iog;
			while(tmp&0x1)
				tmp = *iog;
		
			tmp = 0;
			tmp = tmp|I2C_WR|I2C_NACK;
			*iog = tmp;

		}

		tmp = *iog;
		while(tmp&0x1)
			tmp = *iog;

		tmp = (sub_addr&0xff)<<8;
		tmp = tmp|I2C_WR|I2C_NACK;
		*iog = tmp;

		tmp = *iog;
		while(tmp&0x1)
			tmp = *iog;

		tmp = buf[i]<<8;
		tmp = tmp|I2C_WR|I2C_NACK;
		*iog = tmp;
	
		tmp = *iog;
		while(tmp&0x1)
			tmp = *iog;

		
		tmp = I2C_STOP;
		*iog = tmp;	
	}

	return 0;
}


static int firewall_i2c_read(int type,long long addr,union commondata *mydata)
{
	char c;
	switch(type)
	{
		case 1:
			atp8620_i2c_read((unsigned char)slave_addr,(unsigned char)addr,&c,1);
			memcpy(&mydata->data1,&c,1);
			return 0;
		default:
			return -1;

	}

}
static int firewall_i2c_write(int type,long long addr,union commondata *mydata)
{
	char c;
	switch(type)
	{
	case 1:
		memcpy(&c,&mydata->data1,1);
		atp8620_i2c_write((unsigned char)slave_addr,(unsigned char)addr,&c,1);
		return 0;
	default :
		return -1;
	
	}

}
static int i2cs(int argc,char *argv[])
{
	if(argc<2) 
		return -1;
	printf("i2c\n");
	
	i2cslot=strtoul(argv[1],0,0);

	switch(i2cslot)
	{
	case 0:
		slave_addr = strtoul(argv[2],0,0);
		if(slave_addr==0xde||slave_addr == 0xae)
		{
			word_addr = 1;
			printf("rtc opreation!\n");	
		}
		else
			word_addr = 0;
		syscall1 = (void *)firewall_i2c_read;
		syscall2 = (void *)firewall_i2c_write;
		break;
	case 1:
		slave_addr = strtoul(argv[2],0,0);
		if(slave_addr==0xde||slave_addr == 0xae)
		{
			word_addr = 1;
			printf("rtc opreation!\n");	
		}
		else
			word_addr = 0;
		syscall1 = (void *)sm502SPDRead;
		syscall2 = (void *)sm502SPDWrite;
		break;
	case 2:	
		i2c_test();
		break;
	case 3:
		syscall1=(void *)rom_ddr_reg_read;
		syscall2=(void *)rom_ddr_reg_write;
		if(argc==3 && !strcmp(argv[2][2],"revert"))
		{
			extern char ddr2_reg_data,_start;
			extern char ddr2_reg_data1;
			printf("revert to default ddr setting\n");
			// tgt_flashprogram(0xbfc00000+((int)&ddr2_reg_data -(int)&_start),30*8,&ddr2_reg_data1,TRUE);
		}
		break;

	default:
		return -1;
	}

	return 0;
}
#else
