#ifndef __LS1X_SPI_H_
#define __LS1X_SPI_H_

#define LS1X_SPI0_BASE			0xbfe80000
#define LS1X_SPI1_BASE			0xbfec0000

#define REG_SPCR		0x00	//控制寄存器
#define REG_SPSR		0x01	//状态寄存器
#define REG_TXFIFO		0x02	//数据传输寄存器 输出
#define REG_RXFIFO		0x02	//数据传输寄存器 输入
#define REG_SPER		0x03	//外部寄存器
#define REG_PARAM		0x04	//SPI Flash参数控制寄存器
#define REG_SOFTCS		0x05	//SPI Flash片选控制寄存器
#define REG_TIMING		0x06	//SPI Flash时序控制寄存器

struct ls1x_spi {
	u32 base;
	u8 chip_select;
	unsigned long clk;
	unsigned int div;
	unsigned int speed_hz;
	unsigned int mode;
};

struct spi_device {
	struct ls1x_spi *hw;

	u32			max_speed_hz;
	u8			chip_select;
	u8			mode;
#define	SPI_CPHA	0x01			/* clock phase */
#define	SPI_CPOL	0x02			/* clock polarity */
#define	SPI_MODE_0	(0|0)			/* (original MicroWire) */
#define	SPI_MODE_1	(0|SPI_CPHA)
#define	SPI_MODE_2	(SPI_CPOL|0)
#define	SPI_MODE_3	(SPI_CPOL|SPI_CPHA)
#define	SPI_CS_HIGH	0x04			/* chipselect active high? */
#define	SPI_LSB_FIRST	0x08			/* per-word bits-on-wire */
#define	SPI_3WIRE	0x10			/* SI/SO signals shared */
#define	SPI_LOOP	0x20			/* loopback mode */
#define	SPI_NO_CS	0x40			/* 1 dev/bus, no chipselect */
#define	SPI_READY	0x80			/* slave pulls low to pause */
	u8			bits_per_word;
	int			irq;
};

void ls1x_spi_chipselect(struct spi_device *spi, int is_active);
void ls1x_spi_flash_ren(struct spi_device *spi, int en);
void ls1x_spi_writeb(struct spi_device *spi, u8 value);
u8 ls1x_spi_readb(struct spi_device *spi);
int ls1x_spi_probe(void);

#endif
