/*
	PMON Graphic Library
	Designed by Zhouhe(÷‹∫’) USTC 04011
*/

#include <termio.h>
#include <pmon.h>
#include <stdio.h>
#include <linux/types.h>
#include "window.h"
#include <sys/ioctl.h>
#include <stdio.h>
#include "mod_framebuffer.h"
#include <linux/io.h>
#define VIDEO_FONT_WIDTH	8
#define VIDEO_FONT_HEIGHT	16
typedef struct 
{

	char text;
	unsigned char color;	
}scr;
scr background[25][80];
scr foreground[25][80];
#define cr(b,t) ((b<<4)+t)

typedef struct 
{
	int length,speed;
	int x,y16;
}drop;
drop drops[256];
int rpro=-256;
int tpro=0,wpro=0,newpage=0,oldpage=0,xo;
int currentid=0,enterid=0,overid=1,overx1,overy1,overx2,overy2,nextab[4],nextco[4],nextid[4],winit=1;
int theme=0;
unsigned char bcolor=0xe0;
unsigned char btncolortheme[2][3]={{0x00,0x30,0xe0},{0xe0,0xc0,0xf0}},btncolor[3];
extern void video_drawchars (int xx, int yy, unsigned char *s, int count);
void w_setpage(int i)
{
	newpage=i;
}
int w_getpage()
{
	return oldpage;
}
int w_focused()
{
	return currentid==overid;
}
int w_entered()
{
	return currentid==enterid;
}
void w_setfocusid(int n)
{
	overid=n;
}
int w_getfocusid()
{
	return overid;
}
static int inside(int x,int y)
{
	return (x<80 && x>=0 && y<25 && y>=0);
}
static void w_resetdrop(drop *d)
{
	d->x=(rand()&127);
	d->length=(rand()&31)+5;
	d->y16=-(d->length-(rand()&3)+2)<<16;
	
	d->speed=(rand()& 300)+40;
}
static void w_initbackground(void)
{
	int i,j;
		//Ë∞??≤Ê??http://www.stanford.edu/class/cs140/projects/pintos/specs/freevga/vga/colorreg.htm
		for(i=0;i<25;i++)
			for(j=0;j<80;j++)
			{
				background[i][j].text=rand()&127;
				background[i][j].color=1;
			}
		for(i=0;i<256;i++)
			w_resetdrop(drops+i);
}
unsigned int pal[16];
scr finalbuf[25][80];
#if NMOD_FRAMEBUFFER
extern unsigned int eorx, fgx, bgx;
extern void video_disableoutput();
extern void video_enableoutput();
#else
extern unsigned char* vgabh;
#endif

static void w_cls()
{
	int i;
	if(!vga_available)
	{
		printf("\e[2J");
		memset(finalbuf,0x20,sizeof(foreground));
	}
	else
	{
		#if NMOD_FRAMEBUFFER
		video_cls();
		memset(finalbuf,0x01,sizeof(foreground));
		#else
		for(i=0;i<80*25*2;i+=8)
			*((long long *)(vgabh+i))=(long long)(0x0f000f000f000f00<<32)+0x0f000f000f000f00;
		#endif
	}
}
void w_enterconsole()
{
#if  NMOD_FRAMEBUFFER
	if(vga_available)
	{
		video_enableoutput();
		eorx=fgx=0xffffffff;
		bgx=0;
	}
#endif
	w_cls();
}
void w_leaveconsole()
{
	w_cls();
#if  NMOD_FRAMEBUFFER
	if(vga_available)
		video_disableoutput();
#endif
}

