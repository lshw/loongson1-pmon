#include <target/i2c.h>

int i2c_init(int v)
{
static int inited=0;
if(inited)return 0;
inited=1;
 * FCR_SOC_I2C_PRER_LO = 0x64;
 * FCR_SOC_I2C_PRER_HI = 0x0;
 * FCR_SOC_I2C_CTR = 0x80;
  if(v)printf("* FCR_SOC_I2C_PRER_LO=0x%x\n",* FCR_SOC_I2C_PRER_LO);
  if(v)printf("* FCR_SOC_I2C_PRER_HI=0x%x\n",* FCR_SOC_I2C_PRER_HI);
  if(v)printf("* FCR_SOC_I2C_CTR=0x%x\n",* FCR_SOC_I2C_CTR);
  if((* FCR_SOC_I2C_PRER_LO != 0x64) || (* FCR_SOC_I2C_PRER_HI != 0x0))
  return -1;
  else return 0;
}

unsigned char i2c_read(unsigned char addr,int v)
{
  unsigned char tmp;
  unsigned char result;
  i2c_init(v);

  * FCR_SOC_I2C_TXR = 0x64;
  * FCR_SOC_I2C_CR  = 0x90;
  do  tmp = * FCR_SOC_I2C_SR;
  while( tmp & 0x02);
  if((* FCR_SOC_I2C_SR) & 0x80)
  {
    if(v)printf("write slave address dnot receive ack \n");
//          return 0;
  }
  else
    if(v)printf("write slave address receive ack");
  * FCR_SOC_I2C_TXR = addr<<4;
  * FCR_SOC_I2C_CR  = 0x10;
  do  tmp = * FCR_SOC_I2C_SR;
  while( tmp & 0x02);
  if((* FCR_SOC_I2C_SR) & 0x80)
  {
   if(v)printf("write reg dnot receive ack\n");
  //        return 0;
  }
  else 
   if(v)printf("write reg receive ack\n");
  * FCR_SOC_I2C_TXR = 0x65;
  * FCR_SOC_I2C_CR  = 0x90;
  do  tmp = * FCR_SOC_I2C_SR;
  while( tmp & 0x02);
  if((* FCR_SOC_I2C_SR) & 0x80)
  {
    if(v)printf("write slave address 2 dnot receive ack\n");
    //     return 0;
  }
  else
    if(v)printf("write slave address 2 receive ack\n");
 

  * FCR_SOC_I2C_CR  = 0x20;
  do  tmp = * FCR_SOC_I2C_SR;
  while( tmp & 0x02);
  if((* FCR_SOC_I2C_SR) & 0x80)
  {
  if(v)printf("read data dnot receive ack\n");
      //    return 0;
  }
  else
  if(v)printf("read data receive ack\n");
    result = * FCR_SOC_I2C_TXR;
  * FCR_SOC_I2C_CR  = 0x40;
    if(v)printf("result=%x\n",result);
    
  
  return result;
}

int i2c_write(unsigned char data, unsigned char addr,int v)
{
  unsigned char tmp;
  i2c_init(v);


  * FCR_SOC_I2C_TXR = 0x64;
  * FCR_SOC_I2C_CR  = 0x90;
  do  tmp = * FCR_SOC_I2C_SR;
  while( tmp & 0x02);
  if((* FCR_SOC_I2C_SR) & 0x80)
  {
     if(v)printf("write slave addresss dnot receive ack\n");
      //   return 0;
   }
   else
   if(v)printf("write slave addresss receive ack\n");
   if(v)printf("addr =0x%x\n",addr);
  * FCR_SOC_I2C_TXR = addr<<4;
  * FCR_SOC_I2C_CR  = 0x10;
  do  tmp = * FCR_SOC_I2C_SR;
  while( tmp & 0x02);
  if((* FCR_SOC_I2C_SR) & 0x80)
  {
   if(v)printf("write reg dnot receive ack\n");
        //  return 0;
   }
   else 
   if(v)printf("write reg receive ack\n");
//   if(v)printf("write reg 2\n");
  * FCR_SOC_I2C_TXR = data;
  * FCR_SOC_I2C_CR  = 0x50;
  do  tmp = * FCR_SOC_I2C_SR;
  while( tmp & 0x02);
  if((* FCR_SOC_I2C_SR) & 0x80)
  {
  if(v)printf("write data dnot receive ack\n");
     //     return 0;
  }
  else
   if(v)printf("write data receive ack\n");
  if(v)printf("write data =%d\n",data);
   return 1;
}


void i2c_send(char ctrl,char addr)
{
  * FCR_SOC_I2C_TXR = addr;
  * FCR_SOC_I2C_CR  = ctrl;
while(i2c_stat()&(I2C_RUN))idle();
}

char i2c_stat()
{
  return * FCR_SOC_I2C_SR;
}

char i2c_recv()
{
char tmp;
  * FCR_SOC_I2C_CR  = I2C_READ;
  while( i2c_stat() & I2C_RUN)idle();
    return * FCR_SOC_I2C_RXR;
}


