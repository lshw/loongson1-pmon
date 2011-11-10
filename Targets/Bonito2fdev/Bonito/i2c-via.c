/*
 * 0 single 
 * 1 smb block
 */
int tgt_i2cread(int type,unsigned char *addr,int addrlen,unsigned char reg,unsigned char *buf,int count)
{
int i;
int device,offset;
char c;
device=addr[0];
offset=reg;
device |= 1;
memset(buf,-1,count);
switch(type&0xff)
{
case I2C_SINGLE:
	for(i=0;i<count;i++)
	{
	linux_outb(device,SMBUS_HOST_ADDRESS);
	linux_outb(offset+i,SMBUS_HOST_COMMAND);
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

	buf[i]=linux_inb(SMBUS_HOST_DATA0);
	}
break;
case I2C_SMB_BLOCK:
linux_outb(device,SMBUS_HOST_ADDRESS); //0xd3
linux_outb(offset,SMBUS_HOST_COMMAND);
linux_outb(count,SMBUS_HOST_DATA0);
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

for(i=0;i<count;i++)
{
buf[i]=linux_inb(SMBUS_HOST_DATA1+1);
}
break;

default: return 0;break;
}
return count;
}

int tgt_i2cwrite(int type,unsigned char *addr,int addrlen,unsigned char reg,unsigned char *buf,int count)
{
int i;
int device,offset;
char c;
device=addr[0];
offset=reg;
device &= ~1;
switch(type)
{
case I2C_SINGLE:
	for(i=0;i<count;i++)
	{
	linux_outb(device,SMBUS_HOST_ADDRESS);
	linux_outb(offset+i,SMBUS_HOST_COMMAND);
	linux_outb(0x8,SMBUS_HOST_CONTROL); 
	if((c=linux_inb(SMBUS_HOST_STATUS))&0x1f)
	{
	linux_outb(c,SMBUS_HOST_STATUS);
	}

	linux_outb(buf[i],SMBUS_HOST_DATA0);

	linux_outb(linux_inb(SMBUS_HOST_CONTROL)|0x40,SMBUS_HOST_CONTROL);

	while(linux_inb(SMBUS_HOST_STATUS)&SMBUS_HOST_STATUS_BUSY);

	if((c=linux_inb(SMBUS_HOST_STATUS))&0x1f)
	{
	linux_outb(c,SMBUS_HOST_STATUS);
	}

	}
break;
case I2C_SMB_BLOCK:
linux_outb(device,SMBUS_HOST_ADDRESS); //0xd3
linux_outb(offset,SMBUS_HOST_COMMAND);
linux_outb(count,SMBUS_HOST_DATA0);
linux_outb(0x14,SMBUS_HOST_CONTROL); //0x14
if((c=linux_inb(SMBUS_HOST_STATUS))&0x1f)
{
linux_outb(c,SMBUS_HOST_STATUS);
}

for(i=0;i<count;i++)
linux_outb(buf[i],SMBUS_HOST_DATA1+1);
c=linux_inb(SMBUS_HOST_CONTROL);
linux_outb(c|0x40,SMBUS_HOST_CONTROL);

while(linux_inb(SMBUS_HOST_STATUS)&SMBUS_HOST_STATUS_BUSY);

if((c=linux_inb(SMBUS_HOST_STATUS))&0x1f)
{
linux_outb(c,SMBUS_HOST_STATUS);
}
break;
default:return -1;break;
}
return count;
}

