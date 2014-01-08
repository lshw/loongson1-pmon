#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <errno.h>
#include <pmon.h>
#include <cpu.h>
#include <pflash.h>
#include <linux/mtd/mtd.h>

#include <target/ls1x_spi.h>
#include <target/fcr.h>

extern struct spi_device spi_flash;

static unsigned char flash_id[3];
static unsigned char init_id = 0;
static struct fl_device *nor_dev;

static int read_status(void)
{
	int val;

	ls1x_spi_chipselect(&spi_flash, 1);
	ls1x_spi_writeb(&spi_flash, 0x05);
	val = ls1x_spi_readb(&spi_flash);
	ls1x_spi_chipselect(&spi_flash, 0);

	return val;
}

static int write_enable(void)
{
	while (read_status() & 0x01);

	ls1x_spi_chipselect(&spi_flash, 1);
	ls1x_spi_writeb(&spi_flash, 0x06);
	ls1x_spi_chipselect(&spi_flash, 0);

	return 1;
}

static int write_status(char val)
{
	write_enable();
	while (read_status() & 0x01);

	ls1x_spi_chipselect(&spi_flash, 1);
	ls1x_spi_writeb(&spi_flash, 0x01);
	ls1x_spi_writeb(&spi_flash, val);
	ls1x_spi_chipselect(&spi_flash, 0);
	return 1;
}

static void read_jedecid(unsigned char *p)
{
	int i;

	ls1x_spi_chipselect(&spi_flash, 1);
	ls1x_spi_writeb(&spi_flash, 0x9f);

	for (i=0; i<3; i++) {
		p[i] = ls1x_spi_readb(&spi_flash);
	}
	ls1x_spi_chipselect(&spi_flash, 0);
}

static int wb_write_status(unsigned int val)
{
	unsigned char lo, hi;
	lo = val;
	hi = (val >> 8) & 0xff;

	write_enable();
	while (read_status() & 0x01);

	ls1x_spi_chipselect(&spi_flash, 1);
	ls1x_spi_writeb(&spi_flash, 0x01);
	ls1x_spi_writeb(&spi_flash, lo);
	ls1x_spi_writeb(&spi_flash, hi);
	ls1x_spi_chipselect(&spi_flash, 0);

	return 1;
}

static int erase_all(int argc, char **argv)
{
	int i = 1;

	write_enable();
	while (read_status() & 0x01);

	ls1x_spi_chipselect(&spi_flash, 1);
	ls1x_spi_writeb(&spi_flash, 0xc7);
	ls1x_spi_chipselect(&spi_flash, 0);

	while (i++) {
		if ((read_status() & 0x1) == 0x1) {
			if(i % 10000 == 0)
				printf(".");
		} else {
			printf("done...\n");
			break;
		}
	}
	return 0;
}

static void spi_flash_write_byte(unsigned int addr, unsigned char data)
{
	unsigned char addr2, addr1, addr0;
	addr2 = (addr & 0xff0000)>>16;
	addr1 = (addr & 0x00ff00)>>8;
	addr0 = (addr & 0x0000ff);
	
	write_enable();
	while (read_status() & 0x01);

	ls1x_spi_chipselect(&spi_flash, 1);

	ls1x_spi_writeb(&spi_flash, 0x02);
	ls1x_spi_writeb(&spi_flash, addr2);
	ls1x_spi_writeb(&spi_flash, addr1);
	ls1x_spi_writeb(&spi_flash, addr0);
	ls1x_spi_writeb(&spi_flash, data);

	ls1x_spi_chipselect(&spi_flash, 0);
}

static void spi_flash_read_id(void)
{
	unsigned char val;

	ls1x_spi_chipselect(&spi_flash, 1);
	/*READ ID CMD*/
	ls1x_spi_writeb(&spi_flash, 0x9f);

	/*Manufacturer’s ID*/
	val = ls1x_spi_readb(&spi_flash);
	printf("Manufacturer's ID:         %x\n",val);

	/*Device ID:Memory Type*/
	val = ls1x_spi_readb(&spi_flash);
	printf("Device ID-memory_type:     %x\n",val);

	/*Device ID:Memory Capacity*/
	val = ls1x_spi_readb(&spi_flash);
	printf("Device ID-memory_capacity: %x\n",val);

	ls1x_spi_chipselect(&spi_flash, 0);
}


