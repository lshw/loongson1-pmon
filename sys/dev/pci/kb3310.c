/*
 * kb3310.c 
 * handle all the thing about the EC kb3310
 * so far, the following functions are included :
 * 1, fixup some patch for YEELOONG platform
 * 2, update EC rom(including version)
 *
 * NOTE :
 *	This device is connected to the LPC bus and then to 
 *	PCI bus through cs5536 chip
 */

#include <stdio.h>
#include <stdarg.h>
#include <sys/linux/types.h>
#include <include/bonito.h>
#include <machine/pio.h> 
#include "include/kb3310.h"

/************************************************************/
/* ec delay time 500us for register and status access */
#define	EC_REG_DELAY	500	//unit : us
/* ec rom flash id size and array : Manufacture ID[1], Device ID[2] */
#define	EC_ROM_ID_SIZE	3
unsigned char ec_rom_id[EC_ROM_ID_SIZE];
/* delay function */
extern void delay(int microseconds);
/* version array */
unsigned char ec_ver[VER_MAX_SIZE];

/* base address for io access */
#undef	MIPS_IO_BASE
#define	MIPS_IO_BASE	(0xbfd00000)

/* EC access port for sci communication */
#define	EC_CMD_PORT	0x66
#define	EC_STS_PORT	0x66
#define	CMD_INIT_IDLE_MODE	0xdd
#define	CMD_EXIT_IDLE_MODE	0xdf
#define	CMD_INIT_RESET_MODE	0xd8
#define	CMD_REBOOT_SYSTEM	0x8c

/* ec internal register */
#define	REG_POWER_MODE		0xF710
#define	FLAG_NORMAL_MODE	0x00
#define	FLAG_IDLE_MODE		0x01
#define	FLAG_RESET_MODE		0x02

/* read and write operation */
#undef	read_port
#undef	write_port
#define	read_port(x)	 (*(volatile unsigned char *)(MIPS_IO_BASE | x))
#define	write_port(x, y) (*(volatile unsigned char *)(MIPS_IO_BASE | x) = y)	

/* ec update program flag */
#define	PROGRAM_FLAG_NONE	0x00
#define	PROGRAM_FLAG_VERSION	0x01
#define	PROGRAM_FLAG_ROM	0x02

/***************************************************************/

/* enable the internal watchdog */
void ec_enable_wdt_mode(void)
{
	wrec(REG_WDTCFG, 0x83);
}

/* disable the internal watchdog */
void ec_disable_wdt_mode(void)
{
	wrec(REG_WDTPF,  0x03);
	wrec(REG_WDTCFG, 0x48);
}

/* enable the chip reset mode */
int ec_init_reset_mode(void)
{
	int timeout;
	unsigned char status = 0;
	
	/* make chip goto reset mode */
	delay(EC_REG_DELAY);
	write_port(EC_CMD_PORT, CMD_INIT_RESET_MODE);
	delay(EC_REG_DELAY);

	timeout = 0x1000;
	status = read_port(EC_STS_PORT);
	while(timeout--){
		if(status & (1 << 1)){
			status = read_port(EC_STS_PORT);
			delay(EC_REG_DELAY);
			printf("ec issued reset command status : 0x%x\n", status);
			continue;
		}
		break;
	}
	if(timeout <= 0){
		printf("ec rom fixup : enter reset mode failed.\n");
		return -1;
	}
	
	/* make the action take active */
	timeout = 1000;
	status = rdec(REG_POWER_MODE) & FLAG_RESET_MODE;
	while(timeout--){
		if(status){
			delay(EC_REG_DELAY);
			break;
		}
		status = rdec(REG_POWER_MODE) & FLAG_RESET_MODE;
		delay(EC_REG_DELAY);
		printf("reset 0xf710 :  0x%x\n", rdec(REG_POWER_MODE));
	}
	if(timeout <= 0){
		printf("ec rom fixup : can't check reset status.\n");
		return -1;
	}
	
	/* set MCU to reset mode */
	delay(EC_REG_DELAY);
	status = rdec(REG_PXCFG);
	status |= (1 << 0);
	wrec(REG_PXCFG, status);
	delay(EC_REG_DELAY);

	/* disable FWH/LPC */
	delay(EC_REG_DELAY);
	status = rdec(REG_LPCCFG);
	status &= ~(1 << 7);
	wrec(REG_LPCCFG, status);
	delay(EC_REG_DELAY);

	printf("entering reset mode ok................\n");

	return 0;
}

