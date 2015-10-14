/* uart driver for loongson 1 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <pmon.h>
#include <machine/pio.h>

#include <include/termio.h>
#include <pmon/dev/ns16550.h>

extern int tgt_apbfreq(void);

/* memory-mapped read/write of the port */
inline u8 uart16550_read(unsigned long base, int reg)
{
	return readb(base + reg);
}

inline void uart16550_write(unsigned long base, int reg, u8 valu)
{
	writeb(valu, base + reg);
}

void ls1x_uart_init(unsigned long base, u8 data, u8 parity, u8 stop)
{
	unsigned long divisor;

	/* disable interrupts */
	uart16550_write(base, NS16550_FIFO, FIFO_ENABLE|FIFO_RCV_RST|FIFO_XMT_RST|FIFO_TRIGGER_4);

	/* set DIAB bit */
	uart16550_write(base, NS16550_CFCR, CFCR_DLAB);

	/* set up buad rate */
	divisor = tgt_apbfreq() / (16 * CONS_BAUD);
	uart16550_write(base, NS16550_DATA, divisor & 0xff);
	uart16550_write(base, NS16550_IER, (divisor & 0xff00) >> 8);

	/* clear DIAB bit */
	uart16550_write(base, NS16550_CFCR, 0x0);

	/* set data format */
	uart16550_write(base, NS16550_CFCR, data | parity | stop);
	uart16550_write(base, NS16550_MCR, MCR_DTR|MCR_RTS);

	uart16550_write(base, NS16550_IER, 0x0);
}
