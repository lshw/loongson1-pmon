/*
 *  Loongson1 gpio support
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <pmon.h>

#include <machine/pio.h>
#include <target/gpio.h>

int gpio_get_value(int gpio)
{
	int offset;
	u32 reg_in;

	switch (gpio/32) {
	case 0:
		offset = gpio;
		reg_in = LS1X_GPIO_IN0;
		break;
	case 1:
		offset = gpio - 32;
		reg_in = LS1X_GPIO_IN1;
		break;
#if defined(LS1ASOC) || defined(LS1CSOC)
	case 2:
		offset = gpio - 64;
		reg_in = LS1X_GPIO_IN2;
		break;
#endif
#if defined(LS1CSOC)
	case 3:
		offset = gpio - 96;
		reg_in = LS1X_GPIO_IN3;
		break;
#endif
	default:
		return -1;
	}

	return (__raw_readl(reg_in) >> offset) & 1;
}

void gpio_set_value(int gpio, int value)
{
	int offset;
	u32 reg_out, val, mask;

	switch (gpio/32) {
	case 0:
		offset = gpio;
		reg_out = LS1X_GPIO_OUT0;
		break;
	case 1:
		offset = gpio - 32;
		reg_out = LS1X_GPIO_OUT1;
		break;
#if defined(LS1ASOC) || defined(LS1CSOC)
	case 2:
		offset = gpio - 64;
		reg_out = LS1X_GPIO_OUT2;
		break;
#endif
#if defined(LS1CSOC)
	case 3:
		offset = gpio - 96;
		reg_out = LS1X_GPIO_OUT3;
		break;
#endif
	default:
		return;
	}

	mask = 1 << offset;
	val = __raw_readl(reg_out);
	if (value)
		val |= mask;
	else
		val &= ~mask;
	__raw_writel(val, reg_out);
}

int ls1x_gpio_direction_input(int gpio)
{
	int offset;
	u32 reg_cfg, reg_oe;
	u32 temp, mask;

	switch (gpio/32) {
	case 0:
		offset = gpio;
		reg_cfg = LS1X_GPIO_CFG0;
		reg_oe = LS1X_GPIO_OE0;
		break;
	case 1:
		offset = gpio - 32;
		reg_cfg = LS1X_GPIO_CFG1;
		reg_oe = LS1X_GPIO_OE1;
		break;
#if defined(LS1ASOC) || defined(LS1CSOC)
	case 2:
		offset = gpio - 64;
		reg_cfg = LS1X_GPIO_CFG2;
		reg_oe = LS1X_GPIO_OE2;
		break;
#endif
#if defined(LS1CSOC)
	case 3:
		offset = gpio - 96;
		reg_cfg = LS1X_GPIO_CFG3;
		reg_oe = LS1X_GPIO_OE3;
		break;
#endif
	default:
		return 0;
	}

	mask = 1 << offset;
	temp = __raw_readl(reg_cfg);
	temp |= mask;
	__raw_writel(temp, reg_cfg);
	temp = __raw_readl(reg_oe);
	temp |= mask;
	__raw_writel(temp, reg_oe);

	return 0;
}

int ls1x_gpio_direction_output(int gpio, int value)
{
	int offset;
	u32 reg_cfg, reg_oe;
	u32 temp, mask;

	gpio_set_value(gpio, value);

	switch (gpio/32) {
	case 0:
		offset = gpio;
		reg_cfg = LS1X_GPIO_CFG0;
		reg_oe = LS1X_GPIO_OE0;
		break;
	case 1:
		offset = gpio - 32;
		reg_cfg = LS1X_GPIO_CFG1;
		reg_oe = LS1X_GPIO_OE1;
		break;
#if defined(LS1ASOC) || defined(LS1CSOC)
	case 2:
		offset = gpio - 64;
		reg_cfg = LS1X_GPIO_CFG2;
		reg_oe = LS1X_GPIO_OE2;
		break;
#endif
#if defined(LS1CSOC)
	case 3:
		offset = gpio - 96;
		reg_cfg = LS1X_GPIO_CFG3;
		reg_oe = LS1X_GPIO_OE3;
		break;
#endif
	default:
		return 0;
	}

	mask = 1 << offset;
	temp = __raw_readl(reg_cfg);
	temp |= mask;
	__raw_writel(temp, reg_cfg);
	temp = __raw_readl(reg_oe);
	temp &= (~mask);
	__raw_writel(temp, reg_oe);

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

void ls1x_gpio_free(int gpio)
{
	int offset;
	u32 reg_cfg, temp;

	switch (gpio/32) {
	case 0:
		offset = gpio;
		reg_cfg = LS1X_GPIO_CFG0;
		break;
	case 1:
		offset = gpio - 32;
		reg_cfg = LS1X_GPIO_CFG1;
		break;
#if defined(LS1ASOC) || defined(LS1CSOC)
	case 2:
		offset = gpio - 64;
		reg_cfg = LS1X_GPIO_CFG2;
		break;
#endif
#if defined(LS1CSOC)
	case 3:
		offset = gpio - 96;
		reg_cfg = LS1X_GPIO_CFG3;
		break;
#endif
	default:
		return;
	}

	temp = __raw_readl(reg_cfg);
	temp &= ~(1 << offset);
	__raw_writel(temp, reg_cfg);
}
