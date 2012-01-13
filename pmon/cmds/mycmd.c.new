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
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/net/if.h>
#include "mod_vgacon.h"
#include "mod_display.h"
void route_init();

#include <pflash.h>

#define rm9000_tlb_hazard(...)
#define CONFIG_PAGE_SIZE_64KB
#include "mipsregs.h"
#include "gzip.h"
#if NGZIP > 0
#include <gzipfs.h>
#endif /* NGZIP */
int cmd_mycfg __P((int, char *[]));
extern char  *heaptop;
void * memcpy(void *s1, const void *s2, size_t n);

#include <errno.h>
unsigned long strtoul(const char *nptr,char **endptr,int base);
#define ULONGLONG_MAX 0xffffffffffffffffUL
int abs(int x)
{
return x>=0?x:-x;
}
unsigned long long
strtoull(const char *nptr,char **endptr,int base)
{
#if __mips >= 3
    int c;
    unsigned long long  result = 0L;
    unsigned long long limit;
    int negative = 0;
    int overflow = 0;
    int digit;

    while ((c = *nptr) && isspace(c)) /* skip leading white space */
      nptr++;

    if ((c = *nptr) == '+' || c == '-') { /* handle signs */
	negative = (c == '-');
	nptr++;
    }

    if (base == 0) {		/* determine base if unknown */
	base = 10;
	if (*nptr == '0') {
	    base = 8;
	    nptr++;
	    if ((c = *nptr) == 'x' || c == 'X') {
		base = 16;
		nptr++;
	    }
	}
    } else if (base == 16 && *nptr == '0') {	
	/* discard 0x/0X prefix if hex */
	nptr++;
	if ((c = *nptr == 'x') || c == 'X')
	  nptr++;
    }

    limit = ULONGLONG_MAX / base;	/* ensure no overflow */

    nptr--;			/* convert the number */
    while ((c = *++nptr) != 0) {
	if (isdigit(c))
	  digit = c - '0';
	else if(isalpha(c))
	  digit = c - (isupper(c) ? 'A' : 'a') + 10;
	else
	  break;
	if (digit < 0 || digit >= base)
	  break;
	if (result > limit)
	  overflow = 1;
	if (!overflow) {
	    result = base * result;
	    if (digit > ULONGLONG_MAX - result)
	      overflow = 1;
	    else	
	      result += digit;
	}
    }
    if (negative && !overflow)	/* BIZARRE, but ANSI says we should do this! */
      result = 0L - result;
    if (overflow) {
	extern int errno;
	errno = ERANGE;
	result = ULONGLONG_MAX;
    }

    if (endptr != NULL)		/* point at tail */
      *endptr = (char *)nptr;
    return result;

#else
return strtoul(nptr,endptr,base);
#endif
}

static union commondata{
		unsigned char data1;
		unsigned short data2;
		unsigned int data4;
		unsigned int data8[2];
		unsigned char c[8];
}mydata,*pmydata;

unsigned int syscall_addrtype=0;
#define syscall_addrwidth ((syscall_addrtype>256?0:(syscall_addrtype>>4))+1)
static int __syscall1(int type,long long addr,union commondata *mydata);
static int __syscall2(int type,long long addr,union commondata *mydata);
int (*syscall1)(int type,long long addr,union commondata *mydata)=(void *)&__syscall1;
int (*syscall2)(int type,long long addr,union commondata *mydata)=(void *)&__syscall2;
static char *str2addmsg[]={"32 bit cpu address","64 bit cpu address","64 bit cached phyiscal address","64 bit uncached phyiscal address"};
#if __mips >= 3
unsigned long long
str2addr(const char *nptr,char **endptr,int base)
{
unsigned long long result;
if(syscall_addrtype%4==0)
{
result=(long)strtoul(nptr,endptr,base);
}
else
{
result=strtoull(nptr,endptr,base);
if(syscall_addrtype%4==3)result|=0x9000000000000000ULL;
else if(syscall_addrtype%4==2)result|=0x9800000000000000ULL;
}
return result;
}
#endif
/*
 *  Execute the 'call' command
 *  ==========================
 *
 *  The call command invokes a function in the same way as
 *  it would be when called from a program. Arguments passed
 *  can be given as values or strings. On return the value
 *  returned is printed out.
 */
#define nr_printf printf
#define nr_gets gets
#define nr_strtol strtoul
#define nr_strtoll strtoull
#if __mips >= 3
#define nr_str2addr str2addr
#else
#define nr_str2addr strtoul
#endif


static	pcitag_t mytag=0;

size_t fread (void *src, size_t size, size_t count, FILE *fp);
size_t fwrite (const void *dst, size_t size, size_t count, FILE *fp);
static char diskname[0x40];

static int __disksyscall1(int type,long long addr,union commondata *mydata)
{
	char fname[0x40];
	FILE *fp;
	if(strncmp(diskname,"/dev/",5)) sprintf(fname,"/dev/disk/%s",diskname);
	else strcpy(fname,diskname);
	fp=fopen(fname,"r+");
	if(!fp){printf("open %s error!\n",fname);return -1;}
	fseek(fp,addr,SEEK_SET);
switch(type)
{
case 1:fread(&mydata->data1,1,1,fp);break;
case 2:fread(&mydata->data2,2,1,fp);break;
case 4:fread(&mydata->data4,4,1,fp);break;
case 8:fread(&mydata->data8,8,1,fp);break;
}
	fclose(fp);
return 0;
}

static int __disksyscall2(int type,long long addr,union commondata *mydata)
{
	char fname[0x40];
	FILE *fp;
	if(strncmp(diskname,"/dev/",5)) sprintf(fname,"/dev/disk/%s",diskname);
	else strcpy(fname,diskname);
	fp=fopen(fname,"r+");
	if(!fp){printf("open %s error!\n",fname);return -1;}
	fseek(fp,addr,SEEK_SET);
switch(type)
{
case 1:fwrite(&mydata->data1,1,1,fp);break;
case 2:fwrite(&mydata->data2,2,1,fp);break;
case 4:fwrite(&mydata->data4,4,1,fp);break;
case 8:fwrite(&mydata->data8,8,1,fp);break;
}
	fclose(fp);
return 0;
}

