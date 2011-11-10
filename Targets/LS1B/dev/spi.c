#include "include/fcr.h"
#include <include/types.h>
#include <stdio.h>
#include <types.h>

#define SPI_REG_BASE 0x1fe80000
#define SPI_BASE     0xbfe80000
#define FCR_SPCR        0x00
#define FCR_SPSR        0x01
#define FCR_SPDR        0x02
#define FCR_SPER        0x03

#define SET_SPI(idx,value) KSEG1_STORE8(SPI_REG_BASE+idx, value)
#define GET_SPI(idx)    KSEG1_LOAD8(SPI_REG_BASE+idx)

void dumpspireg(void);

void spi_init()
{
  SET_SPI(FCR_SPCR, 0x5e);
  SET_SPI(FCR_SPER, 0x01);
  SET_SPI(FCR_SPSR, 0xc0);

//sw: 3210 lagency  
//  *(volatile unsigned char *)0xbf004140=(*(volatile unsigned char *)0xbf004140)|0x01;
//  *(volatile unsigned char *)0xbf004122=(*(volatile unsigned char *)0xbf004122)|0x80;
}

unsigned char  flash_writeb_cmd(unsigned char value)
{
      unsigned char status, ret;
      SET_SPI(FCR_SPDR, value);
      while ((*(volatile unsigned char *)(SPI_BASE+0x1))&0x01);
      return GET_SPI(FCR_SPDR);
}

unsigned char  flash_read_data()
{
return flash_writeb_cmd(0);
}

void spi_read_w25x_id()
{
  int i;
  unsigned char vendor_id;
  unsigned char device_id;
  dumpspireg();
 
  printf("===spi init\n");
  spi_init();
//  *(volatile unsigned char *)0xbf004122=(*(volatile unsigned char *)0xbf004122)&0x7f;
 
//sw: what's this?  
  flash_writeb_cmd(144);
  for(i=0;i<3;i++)
	 flash_writeb_cmd(0);
  vendor_id=flash_read_data(); 
  device_id=flash_read_data();
  printf("===the vendor_id=0x%x,the device_id=0x%x\n",vendor_id,device_id);
//  *(volatile unsigned char *)0xbf004122=(*(volatile unsigned char *)0xbf004122)|0x80;

  dumpspireg();
}

void dumpspireg()
{
	char ret;
	printf("===dump spi regs...\n");
	printf(" reg 0: %02x    ",GET_SPI(0x0));
	ret = GET_SPI(0x1);
	printf(" reg 1: %02x   \n",ret);
	if(ret & 0x1)
		printf(" reg 2(fifo) is empty, value is: 0x00\n");
	else	
		printf(" reg 2(fifo): %02x   \n",GET_SPI(0x2));
	printf(" reg 3: %02x   n",GET_SPI(0x3));
	printf(" reg 4: %02x   \n ",GET_SPI(0x4));
	printf(" reg 5: %02x   n",GET_SPI(0x5));
	printf(" reg 6: %02x   \n",GET_SPI(0x6));
	
}





