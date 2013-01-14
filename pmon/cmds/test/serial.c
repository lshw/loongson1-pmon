#include <pmon.h>
#include <cpu.h>
#include <termio.h>
#include <pmon/dev/ns16550.h>
#define         UART16550_BAUD_2400             2400
#define         UART16550_BAUD_4800             4800
#define         UART16550_BAUD_9600             9600
#define         UART16550_BAUD_19200            19200
#define         UART16550_BAUD_38400            38400
#define         UART16550_BAUD_57600            57600
#define         UART16550_BAUD_115200           115200
#define         UART16550_BAUD_230400           230400
#define         UART16550_BAUD_380400           380400
#define         UART16550_BAUD_460800           460800
#define         UART16550_BAUD_921600           921600

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
#ifdef DEVBD2F_FIREWALL
#define         MAX_BAUD    ( 1843200 / 16 )
#elif defined(CONFIG_HAVE_NB_SERIAL)
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
#define         OFS_FIFO             (2*REG_OFFSET)
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
#ifdef DEVBD2F_FIREWALL
static int serialbase[]={0xbe000000,0xbe000020};
#else
//static int serialbase[]={0xbfd003f8,0xbfd002f8,0xbff003f8};
#ifdef	LS1ASOC
static int serialbase[]={0xbfe40000, 0xbfe44000, 0xbfe48000, 0xbfe4c000};
#else
static int serialbase[]={0xbfe40000, 0xbfe41000, 0xbfe42000, 0xbfe43000, 0xbfe44000, 0xbfe45000, 0xbfe46000, 0xbfe47000, 0xbfe48000, 0xbfe4c000, 0xbfe6c000, 0xbfe7c000};
#endif
#endif
extern void delay(int);

/* memory-mapped read/write of the port */
inline uint8 UART16550_READ(int line,int y)
{
	//delay(10000);
	return (*((volatile uint8*)(serialbase[line] + y)));
}

inline void  UART16550_WRITE(int line,int y, uint8 z)
{
	//delay(10000);
	((*((volatile uint8*)(serialbase[line] + y))) = z);
}

static void debugInit(int line,uint32 baud, uint8 data, uint8 parity, uint8 stop)
{
	/* disable interrupts */
//	UART16550_WRITE(line,OFS_FIFO,FIFO_ENABLE|FIFO_RCV_RST|FIFO_XMT_RST|FIFO_TRIGGER_4);
	UART16550_WRITE(line,OFS_FIFO,FIFO_ENABLE|FIFO_RCV_RST|FIFO_XMT_RST|FIFO_TRIGGER_1);

	/* set up buad rate */
	{
		int pll,ctrl,clk;
		unsigned int ddr_clk,uart_clk;
		uint32 divisor;

		/* set DIAB bit */
		UART16550_WRITE(line,OFS_LINE_CONTROL, 0x80);

		/* set divisor */
		pll	= *(volatile unsigned int *)(0xbfe78030);
		ctrl = *(volatile unsigned int *)(0xbfe78034);	 
		clk = (12+(pll&0x3f))*33333333/2 + ((pll>>8)&0x3ff)*33333333/2/1024;
		ddr_clk = (ctrl&(1<<19))?clk/((ctrl>>14)&0x1f):clk/2;
		uart_clk = ddr_clk/2;
		
//		divisor = MAX_BAUD / baud;
		divisor = uart_clk / (16*MAX_BAUD);
		UART16550_WRITE(line,OFS_DIVISOR_LSB, divisor & 0xff);
		UART16550_WRITE(line,OFS_DIVISOR_MSB, (divisor & 0xff00) >> 8);

		/* clear DIAB bit */
		UART16550_WRITE(line,OFS_LINE_CONTROL, 0x0);
	}

	/* set data format */
	UART16550_WRITE(line,OFS_DATA_FORMAT, data | parity | stop);
	UART16550_WRITE(line,OFS_MODEM_CONTROL, MCR_DTR|MCR_RTS);
}


static uint8 getDebugChar(int line)
{
	while ((UART16550_READ(line,OFS_LINE_STATUS) & 0x1) == 0);
	return UART16550_READ(line,OFS_RCV_BUFFER);
}

//非阻塞
unsigned char get_uart_char(int base)
{
	int line;
	
	if (base == 0xbfe40000){
		line = 0;
	}
	else if (base == 0xbfe41000){
		line = 1;
	}
	else if (base == 0xbfe42000){
		line = 2;
	}
	else if (base == 0xbfe43000){
		line = 3;
	}
	else if (base == 0xbfe44000){
		line = 4;
	}
	else if (base == 0xbfe45000){
		line = 5;
	}
	else if (base == 0xbfe46000){
		line = 6;
	}
	else if (base == 0xbfe47000){
		line = 7;
	}
	else if (base == 0xbfe48000){
		line = 8;
	}
	else if (base == 0xbfe4c000){
		line = 9;
	}
	else if (base == 0xbfe6c000){
		line = 10;
	}
	else if (base == 0xbfe7c000){
		line = 11;
	}
	else{
		line = 8;
	}
	
	while ((UART16550_READ(line, OFS_LINE_STATUS) & 0x1) == 0){
		return 0;
	}
	return UART16550_READ(line, OFS_RCV_BUFFER);
}

static uint8 testDebugChar(int line)
{
	//返回0表示无数据  返回1表示有数据
//	printf("--%x ", UART16550_READ(line,OFS_LINE_STATUS));
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
	debugInit(line,CONS_BAUD, UART16550_DATA_8BIT, UART16550_PARITY_NONE, UART16550_STOP_1BIT);
	return 0;
}

