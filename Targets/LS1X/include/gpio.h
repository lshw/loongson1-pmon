#ifndef _GPIO_H_
#define _GPIO_H_

/* GPIO 0-31 group 0 */
#define LS1X_GPIO_CFG0		0xbfd010c0	/* 配置寄存器 */
#define LS1X_GPIO_OE0		0xbfd010d0	/* 输入使能寄存器 */
#define LS1X_GPIO_IN0		0xbfd010e0	/* 输入寄存器 */
#define LS1X_GPIO_OUT0		0xbfd010f0	/* 输出寄存器 */

/* GPIO 32-63 group 1 */
#define LS1X_GPIO_CFG1		0xbfd010c4
#define LS1X_GPIO_OE1		0xbfd010d4
#define LS1X_GPIO_IN1		0xbfd010e4
#define LS1X_GPIO_OUT1		0xbfd010f4

#ifdef LS1ASOC
/* GPIO 64-87 group 2 */
#define LS1X_GPIO_CFG2		0xbfd010c8
#define LS1X_GPIO_OE2		0xbfd010d8
#define LS1X_GPIO_IN2		0xbfd010e8
#define LS1X_GPIO_OUT2		0xbfd010f8
#endif

#ifdef LS1CSOC
/* GPIO 64-95 group 2 */
#define LS1X_GPIO_CFG2		0xbfd010c8
#define LS1X_GPIO_OE2		0xbfd010d8
#define LS1X_GPIO_IN2		0xbfd010e8
#define LS1X_GPIO_OUT2		0xbfd010f8
/* GPIO 96-127 group 3 */
#define LS1X_GPIO_CFG3		0xbfd010cc
#define LS1X_GPIO_OE3		0xbfd010dc
#define LS1X_GPIO_IN3		0xbfd010ec
#define LS1X_GPIO_OUT3		0xbfd010fc
#endif

int gpio_get_value(int gpio);
void gpio_set_value(int gpio, int value);
int ls1x_gpio_direction_input(int gpio);
int ls1x_gpio_direction_output(int gpio, int value);
int ls1x_gpio_get_value(int gpio);
void ls1x_gpio_set_value(int gpio, int value);
void ls1x_gpio_free(int gpio);

#endif