/* make ec exit from reset mode */
void ec_exit_reset_mode(void)
{
	u8 regval;

	delay(EC_REG_DELAY);
	regval = rdec(0xfe95);
	regval |= (1 << 7);
	wrec(0xfe95, regval);
	delay(EC_REG_DELAY);
	regval = rdec(REG_PXCFG);
	regval &= ~(1 << 0);
	wrec(REG_PXCFG, regval);
	delay(EC_REG_DELAY);

	return;
}

/* make ec goto idle mode */
void ec_init_idle_mode(void)
{
	int timeout;
	unsigned char status = 0;
	
	/* make chip goto idle mode */
	delay(EC_REG_DELAY);
	write_port(EC_CMD_PORT, CMD_INIT_IDLE_MODE);
	delay(EC_REG_DELAY);

	timeout = 0x1000;
	status = read_port(EC_STS_PORT);
	delay(EC_REG_DELAY);
	while(timeout--){
		if(status & (1 << 1)){
			status = read_port(EC_STS_PORT);
			delay(EC_REG_DELAY);
			printf("ec issued init command status : 0x%x\n", status);
			continue;
		}
		break;
	}
	if(timeout <= 0){
		printf("ec rom fixup : enter idle mode failed.\n");
		return;
	}
	
	/* make the action take active */
	timeout = 1000;
	status = rdec(REG_POWER_MODE) & FLAG_IDLE_MODE;
	while(timeout--){
		if(status){
			delay(EC_REG_DELAY);
			break;
		}
		status = rdec(REG_POWER_MODE) & FLAG_IDLE_MODE;
		delay(EC_REG_DELAY);
		printf("0xf710 :  0x%x\n", rdec(0xF710));
	}
	if(timeout <= 0){
		printf("ec rom fixup : can't check out the status.\n");
		return;
	}

	return;
}

/* make ec exit from idle mode */
void ec_exit_idle_mode(void)
{
	int timeout;
	u8 status;
	
	/* make chip exit idle mode */
	delay(EC_REG_DELAY);
	write_port(EC_CMD_PORT, CMD_EXIT_IDLE_MODE);
	delay(EC_REG_DELAY);

	timeout = 0x1000;
	status = read_port(EC_STS_PORT);
	while(timeout--){
		if(status & (1 << 1)){
			status = read_port(EC_STS_PORT);
			delay(EC_REG_DELAY);
			continue;
		}
		break;
	}
	if(timeout <= 0){
		printf("ec rom fixup : exit idle mode failed.\n");
		printf(" status 0x%x\n", rdec(REG_POWER_MODE));
		return;
	}
	
	return;
}

/* reboot system for syncing including EC rom self */
void ec_shutdown_system(void)
{
	int timeout;
	u8 status;
	
	/* make chip exit idle mode */
	delay(EC_REG_DELAY * 50);
	write_port(EC_CMD_PORT, CMD_REBOOT_SYSTEM);
	delay(EC_REG_DELAY);

	timeout = 0x1000;
	status = read_port(EC_STS_PORT);
	printf("ec will shutdown the whole system.\n");
	while(timeout--){
		if(status & (1 << 1)){
			status = read_port(EC_STS_PORT);
			delay(EC_REG_DELAY);
			continue;
		}
		break;
	}
	if(timeout <= 0){
		printf("ec shutdown system failed.\n");
		return;
	}
	
	return;
}

