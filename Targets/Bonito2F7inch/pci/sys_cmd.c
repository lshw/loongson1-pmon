#include <sys/linux/types.h>
#include <sys/param.h>
#include <sys/device.h>
#include <sys/systm.h>
#include <sys/malloc.h>

#include <dev/pci/pcivar.h>
#include <dev/pci/pcireg.h>
#include <dev/pci/nppbreg.h>

#include <machine/bus.h>

#include <include/bonito.h>
#include <include/kb3310.h>
#include <include/cs5536.h>
#include <include/cs5536_pci.h>
#include <pmon.h>
#include "cs5536_io.h"

/*********************************MSR debug*********************************/

static int cmd_rdmsr(int ac, char *av[])
{
	u32 msr, hi, lo;

	if(!get_rsa(&msr, av[1])) {
		printf("msr : access error!\n");
		return -1;
	}

	_rdmsr(msr, &hi, &lo);
	
	printf("msr : address  %x    hi  %x   lo  %x\n", msr, hi, lo);
	
	return 0;
}

static int cmd_wrmsr(int ac, char *av[])
{
	u32 msr, hi, lo;

	if(!get_rsa(&msr, av[1])){
		printf("msr : access error!\n");
		return -1;
	}else if(!get_rsa(&hi, av[2])){
		printf("hi : access error!\n");
		return -1;
	}else if(!get_rsa(&lo, av[3])){
		printf("lo : access error!\n");
		return -1;
	}

	_wrmsr(msr, hi, lo);
	
	return 0;
}

/*******************************Display debug*******************************/

#define	DISPLAY_MODE_NORMAL		0	// Screen(on);	Hsync(on);	Vsync(on)
#define	DISPLAY_MODE_STANDBY	1	// Screen(off);	Hsync(off);	Vsync(on)
#define	DISPLAY_MODE_SUSPEND	2	// Screen(off);	Hsync(on);	Vsync(off)
#define	DISPLAY_MODE_OFF		3	// Screen(off);	Hsync(off);	Vsync(off)
#define	SEQ_INDEX	0x3c4
#define	SEQ_DATA	0x3c5
static inline unsigned char read_seq(int reg)
{
	*((volatile unsigned char *)(0xbfd00000 | SEQ_INDEX)) = reg;
	return (*(volatile unsigned char *)(0xbfd00000 | SEQ_DATA));
}

static inline void write_seq(int reg, unsigned char data)
{
	*((volatile unsigned char *)(0xbfd00000 | SEQ_INDEX)) = reg;
	*((volatile unsigned char *)(0xbfd00000 | SEQ_DATA)) = data;

	return;
}

