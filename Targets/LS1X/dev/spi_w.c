#include <stdio.h>
#include "include/fcr.h"
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#undef _KERNEL
#include <errno.h>
#include <pmon.h>
#include <cpu.h>
//#include <include/types.h>
#include <pflash.h>
#include<linux/mtd/mtd.h>

#if 1
#define K1BASE 0xa0000000
#define KSEG1(addr) ((void *)(K1BASE | (u32)(addr)))

#define KSEG1_STORE8(addr, value) *(volatile u8 *)(KSEG1(addr)) = (value)
#define KSEG1_STORE16(addr, value) *(volatile u16 *)(KSEG1(addr)) = (value)
#define KSEG1_STORE32(addr, value) *(volatile u32 *)(KSEG1(addr)) = (value)

#define KSEG1_LOAD8(addr) *(volatile u8 *)(KSEG1(addr))
#define KSEG1_LOAD16(addr) *(volatile u16 *)(KSEG1(addr))
#define KSEG1_LOAD32(addr) *(volatile u32 *)(KSEG1(addr))

#define STORE8(addr, value) *(volatile u8 *)(addr) = value
#define STORE16(addr, value) *(volatile u16 *)(addr) = value
#define STORE32(addr, value) *(volatile u32 *)(addr) = value

#define LOAD8(addr) *(volatile u8 *)(addr)
#define LOAD16(addr) *(volatile u16 *)(addr)
#define LOAD32(addr) *(volatile u32 *)(addr)

#define PHY(addr) \
    (((u32)addr >= 0x80000000 && (u32)addr < 0xa0000000)? \
    (u32)addr-0x80000000:(u32)addr >= 0xa0000000? \
    (u32)addr-0xa0000000:(u32)addr) 

#endif

#define SPI_BASE  0x1fe80000
#define PMON_ADDR 0xa1000000
#define FLASH_ADDR 0x000000

#define SPCR      0x0
#define SPSR      0x1
#define TXFIFO    0x2
#define RXFIFO    0x2
#define SPER      0x3
#define PARAM     0x4
#define SOFTCS    0x5
#define PARAM2    0x6

#define RFEMPTY 1
#define	TFFULL	8

#define SET_SPI(addr,val)        KSEG1_STORE8(SPI_BASE+addr,val)
#define GET_SPI(addr)            KSEG1_LOAD8(SPI_BASE+addr)
#define dly	do{int i =100; while (i--);}while(0)


/*********************************************************************************/
static unsigned char flash_id[3];
static unsigned char init_id=0;
static struct fl_device *nor_dev;
/*********************************************************************************/

int write_sr(char val);
void spi_initw(void)
{
	SET_SPI(SPSR, 0xc0); 
  	SET_SPI(PARAM, 0x00);	//espr:0100
 	SET_SPI(SPER, 0x04);	//spre:00 
  	SET_SPI(PARAM2,0x01);
	SET_SPI(SPCR, 0x5c);
	SET_SPI(SOFTCS,0x11);
}

void spi_initr(void)
{
  	SET_SPI(PARAM, 0x07);             //espr:0100
}

///////////////////read status reg /////////////////
int read_sr(void)
{
	int val;
	
	SET_SPI(SOFTCS,0x01);
	SET_SPI(TXFIFO,0x05);
	while((GET_SPI(SPSR))&RFEMPTY);

	val = GET_SPI(RXFIFO);
	SET_SPI(TXFIFO,0x00);
	while(((GET_SPI(SPSR)) & RFEMPTY) == RFEMPTY);

	val = GET_SPI(RXFIFO);
	SET_SPI(SOFTCS,0x11);
	return val;
}

////////////set write enable//////////
int set_wren(void)
{
	int res;

	res = read_sr();
	while((res & 0x01) == 1){
		res = read_sr();
	}
	SET_SPI(SOFTCS,0x01);
	SET_SPI(TXFIFO,0x6);
	while(((GET_SPI(SPSR)) & RFEMPTY) == RFEMPTY){
	}
	GET_SPI(RXFIFO);
	SET_SPI(SOFTCS,0x11);
	return 1;
}

///////////////////////write status reg///////////////////////
int write_sr(char val)
{
	int res;
	
	set_wren();
	res = read_sr();
	while((res & 0x01) == 1){
		res = read_sr();
	}
	SET_SPI(SOFTCS,0x01);
	SET_SPI(TXFIFO,0x01);
	while(((GET_SPI(SPSR)) & RFEMPTY) == RFEMPTY){
	}
	GET_SPI(RXFIFO);
	SET_SPI(TXFIFO,val);
	while(((GET_SPI(SPSR)) & RFEMPTY) == RFEMPTY){
	}
	GET_SPI(RXFIFO);
	SET_SPI(SOFTCS,0x11);
	return 1;
}