/* get flash rom product id number */
void ec_get_product_id(void)
{
	u8 regval;
	int i;
	
	/* get product id from ec rom */
	delay(EC_REG_DELAY);
	regval = rdec(XBI_BANK | XBISPICFG);
	regval |= 0x18;
	wrec(XBI_BANK | XBISPICFG, regval);
	delay(EC_REG_DELAY);

	wrec(XBI_BANK | XBISPICMD, 0x9f);
	while( (rdec(XBI_BANK | XBISPICFG)) & (1 << 1) );

	for(i = 0; i < EC_ROM_ID_SIZE; i++){
		wrec(XBI_BANK | XBISPICMD, 0x00);
		while( (rdec(XBI_BANK | XBISPICFG)) & (1 << 1) );
		ec_rom_id[i] = rdec(XBI_BANK | XBISPIDAT);
	}
	
	delay(EC_REG_DELAY);
	regval = rdec(XBI_BANK | XBISPICFG);
	regval &= 0xE7;
	wrec(XBI_BANK | XBISPICFG, regval);
	delay(EC_REG_DELAY);

	return;
}

/* check if flash busy or not */
int ec_flash_busy(void)
{
	unsigned char count = 0;

	while(count < 10){
		wrec(XBI_BANK | XBISPICMD, 5);
		while( (rdec(XBI_BANK | XBISPICFG)) & (1 << 1) );
		if((rdec(XBI_BANK | XBISPIDAT) & 0x01) == 0x00){
			return 0x00;
		}
		count++;
	}

	return 0x01;
}

/* erase the whole flash chip */
int ec_flash_erase(void)
{
	int timeout = 0x10000;
	unsigned char val;

	/* enable spicmd writing. */
	val = rdec(XBI_BANK | XBISPICFG);
	wrec(XBI_BANK | XBISPICFG, val | (1 << 3) | (1 << 0));

	/* check is it busy. */
	if(ec_flash_busy()){
			printf("xbi : flash busy 2.\n");
			return -1;
	}

	/* unprotect the status register */
	wrec(XBI_BANK | XBISPIDAT, 2);
	/* write the status register */
	wrec(XBI_BANK | XBISPICMD, 1);
	timeout = 0x1000;
	while(timeout-- >= 0){
		if( !(rdec(XBI_BANK | XBISPICFG) & (1 << 1)) )
				break;
	}
	if(timeout <= 0){
		printf("xbi : write timeout 3.\n");
		return -1;
	}

	/* enable write spi flash */
	wrec(XBI_BANK | XBISPICMD, 0x06);
	timeout = 0x1000;
	while(timeout-- >= 0){
		if( !(rdec(XBI_BANK | XBISPICFG) & (1 << 1)) )
				break;
	}
	if(timeout <= 0){
		printf("xbi : write timeout 4.\n");
		return -1;
	}

	/* erase the whole chip first */
	wrec(XBI_BANK | XBISPICMD, 0xC7);
	timeout = 0x10000000;
	while(timeout-- >= 0){
		if( !(rdec(XBI_BANK | XBISPICFG) & (1 << 1)) )
				break;
	}
	if(timeout <= 0){
		printf("xbi : write timeout 5.\n");
		return -1;
	}
	/* disable spicmd writing. */
	val = rdec(XBI_BANK | XBISPICFG) & (~((1 << 3) | (1 << 0)));
	wrec(XBI_BANK | XBISPICFG, val);

	return 0;
}