static int devcp(int argc,char **argv)
{
char *buf;
int fp0,fp1;
int n,i;
int bs=0x20000;
int seek=0,skip=0;
char *fsrc=0,*fdst=0;
unsigned int count=-1,nowcount=0;
char pstr[80]="";
int quiet=0;
#if NGZIP > 0
int unzip=0;
#endif
if(argc<3)return -1;
	fsrc=argv[1];
	fdst=argv[2];
	for(i=3;i<argc;i++)
	{
	if(!strncmp(argv[i],"bs=",3))
	 bs=strtoul(&argv[i][3],0,0);
	else if(!strncmp(argv[i],"count=",6))
	 count=strtoul(&argv[i][6],0,0);
	else  if(!strncmp(argv[i],"skip=",5))
	 skip=strtoul(&argv[i][5],0,0);
	else if(!strncmp(argv[i],"seek=",5))
	 seek=strtoul(&argv[i][5],0,0);
	else if(!strncmp(argv[i],"quiet=",6))
	 quiet=strtoul(&argv[i][6],0,0);
#if NGZIP > 0
	else if(!strcmp(argv[i],"unzip=1"))
	unzip=1;
#endif
	}
	if(!fsrc||!fdst)return -1;
	fp0=open(fsrc,O_RDONLY);
	fp1=open(fdst,O_RDWR|O_CREAT|O_TRUNC);

	buf=malloc(bs);
	if(!buf){printf("malloc failed!,please set heaptop bigger\n");return -1;}

	if(!fp0||!fp1){printf("open file error!\n");free(buf);return -1;}
	lseek(fp0,skip*bs,SEEK_SET);
	lseek(fp1,seek*bs,SEEK_SET);
#if NGZIP > 0
	if(unzip)if(gz_open(fp0)==-1)unzip=0;
#endif
	while(count--)
	{
	int rcount=0;
	char *pnow=buf;
#if NGZIP > 0
	if(unzip) 
		while(rcount<bs)
		{
		n=gz_read(fp0,pnow,bs);
		if(n<=0)break; 
		rcount+=n;
		pnow+=n;
		}
	else
#endif
		while(rcount<bs)
		{
		n=read(fp0,pnow,bs-rcount);
		if(n<=0)break; 
		rcount+=n;
		pnow+=n;
		}
	nowcount+=rcount;
	if(!(strstr(argv[1],"/dev/tty")||strstr(argv[2],"/dev/tty")||quiet))
	{
	int i;
	for(i=0;i<strlen(pstr);i++)printf("\b \b");
	sprintf(pstr,"%d",nowcount);
	printf("%s",pstr);
	}
	if(write(fp1,buf,rcount)<rcount||rcount<bs)break;
	}
	free(buf);
#if NGZIP > 0
	if(unzip)gz_close(fp0);
#endif
	close(fp0);
	close(fp1);
return 0;
}

#ifndef NOPCI
static int __pcisyscall1(int type,unsigned long long addr,union commondata *mydata)
{
switch(type)
{
case 1:mydata->data1=_pci_conf_readn(mytag,addr,1);break;
case 2:mydata->data2=_pci_conf_readn(mytag,addr,2);break;
case 4:mydata->data4=_pci_conf_readn(mytag,addr,4);break;
case 8:mydata->data8[0]=_pci_conf_readn(mytag,addr,4);
	   mydata->data8[1]=_pci_conf_readn(mytag,addr+4,4);break;
}
return 0;
}

static int __pcisyscall2(int type,unsigned long long addr,union commondata *mydata)
{
switch(type)
{
case 1: _pci_conf_writen(mytag,addr,mydata->data1,1);return 0;
case 2: _pci_conf_writen(mytag,addr,mydata->data2,2);return 0;
case 4: _pci_conf_writen(mytag,addr,mydata->data4,4);return 0;
case 8:break;
}
return -1;
}
#endif

#if __mips >= 3
#define MYASM asm
#define MYC(...)
#else
#define MYASM(...)
#define MYC(x) x
#endif
static int __syscall1(int type,long long addr,union commondata *mydata)
{
/*
 * use lw to load l[2],will make high bit extension.
 */
switch(type)
{
case 1:
	  MYC(mydata->data1=*(volatile char *)addr;);
	  MYASM("ld $2,%1;lb $2,($2);" \
		  "sb $2,%0;" \
		  ::"m"(mydata->data1),"m"(addr)
		  :"$2","memory"
		 );
	   break;
case 2:
	  MYC(mydata->data2=*(volatile short *)addr;);
	  MYASM("ld $2,%1;lh $2,($2);" \
		  "sh $2,%0;" \
		  ::"m"(mydata->data2),"m"(addr)
		  :"$2"
		 );
	   break;
case 4:
	  MYC(mydata->data4=*(volatile int *)addr;);
	  MYASM("ld $2,%1;lw $2,($2);" \
		   "sw $2,%0;" \
		  ::"m"(mydata->data4),"m"(addr)
		  :"$2"
		 );
	   break;
case 8:
	  MYC( mydata->data8[0]=*(volatile int *)addr;mydata->data8[1]=*(volatile int *)(addr+4);)
	  MYASM("ld $2,%1;ld $2,($2);" \
		  "sd $2,%0;" \
		  ::"m"(mydata->data8[0]),"m"(addr)
		  :"$2"
		 );
	   break;
}
return 0;
}

static int __syscall2(int type,long long addr,union commondata *mydata)
{
switch(type)
{
case 1:
	 MYC(*(volatile char *)addr=mydata->data1;);
	  MYASM("ld $2,%1;lb $3,%0;sb $3,($2);" \
		  ::"m"(mydata->data1),"m"(addr)
		  :"$2","$3"
		 );
	   break;
case 2:
	  MYC(*(volatile short *)addr=mydata->data2;);
	  MYASM("ld $2,%1;lh $3,%0;sh $3,($2);" \
		  ::"m"(mydata->data2),"m"(addr)
		  :"$2","$3"
		 );
	  break;
case 4:
	  MYC(*(volatile int *)addr=mydata->data4;);
	  MYASM("ld $2,%1;lw $3,%0;sw $3,($2);" \
		  ::"m"(mydata->data2),"m"(addr)
		  :"$2","$3"
		 );
	    break;
case 8:
	   MYC(*(volatile int *)addr=mydata->data8[0];*(volatile int *)(addr+4)=mydata->data8[1];);
	  MYASM("ld $2,%1;ld $3,%0;sd $3,($2);" \
		  ::"m"(mydata->data8[0]),"m"(addr)
		  :"$2","$3"
		 );
	   break;
}
return 0;
}

int mypcs(int ac,char **av)
{
	int bus;
	int dev;
	int func;
    int tmp;

#ifndef NOPCI
if(ac==4)
{
	bus=nr_strtol(av[1],0,0);
	dev=nr_strtol(av[2],0,0);
	func=nr_strtol(av[3],0,0);
	mytag=_pci_make_tag(bus,dev,func);
	syscall1=(void *)__pcisyscall1;
	syscall2=(void *)__pcisyscall2;
}
else 
#endif
if(ac==2)
{
	syscall_addrtype=nr_strtol(av[1],0,0);
	syscall1=__syscall1;
	syscall2=__syscall2;
	mytag=-1;
   printf("select normal memory access (%s)\n",str2addmsg[syscall_addrtype%4]);
}
else if(ac==1)
{
int i;
for(i=0;i>-4;i--)
printf("pcs %d : select select normal memory access %s\n",i,str2addmsg[(unsigned)i%4]);
printf("pcs bus dev func : select pci configuration space access with bus dev func\n");
#ifndef NOPCI
if(mytag!=-1)
{
	_pci_break_tag(mytag,&bus,&dev,&func);
	printf("pci select bus=%d,dev=%d,func=%d\n",bus,dev,func);
}
#endif
}

	return (0);
}