int spi_flash_erase_area(unsigned int saddr, unsigned int eaddr, unsigned sectorsize)
{
	unsigned int addr;

	for (addr=saddr; addr<eaddr; addr+=sectorsize) {
		write_enable();
		write_status(0x00);
		while (read_status() & 0x01);
		write_enable();

		ls1x_spi_chipselect(&spi_flash, 1);
		ls1x_spi_writeb(&spi_flash, 0xd8);
		ls1x_spi_writeb(&spi_flash, addr >> 16);
		ls1x_spi_writeb(&spi_flash, addr >> 8);
		ls1x_spi_writeb(&spi_flash, addr);
		ls1x_spi_chipselect(&spi_flash, 0);
		while (read_status() & 0x01);
	}
	while (read_status() & 0x01);
	return 0;
}

int spi_flash_write_area(int flashaddr, char *buffer, int size)
{
	int i;

	write_status(0x00);
	for (i=0; size>0; flashaddr++,size--,i++) {
		spi_flash_write_byte(flashaddr, buffer[i]);
	}
	while (read_status() & 0x01);
	return 0;
}

static void spi_flash_write_byte_fast(unsigned int addr, char *data, unsigned int size)
{
	unsigned int i;
	unsigned char addr2, addr1, addr0;

	addr2 = (addr & 0xff0000)>>16;
	addr1 = (addr & 0x00ff00)>>8;
	addr0 = (addr & 0x0000ff);

	write_enable();
	while (read_status() & 0x01);

	ls1x_spi_chipselect(&spi_flash, 1);

	ls1x_spi_writeb(&spi_flash, 0x02);
	ls1x_spi_writeb(&spi_flash, addr2);
	ls1x_spi_writeb(&spi_flash, addr1);
	ls1x_spi_writeb(&spi_flash, addr0);

	/*send data(one byte)*/
	for (i=0; i<size; i++) {
		ls1x_spi_writeb(&spi_flash, data[i]);
	}
	ls1x_spi_chipselect(&spi_flash, 0);

	while (read_status() & 0x01);
}

static int spi_flash_write_area_sst_AAI(int flashaddr, char *buffer, int size)
{
	unsigned char addr2, addr1, addr0;
	int count = size;
	addr2 = (flashaddr & 0xff0000)>>16;
	addr1 = (flashaddr & 0x00ff00)>>8;
	addr0 = (flashaddr & 0x0000ff);

	write_status(0x00);

	write_enable();
	while (read_status() & 0x01);

	ls1x_spi_chipselect(&spi_flash, 1);
	/*AAI command */
	ls1x_spi_writeb(&spi_flash, 0xad);
	ls1x_spi_writeb(&spi_flash, addr2);
	ls1x_spi_writeb(&spi_flash, addr1);
	ls1x_spi_writeb(&spi_flash, addr0);

	ls1x_spi_writeb(&spi_flash, buffer[0]);
	ls1x_spi_writeb(&spi_flash, buffer[1]);
	ls1x_spi_chipselect(&spi_flash, 0);

	while (read_status() & 0x01);

	count -= 2;	
	buffer += 2;
	while (count > 0) {
		ls1x_spi_chipselect(&spi_flash, 1);
		/*AAI command */
		ls1x_spi_writeb(&spi_flash, 0xad);
		ls1x_spi_writeb(&spi_flash, *(buffer++));
		ls1x_spi_writeb(&spi_flash, *(buffer++));
		ls1x_spi_chipselect(&spi_flash, 0);
		/* read status */
		while (read_status() & 0x01);

		count -= 2;
	}

	ls1x_spi_chipselect(&spi_flash, 1);
	ls1x_spi_writeb(&spi_flash, 0x04);
	ls1x_spi_chipselect(&spi_flash, 0);

	while (read_status() & 0x01);
}