/* programing one byte to ec rom */
int ec_program_byte(unsigned long addr, unsigned char byte)
{
	int timeout = 0x10000;
	unsigned char val;

	/* enable spicmd writing. */
	val = rdec(XBI_BANK | XBISPICFG);
	wrec(XBI_BANK | XBISPICFG, val | (1 << 3) | (1 << 0));

	/* check is it busy. */
	if(ec_flash_busy()){
			printf("xbi : flash busy 1.\n");
			return 0x00;
	}

	/* enable write spi flash */
	wrec(XBI_BANK | XBISPICMD, 0x06);
	timeout = 0x1000;
	while(timeout-- >= 0){
		if( !(rdec(XBI_BANK | XBISPICFG) & (1 << 1)) )
				break;
	}
	if(timeout <= 0){
		printf("xbi : write timeout 1.\n");
		return -1;
	}
	if(ec_flash_busy()){
			printf("xbi : flash busy 1.\n");
			return 0x00;
	}

	/* write the address */
	wrec(XBI_BANK | XBISPIA2, (unsigned char)((addr & 0xff0000) >> 16));
	wrec(XBI_BANK | XBISPIA1, (unsigned char)((addr & 0x00ff00) >> 8));
	wrec(XBI_BANK | XBISPIA0, (unsigned char)((addr & 0x0000ff) >> 0));
	wrec(XBI_BANK | XBISPIDAT, byte);
	/* start action */
	wrec(XBI_BANK | XBISPICMD, 2);
	timeout = 0x1000;
	while(timeout-- >= 0){
		if( !(rdec(XBI_BANK | XBISPICFG) & (1 << 1)) )
				break;
	}
	if(timeout <= 0){
		printf("xbi : write timeout 2.\n");
		return -1;
	}
	if(ec_flash_busy()){
			printf("xbi : flash busy 1.\n");
			return 0x00;
	}

	/* disable spicmd writing. */
	val = rdec(XBI_BANK | XBISPICFG) & (~((1 << 3) | (1 << 0)));
	wrec(XBI_BANK | XBISPICFG, val);

	if(ec_flash_busy()){
			printf("xbi : flash busy 1.\n");
			return 0x00;
	}

	return 0;
}

/* program data to ec flash rom */
int ec_program_data(unsigned char *buf, unsigned long len, int flag)
{
	unsigned char val;
	int i;
	int ret = 0;
	unsigned long addr = 0;
	unsigned char *ptr = NULL;
	unsigned long size = 0;
	
	ptr = (unsigned char *)buf;
	size = len;
	if(flag == PROGRAM_FLAG_NONE){
		return -1;
	}
	else if(flag == PROGRAM_FLAG_VERSION){
		if(ptr[0] == 'D'){
			return 0;
		}
		addr = 0xf300;
		ptr[size - 1] = '\0';
		printf("starting programming ec version : %s\n", ptr);
	}else if(flag == PROGRAM_FLAG_ROM){
		addr = 0x00;
		printf("starting programming ec rom.\n");
	}

	/* program data */
	for(i = 0; i < size; i++){
		ec_program_byte(addr + i, ptr[i]);
		val = rdec(addr + i);
		if(val != ptr[i]){
			//we make the flash data equal to the memory data.
			ec_program_byte(addr + i, ptr[i]);
			val = rdec(addr + i);
			if(val != ptr[i]){
				printf("Second flash program failed at:\t");
				printf("addr : 0x%x, memory : 0x%x, flash : 0x%x\n", addr + i, ptr[i], val);
				ret = -1;
				break;
			}/* if '2' time */
		}/* if '1' time */
		if( (i % 0x400) == 0x00 ){
			printf(".");
		}
	}

	if(flag == PROGRAM_FLAG_VERSION){
		if(ret){
			printf("programming ec version error.\n");
		}else{		
			printf("programming ec version ok.\n");
		}
	}else if(flag == PROGRAM_FLAG_ROM){
		if(ret){
			printf("\nprogramming ec rom error.\n");
		}else{		
			printf("\nprogramming ec rom ok.\n");
		}
	}

	return ret;
}

/***************************************************************/