void w_init(void)
{
	int i,j;
	w_cls();
	if(vga_available)
	{	
#if NMOD_FRAMEBUFFER
		video_disableoutput();
		for(i=0;i<8;i++)
		{
			pal[i]=(i<<2)+(i<<18);
			pal[i+8]=(i<<13)+(i<<8)+31;
			pal[i+8]+=pal[i+8]<<16;
		}	
#else
		// disable blinking
		linux_inb(0x3da);//http://atrevida.comprenica.com/atrtut06.html
		linux_outb(0x30,0x3c0);//CAS=1,INDEX=0x10
		j=linux_inb(0x3c1);
	
		linux_inb(0x3da);
		linux_outb(0x30,0x3c0);
		linux_outb(j&0xf7,0x3c0);
	
		linux_outb(0,0x3c8);
		for(j=0;j<16;j++)
		{
			for(i=0;i<8;i++)
			{
				linux_outb(7,0x3c9);
				linux_outb(0,0x3c9);
				linux_outb(i<<3,0x3c9);
			}
			
			for(i=0;i<8;i++)	//È•±Â??
			{
				linux_outb(i<<3,0x3c9);
				linux_outb(i<<3,0x3c9);
				linux_outb(7<<3,0x3c9);
			}
		}
#endif
	}
	w_initbackground();
	w_defaultcolor();

}
static void w_copy(void *src,void *dest)//ÂØπÈ?Ôº?!!
{
	int i;
	for(i=0;i<80*25*2;i+=8)
		*((long long *)(dest+i))=*((long long *)(src+i));
}
static void w_background(void)
{
	int i,j,y,y1;
	if(theme&1)
	{
		for(i=0;i<256;i++)
		{
			y1=drops[i].y16>>16;
			drops[i].y16+=drops[i].speed;
			y=drops[i].y16>>16;
			if(y!=y1 && drops[i].x<80)
			{
				for(y--,j=0;j<7;y--,j++)
				if(inside(drops[i].x,y))
					background[y][drops[i].x].color=((background[y][drops[i].x].color>1)?background[y][drops[i].x].color-1:1);
			y+=7+drops[i].length;
			if(inside(drops[i].x,y))
			{
				background[y][drops[i].x].text=rand()&127;
				background[y][drops[i].x].color=15;
			}
			for(y--,j=0;j<7;y--,j++)
				if(inside(drops[i].x,y))
					background[y][drops[i].x].color=((background[y][drops[i].x].color>1)?background[y][drops[i].x].color-1:1);
			}
			
			if(drops[i].y16>39*65536)
				w_resetdrop(drops+i);
		}
		w_copy(background,foreground);
	}
	else
	{
		memset(foreground,0,sizeof(foreground));		

		for(i=2;i<23;i++)
			for(j=0;j<80;j++)
				foreground[i][j].color=0xe0;
		for(j=0;j<80;j++)
			foreground[0][j].color=0xf0;
		for(i=1;i<79;i++)
			foreground[2][i].text=foreground[22][i].text=196;
		for(i=3;i<22;i++)
			foreground[i][0].text=foreground[i][79].text=foreground[i][48].text=179;
		foreground[2][0].text=218;
		foreground[2][79].text=191;
		foreground[22][0].text=192;
		foreground[22][79].text=217;
		foreground[2][48].text=194;
		foreground[22][48].text=193;
		w_text(40,0,WA_CENTRE,"LOONGSON BIOS SETUP");
		w_bigtext(0,23,80,2,"TAB:Change Color Schame(1-4)    Left Arrow & Right Arrow: Switch Page    Up Arrow & Down Arrow: Select Item    Enter: Confirm and Switch   ~:Run Command");
	}
	
}
int charin=0,cn=0;
char *chgconcolor[]={"\e[0;37;44m","\e[0;7m"};
void w_present(void)
{
	char c,c0,c1;
	//,d[20];
	
	int cnt;
	int i,j,x,y;	
	//struct termio tbuf;
	if(theme&2)
		rpro=1024;
	if(vga_available)
	{
#if NMOD_FRAMEBUFFER
		video_enableoutput();
		for(j=0,y=VIDEO_FONT_HEIGHT*5/2;j<25;j++,y+=VIDEO_FONT_HEIGHT)
			for(i=0,x=0;i<80;i++,x+=VIDEO_FONT_WIDTH)
				if(*(short *)(&(finalbuf[j][i]))!=*(short *)(&(foreground[j][i])))
				{
					fgx=pal[foreground[j][i].color&0xf];
					bgx=pal[foreground[j][i].color>>4];
					eorx=fgx^bgx;
					video_drawchars (x, y,&(foreground[j][i].text),1);
				}
		video_disableoutput();
		w_copy(foreground,finalbuf);
#else
	w_copy(foreground,vgabh);
#endif
	}
	else		//COM output
	{
	
		rpro=1024;
		x=-2;y=-1;c0=-2;
		for(j=1;j<25;j++)
			for(i=0;i<80;i++)
				if(*(short *)(&(finalbuf[j][i]))!=*(short *)(&(foreground[j][i])))
				{
					if(foreground[j][i].color<0x80)c1=0;else c1=1;
					if(c1!=c0)
						printf(chgconcolor[c1]);
					c0=c1;
					if(foreground[j][i].text<=0)
						c=' ';
					else 
						c=foreground[j][i].text;
					if((x==i-1 && y==j) || (i==0 && x==79 && y==j-1))
						printf("%c",c);
					else
						printf("\e[%d;%dH%c",j,i+1,c);
					x=i;
					y=j;
				}
		w_copy(foreground,finalbuf);
	}
	

	//if(cn)printf("%x\n",cn);
	winit=0;
	w_background();
	if(newpage!=oldpage)
		if(rpro>0 && vga_available && (theme&2)==0)
			rpro-=1;
		else
		{
			//overx=overy=0;
			overid=1;
			if(vga_available && (theme&2)==0)
				rpro=0;
			oldpage=newpage;
			winit=1;
		}
	else
	 if(rpro<1024)
		rpro+=1;
	    else
		rpro=1024;
	if(rpro<=512)
		wpro=rpro;
	else wpro=512;
	if(rpro>512)
		tpro=rpro-512;
	else
		tpro=0;
	xo=40*(512-wpro);
	charin=0;

	enterid=-100;

	switch(cn)
	{
		case '\t':
			if(vga_available)
				theme++;
			w_defaultcolor();
			break;
		case '\n':
			enterid=overid;
			break;
		case '[A':
		case '[B':
		case '[C':
		case '[D':
			overid=nextid[(cn&255)-'A'];
			break;
		default:
			charin=cn;
	}
	
	ioctl (STDIN, FIONREAD, &cnt);
	cn=0;
	if(cnt)
	{
		do
		{
			cn=(cn<<8)+(c=getchar());
		}while(c=='[');
#if NMOD_FRAMEBUFFER
		set_cursor_fb(0,0);
#else
		set_cursor(0,0);
#endif		
	}
	currentid=0;
	nextid[0]=nextid[1]=nextid[2]=nextid[3]=overid;
	nextco[0]=nextco[3]=-1000;
	nextab[0]=nextab[1]=nextab[2]=nextab[3]=nextco[1]=nextco[2]=1000;
}
static void w_text2(int x,int y,int xalign,char *ostr,int len)
{
	int i,t;
	if(y<0 || y> 24)
		return;
	t = len*tpro>>9;
	x -=  t >> xalign ;
	for(i=0;i<t;i++)
		if(ostr[i]!=' ' && ostr[i]!='\n' && inside(x+i,y))
		{
			foreground[y][x+i].text=ostr[i];
			foreground[y][x+i].color&=0xf0;
			foreground[y][x+i].color|=((~(foreground[y][x+i].color>>4))&0xf);
		}
}
void w_text(int x,int y,int xalign,char *ostr)
{
	w_text2(x,y,xalign,ostr,strlen(ostr));
}
void w_window(int x,int y,int w,int h,char *text)
{
	int x1,y1,i,lpitch;
	scr *tscr;	

	x=( x * wpro + xo )>> 9;
	y=( y * wpro ) >> 9;
	x1=x+(w*wpro>>9);
	y1=y+(h*wpro>>9);

	if(x1<1 || x>79 || y1<1 || y>24)
		return;
	
	if(x<0)
		x=0;
	if(y<0)
		y=0;
	if(x1>80)
		x1=80;
	if(y1>25)
		y1=25;
	
	tscr=&(foreground[y][x]);
	lpitch=80-(x1-x);
	for(i=x;i<x1;i++,tscr++)
	{
		tscr->text=' ';
		tscr->color=bcolor;
	}
	w_text((x+x1)>>1,y,WA_CENTRE,text);
	for(y++;y<y1;y++)
	{
		tscr+=lpitch;
		for(i=x;i<x1;i++,tscr++)
			tscr->color=(tscr->color>>2&0xf3);
	}
}

