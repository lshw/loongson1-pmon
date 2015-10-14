#ifndef __I2C_LS1X_H_
#define __I2C_LS1X_H_

#define LS1X_I2C0_BASE			0xbfe58000
#define LS1X_I2C1_BASE			0xbfe68000
#define LS1X_I2C2_BASE			0xbfe70000

#define I2C_CLOCK		100000		/* Hz. max 400 Kbits/sec */

/* registers */
#define OCI2C_PRELOW		0
#define OCI2C_PREHIGH		1
#define OCI2C_CONTROL		2
#define OCI2C_DATA		3
#define OCI2C_CMD		4 /* write only */
#define OCI2C_STATUS		4 /* read only, same address as OCI2C_CMD */

#define OCI2C_CTRL_IEN		0x40
#define OCI2C_CTRL_EN		0x80

#define OCI2C_CMD_START		0x90
#define OCI2C_CMD_STOP		0x40
#define OCI2C_CMD_READ		0x20
#define OCI2C_CMD_WRITE		0x10
#define OCI2C_CMD_READ_ACK	0x20
#define OCI2C_CMD_READ_NACK	0x28
#define OCI2C_CMD_IACK		0x00

#define OCI2C_STAT_IF		0x01
#define OCI2C_STAT_TIP		0x02
#define OCI2C_STAT_ARBLOST	0x20
#define OCI2C_STAT_BUSY		0x40
#define OCI2C_STAT_NACK		0x80

struct i2c_client {
	unsigned short flags;		/* div., see below		*/
	unsigned short addr;		/* chip address - NOTE: 7bit	*/
};

struct i2c_msg {
	u16 addr;	/* slave address			*/
	u16 flags;
#define I2C_M_TEN		0x0010	/* this is a ten bit chip address */
#define I2C_M_RD		0x0001	/* read data, from slave to master */
#define I2C_M_NOSTART		0x4000	/* if I2C_FUNC_PROTOCOL_MANGLING */
#define I2C_M_REV_DIR_ADDR	0x2000	/* if I2C_FUNC_PROTOCOL_MANGLING */
#define I2C_M_IGNORE_NAK	0x1000	/* if I2C_FUNC_PROTOCOL_MANGLING */
#define I2C_M_NO_RD_ACK		0x0800	/* if I2C_FUNC_PROTOCOL_MANGLING */
#define I2C_M_RECV_LEN		0x0400	/* length will be first received byte */
	u16 len;		/* msg length				*/
	u8 *buf;		/* pointer to msg data			*/
};

struct ls1x_i2c {
	u32 base;
//	struct i2c_adapter adap;
};

int i2c_master_send(const struct i2c_client *client, const char *buf, int count);
int i2c_master_recv(const struct i2c_client *client, char *buf, int count);

int ls1x_i2c_probe(void);
int pcf857x_init(void);

void pca953x_gpio_direction_output(int addr, int off);
void pca953x_gpio_set_value(int addr, int off, int val);

#endif