/**********************************************************/		//lxy
void read_deviceid (void)
{
	int i;
	unsigned char id;

	spi_initw();
	set_wren();
	SET_SPI(SOFTCS,0x11);	//cs = hight
	dly;

	SET_SPI(SOFTCS,0x01);	//cs = low
	dly;
	SET_SPI(TXFIFO,0x90);	//command = read_id
	while(((GET_SPI(SPSR)) & RFEMPTY) == RFEMPTY);
	GET_SPI(RXFIFO);

	SET_SPI(TXFIFO,0x00);
	while(((GET_SPI(SPSR)) & RFEMPTY) == RFEMPTY);
	GET_SPI(RXFIFO);

	SET_SPI(TXFIFO,0x00);
	while(((GET_SPI(SPSR)) & RFEMPTY) == RFEMPTY);
	GET_SPI(RXFIFO);

	SET_SPI(TXFIFO,0x00);
	while(((GET_SPI(SPSR)) & RFEMPTY) == RFEMPTY);
	GET_SPI(RXFIFO);


	for(i=0; i<2; i++){
		SET_SPI(TXFIFO,0);     
		while((GET_SPI(SPSR))&RFEMPTY);
		id = GET_SPI(RXFIFO);
		printf ("id[%d] = 0x%x, ",i,id);
	}

	SET_SPI(SOFTCS,0x11);	//cs = hight
}



void read_jedecid(unsigned char *p)
{
	int i;

	spi_initw();
	SET_SPI(SOFTCS,0x11);	//cs = hight
	dly;

	SET_SPI(SOFTCS,0x01);	//cs = low
	SET_SPI(TXFIFO,0x9F);	//command = read_id
	while(((GET_SPI(SPSR)) & RFEMPTY) == RFEMPTY);
	GET_SPI(RXFIFO);

	for(i=0; i<3; i++){
		SET_SPI(TXFIFO,0);     
		while((GET_SPI(SPSR))&RFEMPTY);
		p[i] = GET_SPI(RXFIFO);
	}
	printf ("nor-flash id = 0x%x, 0x%x, 0x%x !\n", p[0], p[1], p[2]);

	SET_SPI(SOFTCS,0x11);	//cs = hight
}

int wb_write_sr(unsigned int val)
{
	int res;
	unsigned char lo,hi;
	lo = val ;
	hi = (val >> 8) & 0xff;

	spi_initw();
	set_wren();

	res = read_sr();
	while((res & 0x01) == 1){
		res = read_sr();
	}
	SET_SPI(SOFTCS,0x01);

	SET_SPI(TXFIFO,0x01);
	while(((GET_SPI(SPSR)) & RFEMPTY) == RFEMPTY){

	}
	GET_SPI(RXFIFO);

//	SET_SPI(TXFIFO,val);
	SET_SPI(TXFIFO,lo);
	while(((GET_SPI(SPSR)) & RFEMPTY) == RFEMPTY);
	GET_SPI(RXFIFO);

	SET_SPI(TXFIFO,hi);
	while(((GET_SPI(SPSR)) & RFEMPTY) == RFEMPTY);
	GET_SPI(RXFIFO);

	SET_SPI(SOFTCS,0x11);

	return 1;
}

///////////erase all memory/////////////
int erase_all(void)
{
	int res;
	int i=1;
	
	spi_initw();
	set_wren();
	
	res = read_sr();
	while((res & 0x01) == 1){
		res = read_sr();
	}
	SET_SPI(SOFTCS,0x1);

	SET_SPI(TXFIFO,0xC7);
	while(((GET_SPI(SPSR)) & RFEMPTY) == RFEMPTY){
	}
	GET_SPI(RXFIFO);

	SET_SPI(SOFTCS,0x11);
	while(i++){
		if((read_sr() & 0x1) == 0x1){
			if(i % 10000 == 0)
				printf(".");
		}else{
			printf("done...\n");
			break;
		}
	}
	return 1;
}

