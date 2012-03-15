/*
 *  STLS1B GPIO Support
 *
 *  
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 */

#define	EINVAL		22	/* Invalid argument */

#define STLS1B_N_GPIO		64
#define STLS1B_GPIO_IN_OFFSET	16

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

typedef unsigned long  u32;
/*
int gpio_get_value(unsigned gpio);
void gpio_set_value(unsigned gpio, int state);
int gpio_cansleep(unsigned gpio);
int ls1b_gpio_direction_input(struct gpio_chip *chip, unsigned gpio);
int ls1b_gpio_direction_output(struct gpio_chip *chip, unsigned gpio, int level);
int ls1b_gpio_get_value(struct gpio_chip *chip, unsigned gpio);
void ls1b_gpio_set_value(struct gpio_chip *chip, unsigned gpio, int value);
*/

int gpio_get_value(unsigned gpio)
{
	u32 val;
	u32 mask;

	if (gpio >= STLS1B_N_GPIO)
		return -1;

	if(gpio >= 32){
		mask = 1 << (gpio - 32);
		val = LOONGSON_GPIOIN1;
	}else{
		mask = 1 << gpio;
		val = LOONGSON_GPIOIN0;
	}

	return ((val & mask) != 0);
}
EXPORT_SYMBOL(gpio_get_value);

void gpio_set_value(unsigned gpio, int state)
{
	u32 val;
	u32 mask;

	if (gpio >= STLS1B_N_GPIO) {
		return ;
	}

	if(gpio >= 32){
		mask = 1 << (gpio - 32);
		val = LOONGSON_GPIOOUT1;
		if (state)
			val |= mask;
		else
			val &= (~mask);
		LOONGSON_GPIOOUT1 = val;
	}else{
		mask = 1 << gpio;
		val = LOONGSON_GPIOOUT0;
		if(state)	
			val |= mask;
		else
			val &= ~mask;
		LOONGSON_GPIOOUT0 = val;
	}
}
EXPORT_SYMBOL(gpio_set_value);

int gpio_cansleep(unsigned gpio)
{
	if (gpio < STLS1B_N_GPIO)
		return 0;
	else
		return -1;
}
EXPORT_SYMBOL(gpio_cansleep);

int ls1b_gpio_direction_input(struct gpio_chip *chip, unsigned gpio)
{
	u32 temp;
	u32 mask;

	if (gpio >= STLS1B_N_GPIO)
		return -EINVAL;

	if(gpio >= 32){
		mask = 1 << (gpio - 32);
		temp = LOONGSON_GPIOCFG1;
		temp |= mask;
		LOONGSON_GPIOCFG1 = temp;
		temp = LOONGSON_GPIOIE1;
		temp |= mask;
		LOONGSON_GPIOIE1 = temp;
	}else{
		mask = 1 << gpio;
		temp = LOONGSON_GPIOCFG0;
		temp |= mask;
		LOONGSON_GPIOCFG0 = temp;
		temp = LOONGSON_GPIOIE0;
		temp |= mask;
		LOONGSON_GPIOIE0 = temp;
	}

	return 0;
}

int ls1b_gpio_direction_output(struct gpio_chip *chip,
		unsigned gpio, int level)
{
	u32 temp;
	u32 mask;

	if (gpio >= STLS1B_N_GPIO)
		return -EINVAL;

	gpio_set_value(gpio, level);
	
	if(gpio >= 32){
		mask = 1 << (gpio - 32);
		temp = LOONGSON_GPIOCFG1;
		temp |= mask;
		LOONGSON_GPIOCFG1 = temp;
		temp = LOONGSON_GPIOIE1;
		temp &= (~mask);
		LOONGSON_GPIOIE1 = temp;
	}else{
		mask = 1 << gpio;
		temp = LOONGSON_GPIOCFG0;
		temp |= mask;
		LOONGSON_GPIOCFG0 = temp;
		temp = LOONGSON_GPIOIE0;
		temp &= (~mask);
		LOONGSON_GPIOIE0 = temp;
	}

	return 0;
}

int ls1b_gpio_get_value(struct gpio_chip *chip, unsigned gpio)
{
	return gpio_get_value(gpio);
}

void ls1b_gpio_set_value(struct gpio_chip *chip,
		unsigned gpio, int value)
{
	gpio_set_value(gpio, value);
}