static int spi_flash_write_area_sst_fast(int flashaddr, char *buffer, int size)
{
	int count = size;
	int temp;
	temp = count % 2;
	
	if (count % 2) {
		spi_flash_write_area(flashaddr, buffer, 1);
		count--;
		buffer++;
		flashaddr++;		
	}
	if (count != 0) {
		spi_flash_write_area_sst_AAI(flashaddr, buffer, count);
	}
}

static int spi_flash_write_area_fast(int flashaddr, char *buffer, int size)
{
	int i;

	write_status(0x00);

	for (i=0; size>0;) {
		if (size >= 256) {
			spi_flash_write_byte_fast(flashaddr, &buffer[i], 256);
			size -= 256;
			i += 256;
			flashaddr += 256;
		} else {
			spi_flash_write_byte_fast(flashaddr, &buffer[i], size);
			break;
		}
	}

	while (read_status() & 0x01);
	return 0;
}

static int spi_flash_read_area_fast(loff_t flashaddr, unsigned char *buffer, size_t size)
{
	unsigned int i;

	/* 1A/1B 的SPI控制器 支持SPI Flash快速(高速 双IO)读取 但只支持最大8MB容量
	   所以需要快速读取的分区如内核区，尽量设置在8MB内
	*/
	if (flashaddr+size < 0x800000) {
/*		SET_SPI(SPSR, 0xc0); 
	  	SET_SPI(PARAM, 0x0f);	//double I/O 模式 部分SPI flash可能不支持
	 	SET_SPI(SPER, 0x04);	//spre:00
	  	SET_SPI(TIMING, 0x05);
		SET_SPI(SPCR, 0x5d);*/
		ls1x_spi_chipselect(&spi_flash, 1);
		ls1x_spi_flash_ren(&spi_flash, 1);
		unsigned char *flash_addr = (unsigned char *)(0xbf000000 + flashaddr);
		for(i=0; i<size; i++) {
			*(buffer++) = *(flash_addr++);
		}
//		SET_SPI(PARAM, 0x01);
		ls1x_spi_flash_ren(&spi_flash, 0);
		ls1x_spi_chipselect(&spi_flash, 0);
	} else {
		ls1x_spi_chipselect(&spi_flash, 1);
		ls1x_spi_writeb(&spi_flash, 0x0b);
		ls1x_spi_writeb(&spi_flash, flashaddr >> 16);
		ls1x_spi_writeb(&spi_flash, flashaddr >> 8);
		ls1x_spi_writeb(&spi_flash, flashaddr);
		ls1x_spi_writeb(&spi_flash, 0x00);

		for (i=0; i<size; i++) {
			buffer[i] = ls1x_spi_readb(&spi_flash);
		}
		ls1x_spi_chipselect(&spi_flash, 0);
	}

	return 0;
}

int spi_flash_read_area(int flashaddr, char *buffer, int size)
{
	int i;

	ls1x_spi_chipselect(&spi_flash, 1);
	ls1x_spi_writeb(&spi_flash, 0x0b);
	ls1x_spi_writeb(&spi_flash, flashaddr >> 16);
	ls1x_spi_writeb(&spi_flash, flashaddr >> 8);
	ls1x_spi_writeb(&spi_flash, flashaddr);
	ls1x_spi_writeb(&spi_flash, 0x00);

	for (i=0; i<size; i++) {
		*(buffer++) = ls1x_spi_readb(&spi_flash);
	}
	ls1x_spi_chipselect(&spi_flash, 0);

	while (read_status() & 0x01);
	return 0;
}

struct fl_device *fl_devident(void *base, struct fl_map **m)
{
	struct fl_device *dev;

	if(m)
		*m = fl_find_map(base);

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
		if (nor_dev->fl_mfg == (char)0xbf) {
			printf (" byte write %s\n", nor_dev->fl_name);
			spi_flash_write_area_sst_fast(off, data_base, data_size);	/* SST */
		}
		else if (nor_dev->fl_mfg == (char)0xef) {
			printf (" byte write %s\n", nor_dev->fl_name);
			spi_flash_write_area_fast(off, data_base, data_size);		/* winbond */
		}
	}
	else {
		printf (" byte write unknow flash type\n");
		spi_flash_write_area(off, data_base, data_size);
	}