void spi_read_id(void)
{
	unsigned char val;
	
	spi_initw();
	/*CE 0*/
	SET_SPI(SOFTCS,0x01);
	/*READ ID CMD*/
	SET_SPI(TXFIFO,0x9f);
	while(((GET_SPI(SPSR)) & RFEMPTY) == RFEMPTY){
	}
	GET_SPI(RXFIFO);

	/*Manufacturer’s ID*/
	SET_SPI(TXFIFO,0x00);
	while(((GET_SPI(SPSR)) & RFEMPTY) == RFEMPTY){
	}
	val = GET_SPI(RXFIFO);
	printf("Manufacturer's ID:         %x\n",val);

	/*Device ID:Memory Type*/
	SET_SPI(TXFIFO,0x00);
	while(((GET_SPI(SPSR)) & RFEMPTY) == RFEMPTY){
	}
	val = GET_SPI(RXFIFO);
	printf("Device ID-memory_type:     %x\n",val);

	/*Device ID:Memory Capacity*/
	SET_SPI(TXFIFO,0x00);
	while(((GET_SPI(SPSR)) & RFEMPTY) == RFEMPTY){
	}
	val = GET_SPI(RXFIFO);
	printf("Device ID-memory_capacity: %x\n",val);

	/*CE 1*/
	SET_SPI(SOFTCS,0x11);
}

void spi_write_byte(unsigned int addr,unsigned char data)
{
	/*byte_program,CE 0, cmd 0x2,addr2,addr1,addr0,data in,CE 1*/
	unsigned char addr2,addr1,addr0;
	unsigned char val;
	addr2 = (addr & 0xff0000)>>16;
	addr1 = (addr & 0x00ff00)>>8;
	addr0 = (addr & 0x0000ff);
	
	set_wren();
	val = read_sr();
	while((val & 0x01) == 1){
		val = read_sr();
	}
	SET_SPI(SOFTCS,0x01);/*CE 0*/

	SET_SPI(TXFIFO,0x2);/*byte_program */
	while(((GET_SPI(SPSR)) & RFEMPTY) == RFEMPTY){
	}
	val = GET_SPI(RXFIFO);

	/*send addr2*/
	SET_SPI(TXFIFO,addr2);     
	while(((GET_SPI(SPSR)) & RFEMPTY) == RFEMPTY){
	}
	val = GET_SPI(RXFIFO);

	/*send addr1*/
	SET_SPI(TXFIFO,addr1);
	while(((GET_SPI(SPSR)) & RFEMPTY) == RFEMPTY){
	}
	val = GET_SPI(RXFIFO);

	/*send addr0*/
	SET_SPI(TXFIFO,addr0);
	while(((GET_SPI(SPSR)) & RFEMPTY) == RFEMPTY){
	}
	val = GET_SPI(RXFIFO);

	/*send data(one byte)*/
	SET_SPI(TXFIFO,data);
	while(((GET_SPI(SPSR)) & RFEMPTY) == RFEMPTY){
	}
	val = GET_SPI(RXFIFO);

	/*CE 1*/
	SET_SPI(SOFTCS,0x11);
}

int write_pmon_byte(int argc,char ** argv)
{
	unsigned int addr;
	unsigned char val;
	
	if(argc != 3){
		printf("\nuse: write_pmon_byte  dst(flash addr) data\n");
		return -1;
	}
	addr = strtoul(argv[1],0,0);
	val = strtoul(argv[2],0,0);
	spi_write_byte(addr,val);
	return 0;
}

int write_pmon(int argc,char **argv)
{
	long int j=0;
	unsigned char val;
	unsigned int ramaddr,flashaddr,size;
	
	if(argc != 4){
		printf("\nuse: write_pmon src(ram addr) dst(flash addr) size\n");
		return -1;
	}

	ramaddr = strtoul(argv[1],0,0);
	flashaddr = strtoul(argv[2],0,0);
	size = strtoul(argv[3],0,0);

	spi_initw();
	write_sr(0);
	// read flash id command
	spi_read_id();
	val = GET_SPI(SPSR);
	printf("====spsr value:%x\n",val);

	SET_SPI(0x5,0x10);
	// erase the flash     
	write_sr(0x00);
	//	erase_all();
	printf("\nfrom ram 0x%08x  to flash 0x%08x size 0x%08x \n\nprogramming      ",ramaddr,flashaddr,size);
	for(j=0;size > 0;flashaddr++,ramaddr++,size--,j++){
		spi_write_byte(flashaddr,*((unsigned char*)ramaddr));
		if(j % 0x1000 == 0)
		printf("\b\b\b\b\b\b\b\b\b\b0x%08x",j);
	}
	printf("\b\b\b\b\b\b\b\b\b\b0x%08x end...\n",j);

	SET_SPI(0x5,0x11);
	return 1;
}