int mydisks(int ac,char **av)
{
if(ac>2)return -1;
if(ac==2)
{
if(nr_strtol(av[1],0,0)==-1)
{
	syscall1=__syscall1;
	syscall2=__syscall2;
	diskname[0]=0;
}
else{ strncpy(diskname,av[1],0x40);diskname[0x3f]=0;}
}

if(diskname[0])
{
	syscall1=(void *)__disksyscall1;
	syscall2=(void *)__disksyscall2;
	printf("disk select %s\n",diskname);
}
else printf("select normal memory access\n");

	return (0);
}

static unsigned long long lastaddr=0;
static int dump(int argc,char **argv)
{
		char type=4;
unsigned long long  addr;
static int count=1;
		int i,j,k;
		char memdata[16];
//		char opts[]="bhwd";
		if(argc>3){nr_printf("d{b/h/w/d} adress count\n");return -1;}

		switch(argv[0][1])
		{
				case '1':	type=1;break;
				case '2':	type=2;break;
				case '4':	type=4;break;
				case '8':	type=8;break;
		}

		if(argc>1)addr=nr_str2addr(argv[1],0,0);
		else addr=lastaddr;
		if(argc>2)count=nr_strtol(argv[2],0,0);
		else if(count<=0||count>=1024) count=1;

		for(j=0;j<count;j=j+16/type,addr=addr+16/syscall_addrwidth)
		{
		nr_printf("%08llx: ",addr);

		pmydata=(void *)memdata;
		for(i=0;type*i<16;i++)
		{
		if(syscall1(type,addr+i*type/syscall_addrwidth,pmydata)<0){nr_printf("read address %p error\n",addr+i*type/syscall_addrwidth);return -1;}
		pmydata=(void *)((char *)pmydata+type);
		if(j+i+1>=count)break;
		}
		
		pmydata=(void *)memdata;
		for(i=0;type*i<16;i++)
		{
		switch(type)
		{
		case 1:	nr_printf("%02x ",pmydata->data1);break;
		case 2: nr_printf("%04x ",pmydata->data2);break;
		case 4: nr_printf("%08x ",pmydata->data4);break;
		case 8: nr_printf("%08x%08x ",pmydata->data8[1],pmydata->data8[0]);break;
		}
		if(j+i+1>=count){int k;for(i=i+1;type*i<16;i++){for(k=0;k<type;k++)nr_printf("  ");nr_printf(" ");}break;}
		pmydata=(void *)((char *)pmydata+type);
		}
		
		pmydata=(void *)memdata;
		#define CPMYDATA ((char *)pmydata)
		for(k=0;k<16;k++)
		{
		nr_printf("%c",(CPMYDATA[k]<0x20 || CPMYDATA[k]>0x7e)?'.':CPMYDATA[k]);
		if(j+(k+1)/type>=count)break;
		}
		nr_printf("\n");
		}

		lastaddr=addr+count*type;
		return 0;
}

static int getdata(char *str)
{
	static char buf[17];
	char *pstr;
	int negative=0;
	int radix=10;
	unsigned long long result;
	int digit,c;
	pstr=strtok(str," \t\x0a\x0d");

	if(pstr)
	{
		if(pstr[0]=='q')return -1;
		if(pstr[0]=='-')
		{
			negative=1;
			pstr++;
		}
		else if(pstr[0]=='+')
			pstr++;

		if(pstr[0]!='0'){radix=10;}
		else if(pstr[1]=='x'){radix=16;pstr=pstr+2;}

		result=0;

		while ((c = *pstr++) != 0) {
			if(c>='0' && c<='9') digit= c - '0';
			if(c>='a' && c<='f') digit= c - 'a' + 0xa;
			if(c>='A' && c<='F') digit= c - 'A' + 0xa;
			result = result*radix + digit;
		}

		if(negative) result = -result;

		memcpy(mydata.data8,&result,8);


		return 1;
	}
	return 0;
}

static int modify(int argc,char **argv)
{
		char type=4;
		unsigned long long addr;
//		char opts[]="bhwd";
		char str[100];
		int i;

		if(argc<2){nr_printf("m{b/h/w/d} adress [data]\n");return -1;}

		switch(argv[0][1])
		{
				case '1':	type=1;break;
				case '2':	type=2;break;
				case '4':	type=4;break;
				case '8':	type=8;break;
		}
		addr=nr_str2addr(argv[1],0,0);
		if(argc>2)
		{
		 i=2;
	          while(i<argc)
		 {
	       	   getdata(argv[i]);
		   if(syscall2(type,addr,&mydata)<0)
		   {nr_printf("write address %p error\n",addr);return -1;};
		   addr=addr+type/syscall_addrwidth;
		 i++;
		 }
		  return 0;
		}


		while(1)
		{
		if(syscall1(type,addr,&mydata)<0){nr_printf("read address %p error\n",addr);return -1;};
		nr_printf("%08llx:",addr);
		switch(type)
		{
		case 1:	nr_printf("%02x ",mydata.data1);break;
		case 2: nr_printf("%04x ",mydata.data2);break;
		case 4: nr_printf("%08x ",mydata.data4);break;
		case 8: nr_printf("%08x%08x ",mydata.data8[1],mydata.data8[0]);break;
		}
		memset(str,0,100);
		nr_gets(str);
	        i=getdata(str);
		if(i<0)break;	
		else if(i>0) 
		{
		if(syscall2(type,addr,&mydata)<0)
		{nr_printf("write address %p error\n",addr);return -1;};
		}
	addr=addr+type/syscall_addrwidth;
		}	
		lastaddr=addr;	
		return 0;
}
//----------------------------------------------------------------------
extern int novga;
static int setvga(int argc,char **argv)
{
	if(argc>1)
	{
	if(argv[1][0]=='1')vga_available=1;
	else vga_available=0;
	novga=!vga_available;
	}
	printf("vga_available=%d\n",vga_available);
	return 0;
}

static int setkbd(int argc,char **argv)
{
	if(argc>1)
	{
	if(argv[1][0]=='1')kbd_available=1;
	else if(argv[1][0]=='2')usb_kbd_available=1;
	else {kbd_available=0;usb_kbd_available=0;}
	}
	printf("kbd_available=%d,usb_kbd_available=%d\n",kbd_available,usb_kbd_available);
	return 0;
}


static int setinput(int argc,char **argv)
{
extern int input_from_both,output_to_both;
	if(argc==1)
	{
	printf("input_from_both=%d,output_to_both=%d\n",input_from_both,output_to_both);
	return 0;
	}
	if(!strcmp(argv[0],"setinput"))
	{
	 if(argv[1][0]=='1')input_from_both=1;
	 else input_from_both=0;
	}
	else if(!strcmp(argv[0],"setoutput"))
	{
	 if(argv[1][0]=='1')output_to_both=1;
	 else output_to_both=0;
	}
	return 0;
}