static u8 cur_mode;
static u8 SavedSR01, SavedSR20, SavedSR21, SavedSR22, SavedSR23, SavedSR24, SavedSR31, SavedSR34;
static int cmd_powerdebug(int ac, char *av[])
{
	u8 index;
	u8 SR01, SR20, SR21, SR22, SR23, SR24, SR31, SR34;

	if(ac < 2){
		printf("usage : powerdebug %index\n");
		return -1;
	}

	if(!get_rsa(&index, av[1])) {
		printf("powerdebug : access error!\n");
		return -1;
	}
	
	if(cur_mode == DISPLAY_MODE_NORMAL){
		SavedSR01 = read_seq(0x01);
		SavedSR20 = read_seq(0x20);
		SavedSR21 = read_seq(0x21);
		SavedSR22 = read_seq(0x22);
		SavedSR23 = read_seq(0x23);
		SavedSR24 = read_seq(0x24);
		SavedSR31 = read_seq(0x31);
		SavedSR34 = read_seq(0x34);
		printf("normal power register : \n");
		printf("SR01 0x%x   SR20 0x%x   SR21 0x%x   SR22 0x%x   SR23 0x%x\n", 
						SavedSR01, SavedSR20, SavedSR21, SavedSR22, SavedSR23, SavedSR24);
		printf("SR24 0x%x   SR31 0x%x   SR34 0x%x\n", SavedSR24, SavedSR31, SavedSR34);
	}
	SR01 = read_seq(0x01);
	SR20 = read_seq(0x20);
	SR21 = read_seq(0x21);
	SR22 = read_seq(0x22);
	SR23 = read_seq(0x23);
	SR24 = read_seq(0x24);
	SR31 = read_seq(0x31);
	SR34 = read_seq(0x34);

	printf("powerdebug : mode %d\n", index);
	switch(index){
		case	DISPLAY_MODE_NORMAL :
				SR01 &= ~0x20;
				SR20 = SavedSR20;
		        SR21 = SavedSR21;
				SR22 &= ~0x30; 
				SR23 &= ~0xC0;
				SR24 |= 0x01;
				SR31 = SavedSR31;
				SR34 = SavedSR34;
				break;
		case	DISPLAY_MODE_STANDBY :
				SR01 |= 0x20;
				SR20 = (SR20 & ~0xB0) | 0x10;
				SR21 |= 0x88;
				SR22 = (SR22 & ~0x30) | 0x10;
				SR23 = (SR23 & ~0x07) | 0xD8;
				SR24 &= ~0x01;
				SR31 = (SR31 & ~0x07) | 0x00;
				SR34 |= 0x80;
				break;
		case	DISPLAY_MODE_SUSPEND :
				SR01 |= 0x20;
				SR20 = (SR20 & ~0xB0) | 0x10;
				SR21 |= 0x88;
				SR22 = (SR22 & ~0x30) | 0x20;
				SR23 = (SR23 & ~0x07) | 0xD8;
				SR24 &= ~0x01;
				SR31 = (SR31 & ~0x07) | 0x00;
				SR34 |= 0x80;
				break;
		case	DISPLAY_MODE_OFF :
				SR01 |= 0x20;
			    SR20 = (SR20 & ~0xB0) | 0x10;
				SR21 |= 0x88;
				SR22 = (SR22 & ~0x30) | 0x30;
				SR23 = (SR23 & ~0x07) | 0xD8;
				SR24 &= ~0x01;
				SR31 = (SR31 & ~0x07) | 0x00;
				SR34 |= 0x80;
				break;
		default :
				printf("powerdebug : not supported mode setting for display.\n");
				break;
	}

	if( (*((volatile unsigned char *)(0xbfd00000 | 0x3cc))) & 0x01 ){
		while( (*((volatile unsigned char *)(0xbfd00000 | 0x3da))) & 0x08 );
		while( !((*((volatile unsigned char *)(0xbfd00000 | 0x3da))) & 0x08) );
	}else{
		while( (*((volatile unsigned char *)(0xbfd00000 | 0x3ba))) & 0x08 );
		while( !((*((volatile unsigned char *)(0xbfd00000 | 0x3ba))) & 0x08) );
	}

	write_seq(0x01, SR01);
	write_seq(0x20, SR20);
	write_seq(0x21, SR21);
	write_seq(0x22, SR22);
	write_seq(0x23, SR23);
	write_seq(0x24, SR24);
	write_seq(0x31, SR31);
	write_seq(0x34, SR34);

	cur_mode = index;

	return 0;
}

/***********************************EC debug*******************************/

/*
 * rdecreg : read a ec register area.
 */
static int cmd_rdecreg(int ac, char *av[])
{
	u32 start, size, reg;

	size = 0;

	if(ac < 3){
		printf("usage : rdecreg start_addr size\n");
		return -1;
	}

	if(!get_rsa(&start, av[1])) {
		printf("ecreg : access error!\n");
		return -1;
	}

	if(!get_rsa(&size, av[2])){
		printf("ecreg : size error\n");
		return -1;
	}

	if((start > 0x10000) || (size > 0x10000)){
		printf("ecreg : start not available.\n");
	}

	printf("ecreg : reg=0x%x size=0x%x\n",start, size);
	reg = start;
	while(size > 0){
		printf("reg address : 0x%x,  value : 0x%x, value : %c\n", reg, rdec(reg), rdec(reg));
		reg++;
		size--;
	}

	return 0;
}

/*
 * wrecreg : write a ec register area.
 */
static int cmd_wrecreg(int ac, char *av[])
{
	u32 addr;
	u32 value;
	u8 val8;

	if(ac < 3){
		printf("usage : wrecreg addr val\n");
		return -1;
	}

	if(!get_rsa(&addr, av[1])) {
		printf("ecreg : access error!\n");
		return -1;
	}

	if(!get_rsa(&value, av[2])){
		printf("ecreg : size error\n");
		return -1;
	}

	val8 = (u8)value;

	if((addr > 0x10000)){
		printf("ecreg : addr not available.\n");
	}


	printf("ecreg : addr=0x%x value=0x%x\n",addr , val8);
	wrec(addr,val8);

	return 0;
}

/*
 * rdbat : read a register of battery.
 */
static int cmd_rdbat(int ac, char *av[])
{
	u8 i;
	u8 index;
	u8 val;

	if(ac < 2){
		printf("usage : rdbat %index\n");
		return -1;
	}

	if(!get_rsa(&index, av[1])) {
		printf("rdbat : access error!\n");
		return -1;
	}

	for(i = 0; i < SMBDAT_SIZE; i++){
		wrec(REG_SMBDAT_START + i, 0x00);
	}
	printf("battery : index 0x%x \t", index);
	wrec(REG_SMBSTS, 0x00);
	wrec(REG_SMBCNT, 0x01);
	val = rdec(REG_SMBPIN);
	val = (val & 0xfc) | (1 << 0);
	wrec(REG_SMBPIN, val);
	wrec(REG_SMBADR, 0x6c|0x01);
	wrec(REG_SMBCMD, index);
	wrec(REG_SMBPRTCL, 0x07);
	while(!(rdec(REG_SMBSTS) & (1 << 7)));

	val = rdec(REG_SMBDAT_START);
	printf("value 0x%x\n", val);

	return 0;
}