int read_pmon_byte(unsigned int addr,unsigned int num)
{
	unsigned char val,data;
	val = read_sr();
	while((val & 0x01) == 1){
		val = read_sr();
	}

	SET_SPI(0x5,0x01);
	// read flash command 
	SET_SPI(TXFIFO,0x03);
	while(((GET_SPI(SPSR)) & RFEMPTY) == RFEMPTY){
	}
	GET_SPI(RXFIFO);

	// addr
	SET_SPI(TXFIFO,0x00);
	while(((GET_SPI(SPSR)) & RFEMPTY) == RFEMPTY){
	}
	GET_SPI(RXFIFO);

	SET_SPI(TXFIFO,0x00);
	while(((GET_SPI(SPSR)) & RFEMPTY) == RFEMPTY){
	}
	GET_SPI(RXFIFO);

	SET_SPI(TXFIFO,0x00);
	while(((GET_SPI(SPSR)) & RFEMPTY) == RFEMPTY){
	}
	GET_SPI(RXFIFO);

	SET_SPI(TXFIFO,0x00);
	while(((GET_SPI(SPSR)) & RFEMPTY) == RFEMPTY){
	}
	data = GET_SPI(RXFIFO);
	SET_SPI(0x5,0x11);
	return data;
}

int read_pmon(int argc,char **argv)
{
	unsigned char data;
	int val,base=0;
	int addr;
	int i;
	if(argc != 3){
		printf("\nuse: read_pmon addr(flash) size\n");
		return -1;
	}
	addr = strtoul(argv[1],0,0);
	i = strtoul(argv[2],0,0);
	spi_initw();
	val = read_sr();
	while((val & 0x01) == 1){
		val = read_sr();
	}

	SET_SPI(0x5,0x01);
	// read flash command 
	SET_SPI(TXFIFO,0x03);
	while(((GET_SPI(SPSR)) & RFEMPTY) == RFEMPTY){
	}
	GET_SPI(RXFIFO);

	// addr
	SET_SPI(TXFIFO,((addr >> 16)&0xff));
	while(((GET_SPI(SPSR)) & RFEMPTY) == RFEMPTY){
	}
	GET_SPI(RXFIFO);

	SET_SPI(TXFIFO,((addr >> 8)&0xff));
	while(((GET_SPI(SPSR)) & RFEMPTY) == RFEMPTY){
	}
	GET_SPI(RXFIFO);

	SET_SPI(TXFIFO,(addr & 0xff));
	while(((GET_SPI(SPSR)) & RFEMPTY) == RFEMPTY){
	}
	GET_SPI(RXFIFO);
	// addr end
	
	printf("\n");
	while(i--){
		SET_SPI(TXFIFO,0x00);
		while(((GET_SPI(SPSR)) & RFEMPTY) == RFEMPTY){
		}
		data = GET_SPI(RXFIFO);
		if(base % 16 == 0 ){
			printf("0x%08x    ",base);
		}
		printf("%02x ",data);
		if(base % 16 == 7)
		printf("  ");
		if(base % 16 == 15)
		printf("\n");
		base++;
	}
	printf("\n");
	return 1;
}

int spi_erase_area(unsigned int saddr,unsigned int eaddr,unsigned sectorsize)
{
	unsigned int addr;
	
	spi_initw(); 
	for(addr=saddr;addr<eaddr;addr+=sectorsize){
		SET_SPI(SOFTCS,0x11);
		set_wren();
		write_sr(0x00);
		while(read_sr()&1);
		set_wren();
		SET_SPI(SOFTCS,0x01);
		SET_SPI(TXFIFO,0xd8);
		while((GET_SPI(SPSR))&RFEMPTY);
		GET_SPI(RXFIFO);
		SET_SPI(TXFIFO,addr >> 16);
		while((GET_SPI(SPSR))&RFEMPTY);
		GET_SPI(RXFIFO);
		SET_SPI(TXFIFO,addr >> 8);
		while((GET_SPI(SPSR))&RFEMPTY);
		GET_SPI(RXFIFO);
		SET_SPI(TXFIFO,addr);
		while((GET_SPI(SPSR))&RFEMPTY);
		GET_SPI(RXFIFO);
		SET_SPI(SOFTCS,0x11);
		while(read_sr()&1);
	}
	SET_SPI(SOFTCS,0x11);
	delay(10);
	return 0;
}

