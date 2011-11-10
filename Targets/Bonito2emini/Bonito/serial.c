#define         UART16550_BAUD_2400             2400
#define         UART16550_BAUD_4800             4800
#define         UART16550_BAUD_9600             9600
#define         UART16550_BAUD_19200            19200
#define         UART16550_BAUD_38400            38400
#define         UART16550_BAUD_57600            57600
#define         UART16550_BAUD_115200           115200

#define         UART16550_PARITY_NONE           0
#define         UART16550_PARITY_ODD            0x08
#define         UART16550_PARITY_EVEN           0x18
#define         UART16550_PARITY_MARK           0x28
#define         UART16550_PARITY_SPACE          0x38

#define         UART16550_DATA_5BIT             0x0
#define         UART16550_DATA_6BIT             0x1
#define         UART16550_DATA_7BIT             0x2
#define         UART16550_DATA_8BIT             0x3

#define         UART16550_STOP_1BIT             0x0
#define         UART16550_STOP_2BIT             0x4


/* === CONFIG === */
#ifdef CONFIG_HAVE_NB_SERIAL
#define         MAX_BAUD    ( 3686400 / 16 )
#else
#define         MAX_BAUD    ( 1843200 / 16 )
#endif


/* === END OF CONFIG === */

#define         REG_OFFSET              1

/* register offset */
#define         OFS_RCV_BUFFER          0
#define         OFS_TRANS_HOLD          0
#define         OFS_SEND_BUFFER         0
#define         OFS_INTR_ENABLE         (1*REG_OFFSET)
#define         OFS_INTR_ID             (2*REG_OFFSET)
#define         OFS_DATA_FORMAT         (3*REG_OFFSET)
#define         OFS_LINE_CONTROL        (3*REG_OFFSET)
#define         OFS_MODEM_CONTROL       (4*REG_OFFSET)
#define         OFS_RS232_OUTPUT        (4*REG_OFFSET)
#define         OFS_LINE_STATUS         (5*REG_OFFSET)
#define         OFS_MODEM_STATUS        (6*REG_OFFSET)
#define         OFS_RS232_INPUT         (6*REG_OFFSET)
#define         OFS_SCRATCH_PAD         (7*REG_OFFSET)

#define         OFS_DIVISOR_LSB         (0*REG_OFFSET)
#define         OFS_DIVISOR_MSB         (1*REG_OFFSET)


typedef unsigned char uint8;
typedef unsigned int uint32;
static int serialbase[]={0xbfd003f8,0xbfd002f8,0xbff003f8};
extern void delay1(int);
/* memory-mapped read/write of the port */
inline uint8 UART16550_READ(int line,int y){
delay1(100);
return (*((volatile uint8*)(serialbase[line] + y)));
}
inline void  UART16550_WRITE(int line,int y, uint8 z){
delay1(100);
((*((volatile uint8*)(serialbase[line] + y))) = z);
}

static void debugInit(int line,uint32 baud, uint8 data, uint8 parity, uint8 stop)
{
	/* disable interrupts */
	UART16550_WRITE(line,OFS_INTR_ENABLE, 0);

	/* set up buad rate */
	{
		uint32 divisor;

		/* set DIAB bit */
		UART16550_WRITE(line,OFS_LINE_CONTROL, 0x80);

		/* set divisor */
		divisor = MAX_BAUD / baud;
		UART16550_WRITE(line,OFS_DIVISOR_LSB, divisor & 0xff);
		UART16550_WRITE(line,OFS_DIVISOR_MSB, (divisor & 0xff00) >> 8);

		/* clear DIAB bit */
		UART16550_WRITE(line,OFS_LINE_CONTROL, 0x0);
	}

	/* set data format */
	UART16550_WRITE(line,OFS_DATA_FORMAT, data | parity | stop);
}


static uint8 getDebugChar(int line)
{

	while ((UART16550_READ(line,OFS_LINE_STATUS) & 0x1) == 0);
	return UART16550_READ(line,OFS_RCV_BUFFER);
}

static uint8 testDebugChar(int line)
{

	return (UART16550_READ(line,OFS_LINE_STATUS) & 0x1) ;
}

static int putDebugChar(int line,uint8 byte)
{
	while ((UART16550_READ(line,OFS_LINE_STATUS) & 0x20) == 0);
	UART16550_WRITE(line,OFS_SEND_BUFFER, byte);
	return 1;
}

static int initserial(int line)
{
		debugInit(line,UART16550_BAUD_115200,
			  UART16550_DATA_8BIT,
			  UART16550_PARITY_NONE, UART16550_STOP_1BIT);
	return 0;
}

#define TIMEOUT 50
static int serialtest()
{
	int i,j;

	printf("serial test\n");
	initserial(0);
	initserial(1);
	
	for(i=0;i<16;i++)
	{
	if(testDebugChar(0))getDebugChar(0);
	else break;
	}

	for(i=0;i<16;j++)
	{
	if(testDebugChar(1))getDebugChar(1);
	else break;
	}

	printf("serial 0 send data to serial 1...");
	for(i=0;i<10;i++)
	{
	putDebugChar(0,'a'+i);
	for(j=0;j<TIMEOUT;j++)
	{
	if(testDebugChar(1))break;
	}
	if(j==TIMEOUT){printf("timeout");break;}
	printf("%c",getDebugChar(1));
	}
	printf("\n");

	for(i=0;i<16;i++)
	{
	if(testDebugChar(0))getDebugChar(0);
	else break;
	}

	for(i=0;i<16;j++)
	{
	if(testDebugChar(1))getDebugChar(1);
	else break;
	}
	printf("serial 1 send data to serial 0...");

	for(i=0;i<10;i++)
	{
	putDebugChar(1,'a'+i);
	for(j=0;j<TIMEOUT;j++)
	{
	if(testDebugChar(0))break;
	}
	if(j==TIMEOUT){printf("timeout");break;}
	printf("%c",getDebugChar(0));
	}
	printf("\n");
	return 0;
}

static int cmd_serial(int argc,char **argv)
{
int line;
if(argc<3)return -1;
line=argv[2][0]-'0';

switch(argv[1][0])
{
case 'i':
	initserial(line);
	break;
case 'r':
	printf("%c\n",getDebugChar(line));break;
case 'w':
	putDebugChar(line,argv[3][0]);
	break;
case 't':
	printf("%d\n",testDebugChar(line));
	break;
}
return 0;	
	
}