/* 
 * rdfan : read a register of temperature IC. 
 */
static int cmd_rdfan(int ac, char *av[])
{
	u8 index;
	u8 val;
	u8 i;

	wrec(REG_SMBCFG, rdec(REG_SMBCFG) & (~0x1f) | 0x04);
	if(ac < 2){
		printf("usage : rdfan %index\n");
		return -1;
	}

	if(!get_rsa(&index, av[1])) {
		printf("rdfan : access error!\n");
		return -1;
	}

	for(i = 0; i < SMBDAT_SIZE; i++){
		wrec(REG_SMBDAT_START + i, 0x00);
	}
	printf("fan : index 0x%x \t", index);
	wrec(REG_SMBSTS, 0x00);
	wrec(REG_SMBCNT, 0x02);
	val = rdec(REG_SMBPIN);
	val = (val & 0xfc) | (1 << 1);
	wrec(REG_SMBPIN, val);
	wrec(REG_SMBADR, 0x90|0x01);
	wrec(REG_SMBCMD, index);
	wrec(REG_SMBPRTCL, 0x09);
	while(!(rdec(REG_SMBSTS) & (1 << 7)));

	val = rdec(REG_SMBDAT_START);
	printf("value 0x%x\t", val);
	val = rdec(REG_SMBDAT_START + 1);
	printf("value2 0x%x\n", val);

	return 0;
}

static inline int ec_flash_busy(void)
{
	unsigned char count = 0;

	while(count < 10){
		wrec(XBI_BANK | XBISPICMD, 5);
		while( (rdec(XBI_BANK | XBISPICFG)) & (1 << 1) );
		if( (rdec(XBI_BANK | XBISPIDAT) & (1 << 0)) == 0x00 ){
			return 0x00;
		}
		count++;
	}

	return 0x01;
}

extern void delay(int us);
extern void ec_init_idle_mode(void);
extern void ec_exit_idle_mode(void);
extern void ec_get_product_id(void);
extern unsigned char ec_rom_id[3];
static int cmd_readspiid(int ac, char *av[])
{
	u8 cmd;

	if(!get_rsa(&cmd, av[1])) {
		printf("xbi : access error!\n");
		return -1;
	}
	printf("read spi id command : 0x%x\n", cmd);


	/* goto idle mode */
	ec_init_idle_mode();

	/* get product id */
	ec_get_product_id();

	/* out of idle mode */
	ec_exit_idle_mode();
	
	printf("Manufacture ID 0x%x, Device ID : 0x%x 0x%x\n", ec_rom_id[0], ec_rom_id[1], ec_rom_id[2]);
	return 0;
}

/*
 * xbird : read an EC rom address data througth XBI interface.
 */
extern unsigned char ec_rom_id[3];
static int cmd_xbird(int ac, char *av[])
{
	u8 val;
	u32 addr;
	u8 data;
	int timeout;

	if(ac < 2){
		printf("usage : xbird start_address\n");
		return -1;
	}

	if(!get_rsa(&addr, av[1])) {
		printf("xbi : access error!\n");
		return -1;
	}

	/* enable spicmd writing. */
	val = rdec(XBI_BANK | XBISPICFG);
	wrec(XBI_BANK | XBISPICFG, val | (1 << 3) | (1 << 0));

	/* check is it busy. */
	if(ec_flash_busy()){
		printf("xbi : flash busy 1.\n");
		return -1;
	}

	/* enable write spi flash */
	wrec(XBI_BANK | XBISPICMD, 6);

	/* write the address */
	wrec(XBI_BANK | XBISPIA2, (addr & 0xff0000) >> 16);
	wrec(XBI_BANK | XBISPIA1, (addr & 0x00ff00) >> 8);
	wrec(XBI_BANK | XBISPIA0, (addr & 0x0000ff) >> 0);

	/* start action */
	switch(ec_rom_id[0]){
		case EC_ROM_PRODUCT_ID_SPANSION :
			wrec(XBI_BANK | XBISPICMD, 0x03);
			break;
		case EC_ROM_PRODUCT_ID_MXIC :
			wrec(XBI_BANK | XBISPICMD, 0x0b);
			break;
		case EC_ROM_PRODUCT_ID_AMIC :
			wrec(XBI_BANK | XBISPICMD, 0x0b);
			break;
		case EC_ROM_PRODUCT_ID_EONIC :
			wrec(XBI_BANK | XBISPICMD, 0x0b);
			break;
		default :
			printf("ec rom type not supported.\n");
			return -1;
	}

	timeout = 0x10000;
	while(timeout-- >= 0){
		if( !(rdec(XBI_BANK | XBISPICFG) & (1 << 1)) )
				break;
	}
	if(timeout <= 0){
		printf("xbi : read timeout.\n");
		return -1;
	}

	/* get data */
	data = rdec(XBI_BANK | XBISPIDAT);
	printf("ec : addr 0x%x\t, data 0x%x, %c\n", addr, data, data);

	val = rdec(XBI_BANK | XBISPICFG) & (~((1 << 3) | (1 << 0)));
	wrec(XBI_BANK | XBISPICFG, val);

	return 0;
}