int spi_write_area(int flashaddr,char *buffer,int size)
{
	int j;
	
	spi_initw();
	SET_SPI(0x5,0x10);
	write_sr(0x00);
	for(j=0;size > 0;flashaddr++,size--,j++){
		spi_write_byte(flashaddr,buffer[j]);
	}
	SET_SPI(SOFTCS,0x11);
	delay(10);
	return 0;
}


/***********************************************************/	//lxy
void spi_write_byte_fast(unsigned int addr,unsigned char *data, unsigned int size)
{
	/*byte_program,CE 0, cmd 0x2,addr2,addr1,addr0,data in,CE 1*/
	unsigned int i;
	unsigned char addr2,addr1,addr0;
	unsigned char val;
	addr2 = (addr & 0xff0000)>>16;
	addr1 = (addr & 0x00ff00)>>8;
	addr0 = (addr & 0x0000ff);

	//	printf ("lxy: byte_fast, addr = 0x%x, size = %d !\n", addr, size);
	set_wren();
	val = read_sr();
	while((val & 0x01) == 1){
		val = read_sr();
	}
	SET_SPI(SOFTCS,0x01);/*CE 0*/

	SET_SPI(TXFIFO,0x2);/*byte_program */
	while(((GET_SPI(SPSR)) & RFEMPTY) == RFEMPTY){
	}
	val = GET_SPI(RXFIFO);

	/*send addr2*/
	SET_SPI(TXFIFO,addr2);     
	while(((GET_SPI(SPSR)) & RFEMPTY) == RFEMPTY){
	}
	val = GET_SPI(RXFIFO);

	/*send addr1*/
	SET_SPI(TXFIFO,addr1);
	while(((GET_SPI(SPSR)) & RFEMPTY) == RFEMPTY){
	}
	val = GET_SPI(RXFIFO);

	/*send addr0*/
	SET_SPI(TXFIFO,addr0);
	while(((GET_SPI(SPSR)) & RFEMPTY) == RFEMPTY){
	}
	val = GET_SPI(RXFIFO);

	/*send data(one byte)*/
	for (i=0; i<size; i++){
		SET_SPI(TXFIFO,data[i]);
		while(((GET_SPI(SPSR)) & RFEMPTY) == RFEMPTY);
		val = GET_SPI(RXFIFO);
	}

	/*CE 1*/
	SET_SPI(SOFTCS,0x11);
#if 1
	val = read_sr();
	while((val & 0x01) == 1){
		val = read_sr();
	}
#endif
}


int spi_write_area_sst_AAI(int flashaddr,char *buffer,int size)		//lxy
{
	unsigned char val;
	unsigned char addr2,addr1,addr0;
	int count = size;
	addr2 = (flashaddr & 0xff0000)>>16;
	addr1 = (flashaddr & 0x00ff00)>>8;
	addr0 = (flashaddr & 0x0000ff);

	spi_initw();
	SET_SPI(0x5,0x10);
	write_sr(0x00);

	set_wren();
	val = read_sr();
	while((val & 0x01) == 1){
		val = read_sr();
	}
	SET_SPI(SOFTCS,0x01);/*CE 0*/
	SET_SPI(TXFIFO,0xAD);	/*AAI command */
	while(((GET_SPI(SPSR)) & RFEMPTY) == RFEMPTY);
	val = GET_SPI(RXFIFO);

	SET_SPI(TXFIFO,addr2);     /*send addr2*/
	while(((GET_SPI(SPSR)) & RFEMPTY) == RFEMPTY);
	val = GET_SPI(RXFIFO);

	SET_SPI(TXFIFO,addr1);		/*send addr1*/
	while(((GET_SPI(SPSR)) & RFEMPTY) == RFEMPTY);
	val = GET_SPI(RXFIFO);

	SET_SPI(TXFIFO,addr0);		/*send addr0*/
	while(((GET_SPI(SPSR)) & RFEMPTY) == RFEMPTY);
	val = GET_SPI(RXFIFO);

	SET_SPI(TXFIFO, buffer[0]);		/*send data0*/
	while(((GET_SPI(SPSR)) & RFEMPTY) == RFEMPTY);
	val = GET_SPI(RXFIFO);

	SET_SPI(TXFIFO, buffer[1]);		/*send data1*/
	while(((GET_SPI(SPSR)) & RFEMPTY) == RFEMPTY);
	val = GET_SPI(RXFIFO);

	SET_SPI(SOFTCS,0x11);		/* read status */
	val = read_sr();
	while((val & 0x01) == 1){
		val = read_sr();
	}

	count 	-= 2;	
	buffer 	+= 2;
	while(count > 0){
		SET_SPI(SOFTCS,0x01);
		SET_SPI(TXFIFO,0xAD);	/*AAI command */
		while(((GET_SPI(SPSR)) & RFEMPTY) == RFEMPTY);
		val = GET_SPI(RXFIFO);

		SET_SPI(TXFIFO, *(buffer++));		/*send data(n-1)*/
		while(((GET_SPI(SPSR)) & RFEMPTY) == RFEMPTY);
		val = GET_SPI(RXFIFO);

		SET_SPI(TXFIFO, *(buffer++));		/*send data(n)*/
		while(((GET_SPI(SPSR)) & RFEMPTY) == RFEMPTY);
		val = GET_SPI(RXFIFO);

		SET_SPI(SOFTCS,0x11);		/* read status */
		val = read_sr();
		while((val & 0x01) == 1){
			val = read_sr();
		}		

		count 	-= 2;
	}

	SET_SPI(SOFTCS,0x01);
	SET_SPI(TXFIFO,0x04);	/*write-disable command */
	while(((GET_SPI(SPSR)) & RFEMPTY) == RFEMPTY);
	val = GET_SPI(RXFIFO);

	SET_SPI(SOFTCS,0x11);		/* read status */
	val = read_sr();
	while((val & 0x01) == 1){
		val = read_sr();
	}
}

