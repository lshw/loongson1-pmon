#include <stdio.h>
#include <termio.h>
#include <string.h>
#include <setjmp.h>
#include <sys/endian.h>
#include <ctype.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#ifdef _KERNEL
#undef _KERNEL
#include <sys/ioctl.h>
#define _KERNEL
#else
#include <sys/ioctl.h>
#endif

#include <machine/cpu.h>

#include <pmon.h>
#include <dev/pci/pcivar.h>
#include <dev/pci/pcidevs.h>
#include <flash.h>
#include <time.h>
#include "mod_vgacon.h"

/* header files */
#include <linux/zlib.h>
#include <stdio.h>
#include <stdarg.h>
#include "mod_framebuffer.h"
void get_line1 (char *p,int usehist);
#include "setup.h"

#define PREV	CNTRL('P')
#define NEXT	CNTRL('N')
#define FORW	CNTRL('F')
#define BACK	CNTRL('B')
#define BEGIN	CNTRL('A')
#define END	CNTRL('E')
#define DELETE	CNTRL('D')
#define DELLINE	CNTRL('K')
#define MARK	CNTRL(' ')
#define KILL	CNTRL('W')
#define CLEAN	CNTRL('L')
#define UPDATE  0x100
#define UPDATE1  0x101
#define TIMER   0x102
#define MBEGIN 0x103
#define MEND 	0x104
#define RUN -1

static char cvtab[] = { PREV, NEXT, FORW, BACK, 'E', END, 'G', BEGIN };

enum esc_state { NONE, HAVE_ESC, HAVE_LB };

extern int ticks;
int getch (void)
{
	int cnt,oldticks=ticks;
	enum esc_state esc_state = NONE;
	int c, oc;
	struct termio tbuf;

	ioctl (STDIN, SETNCNE, &tbuf);


    for(;;) {
    while(1){
	ioctl (STDIN, FIONREAD, &cnt);
    if(cnt)break;
    if(abs(ticks-oldticks)>=100)return TIMER;
     }
	c = getchar ();
	oc=c;
	switch (esc_state) {
	case NONE:
		if (c == 0x1b) {
			esc_state = HAVE_ESC;
			continue;
		}
		break;

	case HAVE_ESC:
		if (c == '[') {
			esc_state = HAVE_LB;
			continue;
		}
		esc_state = NONE;
		break;

	case HAVE_LB:
		if (c >= 'A' && c <= 'H' ) {
			c = cvtab[c - 'A'];
		}
		esc_state = NONE;
		break;

	default:
		esc_state = NONE;
		break;
	}
	break;
}
    ioctl (STDIN, TCSETAW, &tbuf);
	return c;
}

void cprintfb(int y, int x,int width,char color, const char *buf);
static void popupS(int y, int x,int height,int width);
static void popdownS(int y, int x,int height,int width);
static void msgboxS(int yy,int xx,int height,int width,char *msg);
static void cprintS(int y, int x,int width,char color, const char *text);
static void set_cursorS(unsigned char x,unsigned char y);
extern void set_cursor(unsigned char x,unsigned char y);
extern void set_cursor_fb(unsigned char x,unsigned char y);
static void scr_clearS();
void (*__popup)(int y, int x,int height,int width)=popupS;
void (*__popdown)(int y, int x,int height,int width)=popdownS;
void (*__cprint)(int y, int x,int width,char color, const char *text)=cprintS;
void (*__msgbox)(int yy,int xx,int height,int width,char *msg)=msgboxS;
void (*__set_cursor)(unsigned char x,unsigned char y)=set_cursorS;
void (*__scr_clear)()=scr_clearS;

char popbuffer[80*25*2];
#if NMOD_VGACON
extern char *vgabh;
extern char *heaptop;


