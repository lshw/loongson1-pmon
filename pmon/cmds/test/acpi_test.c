#include <stdio.h>
//#include <termio.h>
#include <string.h>
#include <setjmp.h>
#include <sys/endian.h>
#include <ctype.h>
//#include <unistd.h>
//#include <stdlib.h>
//#include <fcntl.h>
//#ifdef _KERNEL
//#undef _KERNEL
//#include <sys/ioctl.h>
//#define _KERNEL
//#else
//#include <sys/ioctl.h>
//#endif
#include <machine/cpu.h>
#include <pmon.h>
//#include <stdio.h>

#define LOONGSON_REG(x)	\
	*(volatile unsigned int *)((x))

#define LOONGSON_GPIOCFG0	LOONGSON_REG(0xbfd010c0)
#define LOONGSON_GPIOCFG1	LOONGSON_REG(0xbfd010c4)
#define LOONGSON_GPIOCFG2	LOONGSON_REG(0xbfd010c8)
#define LOONGSON_GPIOIE0 	LOONGSON_REG(0xbfd010d0)
#define LOONGSON_GPIOIE1	LOONGSON_REG(0xbfd010d4)
#define LOONGSON_GPIOIE2	LOONGSON_REG(0xbfd010d8)
#define LOONGSON_GPIOIN0	LOONGSON_REG(0xbfd010e0)
#define LOONGSON_GPIOIN1	LOONGSON_REG(0xbfd010e4)
#define LOONGSON_GPIOIN2	LOONGSON_REG(0xbfd010e8)
#define LOONGSON_GPIOOUT0	LOONGSON_REG(0xbfd010f0)
#define LOONGSON_GPIOOUT1	LOONGSON_REG(0xbfd010f4)
#define LOONGSON_GPIOOUT2	LOONGSON_REG(0xbfd010f8)

static void ls1a_save_gpio_val(unsigned int *ptr)
{
	*ptr++ = LOONGSON_GPIOCFG0;
	*ptr++ = LOONGSON_GPIOCFG1;
	*ptr++ = LOONGSON_GPIOCFG2;

	*ptr++ = LOONGSON_GPIOIE0;
	*ptr++ = LOONGSON_GPIOIE1;
	*ptr++ = LOONGSON_GPIOIE2;
	
	*ptr++ = LOONGSON_GPIOIN0;
	*ptr++ = LOONGSON_GPIOIN1;
	*ptr++ = LOONGSON_GPIOIN2;

	*ptr++ = LOONGSON_GPIOOUT0;
	*ptr++ = LOONGSON_GPIOOUT1;
	*ptr++ = LOONGSON_GPIOOUT2;

	*ptr = *(volatile unsigned int *)0xbfd00420;
	
//	printf ("gpio_cfg0 = %x !\n",LOONGSON_GPIOCFG0);
//	printf ("gpio_cfg0 = %x !\n",LOONGSON_GPIOCFG1);
//	printf ("gpio_cfg0 = %x !\n",LOONGSON_GPIOCFG2);
}

static void ls1a_restore_gpio_val(unsigned int *ptr)
{
	*(volatile int *)0xbfd00420 &= ~0x200000;/* enable USB */
	*(volatile int *)0xbff10204 = 0;
	delay(1005);
	*(volatile int *)0xbff10204 |= 0x40000000;/* ls1a usb reset stop */

	LOONGSON_GPIOCFG0 = *ptr++;
	LOONGSON_GPIOCFG1 = *ptr++; 
	LOONGSON_GPIOCFG2 = *ptr++; 
                                 
	LOONGSON_GPIOIE0 = *ptr++; 
	LOONGSON_GPIOIE1 = *ptr++; 
	LOONGSON_GPIOIE2 = *ptr++; 
	                  
	LOONGSON_GPIOIN0 = *ptr++; 
	LOONGSON_GPIOIN1 = *ptr++; 
	LOONGSON_GPIOIN2 = *ptr++; 
                                 
	LOONGSON_GPIOOUT0 = *ptr++; 
	LOONGSON_GPIOOUT1 = *ptr++; 
	LOONGSON_GPIOOUT2 = *ptr++; 

	*(volatile unsigned int *)0xbfd00420 = *ptr;
}

static void acpi_test(int ac, char **av)
{
	unsigned int i;
	unsigned int k;

	unsigned int sleep_gpio_save[13];
	ls1a_save_gpio_val(sleep_gpio_save);

	suspend_save();

	i = strtoul(av[1], 0, 0);
//	printf ("you set %d for test !\n", i);
	if (i & 1)
		*(volatile unsigned int *)0xbfe7c004 = 1 << 8;	//power button
	k = *(volatile unsigned int *)0xbfe7c024;
	k &= ~((1 << 8) | (1 << 9));
	*(volatile unsigned int *)0xbfe7c024 = (i & 0x3) << 8;	//[8:9]RI_EN、PME_EN
//	*(volatile unsigned int *)0xbfe7c008 = (1 << 13) | (5 << 10);	//sleep to ram
	flushcache();

	__asm__ volatile (
			"la	$2, 2f\n\t"
			"li	$3, 0xa01ffc00\n\t"
			"sw	$2, 0x0($3)\n\t"		//save return address
			"li	$2, 0xaffffe34\n\t"
			"lw	$3, 0x0($2)\n\t"
			"or	$3, 0x1\n\t"
			"sw	$3, 0x0($2)\n\t"	//enable ddr autorefresh
			"li	$2, 0xbfe7c008\n\t"
			"li	$3, (1<<13) | (5<<10)\n\t"
			"sw	$3, 0x0($2)\n\t"		//go to sleep
			"1:\n\t"
			"bal 1b\n\t"
			"nop\n\t"
			"2:\n\t"
			::
			: "$2","$3","memory"
//			"move %0,$2\n\t"
//			: "=r" (p)
//			: "0" (p), "r" (len), "r" (1)
//			: "$2","$3","$4","$5"
	);

	ls1a_restore_gpio_val(sleep_gpio_save);
	dc_init();
	outstring("acpi resume back here !\n");
//	printf ("acpi resume back here !\n");

}


static const Cmd Cmds[] =
{
	{"MyCmds"},
	{"acpi_test","val",0,"cpu sleep test: 1|power_btn, 2|RI_EN, 4|PME_EN.",acpi_test,0,99,CMD_REPEAT},
	{0,0}
};

static void init_cmd __P((void)) __attribute__ ((constructor));
static void init_cmd()
{
	cmdlist_expand(Cmds, 1);
}
