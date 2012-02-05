#include <pmon.h>
#include <target/i2c.h>



//----------------------------------------------------





unsigned char i2c_rec_s(unsigned char *addr,int addrlen,unsigned char* buf ,int count)
{
int i;
int j;
	unsigned char value;
	for(i=0;i<count;i++)
	{
		for(j=0;j<addrlen;j++)
		{
		/*write slave_addr*/
  * LS232_I2C_TXR = addr[j];
  * LS232_I2C_CR  = (j == 0)? (CR_START|CR_WRITE):CR_WRITE; /* start on first addr */
   while(*LS232_I2C_SR & SR_TIP);

  if((* LS232_I2C_SR) & SR_NOACK) return i;
		}

		/*write slave_addr*/
  * LS232_I2C_TXR = addr[0]|1;
  * LS232_I2C_CR  = CR_START|CR_WRITE; /* start on first addr */
   while(*LS232_I2C_SR & SR_TIP);

 if((* LS232_I2C_SR) & SR_NOACK) return i;

  * LS232_I2C_CR  = CR_READ|I2C_WACK; /*last read not send ack*/

  while(*LS232_I2C_SR & SR_TIP);

    buf[i] = * LS232_I2C_TXR;
  * LS232_I2C_CR  = CR_STOP;
  * LS232_I2C_SR;
 
	}

	return count;
}

unsigned char i2c_send_s(unsigned char *addr,int addrlen,unsigned char * buf ,int count)
{
int i;
int j;
	for(i=0;i<count;i++)
	{
		for(j=0;j<addrlen;j++)
		{
		/*write slave_addr*/
  * LS232_I2C_TXR = addr[j];
  * LS232_I2C_CR  = j == 0? (CR_START|CR_WRITE):CR_WRITE; /* start on first addr */
   while(*LS232_I2C_SR & SR_TIP);

  if((* LS232_I2C_SR) & SR_NOACK) return i;
		}


  * LS232_I2C_TXR = buf[i];
  * LS232_I2C_CR = CR_WRITE|CR_STOP;
  while(*LS232_I2C_SR & SR_TIP);

  if((* LS232_I2C_SR) & SR_NOACK) return i;
	}
  while(*LS232_I2C_SR & SR_BUSY);
	return count;
}


unsigned char i2c_rec_b(unsigned char *addr,int addrlen,unsigned char* buf ,int count)
{
int i;
int j;

	
	unsigned char value;

		for(j=0;j<addrlen;j++)
		{
		/*write slave_addr*/
  * LS232_I2C_TXR = addr[j];
  * LS232_I2C_CR  = j == 0? (CR_START|CR_WRITE):CR_WRITE; /* start on first addr */
   while(*LS232_I2C_SR & SR_TIP);

  if((* LS232_I2C_SR) & SR_NOACK) return i;
		}


  * LS232_I2C_TXR = addr[0]|1;
  * LS232_I2C_CR  = CR_START|CR_WRITE;
  if((* LS232_I2C_SR) & SR_NOACK) return i;

		for(i=0;i<count;i++)
		{
  * LS232_I2C_CR  = CR_READ;
  while(*LS232_I2C_SR & SR_TIP);

    buf[i] = * LS232_I2C_TXR;
		}
  * LS232_I2C_CR  = CR_STOP;

	return count;
}
unsigned char i2c_send_b(unsigned char *addr,int addrlen,unsigned char * buf ,int count)
{
int i;
int j;
		for(j=0;j<addrlen;j++)
		{
		/*write slave_addr*/
  * LS232_I2C_TXR = addr[j];
  * LS232_I2C_CR  = j == 0? (CR_START|CR_WRITE):CR_WRITE; /* start on first addr */
   while(*LS232_I2C_SR & SR_TIP);

  if((* LS232_I2C_SR) & SR_NOACK) return i;
		}


	for(i=0;i<count;i++)
	{	
  * LS232_I2C_TXR = buf[i];
  * LS232_I2C_CR = CR_WRITE;
  while(*LS232_I2C_SR & SR_TIP);

  if((* LS232_I2C_SR) & SR_NOACK) return i;

	}
  * LS232_I2C_CR  = CR_STOP;
  while(*LS232_I2C_SR & SR_BUSY);
	return count;
}

/*
 * 0 single: 每次读一个
 * 1 smb block
 */
int tgt_i2cread(int type,unsigned char *addr,int addrlen,unsigned char *buf,int count)
{
int i;
tgt_i2cinit();
memset(buf,-1,count);
switch(type)
{
case I2C_SINGLE:
return i2c_rec_s(addr,addrlen,buf,count);
break;
case I2C_BLOCK:
return i2c_rec_b(addr,addrlen,buf,count);
break;

default: return 0;break;
}
return 0;
}

int tgt_i2cwrite(int type,unsigned char *addr,int addrlen,unsigned char *buf,int count)
{
tgt_i2cinit();
switch(type&0xff)
{
case I2C_SINGLE:
i2c_send_s(addr,addrlen,buf,count);
break;
case I2C_BLOCK:
return i2c_send_b(addr,addrlen,buf,count);
break;
case I2C_SMB_BLOCK:
break;
default:return -1;break;
}
return -1;
}


int tgt_i2cinit()
{
static int inited=0;
if(inited)return 0;
inited=1;
 * LS232_I2C_PRER_LO = 0x64;
 * LS232_I2C_PRER_HI = 0;
 * LS232_I2C_CTR = 0x80;
}