inline int dist(int a1,int a2,int b1,int b2)//called by next fun
{
	if(b2>a1)
		if(b1<a2)
			return 0;
		else
			return b1-a2;
	else return a1-b2;
}

static void w_window2(int x,int y,int w,int h,char *text)
{
	currentid++;
	if(currentid==overid)
	{
		bcolor=btncolor[2];
		overx1=x;
		overy1=y;
		overx2=x+w-1;
		overy2=y+h-1;
	}
	else 
	{
		if(y+h-1<overy2 && (nextab[0]>dist(overx1,overx2,x,x+w-1) || (nextab[0]==dist(overx1,overx2,x,x+w-1) && y+h-1>nextco[0])))
		{
			nextab[0]=dist(overx1,overx2,x,x+w-1);
			nextco[0]=y+h-1;
			nextid[0]=currentid;
		}
		if(y>overy1 && (nextab[1]>dist(overx1,overx2,x,x+w-1) || (nextab[1]==dist(overx1,overx2,x,x+w-1) && y<nextco[1])))
		{
			nextab[1]=dist(overx1,overx2,x,x+w-1);
			nextco[1]=y;
			nextid[1]=currentid;
		}
		if(x>overx1 && (nextab[2]>dist(overy1,overy2,y,y+h-1) || (nextab[2]==dist(overy1,overy2,y,y+h-1) && x<nextco[2])))
		{
			nextab[2]=dist(overy1,overy2,y,y+h-1);
			nextco[2]=x;
			nextid[2]=currentid;
		}
		if(x+w-1<overx2 && (nextab[3]>dist(overy1,overy2,y,y+h-1) || (nextab[3]==dist(overy1,overy2,y,y+h-1) && x+w-1>nextco[3])))
		{
			nextab[3]=dist(overy1,overy2,y,y+h-1);
			nextco[3]=x+w-1;
			nextid[3]=currentid;
		}
		bcolor=btncolor[1];
	}
	w_window(x,y,w,h,text);
	bcolor=btncolor[0];
}
int w_button(int x,int y,int w,char *caption)
{
	w_window2(x,y,w,1,caption);
	return currentid==enterid;
}
int w_select(int x,int y,int w,char *caption,char **options,int *current)
{
	w_text(x,y,WA_RIGHT,caption);
	w_window2(x,y,w,1,options[*current]);
	if(currentid==enterid)
	{
		(*current)++;
		if(options[*current] == 0)
			*current=0;
	}
	return currentid==enterid;
}
int w_switch(int x,int y,char *caption,int *base,int mask)
{
	w_window2(x,y,1,1,(((*base)&mask)?"X":" "));
	if(currentid == enterid)	
		(*base)^=mask;
	w_text(x+1,y,WA_LEFT,caption);
	return currentid==enterid;
}
int w_input(int x,int y,int w,char *caption,char * text,int buflen)
{
	int l=strlen(text);	
	w_text(x,y,WA_RIGHT,caption);
	w_window2(x,y,w,1,((w>l)?text:text+l-w));
	if(currentid==overid)	
	{
		if(charin==127 || charin==7) 
		{
			if(l)
				text[l-1]=0;
		}
		else if(charin>=32 && l<buflen-1)		
		{
			text[l]=charin;
			text[l+1]=0;
		}
	}
	return currentid==enterid;
}
void w_bigtext(int x,int y,int w,int h,char *text)
{
	int i,j,k;
	h+=y;
	for(k=y;k<h;k++)
	{
		for(i=0;i<w+1;i++)
			switch(text[i])
			{
				case 0:
					j=i;
					w_text(x,k,WA_LEFT,text);
					return;
				case '\n':
					j=i+1;
					goto end;
				case ' ':
					j=i+1;
			}
end:
		w_text2(x,k,WA_LEFT,text,j);
		text+=j;
	}
}
void w_setcolor(char windowcolor,char buttonunused,char buttonused)
{
	bcolor=btncolor[0]=windowcolor;
	btncolor[1]=buttonunused;
	btncolor[2]=buttonused;
}
void w_defaultcolor()
{
	memcpy(btncolor,btncolortheme[theme&1],3);
	bcolor=btncolor[0];
}
int w_keydown(int kin)
{
	if(kin==cn)
	{
		cn=0;
		return 1;
	}
	return 0;
}
int w_selectinput(int x,int y,int w,char *caption,char **options,char *current,int buflen)
{
	if(winit)
	{
		for(x=0;options[x];x++)
		if((unsigned char)(current[buflen-1])>=x)
			current[buflen-1]=0;
		for(x=0;x<buflen-1 && current[x];x++);
		if(x==0 || x==buflen-1)
			strcpy(current,options[current[buflen-1]]);
		return 0;
	}
	if(w_input(x,y,w,caption,current,buflen-1))
	{
		current[buflen-1]++;
		if(options[current[buflen-1]] == 0)
			current[buflen-1]=0;
		strcpy(current,options[current[buflen-1]]);
	}
	return currentid==enterid;
}
int w_biginput(int x,int y,int w,int h,char *caption,char * text,int buflen)
{
	int l=strlen(text);	
	w_window2(x,y,w,h,caption);
	if(currentid==overid)	
	{
		text[l]=219;
		text[l+1]=0;
		w_bigtext(x,y+1,w,h-1,text);
		text[l]=0;

		if(charin==127) 
		{
			if(l)
				text[l-1]=0;
		}
		else if(charin>=32 && l<buflen-2)
		{
			text[l]=charin;
			text[l+1]=0;
		}
	}
	else 
	w_bigtext(x,y+1,w,h-1,text);
	return currentid==enterid;
}
int w_hitanykey()
{
	int cnt;
	while(cnt==0)
		ioctl (STDIN, FIONREAD, &cnt);
	return getchar();
}