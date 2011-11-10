/* $Id: termio.h,v 1.1.1.1 2006/09/14 01:59:06 root Exp $ */
#ifndef _TERMIO_
#define _TERMIO_

#include "mod_usb_kbd.h"

/* operation codes for ioctl */
#define TCGETA		1
#define TCSETAF		2
#define TCSETAW		3
#define SETINTR		4
#define SETSANE		5
#define GETINTR		7
#define GETTERM		8
#define SETTERM		9
#define SETNCNE		10
#define CBREAK		11
#define TERMTYPE	12

/* iflags */
#define ISTRIP 0x0020
#define ICRNL  0x0040
#define IXON   0x0400
#define IXANY  0x0800
#define IXOFF  0x1000

/* oflags */
#define ONLCR  0x0004

/* lflags */
#define ISIG   0x0001
#define ICANON 0x0002
#define ECHO   0x0008
#define ECHOE  0x0010

/* cflags */
#define	B0	0
#define	B50	50	
#define	B75	75
#define	B110	110
#define	B134	134
#define	B150	150
#define	B200	200
#define	B300	300
#define	B600	600
#define	B1200	1200
#define	B1800	1800
#define	B2400	2400
#define	B4800	4800
#define	B9600	9600
#define	B19200	19200
#define	B38400	38400
#define	B57600	57600
#define	B115200	115200

/* cc definitions */
#define VINTR	0
#define VERASE	2
#define VEOL	5
#define VEOL2	6
#define V_START	8
#define V_STOP	9

/* operation codes for device specific driver */
#define OP_INIT		1
#define OP_TX		2
#define OP_RX		3
#define OP_RXRDY	4
#define OP_TXRDY 	5
#define OP_BAUD		6
#define OP_RXSTOP	7
#define OP_FLUSH	8
#define OP_RESET	9
#define OP_OPEN		10
#define OP_CLOSE	11
#define OP_XBAUD	12

extern int vga_available;
extern int kbd_available;
extern int usb_kbd_available;

#define DEV_MAX		8
#define STDIN		((kbd_available|usb_kbd_available)?3:0)
#define STDOUT		(vga_available?4:1)
#define STDERR		(vga_available?4:2)

/* operation codes for ttctl */
#define TT_CM		1	/* cursor movement */
#define TT_CLR		2	/* clear screen */
#define TT_CUROFF	3	/* switch cursor off */
#define TT_CURON 	4	/* switch cursor on */

#define CNTRL(x) (x & 0x1f)

#define NCC 23
struct termio {
	unsigned short c_iflag;
	unsigned short c_oflag;
	unsigned short c_cflag;
	unsigned short c_lflag;
	unsigned char c_cc[NCC];
	int	c_ispeed;
	int	c_ospeed;
};

#include "stdio.h"

struct DevEntry;

typedef struct ConfigEntry {
	char *devinfo;
	int chan;
	int(*handler) (int, struct DevEntry *, unsigned long, int);
	int rxqsize;
	int brate;
	unsigned long freq;
	unsigned int flag;
} ConfigEntry;

#include "queue.h"


typedef struct DevEntry {
	int txoff;
	int qsize;
	Queue *rxq;
	Queue *txq;
	char *sio;
	int chan;
	int rxoff;
	int(*handler) (int, struct DevEntry *, unsigned long, int);
	struct jmp_buf *intr;
	char *tname;
	int(*tfunc) (int, int, int, int);
	struct termio t;
	unsigned long freq;
	int nopen;
} DevEntry;

extern DevEntry DevTable[DEV_MAX];
extern int *curlst;

void	reschedule __P((char *)); 
void	scandevs __P((void));
int	devinit __P((void));

int	tvi920 __P((int, int, int, int));
int	vt100 __P((int, int, int, int));

extern FILE *logfp;
#endif /* _TERMIO_ */