int spi_write_area_sst_fast(int flashaddr,char *buffer,int size)		//lxy
{
	int count = size;
	int temp;
	temp = count % 2;
	
	if (count % 2){		
		spi_write_area(flashaddr, buffer, 1);
		count--;
		buffer++;
		flashaddr++;		
	}
	if (count != 0){
		spi_write_area_sst_AAI(flashaddr, buffer, count);
	}
}

int spi_write_area_fast(int flashaddr,char *buffer,int size)		//lxy
{
	int j;
	
	spi_initw();
	SET_SPI(0x5,0x10);
	write_sr(0x00);
	for(j=0;size > 0;){
		if (size >= 256){
			spi_write_byte_fast(flashaddr, &buffer[j], 256);
			size -= 256;
			j += 256;
			flashaddr += 256;
		}
		else{
			spi_write_byte_fast(flashaddr, &buffer[j], size);
			break;
		}
	}

	SET_SPI(SOFTCS,0x11);
	delay(10);
	return 0;
}

int spi_read_area_fast(loff_t flashaddr, unsigned char *buffer, size_t size)
{
	unsigned int i;

	/* 1A/1B 的SPI控制器 支持SPI Flash快速(高速 双IO)读取 但只支持最大8MB容量
	   所以需要快速读取的分区如内核区，尽量设置在8MB内
	*/
	if (flashaddr+size < 0x800000) {
		SET_SPI(SPSR, 0xc0); 
	  	SET_SPI(PARAM, 0x0f);	//double I/O 模式 部分SPI flash可能不支持
	 	SET_SPI(SPER, 0x04);	//spre:00
	  	SET_SPI(PARAM2, 0x01);
		SET_SPI(SPCR, 0x5c);
		unsigned char *flash_addr = 0xbf000000 + flashaddr;
		for(i=0; i<size; i++) {
			*(buffer++) = *(flash_addr++);
		}
		SET_SPI(PARAM, 0x01);
	} else {
		spi_initw();

		SET_SPI(SOFTCS, 0x01);
		SET_SPI(TXFIFO, 0x0b);
		while((GET_SPI(SPSR))&RFEMPTY);
		GET_SPI(RXFIFO);

		SET_SPI(TXFIFO,flashaddr>>16);     
		while((GET_SPI(SPSR))&RFEMPTY);
		GET_SPI(RXFIFO);

		SET_SPI(TXFIFO,flashaddr>>8);     
		while((GET_SPI(SPSR))&RFEMPTY);
		GET_SPI(RXFIFO);

		SET_SPI(TXFIFO,flashaddr);     
		while((GET_SPI(SPSR))&RFEMPTY);
		GET_SPI(RXFIFO);

		SET_SPI(TXFIFO, 0x00);     
		while((GET_SPI(SPSR))&RFEMPTY);
		GET_SPI(RXFIFO);

		for(i=0; i<size; i++) {
			SET_SPI(TXFIFO,0);     
			while((GET_SPI(SPSR))&RFEMPTY);
			buffer[i] = GET_SPI(RXFIFO);
		}

		SET_SPI(SOFTCS, 0x11);
	}

	return 0;
}
/***********************************************************/	//lxy