#else
	map = fl_find_map(fl_base);
	off = (int)(fl_base - map->fl_map_base) + map->fl_map_offset;
	spi_flash_write_area(off,data_base,data_size);
#endif

	return 0;
}


int fl_erase_device(void *fl_base, int size, int verbose)
{
	struct fl_map *map;
	int off;

	map = fl_find_map(fl_base);
	off = (int)(fl_base - map->fl_map_base) + map->fl_map_offset;
	spi_flash_erase_area(off, off+size, 0x10000);

	return 0;
}

/*************************************************************************/
#ifdef NORFLASH_PARTITION
static int nor_mtd_read(struct mtd_info *mtd, loff_t from, size_t len,
		     size_t *retlen, uint8_t *buf)
{
	if ((from + len) > mtd->size)
		return -EINVAL;
	if (!len)
		return 0;

	spi_flash_read_area_fast(from, buf, len);
	*retlen = len;	
}

static int nor_mtd_write(struct mtd_info *mtd, loff_t to, size_t len,
			  size_t *retlen, const uint8_t *buf)
{
	unsigned int addr = (unsigned int)to;

	if ((addr + len) > mtd->size)
		return -EINVAL;
	if (!len)
		return 0;

	fl_program_device(0xbfc00000+addr, buf, len, 0);
	*retlen = len;
}

static int nor_mtd_erase(struct mtd_info *mtd, struct erase_info *instr)
{
	int start_addr, end_addr;
	start_addr	= instr->addr;
	end_addr	= instr->len + start_addr;

	spi_flash_erase_area(start_addr, end_addr, 0x10000);
}

static void print_sector(void)
{
	int i;
	unsigned char val[512];

	write_enable();
	memset (val, 0x55, 512);
	spi_flash_read_area_fast(0x0, val, 512);

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
#ifdef W25Q128
	nor_mtd->size		= 0x1000000;
#elif W25X64
	nor_mtd->size		= 0x800000;
#else
	nor_mtd->size		= 0x80000;
#endif
	nor_mtd->erasesize	= 64 * 1024;
	nor_mtd->type		= MTD_NORFLASH;
	nor_mtd->name		= "ls1b-nor";

	/* W25Q128 16MB
	   使用winb25x128bf 可能需要修改SPI控制器的SPER寄存器，提高分频值，winb25x128bf的工作频率不能太高
	 */
#ifdef W25Q128
	/* 1A/1B 的SPI控制器 支持SPI Flash快速(高速 双IO)读取 但只支持最大8MB容量
	   所以需要快速读取的分区如内核区，尽量设置在8MB内
	*/
	add_mtd_device(nor_mtd, 0, 512*1024, "pmon_nor");					//512KB
	add_mtd_device(nor_mtd, 512*1024, (512+7*1024)*1024, "kernel_nor");	//7.5MB
	add_mtd_device(nor_mtd, 8*1024*1024, 8*1024*1024, "fs_nor");		//8MB
#elif W25X64
	add_mtd_device(nor_mtd, 0, 512*1024, "pmon");					//512KB
	add_mtd_device(nor_mtd, 512*1024, (512+6*1024)*1024, "kernel");	//6.5MB
	add_mtd_device(nor_mtd, 7*1024*1024, 1*1024*1024, "fs");		//1MB
#else
	add_mtd_device(nor_mtd, 0, 0x80000, "pmon");	//512KB
#endif

	wb_write_status(0);
}
#endif
/*************************************************************************/

static const Cmd Cmds[] =
{
	{"MyCmds"},
	{"erase_all", "", 0, "erase all chip", erase_all, 0, 99, CMD_REPEAT},
//	{"spi_flash_read_id", "", 0, "read_flash_id(sst25vf080b)", spi_flash_read_id, 0, 99, CMD_REPEAT},
	{0,0}
};

static void init_cmd __P((void)) __attribute__ ((constructor));

static void init_cmd(void)
{
	cmdlist_expand(Cmds, 1);
}