#if NMOD_VGACON
extern int kbd_initialize(void);
static int initkbd(int argc,char **argv)
{
 return kbd_initialize();
}
#endif

static int setcache(int argc,char **argv)
{
	if(argc==2)
	{
		if(argv[1][0]=='1')
		{
		cacheflush();
	    __asm__ volatile(
		".set mips2;\r\n"
        "mfc0   $4,$16;\r\n"
        "and    $4,$4,0xfffffff8;\r\n"
        "or     $4,$4,0x3;\r\n"
        "mtc0   $4,$16;\r\n"
		".set mips0;"
		::
		:"$4"
		);
		}
		else
		{
		cacheflush();
		__asm__ volatile(
		 ".set mips2;\r\n"
		 "mfc0   $4,$16;\r\n"
        "and    $4,$4,0xfffffff8;\r\n"
        "or     $4,$4,0x2;\r\n"
        "mtc0   $4,$16;\r\n"
		".set mips0;\r\n"
		::
		: "$4"
		);
		}
	}
return 0;
}

#if NMOD_FLASH_ST
#include <include/pflash.h>
static int erase(int argc,char **argv)
{
struct fl_map *map;
int offset;
if(argc!=2)return -1;
map=tgt_flashmap();
map++;
printf("%x,%d,%d,",map,map->fl_map_width,map->fl_map_bus);
offset=strtoul(argv[1],0,0);
printf(",%x\n",offset);
fl_erase_sector_st(map,0,offset);
return 0;
}

static int program(int argc,char **argv)
{
struct fl_map *map;
int offset;
int i;
if(argc!=3)return -1;
map=tgt_flashmap();
map++;
printf("%x,%d,%d,",map,map->fl_map_width,map->fl_map_bus);
offset=strtoul(argv[1],0,0);
for(i=0;i<strlen(argv[2]);i=i+2)
fl_program_st(map,0,offset+i,&argv[2][i]);
return 0;
}
#endif
//------------------------------------------------------------------------------------------
static int loopcmd(int ac,char **av)
{
static char buf[0x100];
int count,i;
int n=1;
	if(ac<3)return -1;
	count=strtol(av[1],0,0);
	while(count--)
	{
	buf[0]=0;
		for(i=2;i<ac;i++)
		{
		strcat(buf,av[i]);
		strcat(buf," ");
		}
	if(av[0][0]=='l')printf("NO %d\n",n++);
	do_cmd(buf);
	}
	return 0;
}

static int checksum(int argc,char **argv)
{
	int i;
	unsigned long size,left,total,bs,sum,want,count,idx;
	int quiet,checkwant,fp0,ret;
	int fsrc;
	char *buf,*cmd_buf=0;
	char *cmd=0;

	if (argc<2)
		return -1;

	bs=0x20000;
	total=0;
	size=0x7fffffff;
	checkwant=0;
	count=1;
	quiet=0;

	fsrc=argv[1];
	for(i=2;i<argc;i++)
	{
	if(!strncmp(argv[i],"bs=",3))
	 bs=strtoul(&argv[i][3],0,0);
	else if(!strncmp(argv[i],"count=",6))
	 count=strtoul(&argv[i][6],0,0);
	else if(!strncmp(argv[i],"size=",5))
	 size=strtoul(&argv[i][5],0,0);
	else if(!strncmp(argv[i],"want=",5))
	{
	 want=strtoul(&argv[i][5],0,0);
	 checkwant=1;
	}
	else if(!strncmp(argv[i],"quiet=",6))
	 quiet=strtoul(&argv[i][6],0,0);
	}
	

	if(!fsrc)return -1;
	buf=malloc(bs);
	if(!buf){printf("malloc failed!,please set heaptop bigger\n");return -1;}
	if((cmd=getenv("checksum"))) cmd_buf=malloc(strlen(cmd)+1);
	for(idx=0;idx<count;idx++)
	{
	 total=0;
	 left=size;
	 sum=0;

	if(cmd){
	strcpy(cmd_buf,cmd);
	do_cmd(cmd_buf);
	}

	fp0=open(fsrc,O_RDONLY);

	
	while(left)
	{
	 ret=read(fp0,buf,min(left,bs));
	 if(ret>0){
	 for(i=0;i<ret;i++) sum += buf[i];
	 total+=ret;
	 left -=ret;
	 }
	 else break;
	}

	close(fp0);

/*quiet:
 * 0:print everything
 * 1:only print when error
 * 2:goto cmdline when error
 * 3:reload and print error when error
 * 4:reload no msg
 */
	if(quiet<1)printf("%d:checksuming  0x%lx,size=0x%lx\n",idx,sum,total);
	if(checkwant && (want!=sum)){
	if(quiet<4)printf("%d:checksum error want 0x%x, got 0x%x\n",idx,want,sum);
	if(quiet>2) count++;
	else if(quiet==2)main();
	else break;
	}
   }
	free(buf);
	if(cmd_buf)free(cmd_buf);

	return 0;
}

static int testide(int ac,char **av)
{
	FILE *fp;
	char *buf;
	long bufsize;
	unsigned long count,x,tmp;
	bufsize=(ac==1)?0x100000:strtoul(av[1],0,0);
	buf=heaptop;
	x=count=0;
	tmp=bufsize>>24;
	fp=fopen("/dev/disk/wd0","r");
	if(!fp){printf("open error!\n");return -1;}
	while(!feof(fp))
	{
		fread(buf,bufsize,1,fp);
		x +=bufsize;
		if((x&0xffffff)==0){count+=tmp?tmp:16;printf("%d\n",count);}
	}
	fclose(fp);
	return 0;
}
//------------------------------------------------------------------------------------------
static struct parttype
{char *name;
unsigned char type;
} known_parttype[]=
 {
	"FAT12",1,
	"FAT16",4,
	"W95 FAT32",0xb,
	"W95 FAT32 (LBA)",0xc,
	"W95 FAT16 (LBA)",0xe,
	"W95 Ext'd (LBA)",0xf,
	"Linux",0x83,
	"Linux Ext'd" , 0x85,
	"Dos Ext'd" , 5,
	"Linux swap / Solaris" , 0x82,
	"Linux raid" , 0xfd,	/* autodetect RAID partition */
	"Freebsd" , 0xa5,    /* FreeBSD Partition ID */
	"Openbsd" , 0xa6,    /* OpenBSD Partition ID */
	"Netbsd" , 0xa9,   /* NetBSD Partition ID */
	"Minix" , 0x81,  /* Minix Partition ID */
};
struct partition {
    unsigned char boot_ind;     /* 0x80 - active */
    unsigned char head;     /* starting head */
    unsigned char sector;       /* starting sector */
    unsigned char cyl;      /* starting cylinder */
    unsigned char sys_ind;      /* What partition type */
    unsigned char end_head;     /* end head */
    unsigned char end_sector;   /* end sector */
    unsigned char end_cyl;      /* end cylinder */
    unsigned int start_sect;    /* starting sector counting from 0 */
    unsigned int nr_sects;      /* nr of sectors in partition */
} __attribute__((packed));