static void cprint(int y, int x,int width,char color, const char *text)
{
	register int i;
	char *dptr;

	dptr = (char *)(vgabh + (160*y) + (2*x));
	for (i=0;text && text[i]; i++) {
		*dptr = text[i];
		dptr[1]=color;
		dptr += 2;
		if(width && i==width-1)return;
        }
	if(width)
	{
		char *end=(char *)(vgabh + (160*y) + (2*(x+width)));
		for(;dptr<end;dptr+=2)
		{
		dptr[0]=0x20;
		dptr[1]=color;
		}
	}
}


static void popup(int y, int x,int height,int width)
{
	register int i;
	char *sptr,*dptr;
  
   for(dptr=popbuffer;height;height--,y++)
   {
	sptr = (char *)(vgabh + (160*y) + (2*x));
	for (i=0;i<width; i++) {
		dptr[0] = sptr[0];
		dptr[1]= sptr[1];
		sptr[0]=0x20;
		sptr[1]=0x17;
		dptr += 2;
		sptr+=2;
        }
   }
	for (i=0; i<0 + INFO_W; i++) { 
			dptr = (char *)(vgabh + (INFO_Y * 160) + (i * 2));
			dptr[0]=0x20;
			dptr[1] = 0x70;		/* Change Background to black */
	}
}

static void scr_clear()
{
	int x,y;
   for(y=0;y<25;y++)
   {
	for (x=0;x<80; x++) {
		char *p=(char *)(vgabh + (160*y) + (2*x));
		p[0]=0x20;
		p[1]=0x7;
        }
   }
}

static void popdown(int y, int x,int height,int width)
{
	register int i;
	char *sptr,*dptr;
  
   for(sptr=popbuffer;height;height--,y++)
   {
	dptr = (char *)(vgabh + (160*y) + (2*x));
	for (i=0;i<width; i++) {
		dptr[0] = sptr[0];
		dptr[1]= sptr[1];
		dptr += 2;
		sptr+=2;
        }
   }
	for (i=0; i<0 + INFO_W; i++) { 
			dptr = (char *)(vgabh + (INFO_Y * 160) + (i * 2));
			dptr[0] = 0x20;		/* Change Background to black */
			dptr[1] = 0x7;		/* Change Background to black */
	}
}

static void scrollwindow(int y, int x,int height,int width)
{
	register int i;
	char *sptr,*dptr;
  
   for(;height;height--)
   {
	dptr = (char *)(vgabh + (160*y) + (2*x));
	y++;
	sptr = (char *)(vgabh + (160*y) + (2*x));
	for (i=0;i<width; i++) {
		dptr[0] = sptr[0];
		dptr[1]= sptr[1];
		dptr += 2;
		sptr+=2;
        }
   }
}

void msgbox(int yy,int xx,int height,int width,char *msg)
{
int y=0;
int x=0;
char *iptr=msg;
char *myline=malloc(1000);
char *optr=myline;
popup(yy,xx,height,width);
for(;*iptr;iptr++)
{
  if(y==height){
	cprint(INFO_Y,0,INFO_W,0x70,"press any key to view next line");
	while(getch()>=0x100);
	scrollwindow(yy,xx,height-1,width);
	y--;
  optr=myline;
  }

 if(*iptr=='\n'||*iptr=='\r')
 {
  *optr=0;
  cprint(y+yy,xx,width,0x17,myline);
  optr=myline;
  y++;x=0;
  continue;
 }
 *optr++=*iptr;x++;
 if(x==width)
 {
 *optr=0;
  cprint(y+yy,xx,width,0x17,myline);
  optr=myline;
  y++;x=0;
  continue;
 }
}
cprint(INFO_Y,0,INFO_W,0x70,"press any key to exit");
	while(getch()!='\n');
popdown(yy,xx,height,width);
free(myline);
}
#endif

//---------------------------------------------------------------------------
#if NMOD_FRAMEBUFFER
void video_cls(void);
static void popupfb(int y, int x,int height,int width)
{
video_cls();
#if 0
  for(;height;height--,y++)
	__cprint(y,x,width,0x17,0);
	__cprint(INFO_Y,0,80,0x70,0);
#endif
}

