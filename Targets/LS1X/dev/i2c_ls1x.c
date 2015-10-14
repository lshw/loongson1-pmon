/*
 *  Copyright (c) 2013 Tang, Haifeng <tanghaifeng-gz@loongson.cn>
 *
 *  This file is subject to the terms and conditions of the GNU General Public
 *  License. See the file COPYING in the main directory of this archive for
 *  more details.
 */

#include <stdlib.h>
#include <stdio.h>

#include <pmon.h>
#include <machine/pio.h>
#include <target/i2c-ls1x.h>

extern int tgt_apbfreq(void);

struct ls1x_i2c ls1x_i2c0;
struct ls1x_i2c *i2c = &ls1x_i2c0;

#define udelay(x) delay(x)

static inline void i2c_writeb(struct ls1x_i2c *i2c, int reg, u8 value)
{
	writeb(value, i2c->base + reg);
}

static inline u8 i2c_readb(struct ls1x_i2c *i2c, int reg)
{
	return readb(i2c->base + reg);
}

/*
 * Poll the i2c status register until the specified bit is set.
 * Returns 0 if timed out (100 msec).
 */
static short ls1x_poll_status(struct ls1x_i2c *i2c, unsigned long bit)
{
	int loop_cntr = 20000;

	do {
		udelay(1);
	} while ((i2c_readb(i2c, OCI2C_STATUS) & bit) && (--loop_cntr > 0));

	return (loop_cntr > 0);
}

static int ls1x_xfer_read(struct ls1x_i2c *i2c, unsigned char *buf, int length) 
{
	int x;

	for (x=0; x<length; x++) {
		/* send ACK last not send ACK */
		if (x != (length -1)) 
			i2c_writeb(i2c, OCI2C_CMD, OCI2C_CMD_READ_ACK);
		else
			i2c_writeb(i2c, OCI2C_CMD, OCI2C_CMD_READ_NACK);

		if (!ls1x_poll_status(i2c, OCI2C_STAT_TIP)) {
			printf("READ timeout\n");
			return -1;
		}
		*buf++ = i2c_readb(i2c, OCI2C_DATA);
	}
	i2c_writeb(i2c,OCI2C_CMD, OCI2C_CMD_STOP);
		
	return 0;
}

static int ls1x_xfer_write(struct ls1x_i2c *i2c, unsigned char *buf, int length)
{
	int x;

	for (x=0; x<length; x++) {
		i2c_writeb(i2c, OCI2C_DATA, *buf++);
		i2c_writeb(i2c, OCI2C_CMD, OCI2C_CMD_WRITE);
		if (!ls1x_poll_status(i2c, OCI2C_STAT_TIP)) {
			printf("WRITE timeout\n");
			return -1;
		}
		if (i2c_readb(i2c, OCI2C_STATUS) & OCI2C_STAT_NACK) {
			i2c_writeb(i2c, OCI2C_CMD, OCI2C_CMD_STOP);
			return length;
		}
	}
	i2c_writeb(i2c, OCI2C_CMD, OCI2C_CMD_STOP);

	return 0;
}

static int ls1x_xfer(struct ls1x_i2c *i2c, struct i2c_msg *pmsg, int num)
{
	int i, ret;

//	dev_dbg(&adap->dev, "ls1x_xfer: processing %d messages:\n", num);

	for (i = 0; i < num; i++) {
/*		dev_dbg(&adap->dev, " #%d: %sing %d byte%s %s 0x%02x\n", i,
			pmsg->flags & I2C_M_RD ? "read" : "writ",
			pmsg->len, pmsg->len > 1 ? "s" : "",
			pmsg->flags & I2C_M_RD ? "from" : "to",	pmsg->addr);*/

		if (!ls1x_poll_status(i2c, OCI2C_STAT_BUSY)) {
			return -1;
		}

		i2c_writeb(i2c, OCI2C_DATA, (pmsg->addr << 1)
			| ((pmsg->flags & I2C_M_RD) ? 1 : 0));
		i2c_writeb(i2c, OCI2C_CMD, OCI2C_CMD_START);

		/* Wait until transfer is finished */
		if (!ls1x_poll_status(i2c, OCI2C_STAT_TIP)) {
			printf("TXCOMP timeout\n");
			return -1;
		}

		if (i2c_readb(i2c, OCI2C_STATUS) & OCI2C_STAT_NACK) {
			printf("slave addr no ack !!\n");
			i2c_writeb(i2c, OCI2C_CMD, OCI2C_CMD_STOP);
			return 0;
		}

 		if (pmsg->flags & I2C_M_RD)
			ret = ls1x_xfer_read(i2c, pmsg->buf, pmsg->len);
  		else
			ret = ls1x_xfer_write(i2c, pmsg->buf, pmsg->len);

		if (ret)
			return ret;
//		printf("transfer complete\n");
		pmsg++;
	}
	return i;
}

int i2c_master_send(const struct i2c_client *client, const char *buf, int count)
{
	int ret;
	struct i2c_msg msg;

	msg.addr = client->addr;
	msg.flags = client->flags & I2C_M_TEN;
	msg.len = count;
	msg.buf = (unsigned char *)buf;

//	ret = i2c_transfer(adap, &msg, 1);
	ret = ls1x_xfer(i2c, &msg, 1);

	/* If everything went ok (i.e. 1 msg transmitted), return #bytes
	   transmitted, else error code. */
	return (ret == 1) ? count : ret;
}