static int fdisk(int argc,char **argv)
{
int j,n,type_counts;
struct partition *p0;
FILE *fp;
char device[0x40];
int buf[0x10];
	if(strncmp(argv[1],"/dev/",5)) sprintf(device,"/dev/disk/%s",argv[1]);
	else strcpy(device,diskname);
sprintf(device,"/dev/disk/%s",(argc==1)?"wd0":argv[1]);
type_counts=sizeof(known_parttype)/sizeof(struct parttype);
fp=fopen(device,"rb");
if(!fp)return -1;
fseek(fp,446,0);
fread(buf,0x40,1,fp);
fclose(fp);

printf("Device Boot %-16s%-16s%-16sId System\n","Start","End","Sectors");
for(n=0,p0=(void *)buf;n<4;n++,p0++)
{
if(!p0->sys_ind)continue;

for(j=0;j<type_counts;j++)
if(known_parttype[j].type==p0->sys_ind)break;

printf("%-6d %-4c %-16d%-16d%-16d%x %s\n",n,(p0->boot_ind==0x80)?'*':' ',p0->start_sect,p0->start_sect+p0->nr_sects,p0->nr_sects,\
p0->sys_ind,j<type_counts?known_parttype[j].name:"unknown");
}
return 0;
}
#include <sys/netinet/in.h>
#include <sys/netinet/in_var.h>
#define SIN(x) ((struct sockaddr_in *)&(x))
extern char activeif_name[];

static void
setsin (struct sockaddr_in *sa, int family, u_long addr)
{
    bzero (sa, sizeof (*sa));
    sa->sin_len = sizeof (*sa);
    sa->sin_family = family;
    sa->sin_addr.s_addr = addr;
}

static int cmd_testnet(int argc,char **argv)
{
	char buf[100];
	int i,j;
	int s;
	struct sockaddr addr;

	if(argc<3)return -1;
	addr.sa_len=sizeof(addr);
	strcpy(addr.sa_data,argv[1]);

	s= socket (AF_UNSPEC, SOCK_RAW, 0);
	if(s==-1){
	printf("please select raw_ether\n");
	return -1;
	}
	if(!strcmp(argv[2],"send"))
	{
		for(j=0;;j++)
		{
			memset(buf,0xff,12);
			buf[12]=8;buf[13]=0;
			for(i=14;i<100;i++) buf[i]=i+j;
			sendto(s,buf,100,0,&addr,sizeof(addr));
			delay1(500);
			printf("%d\r",j);
		}
	}
	else if(!strcmp(argv[2],"recv"))
	{
		bind(s,&addr,sizeof(addr));
		while(1)
		{
			unsigned char buf[1500];
			int len;
			len=recv(s,buf,1500,0);
			for(i=0;i<len;i++)
			{
				if((i&15)==0)printf("\n%02x: ",i);
				printf("%02x ",buf[i]);
			}
		}
	}
	else
	{
	int errors=0;
		bind(s,&addr,sizeof(addr));
		while(1)
		{
			unsigned char buf[1500];
			int len;
		for(j=0;;j++)
		{
			memset(buf,0xff,12);
			buf[12]=8;buf[13]=0;
			for(i=14;i<100;i++) buf[i]=i-12+j;
			sendto(s,buf,100,0,&addr,sizeof(addr));
			len=recv(s,buf,100,0);
			for(i=12;i<100-4;i++) 
			{
			if(buf[i]!=i-12+j)break;
			}

			if(i==100-4)
			{
			printf("\r%d,%d",j,errors);
			}
			else
			{
			errors++;
				for(i=0;i<len;i++)
				{
					if((i&15)==0)printf("\n%02x: ",i);
					printf("%02x ",buf[i]);
				}
			}
			delay1(500);
		}
		}

	}
	close(s);
	return 0;
}

static int del_if_rt(char *ifname);
static int cmd_ifconfig(int argc,char **argv)
{
struct ifreq *ifr;
struct in_aliasreq *ifra;
struct in_aliasreq data;
int s = socket (AF_INET, SOCK_DGRAM, 0);
	if (s < 0) {
		perror("ifconfig: socket");
		return -1;
	}
ifra=(void *)&data;
ifr=(void *)&data;
bzero (ifra, sizeof(*ifra));
strcpy(ifr->ifr_name,argv[1]);
if(argc==2)
{
(void) ioctl(s, SIOCGIFADDR, ifr);
printf("ip:%s\n",inet_ntoa(satosin(&ifr->ifr_addr)->sin_addr));
(void) ioctl(s,SIOCGIFNETMASK, ifr);
printf("netmask:%s\n",inet_ntoa(satosin(&ifr->ifr_addr)->sin_addr));
(void) ioctl(s,SIOCGIFBRDADDR, ifr);
printf("boradcast:%s\n",inet_ntoa(satosin(&ifr->ifr_addr)->sin_addr));
(void) ioctl(s,SIOCGIFFLAGS,ifr);
printf("status:%s %s\n",ifr->ifr_flags&IFF_UP?"up":"down",ifr->ifr_flags&IFF_RUNNING?"running":"stoped");
}
else if(argc>=3)
{
char *cmds[]={"down","up","remove","stat","setmac","readrom","writerom","readphy","writephy","0x"};
int i;
	for(i=0;i<sizeof(cmds)/sizeof(char *);i++)
	if(!strncmp(argv[2],cmds[i],strlen(cmds[i])))break;

	switch(i)
	{
	case 0://down
		(void) ioctl(s,SIOCGIFFLAGS,ifr);
		ifr->ifr_flags &=~IFF_UP;
		(void) ioctl(s,SIOCSIFFLAGS,ifr);
		break;
	case 1://up
		(void) ioctl(s,SIOCGIFFLAGS,ifr);
		ifr->ifr_flags |=IFF_UP;
		(void) ioctl(s,SIOCSIFFLAGS,ifr);
		break;
	case 2://remove
		(void) ioctl(s,SIOCGIFFLAGS,ifr);
		ifr->ifr_flags &=~IFF_UP;
		(void) ioctl(s,SIOCSIFFLAGS,ifr);
		while(ioctl(s, SIOCGIFADDR, ifra)==0)
		{
		(void) ioctl(s, SIOCDIFADDR, ifr);
		}
		del_if_rt(argv[1]);
		break;
	case 3: //stat
		{
		register struct ifnet *ifp;
		ifp = ifunit(argv[1]);
		if(!ifp){printf("can not find dev %s.\n",argv[1]);return -1;}
		printf("RX packets:%d,TX packets:%d,collisions:%d\n" \
			   "RX errors:%d,TX errors:%d\n" \
			   "RX bytes:%d TX bytes:%d\n" ,
		ifp->if_ipackets, 
		ifp->if_opackets, 
		ifp->if_collisions,
		ifp->if_ierrors, 
		ifp->if_oerrors,  
		ifp->if_ibytes, 
		ifp->if_obytes 
		);
		if(ifp->if_baudrate)printf("link speed up to %d Mbps\n",ifp->if_baudrate);
		}
		break;
	case 4: //setmac
	    {
		struct ifnet *ifp;
		ifp = ifunit(argv[1]);
		if(ifp)
		{
		long arg[2]={argc-2,(long)&argv[2]};
		ifp->if_ioctl(ifp,SIOCETHTOOL,arg);
		}
	    }
		break;
        case 5: //read eeprom
            {
                struct ifnet *ifp;
                ifp = ifunit(argv[1]);
                if(ifp)
                {
                long arg[2]={argc-2,(long)&argv[2]};
                ifp->if_ioctl(ifp,SIOCRDEEPROM,arg);
                }
            }
                break;
        case 6: //write eeprom
            {
                struct ifnet *ifp;
                ifp = ifunit(argv[1]);
                if(ifp)
                {
                long arg[2]={argc-2,(long)&argv[2]};
                ifp->if_ioctl(ifp,SIOCWREEPROM,arg);
                }
            }
                break;
		case 7: //readphy
            {
                struct ifnet *ifp;
                ifp = ifunit(argv[1]);
                if(ifp)
                {
                long arg[2]={argc-2,(long)&argv[2]};
                ifp->if_ioctl(ifp,SIOCRDPHY,arg);
                }
            }
                break;
		case 8: //writephy
			{
                struct ifnet *ifp;
                ifp = ifunit(argv[1]);
                if(ifp)
                {
                long arg[2]={argc-2,(long)&argv[2]};
                ifp->if_ioctl(ifp,SIOCWRPHY,arg);
                }
			}
			break;
		case sizeof(cmds)/sizeof(cmds[0]) -1:
			{
                struct ifnet *ifp;
                ifp = ifunit(argv[1]);
                if(ifp)
                {
                long arg[2]={argc-2,(long)&argv[2]};
                ifp->if_ioctl(ifp,strtoul(argv[2],0,0),arg);
                }
			}
			break;

	default:
		while(ioctl(s, SIOCGIFADDR, ifra)==0)
		{
		(void) ioctl(s, SIOCDIFADDR, ifr);
		}
		setsin (SIN(ifra->ifra_addr), AF_INET, inet_addr(argv[2]));
		(void) ioctl(s, SIOCSIFADDR, ifra);
		if(argc>=4)
		 {
		 setsin (SIN(ifra->ifra_addr), AF_INET, inet_addr(argv[3]));
		 (void) ioctl(s,SIOCSIFNETMASK, ifra);
		 }
		 break;
	}
}
close(s);
return 0;
}