int spi_read_area(int flashaddr,char *buffer,int size)
{
	int i;
	spi_initw();
	SET_SPI(SOFTCS,0x01);

	SET_SPI(TXFIFO,0x03);

	while((GET_SPI(SPSR))&RFEMPTY);
	GET_SPI(RXFIFO);

	SET_SPI(TXFIFO,flashaddr>>16);     
	while((GET_SPI(SPSR))&RFEMPTY);
	GET_SPI(RXFIFO);

	SET_SPI(TXFIFO,flashaddr>>8);     
	while((GET_SPI(SPSR))&RFEMPTY);
	GET_SPI(RXFIFO);

	SET_SPI(TXFIFO,flashaddr);     
	while((GET_SPI(SPSR))&RFEMPTY);
	GET_SPI(RXFIFO);


	for(i=0;i<size;i++) {
		SET_SPI(TXFIFO,0);     
		while((GET_SPI(SPSR))&RFEMPTY);
		buffer[i] = GET_SPI(RXFIFO);
	}

	SET_SPI(SOFTCS,0x11);
	delay(10);
	return 0;
}


#if 0
struct fl_device myflash = {
	.fl_name="spiflash",
	.fl_size	= 0x800000,	//0x100000,	//lxy
	.fl_secsize	= 0x10000,	//0x10000,
};
#endif

struct fl_device *fl_devident(void *base, struct fl_map **m)
{
	struct fl_device *dev;

	if (init_id == 0) {
		nor_dev = NULL;
		read_jedecid(flash_id);
		init_id = 1;

		for(dev = &fl_known_dev[0]; dev->fl_name != 0; dev++) {
			if(dev->fl_mfg == (char)flash_id[0] && dev->fl_id == (char)flash_id[2]) {
				nor_dev = dev;
				return(dev);	/* GOT IT! */
			}
		}
	}
	return nor_dev;
}


int fl_program_device(void *fl_base, void *data_base, int data_size, int verbose)
{
	struct fl_map *map;
	int off;

#if 1
	fl_devident(fl_base, NULL);
	map = fl_find_map(fl_base);
	off = (int)(fl_base - map->fl_map_base) + map->fl_map_offset;
	if (nor_dev != NULL){
		if (!strcmp(nor_dev->fl_name, "sst25vf080")){
			printf ("flash sst25vf080.....\n");
			spi_write_area_sst_fast(off, data_base, data_size);
		}
		//使用winb25x128bf 可能需要修改SPI控制器的SPER寄存器，提高分频值，winb25x128bf的工作频率不能太高
		else if (!strcmp(nor_dev->fl_name, "winb25x128")){
			printf ("flash winb25x128.....\n");
			spi_write_area_fast(off, data_base, data_size);
		}
		else if (!strcmp(nor_dev->fl_name, "winb25x64")){
			printf ("flash winb25x64.....\n");
			spi_write_area_fast(off, data_base, data_size);
		}
		else if (!strcmp(nor_dev->fl_name, "winb25x40")){
			printf ("flash winb25x40.....\n");
			spi_write_area_fast(off, data_base, data_size);
		}
	}
	else{
		printf ("unknow flash type. try to program .....\n");
		spi_write_area(off, data_base, data_size);
	}
#else
	map = fl_find_map(fl_base);
	off = (int)(fl_base - map->fl_map_base) + map->fl_map_offset;
	spi_write_area(off,data_base,data_size);
#endif
	spi_initr();
	return 0;
}


int fl_erase_device(void *fl_base, int size, int verbose)
{
	struct fl_map *map;
	int off;
	map = fl_find_map(fl_base);
	off = (int)(fl_base - map->fl_map_base) + map->fl_map_offset;
	spi_erase_area(off,off+size,0x10000);
	spi_initr();
	return 0;
}

/*************************************************************************/	//lxy
#ifdef NORFLASH_PARTITION
int nor_mtd_read(struct mtd_info *mtd, loff_t from, size_t len,
		     size_t *retlen, uint8_t *buf)
{
	if ((from + len) > mtd->size)
		return -EINVAL;
	if (!len)
		return 0;
	spi_read_area_fast(from, buf, len);

	spi_initr();
	*retlen = len;	
}