/*
 * xbiwr : write a byte to EC flash.
 */
static int cmd_xbiwr(int ac, char *av[])
{
	u8 val;
	u8 cmd;
	u32 addr;
	u8 data;
	int timeout;

	if(ac < 3){
		printf("usage : xbiwr start_address val\n");
		return -1;
	}

	if(!get_rsa(&addr, av[1])) {
		printf("xbi : access error!\n");
		return -1;
	}
	if(!get_rsa(&data, av[2])) {
		printf("xbi : access error!\n");
		return -1;
	}

	/* enable spicmd writing. */
	val = rdec(XBI_BANK | XBISPICFG);
	wrec(XBI_BANK | XBISPICFG, val | (1 << 3) | (1 << 0));
	/* enable write spi flash */
	wrec(XBI_BANK | XBISPICMD, 6);
	timeout = 0x1000;
	while(timeout-- >= 0){
		if( !(rdec(XBI_BANK | XBISPICFG) & (1 << 1)) )
				break;
	}
	if(timeout <= 0){
		printf("xbi : write timeout 1.\n");
		return -1;
	}

	/* write the address and data */
	wrec(XBI_BANK | XBISPIA2, (addr & 0xff0000) >> 16);
	wrec(XBI_BANK | XBISPIA1, (addr & 0x00ff00) >> 8);
	wrec(XBI_BANK | XBISPIA0, (addr & 0x0000ff) >> 0);
	wrec(XBI_BANK | XBISPIDAT, data);
	/* start action */
	wrec(XBI_BANK | XBISPICMD, 2);
	timeout = 0x1000;
	while(timeout-- >= 0){
		if( !(rdec(XBI_BANK | XBISPICFG) & (1 << 1)) )
				break;
	}
	if(timeout <= 0){
		printf("xbi : read timeout.\n");
		return -1;
	}
	val = rdec(XBI_BANK | XBISPICFG) & (~((1 << 3) | (1 << 0)));
	wrec(XBI_BANK | XBISPICFG, val);

	return 0;
}

int cmd_testvideo(int ac, char *av[])
{
	unsigned long addr;
	if(ac < 2)
			return -1;
	addr = strtoul(av[1], NULL, 16);
	printf("addr = 0x%x", addr);
//	video_display_bitmap(addr, 280, 370);
	return 1;
}

static const Cmd Cmds[] =
{
	{"cs5536 debug"},
	{"powerdebug", "reg", NULL, "for debug the power state of sm712 graphic", cmd_powerdebug, 2, 99, CMD_REPEAT},
	{"rdmsr", "reg", NULL, "msr read test", cmd_rdmsr, 2, 99, CMD_REPEAT},
	{"wrmsr", "reg", NULL, "msr write test", cmd_wrmsr, 2, 99, CMD_REPEAT},
	
	{"rdecreg", "reg", NULL, "KB3310 EC reg read test", cmd_rdecreg, 2, 99, CMD_REPEAT},
	{"wrecreg", "reg", NULL, "KB3310 EC reg write test", cmd_wrecreg, 2, 99, CMD_REPEAT},
	{"readspiid", "reg", NULL, "KB3310 read spi device id test", cmd_readspiid, 2, 99, CMD_REPEAT},
	{"rdbat", "reg", NULL, "KB3310 smbus battery reg read test", cmd_rdbat, 2, 99, CMD_REPEAT},
	{"rdfan", "reg", NULL, "KB3310 smbus fan reg read test", cmd_rdfan, 2, 99, CMD_REPEAT},
	{"xbiwr", "reg", NULL, "for debug write data to xbi interface of ec", cmd_xbiwr, 2, 99, CMD_REPEAT},
	{"xbird", "reg", NULL, "for debug read data from xbi interface of ec", cmd_xbird, 2, 99, CMD_REPEAT},
	{"testvideo", "reg", NULL, "for debug read data from xbi interface of ec", cmd_testvideo, 2, 99, CMD_REPEAT},
	{0},
};


static void init_cmd __P((void)) __attribute__ ((constructor));

static void
init_cmd()
{
	cmdlist_expand(Cmds, 1);
}