static int cmd_ifdown(int argc,char **argv)
{
struct ifreq ifr;
int s = socket (AF_INET, SOCK_DGRAM, 0);
	if (s < 0) {
		perror("ifconfig: socket");
		return -1;
	}
bzero (&ifr, sizeof(ifr));
strcpy(ifr.ifr_name,argv[1]);
(void) ioctl(s, SIOCGIFADDR, &ifr);
printf("%s",inet_ntoa(satosin(&ifr.ifr_addr)->sin_addr));
(void) ioctl(s, SIOCDIFADDR, &ifr);
ifr.ifr_flags=0;
(void) ioctl(s,SIOCSIFFLAGS,(void *)&ifr);
close(s);
return 0;
}

static int cmd_ifup(int argc,char **argv)
{
struct ifreq ifr;
int s = socket (AF_INET, SOCK_DGRAM, 0);
	if (s < 0) {
		perror("ifconfig: socket");
		return -1;
	}
bzero (&ifr, sizeof(ifr));
strcpy(ifr.ifr_name,argv[1]);
ifr.ifr_flags=IFF_UP;
(void) ioctl(s,SIOCSIFFLAGS,(void *)&ifr);
close(s);
return 0;
}
static int  cmd_rtlist(int argc,char **argv)
{
	db_show_arptab();
return 0;
}
#include <sys/net/route.h>
static int cmd_rtdel(int argc,char **argv)
{
	struct sockaddr dst;
	bzero(&dst,sizeof(dst));
	setsin (SIN(dst), AF_INET, inet_addr("10.0.0.3"));
	rtrequest(RTM_DELETE, &dst, 0, 0, 0,0);
	return 0;
}

static int mydelrt(rn, w)
	struct radix_node *rn;
	void *w;
{
	struct rtentry *rt = (struct rtentry *)rn;
	register struct ifaddr *ifa;

	if (*(char *)w=='*'||rt->rt_ifp)
	{
		if(*(char *)w=='*'||!strcmp(rt->rt_ifp->if_xname,(char *)w))
		{
		rtrequest(RTM_DELETE, rt_key(rt), rt->rt_gateway, rt_mask(rt), 0,0);
		}
	}

	return (0);
}

/*
 * Function to print all the route trees.
 * Use this from ddb:  "call db_show_arptab"
 */
static int del_if_rt(char *ifname)
{
	struct radix_node_head *rnh;
	rnh = rt_tables[AF_INET];
	rn_walktree(rnh, mydelrt, ifname);
	if (rnh == NULL) {
		printf(" (not initialized)\n");
		return (0);
	}
	return (0);
}



static int cmd_sleep(int argc,char **argv)
{
	unsigned long long microseconds;
	unsigned long long total, start;
	int i;

	microseconds=strtoul(argv[1],0,0);
//	printf("%d\n",microseconds);
	for(i=0;i<microseconds;i++)
	{
	delay(1000);
	}
	return 0;
}

static int cmd_sleep1(int argc,char **argv)
{
	unsigned long long microseconds;
	unsigned long long total, start;
	int i;

	microseconds=strtoul(argv[1],0,0);
//	printf("%d\n",microseconds);
	for(i=0;i<microseconds;i++)
	{
	delay1(1000);
	}
	return 0;
}

#if NMOD_VGACON
static void cmd_led(int argc,char **argv)
{
	int led;
	led=strtoul(argv[1],0,0);
	pckbd_leds(led);
}
#endif


int highmemcpy(long long dst,long long src,long long count);

int highmemcpy(long long dst,long long src,long long count)
{
#if __mips >= 3
asm(
".set noreorder\n"
"1:\n"
"beqz %2,2f\n"
"nop\n"
"lb $2,(%0)\n"
"sb $2,(%1)\n"
"daddiu %0,1\n"
"daddiu %1,1\n"
"b 1b\n"
"daddiu %2,-1\n"
"2:\n"
".set reorder\n"
::"r"(src),"r"(dst),"r"(count)
:"$2"
);
#else
 memcpy(dst,src,count);
#endif
return 0;
}

int highmemset(long long addr,char c,long long count)
{
#if __mips >= 3
asm(
".set noreorder\n"
"1:\n"
"beqz %2,2f\n"
"nop\n"
"sb %1,(%0)\n"
"daddiu %0,1\n"
"b 1b\n"
"daddiu %2,-1\n"
"2:\n"
".set reorder\n"
::"r"(addr),"r"(c),"r"(count)
:"$2"
);
#else
memset(addr,c,count);
#endif
return 0;
}