static void scr_clearfb()
{
video_cls();
}
#endif

static void cprintS(int y, int x,int width,char color, const char *text)
{
 int i,l;
  char c=0x20;
  char *colorstr=(color==0x17)?"\e[0;37;44m":(color==0x70)?"\e[0;7m":"\e[0m";
  fprintf(stdout,"\e[%d;%dH%s",y,x,colorstr);
  if(text && (l=strlen(text)))fwrite(text,(width && l>width)?width:l,1,stdout);
  else l=0;
  if(l<width)for(i=0;i<width-l;i++)fwrite(&c,1,1,stdout);
}

static void popupS(int y, int x,int height,int width)
{
	 fprintf(stdout,"\e[0m\e[2J");
  for(;height;height--,y++)
	cprintS(y,x,width,0x17,0);
	cprintS(INFO_Y,0,80,0x70,0);
}

static void scr_clearS()
{
	 fprintf(stdout,"\e[0m\e[2J");
}

static void popdownS(int y, int x,int height,int width)
{
   fprintf(stdout,"\e[0m[H[2J");
}

static void set_cursorS(unsigned char x,unsigned char y)
{
fprintf(stdout,"\e[%d;%dH",y,x+1);
}

extern char popbuffer[];

#define nextline() \
 if(!optr || optr>popbuffer+80*25) optr=popbuffer;\
 if(y<height){lidx=(lidx+1)%25;lines[lidx]=optr;}

static void msgboxS(int yy,int xx,int height,int width,char *msg)
{
int y=0;
int x=0;
char *iptr=msg;
char *optr=0;
int lidx=0,i;
char *lines[25];
nextline();
__popup(yy,xx,height,width);
for(;*iptr;iptr++)
{
  if(y==height){
	__cprint(INFO_Y,0,INFO_W,0x70,"press enter to view next line");
	while(getch()>=0x100);
	for(i=0;i<height-1;i++) __cprint(yy+i,xx,width,0x17,lines[(lidx-height+2+25+i)%25]);//scroll window
	y--;
   nextline();
  }

 if(*iptr=='\n'||*iptr=='\r')
 {
  *optr++=0;
  __cprint(y+yy,xx,width,0x17,lines[lidx]);
  y++;x=0;
   nextline();
  continue;
 }
 *optr++=*iptr;x++;
 if(x==width)
 {
 *optr++=0;
  __cprint(y+yy,xx,width,0x17,lines[lidx]);
  y++;x=0;
   nextline();
  continue;
 }
}

__cprint(INFO_Y,0,INFO_W,0x70,"press enter to exit");
while(getch()>=0x100);
__popdown(yy,xx,height,width);
}

void cprintf(int y, int x,int width,char color,const char *fmt, ...)
{
    int             len;
    va_list	    ap;
    char *buf=heaptop;
    va_start(ap, fmt);
    len = vsprintf (buf, fmt, ap);
    va_end(ap);
	__cprint(y,x,width,color,buf);
}

void __console_init()
{
 if(!vga_available)
 {
  __cprint=cprintS;
  __popup=popupS;
  __popdown=popdownS;
  __msgbox=msgboxS;
  __set_cursor=set_cursorS;
  __scr_clear=scr_clearS;
 }
#if NMOD_VGACON
 else
 {
#if NMOD_FRAMEBUFFER
  __cprint=cprintfb;
  __popup=popupfb;
  __popdown=popupfb;
  __msgbox=msgboxS;
  __set_cursor=set_cursor_fb;
  __scr_clear=scr_clearfb;
#else
  __cprint=cprint;
  __popup=popup;
  __popdown=popdown;
  __msgbox=msgbox;
  __set_cursor=set_cursor;
  __scr_clear=scr_clear;
#endif
 }
#endif
}

