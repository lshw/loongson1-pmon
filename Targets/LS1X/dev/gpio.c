/*
 *  STLS1X GPIO Support
 *
 *  
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 */
#include "include/gpio.h"

typedef unsigned long  u32;

int gpio_get_value(int gpio)
{
	u32 val;
	u32 mask;

	if (gpio >= STLS1X_N_GPIO)
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

void gpio_set_value(int gpio, int state)
{
	u32 val;
	u32 mask;

	if (gpio >= STLS1X_N_GPIO) {
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

int gpio_cansleep(int gpio)
{
	if (gpio < STLS1X_N_GPIO)
		return 0;
	else
		return -1;
}

int ls1x_gpio_direction_input(int gpio)
{
	u32 temp;
	u32 mask;

	if (gpio >= STLS1X_N_GPIO)
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

int ls1x_gpio_direction_output(int gpio, int level)
{
	u32 temp;
	u32 mask;

	if (gpio >= STLS1X_N_GPIO)
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

int ls1x_gpio_get_value(int gpio)
{
	return gpio_get_value(gpio);
}

void ls1x_gpio_set_value(int gpio, int value)
{
	gpio_set_value(gpio, value);
}

void ls1b_gpio_free(int gpio)
{
	u32 temp;
	u32 mask;

	if (gpio >= STLS1X_N_GPIO)
		return -EINVAL;

	if(gpio >= 32){
		mask = 1 << (gpio - 32);
		temp = LOONGSON_GPIOCFG1;
		temp &= ~mask;
		LOONGSON_GPIOCFG1 = temp;
	}else{
		mask = 1 << gpio;
		temp = LOONGSON_GPIOCFG0;
		temp &= ~mask;
		LOONGSON_GPIOCFG0 = temp;
	}
}
