#include <stdio.h>
#include <termio.h>
#include <string.h>
#include <setjmp.h>
#include <sys/endian.h>
#include <ctype.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <file.h>
#include <mtdfile.h>
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
#include <sys/ioctl.h>
#include "mod_vgacon.h"
#include "mod_display.h"

#define rm9000_tlb_hazard(...)
#define CONFIG_PAGE_SIZE_64KB
#include "mipsregs.h"
#include "gzip.h"
#if NGZIP > 0
#include <gzipfs.h>
#endif /* NGZIP */

#include <errno.h>

extern char *heaptop;

void *memcpy(void *s1, const void *s2, size_t n);

#define ULONGLONG_MAX 0xffffffffffffffffUL

static unsigned long long lastaddr = 0;

static union commondata {
	unsigned char data1;
	unsigned short data2;
	unsigned int data4;
	unsigned int data8[2];
	unsigned char c[8];
}mydata, *pmydata;

unsigned int syscall_addrtype = 0;
#define syscall_addrwidth ((syscall_addrtype>256?0:(syscall_addrtype>>4))+1)
static int __syscall1(int type,long long addr,union commondata *mydata);
static int __syscall2(int type,long long addr,union commondata *mydata);
int (*syscall1)(int type,long long addr,union commondata *mydata) = (void *)&__syscall1;
int (*syscall2)(int type,long long addr,union commondata *mydata) = (void *)&__syscall2;

static char *str2addmsg[] = {
	"32 bit cpu address",
	"64 bit cpu address",
	"64 bit cached phyiscal address",
	"64 bit uncached phyiscal address"
};