int nor_mtd_write(struct mtd_info *mtd, loff_t to, size_t len,
			  size_t *retlen, const uint8_t *buf)
{
	unsigned int addr = (unsigned int)to;
	if ((addr + len) > mtd->size)
		return -EINVAL;
	if (!len)
		return 0;

	fl_program_device(0xbfc00000+addr, buf, len, 0);

	spi_initr();
	*retlen = len;
}

int nor_mtd_erase (struct mtd_info *mtd, struct erase_info *instr)
{
	int start_addr, end_addr;
	start_addr	= instr->addr;
	end_addr	= instr->len + start_addr;

	spi_erase_area(start_addr, end_addr, 0x10000);
	spi_initr();
}

void print_sector(void)
{
	int i;
	unsigned char val[512];

	spi_initw();
	set_wren();
	memset (val, 0x55, 512);
	spi_read_area_fast(0x0, val, 512);

	for (i=0; i<512; i++) {
		if (i%16)	{
			printf ("0x%x\t", val[i]);
		}
		else {
			printf ("\n");
			printf ("0x%x\t", val[i]);
		}
	}
	printf ("\n");

}

void norflash_init(void)
{
	struct mtd_info *nor_mtd;
	nor_mtd = malloc(sizeof(struct mtd_info));	
	memset(nor_mtd, 0, sizeof(struct mtd_info));
	nor_mtd->read		= nor_mtd_read;
	nor_mtd->write		= nor_mtd_write;
	nor_mtd->erase		= nor_mtd_erase;
	nor_mtd->size		= 0x800000;
#ifdef W25Q128
	nor_mtd->size		= 0x1000000;
#endif
	nor_mtd->erasesize	= 64 * 1024;
	nor_mtd->type		= MTD_NORFLASH;
	nor_mtd->name		= "ls1b-nor";

	//W25Q128 16MB
#ifdef W25Q128
	/* 1A/1B 的SPI控制器 支持SPI Flash快速(高速 双IO)读取 但只支持最大8MB容量
	   所以需要快速读取的分区如内核区，尽量设置在8MB内
	*/
	add_mtd_device(nor_mtd, 0, 512*1024, "pmon_nor");					//512KB
	add_mtd_device(nor_mtd, 512*1024, (512+7*1024)*1024, "kernel_nor");	//7.5MB
	add_mtd_device(nor_mtd, 8*1024*1024, 8*1024*1024, "fs_nor");		//8MB
#endif

#if 1	//for bobodog program
	add_mtd_device(nor_mtd, 0, 			0x80000, 	"pmon_nor");	
	add_mtd_device(nor_mtd, 0x80000,	0x210000, 	"kernel_nor");
	add_mtd_device(nor_mtd, 0x290000,	0x500000,	"fs_nor");
	add_mtd_device(nor_mtd, 0x790000,	0x70000,	"data_nor");
#else
	add_mtd_device(nor_mtd, 0, 			0x80000, 	"pmon_nor");	
	add_mtd_device(nor_mtd, 0x80000,	0x2c0000, 	"kernel_nor");
	add_mtd_device(nor_mtd, 0x340000, 	0x180000, 	"system_nor");
	add_mtd_device(nor_mtd, 0x520000,	0x2e0000,	"data_nor");
#endif

	wb_write_sr(0);
	spi_initr();
}
#endif

/*************************************************************************/	//lxy
static const Cmd Cmds[] =
{
	{"MyCmds"},
	{"spi_initw","",0,"spi_initw(sst25vf080b)",spi_initw,0,99,CMD_REPEAT},
	{"read_pmon","",0,"read_pmon(sst25vf080b)",read_pmon,0,99,CMD_REPEAT},
	{"write_pmon","",0,"write_pmon(sst25vf080b)",write_pmon,0,99,CMD_REPEAT},
	{"erase_all","",0,"erase_all(sst25vf080b)",erase_all,0,99,CMD_REPEAT},
	{"write_pmon_byte","",0,"write_pmon_byte(sst25vf080b)",write_pmon_byte,0,99,CMD_REPEAT},
	{"read_flash_id","",0,"read_flash_id(sst25vf080b)",spi_read_id,0,99,CMD_REPEAT},
	{0,0}
};

static void init_cmd __P((void)) __attribute__ ((constructor));

static void
init_cmd()
{
	cmdlist_expand(Cmds,1);
}