#define TIMEOUT 50
int serial_selftest(int channel)
{
	int i,j,error=0;
	char c;
/*
#ifdef CONFIG_CHINESE
	printf("自检串口编号：%d\n", channel);
#else
	printf("serial selftest channel %d\n", channel);
#endif
*/
	initserial(channel);
	
	for(i=0;i<16;j++)
	{
		if(testDebugChar(channel))
			getDebugChar(channel);
		else break;
	}
/*
#ifdef CONFIG_CHINESE
	printf("串口 %d 发送数据到串口 %d...", channel, channel);
#else
	printf("serial %d send data to serial %d...", channel, channel);
#endif
*/
	for(i=0;i<30;i++)
	{
		putDebugChar(channel,'a'+i);
		for(j=0;j<TIMEOUT;j++)
		{
			delay(1000);
			if(testDebugChar(channel))
				break;
		}
		if(j==TIMEOUT){
//			printf("timeout");
			error=1;
			continue;
		}
		c = getDebugChar(channel);
//		printf("%c", c);
		if(c!='a'+i)error=1;
	}
#ifdef CONFIG_CHINESE
	printf("串口%d测试...%s\n", channel, error?"错误!!":"通过");
#else
	printf("serial %d test...%s\n", channel, error?"error":"ok");
#endif
	return 0;
}

int serialtest(void)
{
	int i,j;

	printf("serial test\n");
	initserial(0);
	initserial(1);
	
	for(i=0;i<16;i++){
		if(testDebugChar(0))getDebugChar(0);
		else break;
	}

	for(i=0;i<16;j++){
		if(testDebugChar(1))getDebugChar(1);
		else break;
	}

	printf("serial 0 send data to serial 1...");
	for(i=0;i<10;i++){
		putDebugChar(0,'a'+i);
		for(j=0;j<TIMEOUT;j++){
			if(testDebugChar(1))break;
		}
		if(j==TIMEOUT){printf("timeout");break;}
		printf("%c",getDebugChar(1));
	}
	printf("\n");

	for(i=0;i<16;i++){
		if(testDebugChar(0))getDebugChar(0);
		else break;
	}

	for(i=0;i<16;j++){
		if(testDebugChar(1))getDebugChar(1);
		else break;
	}
	printf("serial 1 send data to serial 0...");

	for(i=0;i<10;i++){
		putDebugChar(1,'a'+i);
		for(j=0;j<TIMEOUT;j++){
			if(testDebugChar(0))break;
		}
		if(j==TIMEOUT){printf("timeout");break;}
		printf("%c",getDebugChar(0));
	}
	printf("\n");
	return 0;
}

int ls1a_serialtest(void)
{
#ifdef CONFIG_CHINESE
	printf("串口测试\n说明：\n");
	printf("1.请把串口的输出引脚(Tx)连接到输入引脚(Rx)\n");
	printf("2.COM1接口对应串口0，JP20对应串口9，JP21对应串口10\n");
	printf("3.串口0测试需要把JP2插针的跳线帽拿掉\n");
#else
	printf("Plese plug net wire into fxp0\n");
#endif
	pause();
	/* UART0和GMAC1复用 使能UART0 */
	*((volatile unsigned int*)0xbfd00420) &= ~0x40;
	serial_selftest(0);
	serial_selftest(2);
	serial_selftest(3);
#ifdef CONFIG_CHINESE
	printf("\n串口测试结速\n");
#else
	printf("\nserial test done\n");
#endif
	return 0;
}

int ls1b_serialtest(void)
{
#ifdef CONFIG_CHINESE
	printf("串口测试\n说明：\n");
	printf("1.请把串口的输出引脚(Tx)连接到输入引脚(Rx)\n");
	printf("2.COM1接口对应串口0，JP20对应串口9，JP21对应串口10\n");
	printf("3.串口0测试需要把JP2插针的跳线帽拿掉\n");
#else
	printf("Plese plug net wire into fxp0\n");
#endif
	pause();
	/* UART0和GMAC0复用 使能UART0 */
	*(volatile unsigned int *)0xbfd00420 &= ~(0x18);
	serial_selftest(0);
	/* UART0和GMAC0复用 使能UART0 */
	*(volatile unsigned int *)0xbfd00420 |= 0x18;
//	serial_selftest(4);
//	serial_selftest(8);
	serial_selftest(9);
	serial_selftest(10);
//	serial_selftest(11);
#ifdef CONFIG_CHINESE
	printf("\n串口测试结速\n");
#else
	printf("\nserial test done\n");
#endif
	return 0;
}

int cmd_serial(int argc,char **argv)
{
	int line;
	
	if(argc != 3)return -1;
	if(argv[2][1] == 0)
		line = argv[2][0]-'0';
	else
		line = (argv[2][0]-'0') * 10 + (argv[2][1]-'0');

	switch(argv[1][0])
	{
		case 'i':
			initserial(line);
			break;
		case 'r':
			printf("%c\n",getDebugChar(line));break;
		case 'w':
			putDebugChar(line,'a');
			break;
		case 't':
			printf("%d\n",testDebugChar(line));
			break;
	}
	return 0;
}

static const Cmd Cmds[] =
{
	{"MyCmds"},
	{"serial","val",0,"hardware test",cmd_serial,0,99,CMD_REPEAT},
	{0,0}
};

static void init_cmd __P((void)) __attribute__ ((constructor));
static void init_cmd()
{
	cmdlist_expand(Cmds, 1);
}