#if __mips >= 3
unsigned long long str2addr(const char *nptr, char **endptr, int base)
{
	unsigned long long result;
	if ((syscall_addrtype % 4) == 0) {
		result = (long)strtoul(nptr, endptr, base);
	}
	else {
		result = strtoull(nptr, endptr, base);
		if ((syscall_addrtype % 4) == 3)
			result |= 0x9000000000000000ULL;
		else if((syscall_addrtype % 4) == 2)
			result |= 0x9800000000000000ULL;
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

#if __mips >= 3
#define MYASM asm
#define MYC(...)
#else
#define MYASM(...)
#define MYC(x) x
#endif
static int __syscall1(int type, long long addr, union commondata *mydata)
{
	/* use lw to load l[2],will make high bit extension. */
	switch(type) {
	case 1:
		MYC(mydata->data1 = *(volatile char *)addr;);
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

static int __syscall2(int type, long long addr, union commondata *mydata)
{
	switch(type) {
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

static int dump(int argc, char **argv)
{
	char type = 4;
	unsigned long long  addr;
	static int count = 1;
	int i, j, k;
	char memdata[16];

	if (argc > 3) {
		nr_printf("d{b/h/w/d} adress count\n");
		return -1;
	}

	switch (argv[0][1]) {
		case '1':	type=1;break;
		case '2':	type=2;break;
		case '4':	type=4;break;
		case '8':	type=8;break;
	}

	if (argc > 1)
		addr = nr_str2addr(argv[1], 0, 0);
	else
		addr = lastaddr;
	if (argc > 2)
		count = nr_strtol(argv[2], 0, 0);
	else if((count<=0) || (count>=1024))
		count = 1;

	for (j=0; j<count; j=j+16/type,addr=addr+16/syscall_addrwidth) {
		nr_printf("%08llx: ",addr);

		pmydata = (void *)memdata;
		for (i=0; type*i<16; i++) {
			if(syscall1(type, addr+i*type/syscall_addrwidth, pmydata) < 0) {
				nr_printf("read address %p error\n", addr+i*type/syscall_addrwidth);
				return -1;
			}
			pmydata = (void *)((char *)pmydata+type);
			if ((j+i+1) >= count)
				break;
		}

		pmydata = (void *)memdata;
		for (i=0; type*i<16; i++) {
			switch (type) {
				case 1:	nr_printf("%02x ",pmydata->data1);break;
				case 2: nr_printf("%04x ",pmydata->data2);break;
				case 4: nr_printf("%08x ",pmydata->data4);break;
				case 8: nr_printf("%08x%08x ",pmydata->data8[1],pmydata->data8[0]);break;
			}
			if ((j+i+1) >= count) {
				int k;
				for(i=i+1;type*i<16;i++) {
					for (k=0; k<type; k++)
						nr_printf("  ");
					nr_printf(" ");
				}
			break;
			}
			pmydata = (void *)((char *)pmydata + type);
		}

		pmydata = (void *)memdata;
	#define CPMYDATA ((char *)pmydata)
		for (k=0; k<16; k++) {
			nr_printf("%c",(CPMYDATA[k]<0x20 || CPMYDATA[k]>0x7e)?'.':CPMYDATA[k]);
			if((j+(k+1)/type) >= count)
				break;
		}
		nr_printf("\n");
	}

	lastaddr = addr + count * type;
	return 0;
}

static int getdata(char *str)
{
	char *pstr;
	int negative = 0;
	int radix = 10;
	unsigned long long result;
	int digit, c;

	pstr = strtok(str, " \t\x0a\x0d");

	if (pstr) {
		if(pstr[0]=='q')return -1;
		if(pstr[0]=='-') {
			negative = 1;
			pstr++;
		}
		else if (pstr[0]=='+')
			pstr++;

		if (pstr[0]!='0') {
			radix = 10;
		}
		else if (pstr[1]=='x') {
			radix = 16;
			pstr = pstr + 2;
		}

		result=0;

		while ((c = *pstr++) != 0) {
			if(c>='0' && c<='9') digit= c - '0';
			if(c>='a' && c<='f') digit= c - 'a' + 0xa;
			if(c>='A' && c<='F') digit= c - 'A' + 0xa;
			result = result*radix + digit;
		}

		if (negative)
			result = -result;

		memcpy(mydata.data8, &result, 8);

		return 1;
	}
	return 0;
}

static int modify(int argc, char **argv)
{
	char type = 4;
	unsigned long long addr;
	char str[100];
	int i;

	if (argc < 2) {
		nr_printf("m{b/h/w/d} adress [data]\n");
		return -1;
	}

	switch (argv[0][1]) {
		case '1':	type=1;break;
		case '2':	type=2;break;
		case '4':	type=4;break;
		case '8':	type=8;break;
	}
	addr = nr_str2addr(argv[1], 0, 0);
	if (argc > 2) {
		i = 2;
		while (i < argc) {
			getdata(argv[i]);
			if (syscall2(type, addr, &mydata) < 0) {
				nr_printf("write address %p error\n",addr);
				return -1;
			}
			addr = addr + type / syscall_addrwidth;
			i++;
		}
		return 0;
	}

	while (1) {
		if (syscall1(type,addr,&mydata)<0) {
			nr_printf("read address %p error\n",addr);
			return -1;
		};
		nr_printf("%08llx:", addr);
		switch(type) {
			case 1:	nr_printf("%02x ",mydata.data1);break;
			case 2: nr_printf("%04x ",mydata.data2);break;
			case 4: nr_printf("%08x ",mydata.data4);break;
			case 8: nr_printf("%08x%08x ",mydata.data8[1],mydata.data8[0]);break;
		}
		memset(str, 0, 100);
		nr_gets(str);
		i = getdata(str);
		if (i < 0)
			break;	
		else if (i > 0) {
			if (syscall2(type, addr, &mydata) < 0) {
				nr_printf("write address %p error\n", addr);
				return -1;
			}
		}
		addr = addr + type / syscall_addrwidth;
	}
	lastaddr = addr;	
	return 0;
}

#define MTC0(RT,RD,SEL) ((0x408<<20)|((RT)<<16)|((RD)<<11)|SEL)
#define MFC0(RT,RD,SEL) ((0x400<<20)|((RT)<<16)|((RD)<<11)|SEL)
#define DMTC0(RT,RD,SEL) ((0x40a<<20)|((RT)<<16)|((RD)<<11)|SEL)
#define DMFC0(RT,RD,SEL) ((0x402<<20)|((RT)<<16)|((RD)<<11)|SEL)
static int cp0s_sel = 0;
extern int mycp0ins(void);
extern int mycp0ins1(void);

static int __cp0syscall1(int type, unsigned long long addr, union commondata *mydata)
{
	long data8;
	unsigned long *p = (unsigned long *)mycp0ins;

	if (type != 8)
		return -1;
	memset(mydata->data8, 0, 8);
	addr = addr & 0x1f;
#if __mips>=3
	*p = DMFC0(2, addr, cp0s_sel);
#else
	*p = MFC0(2, addr, cp0s_sel);
#endif
	CPU_IOFlushDCache((long)mycp0ins&~31UL, 32, 1);
	CPU_FlushICache((long)mycp0ins&~31UL, 32);

	asm(".global mycp0ins;mycp0ins:mfc0 $2,$0;move %0,$2" :"=r"(data8)::"$2");

	*(long *)mydata->data8=data8;

	return 0;
}

static int __cp0syscall2(int type, unsigned long long addr, union commondata *mydata)
{
	unsigned long *p = (unsigned long *)mycp0ins1;

	if (type != 8)
		return -1;
	addr = addr&0x1f;
#if __mips>=3
	*p=DMTC0(2,addr,cp0s_sel);
#else
	*p=MTC0(2,addr,cp0s_sel);
#endif
	CPU_IOFlushDCache((long)mycp0ins1&~31UL, 32, 1);
	CPU_FlushICache((long)mycp0ins1&~31UL, 32);

	asm(".global mycp0ins1;move $2,%0;mycp0ins1:mtc0 $2,$0;"::"r"(*(long *)mydata->data8):"$2");

	return 0;
}

static int mycp0s(int argc, char **argv)
{
	syscall1 = __cp0syscall1;
	syscall2 = __cp0syscall2;
	syscall_addrtype = 0x70;
	if (argc > 1)
		cp0s_sel = strtoul(argv[1], 0, 0);
	else
		cp0s_sel = 0;
	return 0;
}

static const Cmd Cmds[] =
{
	{"MyCmds"},
	{"cp0s",	"", 0, "access cp0", mycp0s, 0, 99, CMD_REPEAT},
	{"d1",	"[addr] [count]", 0, "dump address byte", dump, 0, 99, CMD_REPEAT},
	{"d2",	"[addr] [count]", 0, "dump address half world", dump, 0, 99, CMD_REPEAT},
	{"d4",	"[addr] [count]", 0, "dump address world", dump, 0, 99, CMD_REPEAT},
	{"d8",	"[addr] [count]", 0, "dump address double word", dump, 0, 99, CMD_REPEAT},
	{"m1",	"addr [data]", 0, "modify address byte", modify, 0, 99, CMD_REPEAT},
	{"m2",	"addr [data]", 0, "mofify address half world", modify, 0, 99, CMD_REPEAT},
	{"m4",	"addr [data]", 0, "modify address world", modify, 0, 99, CMD_REPEAT},
	{"m8",	"addr [data]", 0, "modify address double word",modify, 0, 99, CMD_REPEAT},
	{0, 0}
};

static void init_cmd __P((void)) __attribute__ ((constructor));

static void init_cmd(void)
{
	cmdlist_expand(Cmds, 1);
}



/* 以下命令不常用，可在调试时打开 */
#if 0
static pcitag_t mytag = 0;

#ifndef NOPCI
static int __pcisyscall1(int type, unsigned long long addr, union commondata *mydata)
{
	switch (type) {
		case 1:mydata->data1=_pci_conf_readn(mytag,addr,1);break;
		case 2:mydata->data2=_pci_conf_readn(mytag,addr,2);break;
		case 4:mydata->data4=_pci_conf_readn(mytag,addr,4);break;
		case 8:mydata->data8[0]=_pci_conf_readn(mytag,addr,4);
			   mydata->data8[1]=_pci_conf_readn(mytag,addr+4,4);break;
	}
	return 0;
}

static int __pcisyscall2(int type, unsigned long long addr, union commondata *mydata)
{
	switch (type) {
		case 1: _pci_conf_writen(mytag,addr,mydata->data1,1);return 0;
		case 2: _pci_conf_writen(mytag,addr,mydata->data2,2);return 0;
		case 4: _pci_conf_writen(mytag,addr,mydata->data4,4);return 0;
		case 8:break;
	}
	return -1;
}
#endif

int mypcs(int ac, char **av)
{
	int bus;
	int dev;
	int func;

#ifndef NOPCI
	if (ac == 4) {
		bus=nr_strtol(av[1],0,0);
		dev=nr_strtol(av[2],0,0);
		func=nr_strtol(av[3],0,0);
		mytag=_pci_make_tag(bus,dev,func);
		syscall1=(void *)__pcisyscall1;
		syscall2=(void *)__pcisyscall2;
	} else 
#endif
	if (ac == 2) {
		syscall_addrtype=nr_strtol(av[1],0,0);
		syscall1=__syscall1;
		syscall2=__syscall2;
		mytag=-1;
		printf("select normal memory access (%s)\n",str2addmsg[syscall_addrtype%4]);
	}
	else if (ac == 1) {
		int i;
		for (i=0; i>-4; i--)
			printf("pcs %d : select select normal memory access %s\n",i,str2addmsg[(unsigned)i%4]);
		printf("pcs bus dev func : select pci configuration space access with bus dev func\n");
		#ifndef NOPCI
		if (mytag != -1) {
			_pci_break_tag(mytag, &bus, &dev, &func);
			printf("pci select bus=%d,dev=%d,func=%d\n", bus, dev, func);
		}
		#endif
	}

	return (0);
}


#include "flashs.c"
#include "disk.c"

extern int novga;
static int setvga(int argc, char **argv)
{
	if (argc > 1) {
		if (argv[1][0] == '1')
			vga_available = 1;
		else
			vga_available = 0;
		novga = !vga_available;
	}
	printf("vga_available=%d\n", vga_available);
	return 0;
}

static int setkbd(int argc, char **argv)
{
	if (argc>1) {
		if (argv[1][0] == '1')
			kbd_available = 1;
		else if (argv[1][0] == '2')
			usb_kbd_available = 1;
		else {
			kbd_available = 0;
			usb_kbd_available = 0;
		}
	}
	printf("kbd_available=%d,usb_kbd_available=%d\n", kbd_available, usb_kbd_available);
	return 0;
}

static int setinput(int argc, char **argv)
{
	extern int input_from_both, output_to_both;

	if (argc == 1) {
		printf("input_from_both=%d,output_to_both=%d\n", input_from_both, output_to_both);
		return 0;
	}
	if (!strcmp(argv[0],"setinput")) {
		if (argv[1][0] == '1')
			input_from_both = 1;
		else
			input_from_both = 0;
	}
	else if (!strcmp(argv[0], "setoutput")) {
		if (argv[1][0] == '1')
			output_to_both = 1;
		else
			output_to_both = 0;
	}
	return 0;
}

#if NMOD_VGACON
extern int kbd_initialize(void);
static int initkbd(int argc, char **argv)
{
	return kbd_initialize();
}
#endif

extern void cacheflush(void);

static int setcache(int argc, char **argv)
{
	if (argc==2) {
		if (argv[1][0]=='1') {
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
		else {
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

static int loopcmd(int ac, char **av)
{
	static char buf[0x100];
	int count,i;
	int n=1;

	if (ac < 3)
		return -1;
	count = strtol(av[1],0,0);
	while (count--) {
		buf[0]=0;
		for (i=2;i<ac;i++) {
			strcat(buf,av[i]);
			strcat(buf," ");
		}
		if (av[0][0]=='l')
			printf("NO %d\n",n++);
		do_cmd(buf);
	}
	return 0;
}

static int checksum(int argc, char **argv)
{
	int i;
	unsigned long size, left, total, bs, sum, want, count, idx;
	int quiet, checkwant, fp0, ret;
	char *fsrc;
	char *buf, *cmd_buf = 0;
	char *cmd = 0;

	if (argc<2)
		return -1;

	bs = 0x20000;
	total = 0;
	size = 0x7fffffff;
	checkwant = 0;
	count = 1;
	quiet = 0;

	fsrc = argv[1];
	for (i=2;i<argc;i++) {
		if(!strncmp(argv[i],"bs=",3))
			bs=strtoul(&argv[i][3],0,0);
		else if(!strncmp(argv[i],"count=",6))
			count=strtoul(&argv[i][6],0,0);
		else if(!strncmp(argv[i],"size=",5))
			size=strtoul(&argv[i][5],0,0);
		else if(!strncmp(argv[i],"want=",5)) {
			want=strtoul(&argv[i][5],0,0);
			checkwant=1;
		}
		else if(!strncmp(argv[i],"quiet=",6))
			quiet=strtoul(&argv[i][6],0,0);
	}

	if (!fsrc)
		return -1;
	buf = malloc(bs);
	if (!buf) {
		printf("malloc failed!,please set heaptop bigger\n");
		return -1;
	}
	if((cmd=getenv("checksum")))
		cmd_buf = malloc(strlen(cmd)+1);
	for (idx=0;idx<count;idx++) {
		total=0;
		left=size;
		sum=0;

		if(cmd){
			strcpy(cmd_buf,cmd);
			do_cmd(cmd_buf);
		}

		fp0=open(fsrc,O_RDONLY);

		while(left) {
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

static int cmd_mymemcpy(int argc, char **argv)
{
	long long src, dst, count;
	int ret;
	if (argc != 4)
		return -1;
	src = nr_str2addr(argv[1], 0, 0);
	dst = nr_str2addr(argv[2], 0, 0);
	count = nr_strtoll(argv[3], 0, 0);
	ret = highmemcpy(dst, src, count);
	return ret;
}

#if NMOD_VGACON
static void cmd_led(int argc,char **argv)
{
	int led;
	led = strtoul(argv[1], 0, 0);
	pckbd_leds(led);
}
#endif

static int mycmp(int argc, char **argv)
{
	unsigned char *s1, *s2;
	int length, left;

	if (argc != 4)
		return -1;
	s1 = (unsigned char *)strtoul(argv[1], 0, 0);
	s2 = (unsigned char *)strtoul(argv[2], 0, 0);
	length = strtoul(argv[3], 0, 0);

	while ((left = bcmp(s1, s2, length))) {
		s1 = s1 + length - left;
		s2 = s2 + length - left;
		length = left;
		printf("[%p]!=[%p](0x%02x!=0x%02x)\n", s1, s2, *s1, *s2);
		s1++;
		s2++;
		length--;
	}
	return 0;
}

static int (*oldwrite)(int fd,char *buf,int len) = 0;
static char *buffer;
static int total;
extern void (*__msgbox)(int yy,int xx,int height,int width,char *msg);
void *restdout(int  (*newwrite) (int fd, const void *buf, size_t n));

static int newwrite(int fd, char *buf, int len)
{
//	if(stdout->fd==fd)
	{
		memcpy(buffer+total,buf,len);
		total += len;
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
	myline = heaptop;
	total = 0;
	buffer = heaptop + 0x100000;
	oldwrite = restdout(newwrite);
	myline[0] = 0;
	for(i=1;i<ac;i++) {
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

static const Cmd misc_cmds[] =
{
	{"MyCmds"},

	{"pcs",	"bus dev func", 0, "select pci dev function", mypcs, 0, 99, CMD_REPEAT},
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
	{"checksum","start size",0,"calculate checksum for a memory section",checksum,0,99,CMD_REPEAT},
	{"memcpy","src dst count",0,"mymemcpy src dst count",cmd_mymemcpy,0,99,CMD_REPEAT},
#if NMOD_VGACON
	{"led","n",0,"led n",cmd_led,2,2,CMD_REPEAT},
#endif
	{"mycmp","s1 s2 len",0,"mecmp s1 s2 len",mycmp,4,4,CMD_REPEAT},
#if NMOD_DISPLAY
	{"mymore","",0,"mymore",mymore,1,99,CMD_REPEAT},
#endif
	{0, 0}
};

static void misc_init_cmd __P((void)) __attribute__ ((constructor));

static void misc_init_cmd(void)
{
	cmdlist_expand(misc_cmds, 1);
}
#endif
