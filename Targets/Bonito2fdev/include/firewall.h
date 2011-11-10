#ifndef FIREWALL_H
#define FIREWALL_H

#define PCI_IDSEL_ATP8260	9
#define IOG			0x4000
#define I2CREG_ADDR		(0xbfd00000|(IOG)|0xB0)
#define I2C_NACK	0x80
#define	I2C_RD		0x20
#define	I2C_WR		0x10
#define	I2C_START	0x80
#define	I2C_STOP	0x40

#define	LS2F_COMA_ADDR 	0xbe000000
#define	LS2F_COMB_ADDR 	0xbe000020
#define	LS2F_PP_ADDR	0xbe000040
#define	LS2F_COM_CLK	1843200

#endif
