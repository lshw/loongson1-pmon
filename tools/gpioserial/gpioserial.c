#include <stdio.h>
#include <sys/io.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <termios.h>
#include <signal.h>

#define		BASEADDR 	0x378
#define 	WAITSTART    	0
#define 	START		1000
#define 	LOWCLK		20
#define	 	HIBEGCLK 	30
#define 	HINEXTCLK 	40
#define 	WAITSEND	50
#define 	INITSEND	55
#define		SENDDATA	60
#define 	ERROR     	100
#define		PPDELAY		10         //parallel port delay
#define gdb_printf(...)

struct termios oldt, newt;

void restoreterm()
{
   tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
	iopl(1);
	exit(0);
}

void palputdata(char ch)
{
	int i = 0;
	unsigned char tmp = 0;
	unsigned char chjust=0;
	for (i = 0; i < 8; i++)
	{
		tmp = ch & 1;
		tmp = tmp << 3;
		tmp = tmp & 8;
		outb(tmp,BASEADDR);
		usleep(PPDELAY);
		tmp = tmp | 4;
		outb(tmp,BASEADDR);
		usleep(PPDELAY);
		chjust = tmp - 4;
		outb(chjust,BASEADDR);
		usleep(2);
		ch = ch >> 1;
	}
		outb(0,BASEADDR);
}


int main()
{
	int STAT = WAITSTART;
	int error = 0;
	int cnt = 8;
	int tick = 0;
	unsigned char crev = 0,crevp = 0;
	unsigned char clk = 0,clkp = 0;              //bit0 for clk
	unsigned char data = 0, datap = 0;	//bit1 for data
	unsigned char cget = 0;			//reorderd data
	int chkcnt = 1;	
	unsigned char cput = 0;             //send data init
	unsigned char sclk = 0;
	unsigned char sdata = 0; 
	int scnt = 0;
	int n = 0,sflg = 0;
	char chin;
    	int ch;


	iopl(3);
	setbuf(stdout,0);
	outb(0,BASEADDR);
	
	
    tcgetattr(STDIN_FILENO, &oldt);
    newt = oldt;
    newt.c_lflag &= ~(ECHO|ICANON);
    tcsetattr(STDIN_FILENO, TCSANOW, &newt);
	signal(SIGKILL,restoreterm);
	
	while(!error)
	{	
		crevp = crev;
		crev = inb(BASEADDR+1);
		crev = crev >> 3;
		crev = crev & 3;
		
		clkp = clk;
		datap = data;
		clk = crev & 1;
		data = crev & 2;
		data = data >> 1;
		if( sflg )
		{
		tick++;
		if(tick > 1000)
		{
		outb(0,BASEADDR);
		STAT = WAITSTART;
		tick = 0;
		sflg = 0;
		}
		}
		ioctl(0,FIONREAD,&n);
		if(n)
		{	
			n = 0;
			chin=getchar();	
			putchar(chin);
			STAT = WAITSEND;
			tick = 0;
			sflg= 1;
		}

	switch (STAT)
		{
		case WAITSTART:
		if ((clkp==1)&&(datap==1)&&(clk==1)&&(data == 0))
		{
		STAT = START;
		if((cnt > 0)&&(cnt < 8) )
		{
		STAT = WAITSTART;
		sflg = 0;
		tick = 0;
		cnt  = 8;
		}
		}
		break;
		
		case START:
		cnt = 0;
		cget = 0;
		if (clk == 0)
		{		
		STAT = LOWCLK;
		gdb_printf("stat = lowclk\n");          //debug		
		}		
		break;
		
		case LOWCLK:
		STAT = HIBEGCLK;
		break;
		
		case HIBEGCLK:
		if (clk == 0 )
		break;
		data = data << cnt;
		data = data & 0xff;
		cget = cget | data;
		data = data >> cnt;
		data = data & 1;
		cnt++;
		
		if (cnt == 8)
		{
			if(/*isascii(cget)*/1||cget == 0x80)
			{			
				if(cget == 0x80)
				{
				
					outb(0x4,BASEADDR);
					STAT = SENDDATA;
					break;
				}	
				else{
		 	  	if(sflg == 1 )
				{	
				}
				if(isascii(cget))
				printf("%c",cget);
				cget = 0;
				}
			}
			else
			{
			STAT = ERROR;
			printf(" ERROR ERROR ERROR char = (%x)H,",cget);
			break;
			}
		}
		STAT = HINEXTCLK;
		break;
		
		case HINEXTCLK:
		if (clk == 0 )
		{
		STAT = LOWCLK;
		if( cnt == 8 )
		STAT = 	WAITSTART;
		}
		if ((clk == 1) && (data != datap))
		{
		gdb_printf("warning at HINEXTCLK\n"); 		
		}		
		break;
		
		case WAITSEND:
		outb(0x4,BASEADDR);
		
		STAT = WAITSTART;
		break;

		case SENDDATA:
		sflg = 0;
		tick = 0;
		palputdata(chin);
		outb(0,BASEADDR);
		STAT = WAITSTART;
		break;
		
		case ERROR:
		error = 1;
		break;

		default:
		break;
		}
	}

	iopl(1);
	return 0;
}