/* fixup ec fan bug */
void ec_fan_fixup(void)
{
	int i;
	unsigned char val;
	unsigned char reg_config;
	
	/* read the fan device config */
	for(i = 0; i < SMBDAT_SIZE; i++){
		wrec(REG_SMBDAT_START + i, 0x00);
	}
	wrec(REG_SMBSTS, 0x00);
	wrec(REG_SMBCNT, 0x01);
	val = rdec(REG_SMBPIN);
	val = (val & 0xfc) | (1 << 1);
	wrec(REG_SMBPIN, val);
	wrec(REG_SMBADR, 0x90|0x01);
	wrec(REG_SMBCMD, 0x01);
	wrec(REG_SMBPRTCL, 0x09);
	while(!(rdec(REG_SMBSTS) & (1 << 7)));
	reg_config = rdec(REG_SMBDAT_START);
					 
	/* enable the fan device */
	for(i = 0; i < SMBDAT_SIZE; i++){
		wrec(REG_SMBDAT_START + i, 0x00);
	}
	wrec(REG_SMBSTS, 0x00);
	wrec(REG_SMBCNT, 0x01);
	val = rdec(REG_SMBPIN);
	val = (val & 0xfc) | (1 << 1);
	wrec(REG_SMBPIN, val);
	wrec(REG_SMBADR, 0x90);
	wrec(REG_SMBCMD, 1);
	wrec(REG_SMBDAT_START, reg_config | (1 << 2));
	wrec(REG_SMBPRTCL, 0x06);

	/* enable fan function, corresponding gpio and read status  */
	val = rdec(0xfc02);
	wrec(0xfc02, val & ~(1 << 4));
		 
	val = rdec(0xfc62);
	wrec(0xfc62, val | (1 << 4));
						 
	val = rdec(0xfe20);
	wrec(0xfe20, val | (1 << 7) | (1 << 0));

	return;
}

/* get ec rom type */
void ec_get_rom_type(void)
{
	/* make chip goto idle mode */
	ec_init_idle_mode();

	/* get product id from ec rom */
	ec_get_product_id();

	/* make chip exit idle mode */
	ec_exit_idle_mode();
	
	printf("ec rom id : PRODUCT ID : 0x%2x, FIRST DEVICE ID : 0x%2x, SECOND DEVICE ID : 0x%2x\n", ec_rom_id[0], ec_rom_id[1], ec_rom_id[2]);
	
	return;
}

/***************************************************************/

/* update ec rom */
void ec_update_rom(void *src, int size)
{
	unsigned char *buf;
	unsigned char val;
	int ret = 0;
	
	buf = src;
	if(size > EC_ROM_MAX_SIZE){
		printf("ec-update : out of range.\n");
		return;
	}
	
	/* goto reset mode */
	ret = ec_init_reset_mode();
	if(ret < 0){
		printf("ec-update : init reset mode failed.\n");
		return;
	}

	/* erase stage */
	printf("erasing start erasing the whole chip.\n");
	ret = ec_flash_erase();
	if(ret < 0){
		printf("erase chip failed for first time.\n");
		val = ec_flash_erase();
		if(val){
			printf("erase chip failed for second time.\n");
			return;
		}
	}
	printf("erasing the whole chip ok.\n");
	delay(1000000);

	/* program rom stage */
	ret = ec_program_data(buf, size, PROGRAM_FLAG_ROM);
	if(ret < 0){
		return;
	}
	
	/* program version stage */
	ret = ec_program_data(ec_ver, VER_MAX_SIZE, PROGRAM_FLAG_VERSION);
	if(ret < 0){
		return;
	}

	/* exit reset mode */
	ec_exit_reset_mode();

	/* reboot bios and EC rom for syncing */
	ec_shutdown_system();
	
	return;
}

/* ec fixup routine */
void ec_fixup(void)
{
	ec_fan_fixup();
	printf("ec fan fixup ok.\n");
	ec_get_rom_type();
	printf("ec rom fixup ok.\n");
}

/* get EC version from EC rom */
unsigned char *get_ecver(void){
	static unsigned char val[VER_MAX_SIZE] = {0};
	int i;
	unsigned char *p;
	unsigned int addr = VER_ADDR;
	for(i = 0; i < VER_MAX_SIZE && rdec(addr) != '\0'; i++){
		val[i] = rdec(addr);
		addr++;
	}
	p = val;
	//if((strncmp(p, "LM8089", 6)) != 0){
	if (*p == 0){
		p = "undefined";
	}
	return p;
}