int i2c_master_recv(const struct i2c_client *client, char *buf, int count)
{
	struct i2c_msg msg;
	int ret;

	msg.addr = client->addr;
	msg.flags = client->flags & I2C_M_TEN;
	msg.flags |= I2C_M_RD;
	msg.len = count;
	msg.buf = (unsigned char *)buf;

//	ret = i2c_transfer(adap, &msg, 1);
	ret = ls1x_xfer(i2c, &msg, 1);

	/* If everything went ok (i.e. 1 msg transmitted), return #bytes
	   transmitted, else error code. */
	return (ret == 1) ? count : ret;
}

static void ls1x_i2c_hwinit(struct ls1x_i2c *i2c)
{
	int prescale;
	u8 ctrl = i2c_readb(i2c, OCI2C_CONTROL);

	/* make sure the device is disabled */
	i2c_writeb(i2c, OCI2C_CONTROL, ctrl & ~(OCI2C_CTRL_EN|OCI2C_CTRL_IEN));

	prescale = tgt_apbfreq();
	prescale = (prescale / (5*I2C_CLOCK)) - 1;
	i2c_writeb(i2c, OCI2C_PRELOW, prescale & 0xff);
	i2c_writeb(i2c, OCI2C_PREHIGH, prescale >> 8);

	/* Init the device */
	i2c_writeb(i2c, OCI2C_CMD, OCI2C_CMD_IACK);
	i2c_writeb(i2c, OCI2C_CONTROL, ctrl | OCI2C_CTRL_IEN | OCI2C_CTRL_EN);
}

int ls1x_i2c_probe(void)
{
	i2c->base = LS1X_I2C0_BASE;
	ls1x_i2c_hwinit(i2c);

#ifdef CONFIG_PCF857X
	pcf857x_init();
#endif

	return 0;
}

int pcf857x_init(void)
{
	struct i2c_client pcf857x;
//	u8 buf[1] = {0xfb};	/* 设置输出默认值 */
	u8 buf[1] = {0xe9};	/* 设置输出默认值 */
	int status;

	pcf857x.addr = 0x24;
	status = i2c_master_send(&pcf857x, buf, 1);
	buf[0] = 0xf9;
	i2c_master_send(&pcf857x, buf, 1);
	return 0;
}

#if 0
int pca953x_init(void)
{
	struct i2c_client pca953x;
	u8 buf[3];
	int status;

	pca953x.addr = 0x26;

	buf[0] = 0x06; buf[1] = 0xff; buf[2] = 0xf7;	/* 设置输出使能 */
	status = i2c_master_send(&pca953x, buf, 3);
	buf[0] = 0x02; buf[1] = 0xff; buf[2] = 0xff;
	status = i2c_master_send(&pca953x, buf, 3);
	printf("pca953x init complete %d\n", status);
	return 0;
}
#endif

void pca953x_gpio_direction_output(int addr, int off)
{
	struct i2c_client pca953x;
	u8 buf[3];
	int offset = 0;

	pca953x.addr = addr;

	buf[0] = 0x06;
	i2c_master_send(&pca953x, buf, 1);
	i2c_master_recv(&pca953x, buf, 2);
	offset = (buf[1] << 8) | buf[0];
	offset &= ~(1 << off);
	buf[0] = 0x06; buf[1] = offset & 0xff; buf[2] = (offset >> 8) & 0xff;
	i2c_master_send(&pca953x, buf, 3);
}

void pca953x_gpio_set_value(int addr, int off, int val)
{
	struct i2c_client pca953x;
	u8 buf[3];
	int offset = 0;

	pca953x.addr = addr;

	buf[0] = 0x02;
	i2c_master_send(&pca953x, buf, 1);
	i2c_master_recv(&pca953x, buf, 2);
	offset = (buf[1] << 8) | buf[0];
	if (val)
		offset |= 1 << off;
	else
		offset &= ~(1 << off);
	buf[0] = 0x02; buf[1] = offset & 0xff; buf[2] = (offset >> 8) & 0xff;
	i2c_master_send(&pca953x, buf, 3);
}

void pca953x_gpio_direction_input(int addr, int off)
{
	struct i2c_client pca953x;
	u8 buf[3];
	int offset = 0;

	pca953x.addr = addr;

	buf[0] = 0x06;
	i2c_master_send(&pca953x, buf, 1);
	i2c_master_recv(&pca953x, buf, 2);
	offset = (buf[1] << 8) | buf[0];
	offset |= (1 << off);
	buf[0] = 0x06; buf[1] = offset & 0xff; buf[2] = (offset >> 8) & 0xff;
	i2c_master_send(&pca953x, buf, 3);
}

int pca953x_gpio_get_value(int addr, int off)
{
	struct i2c_client pca953x;
	u8 buf[3];
	int offset = 0;

	pca953x.addr = addr;

	buf[0] = 0x00;
	i2c_master_send(&pca953x, buf, 1);
	i2c_master_recv(&pca953x, buf, 2);
	offset = (buf[1] << 8) | buf[0];

	return ((offset >> off) & 0x0001);
}