static cmd_mymemcpy(int argc,char **argv)
{
	long long src,dst,count;
	int ret;
	if(argc!=4)return -1;
	src=nr_str2addr(argv[1],0,0);
	dst=nr_str2addr(argv[2],0,0);
	count=nr_strtoll(argv[3],0,0);
	ret=highmemcpy(dst,src,count);	
	return ret;	
}
//------------------------------------------------------------------------------------------
#if defined(NMOD_FLASH_ST)&&defined(FLASH_ST_DEBUG)
#include "c2311.c"
int cmd_testst(void) {
ParameterType fp; /* Contains all Flash Parameters */
ReturnType rRetVal; /* Return Type Enum */
Flash(ReadManufacturerCode, &fp);
printf("Manufacturer Code: %08Xh\r\n",
fp.ReadManufacturerCode.ucManufacturerCode);
Flash(ReadDeviceId, &fp);
printf("Device Code: %08Xh\r\n",
fp.ReadDeviceId.ucDeviceId);
fp.BlockErase.ublBlockNr = 10; /* block number 10 will be erased
*/
rRetVal = Flash(BlockErase, &fp); /* function execution */
return rRetVal;
} /* EndFunction Main*/
#endif


static int mycmp(int argc,char **argv)
{
	unsigned char *s1,*s2;
	int length,left;
	if(argc!=4)return -1;
	s1=strtoul(argv[1],0,0);
	s2=strtoul(argv[2],0,0);
	length=strtoul(argv[3],0,0);
	while(left=bcmp(s1,s2,length))
	{
		s1=s1+length-left;
		s2=s2+length-left;
		length=left;
		printf("[%p]!=[%p](0x%02x!=0x%02x)\n",s1,s2,*s1,*s2);
		s1++;
		s2++;
		length--;
	}
	return 0;
}

static int (*oldwrite)(int fd,char *buf,int len)=0;
static char *buffer;
static int total;
extern void (*__msgbox)(int yy,int xx,int height,int width,char *msg);
void *restdout(int  (*newwrite) (int fd, const void *buf, size_t n));
static int newwrite(int fd,char *buf,int len)
{
//if(stdout->fd==fd)
{
memcpy(buffer+total,buf,len);
total+=len;
}
oldwrite(fd,buf,len);
return len;
}

#if NMOD_DISPLAY
static int mymore(int ac,char **av)
{
int i;
char *myline;
if(ac<2)return -1;
myline=heaptop;
total=0;
buffer=heaptop+0x100000;
oldwrite=restdout(newwrite);
myline[0]=0;
for(i=1;i<ac;i++)
{
strcat(myline,av[i]);
strcat(myline," ");
}
do_cmd(myline);
restdout(oldwrite);
buffer[total]='\n';
buffer[total+1]=0;
__console_init();
__msgbox(0,0,24,80,buffer);
return 0;
}
#endif

static unsigned long flashs_rombase;
static int rom_read(int type,long long addr,union commondata *mydata)
{
memcpy(&mydata->data1,(long)(flashs_rombase+addr),type);
return 0;
}

static int rom_write(int type,long long addr,union commondata *mydata)
{
        char *nvrambuf;
        char *nvramsecbuf;
	    char *nvram;
		int offs;
		struct fl_device *dev=fl_devident(flashs_rombase,0);
		int nvram_size=dev->fl_secsize;

        nvram =flashs_rombase + addr;
		if(fl_program_device(nvram,&mydata->data1,type, FALSE)) {
		return -1;
		}

		if(bcmp(nvram,&mydata->data1,type))
		{
		offs=(int)nvram &(nvram_size - 1);
        nvram  =(int)nvram & ~(nvram_size - 1);

	/* Deal with an entire sector even if we only use part of it */

        /* If NVRAM is found to be uninitialized, reinit it. */

        /* Find end of evironment strings */
	nvramsecbuf = (char *)malloc(nvram_size);

	if(nvramsecbuf == 0) {
		printf("Warning! Unable to malloc nvrambuffer!\n");
		return(-1);
	}

        memcpy(nvramsecbuf, nvram, nvram_size);
        if(fl_erase_device(nvram, nvram_size, FALSE)) {
		printf("Error! Nvram erase failed!\n");
		free(nvramsecbuf);
                return(0);
        }
	    
		nvrambuf = nvramsecbuf + offs;

		memcpy(nvrambuf,&mydata->data1,type);
        
		if(fl_program_device(nvram, nvramsecbuf, nvram_size, FALSE)) {
		printf("Error! Nvram program failed!\n");
		free(nvramsecbuf);
                return(0);
        }
	free(nvramsecbuf);
	 }
        return 0;
}


static int flashs(int ac,char **av)
{
struct fl_device *dev;
if(ac!=2)return -1;
flashs_rombase=strtoul(av[1],0,0);
dev=fl_devident(flashs_rombase,0);
if(!dev){
printf("can not find flash\n");
return -1;
}
else
{
	syscall1=rom_read;
	syscall2=rom_write;
	syscall_addrtype=0;
}
return 0;
}

//----------------------------------
//

#define MTC0(RT,RD,SEL) ((0x408<<20)|((RT)<<16)|((RD)<<11)|SEL)
#define MFC0(RT,RD,SEL) ((0x400<<20)|((RT)<<16)|((RD)<<11)|SEL)
#define DMTC0(RT,RD,SEL) ((0x40a<<20)|((RT)<<16)|((RD)<<11)|SEL)
#define DMFC0(RT,RD,SEL) ((0x402<<20)|((RT)<<16)|((RD)<<11)|SEL)
static int cp0s_sel=0;
static int __cp0syscall1(int type,unsigned long long addr,union commondata *mydata)
{
long data8;
extern int mycp0ins();
unsigned long *p=mycp0ins;
if(type!=8)return -1;
memset(mydata->data8,0,8);
addr=addr&0x1f;
#if __mips>=3
*p=DMFC0(2,addr,cp0s_sel);
#else
*p=MFC0(2,addr,cp0s_sel);
#endif
CPU_IOFlushDCache((long)mycp0ins&~31UL,32,1);
CPU_FlushICache((long)mycp0ins&~31UL,32);

 asm(".global mycp0ins;mycp0ins:mfc0 $2,$0;move %0,$2" :"=r"(data8)::"$2");

 *(long *)mydata->data8=data8;

return 0;
}

static int __cp0syscall2(int type,unsigned long long addr,union commondata *mydata)
{
extern int mycp0ins1();
unsigned long *p=mycp0ins1;
if(type!=8)return -1;
addr=addr&0x1f;
#if __mips>=3
*p=DMTC0(2,addr,cp0s_sel);
#else
*p=MTC0(2,addr,cp0s_sel);
#endif
CPU_IOFlushDCache((long)mycp0ins1&~31UL,32,1);
CPU_FlushICache((long)mycp0ins1&~31UL,32);

 asm(".global mycp0ins1;move $2,%0;mycp0ins1:mtc0 $2,$0;"::"r"(*(long *)mydata->data8):"$2");

return 0;
}

