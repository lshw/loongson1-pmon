#ifndef _GPIO_H_
#define _GPIO_H_

#define	EINVAL		22	/* Invalid argument */

#define STLS1X_N_GPIO		64
#define STLS1X_GPIO_IN_OFFSET	16

#define _ACAST32_
#define KSEG1			0xa0000000
#define CPHYSADDR(a)		((_ACAST32_(a)) & 0x1fffffff)
#define CKSEG1ADDR(a)		(CPHYSADDR(a) | KSEG1)

#define LOONGSON_REG(x)	\
	(*(volatile u32 *)((char *)CKSEG1ADDR(x)))

#define LOONGSON_GPIOCFG0	LOONGSON_REG(0xbfd010c0)
#define LOONGSON_GPIOCFG1	LOONGSON_REG(0xbfd010c4)
#define LOONGSON_GPIOIE0 	LOONGSON_REG(0xbfd010d0)
#define LOONGSON_GPIOIE1	LOONGSON_REG(0xbfd010d4)
#define LOONGSON_GPIOIN0	LOONGSON_REG(0xbfd010e0)
#define LOONGSON_GPIOIN1	LOONGSON_REG(0xbfd010e4)
#define LOONGSON_GPIOOUT0	LOONGSON_REG(0xbfd010f0)
#define LOONGSON_GPIOOUT1	LOONGSON_REG(0xbfd010f4)

int gpio_get_value(int gpio);
void gpio_set_value(int gpio, int state);
int gpio_cansleep(int gpio);
int ls1x_gpio_direction_input(int gpio);
int ls1x_gpio_direction_output(int gpio, int level);
int ls1x_gpio_get_value(int gpio);
void ls1x_gpio_set_value(int gpio, int value);

#endif