static int mycp0s(int argc,char **argv)
{
syscall1=__cp0syscall1;
syscall2=__cp0syscall2;
	syscall_addrtype=0x70;
if(argc>1)cp0s_sel=strtoul(argv[1],0,0);
else cp0s_sel=0;
return 0;	
}

void mycacheflush(long long addrin,unsigned int size,unsigned int rw)
{
unsigned int status;
unsigned long long addr;
addr=addrin&~0x1fULL;
size=(addrin-addr+size+0x1f)&~0x1fUL;

#if __mips >= 3
asm(" #define COP_0_STATUS_REG	$12 \n"
	"	#define SR_DIAG_DE		0x00010000\n"
	"	mfc0	%0, $12		# Save the status register.\n"
	"	li	$2, 0x00010000\n"
	"	mtc0	$2, $12		# Disable interrupts\n"
		:"=r"(status)
		::"$2");
if(rw)
{
	asm("#define HitWBInvalidate_S   0x17 \n"
		"#define HitWBInvalidate_D   0x15 \n"
		".set noreorder\n"
		"1:	\n"
		"sync \n"
		"cache   0x17, 0(%0) \n"
		"daddiu %0,32 \n"
		"addiu %1,-32 \n"
		"bnez %1,1b \n"
		"nop \n"
		".set reorder \n"
			::"r"(addr),"r"(size));
}
else
{
	asm("#define HitInvalidate_S     0x13 \n"
	"#define HitInvalidate_D     0x11\n"
"	.set noreorder\n"
"	1:	\n"
"	sync\n"
"	cache	0x13, 0(%0)\n"
"	daddiu %0,32\n"
"	addiu %1,-32\n"
"	bnez %1,1b\n"
"	nop\n"
"	.set reorder\n"
		::"r"(addr),"r"(size));
}

asm("\n"
"	#define COP_0_STATUS_REG	$12\n"
"	mtc0	%0, $12		# Restore the status register.\n"
		::"r"(status));

#else
CPU_IOFlushDCache(addr,size,rw);
#endif
}

//----------------------------------
static const Cmd Cmds[] =
{
	{"MyCmds"},
	{"testnet",	"", 0, "testnet rtl0 [recv|send|loop]", cmd_testnet, 0, 99, CMD_REPEAT},
	{"cp0s",	"", 0, "access cp0", mycp0s, 0, 99, CMD_REPEAT},
	{"pcs",	"bus dev func", 0, "select pci dev function", mypcs, 0, 99, CMD_REPEAT},
	{"disks",	"disk", 0, "select disk", mydisks, 0, 99, CMD_REPEAT},
	{"d1",	"[addr] [count]", 0, "dump address byte", dump, 0, 99, CMD_REPEAT},
	{"d2",	"[addr] [count]", 0, "dump address half world", dump, 0, 99, CMD_REPEAT},
	{"d4",	"[addr] [count]", 0, "dump address world", dump, 0, 99, CMD_REPEAT},
	{"d8",	"[addr] [count]", 0, "dump address double word", dump, 0, 99, CMD_REPEAT},
	{"m1",	"addr [data]", 0, "modify address byte", modify, 0, 99, CMD_REPEAT},
	{"m2",	"addr [data]", 0, "mofify address half world", modify, 0, 99, CMD_REPEAT},
	{"m4",	"addr [data]", 0, "modify address world", modify, 0, 99, CMD_REPEAT},
	{"m8",	"addr [data]", 0, "modify address double word",modify, 0, 99, CMD_REPEAT},
	{"setvga","[0|1]",0,"set vga_available",setvga,0,99,CMD_REPEAT},
	{"setkbd","[0|1]",0,"set kbd_available",setkbd,0,99,CMD_REPEAT},
	{"setinput","[0|1]",0,"set input_from_both",setinput,0,99,CMD_REPEAT},
	{"setoutput","[0|1]",0,"set output_to_both",setinput,0,99,CMD_REPEAT},
#if NMOD_VGACON
	{"initkbd","",0,"kbd_initialize",initkbd,0,99,CMD_REPEAT},
#endif
	{"cache","[0 1]",0,"cache [0 1]",setcache,0,99,CMD_REPEAT},
	{"loop","count cmd...",0,"loopcmd count cmd...",loopcmd,0,99,CMD_REPEAT},
	{"Loop","count cmd...",0,"loopcmd count cmd...",loopcmd,0,99,CMD_REPEAT},
	{"testide","[onecesize]",0,"test ide dma",testide,0,99,CMD_REPEAT},
	{"checksum","start size",0,"calculate checksum for a memory section",checksum,0,99,CMD_REPEAT},
	{"fdisk","diskname",0,"dump disk partation",fdisk,0,99,CMD_REPEAT},
#if NMOD_FLASH_ST
	{"erase","[0 1]",0,"cache [0 1]",erase,0,99,CMD_REPEAT},
	{"program","[0 1]",0,"cache [0 1]",program,0,99,CMD_REPEAT},
#ifdef FLASH_ST_DEBUG
	{"testst","n",0,"",cmd_testst,0,99,CMD_REPEAT},
#endif
#endif
	{"ifconfig","ifname",0,"ifconig fx0 [up|down|remove|stat|setmac|readrom|setrom|addr [netmask]",cmd_ifconfig,2,99,CMD_REPEAT},
	{"ifup","ifname",0,"ifup fxp0",cmd_ifup,2,99,CMD_REPEAT},
	{"ifdown","ifname",0,"ifdown fxp0",cmd_ifdown,2,99,CMD_REPEAT},
	{"rtlist","",0,"rtlist",cmd_rtlist,0,99,CMD_REPEAT},
	{"rtdel","",0,"rtdel",cmd_rtdel,0,99,CMD_REPEAT},
	{"sleep","ms",0,"sleep ms",cmd_sleep,2,2,CMD_REPEAT},
	{"sleep1","ms",0,"sleep1 s",cmd_sleep1,2,2,CMD_REPEAT},
	{"memcpy","src dst count",0,"mymemcpy src dst count",cmd_mymemcpy,0,99,CMD_REPEAT},
#if NMOD_VGACON
	{"led","n",0,"led n",cmd_led,2,2,CMD_REPEAT},
#endif
	{"mycmp","s1 s2 len",0,"mecmp s1 s2 len",mycmp,4,4,CMD_REPEAT},
#if NMOD_DISPLAY
	{"mymore","",0,"mymore",mymore,1,99,CMD_REPEAT},
#endif
	{"flashs",	"rom", 0, "select flash for read/write", flashs, 0, 99, CMD_REPEAT},
	{"devcp",	"srcfile dstfile [bs=0x20000] [count=-1] [seek=0] [skip=0] [quiet=0]", 0, "copy form src to dst",devcp, 0, 99, CMD_REPEAT},
	{0, 0}
};


static void init_cmd __P((void)) __attribute__ ((constructor));

static void
init_cmd()
{
	cmdlist_expand(Cmds, 1);
}


