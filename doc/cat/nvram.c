/*************************************************************
 * File: lib/nvram.c
 * Purpose: Part of C runtime library
 * Author: Phil Bunce (pjb@carmel.com)
 * Revision History:
 *	970304	Start of revision history
 *	970311	Added flush_cache(DCACHE) for 4010 copy-back
 *	970605	Added writeFlash
 *	970620	Cygnus requires Uchar in flash29.
 *	970910	Added support for Am29F080.
 *	971124	Don't create msg for famcode=0000.
 *	971125	Added code to writeFlash for 4011 copy from boot eprom
 *	980114	Added delay to start of nvType for GHS+4011.
 *	980120	Replaced outb() macro with a function. Fixes prob w
 *		nvType check on 4011. Removed delay.
 *	980324	Fixed problem with outb. It wasn't doing the right thing.
 *	980508	Added needcpy.
 *	980508	Flush D before I in nvCheck.
 *	980509	Discovered that for needcpy=1 outb MUST be a macro.
 *	980509	The new outb macro works on 4011 but fails on 4101.
 *	980510	Reverted to old outb macro until real fix is found.
 *	980602	Delay shrc execution until after the powerup banner.
 *		Do this by setting shrc_needed.
 *	980715	Modified do_shrc and shrc to support 2 separate init
 *		blocks. Before p2 is executed before the banner. After
 *		p2 is executed after the banner. Only commands that
 *		were specified in the "set nvram" command are located
 *		after the p2.
 */

/************************************************************* 
 * Performance data:
 *	3 secs to erase one sector
 *	128 secs/MB for writes
 */

#include <stdio.h>
#include <mips.h>
#include <termio.h>
#include <mon.h>


#define inb(x)	(*(volatile Uchar *)(x))

#if 0
#define outb(a,v)   *((Uchar *)(a))=((Uchar)(v))
#else
/*************************************************************
*  outb(a,v)   
*	store byte with write buffer flush.
*	For 4011 you MUST use Kseg1 addresses for tmp1 and tmp2.
*	980324 Added & for wbflushtmpX and volatile for wbftX.
*/
#define outb(a,v)   					\
{							\
volatile int *wbft1,*wbft2;				\
							\
*((Uchar *)(a))=((Uchar)(v));				\
wbft1 = (int *)(((Ulong)&wbflushtmp1)|K1BASE);		\
wbft2 = (int *)(((Ulong)&wbflushtmp2)|K1BASE);		\
*wbft1 = 0;						\
*wbft2 = *wbft1;					\
}
#endif

int nvType(),nvType_end();
int flash28(),flash28end();
int flash29(),flash29end();
int xl28(),xl28end();

NvType nvTypes[] = {
	{0x01a2,"Am28F010",0,      128*1024,flash28,flash28end}, 
	{0x01a7,"Am28F010",0,	   128*1024,flash28,flash28end},
	{0x89b4,"i28F010", 0,	   128*1024,flash28,flash28end},
	{0x89bd,"i28F020", 0,	   256*1024,flash28,flash28end},
	{0x0120,"Am29F010",16*1024,128*1024,flash29,flash29end},
	{0x01a4,"Am29F040",64*1024,512*1024,flash29,flash29end},
	{0xc0a4,"Am29F040",64*1024,512*1024,flash29,flash29end},
	{0x01d5,"Am29F080",64*1024,1024*1024,flash29,flash29end},
	{0x1f5b,"AT29F040",64*1024,512*1024,flash29,flash29end}, /* atmel */
	{0x5010,"XL28C64", 1,	   1024,xl28,xl28end}, 
	{0}};

NvInfo nvInfo;


#define SPC_BITMSK (3<<21)
#define SPC_8BIT   (1<<21)
#define SPC_16BIT  (2<<21)
#define SPC_32BIT  (3<<21)

int machtype;
volatile int wbflushtmp1;
volatile int wbflushtmp2;
int shrc_needed;

#ifdef TEST

/*************************************************************
*  main()
*/
main(argc,argv)
int argc;
char *argv[];
{
int n,i,j,c;
char nvtype[40];

machtype = getmachtype();
if (!nvCheck(nvtype)) printf("nvCheck FAILED\n");;
printf("nvram: %s\n",nvtype);

printf("reading: ");
for (i=0;;i++) {
	c = nvRead(i);
	if (c == 0 || c == 0xff) break;
	putchar(c);
	}
putchar('\n');

if (argc == 1) return;

for (j=1,i=0;j<argc;j++) {
	if (argv[j][0] == '-') {
		if (argv[j][1] == 'c') nvClear();
		else if (argv[j][1] == 'v') {
			j++;
			for (i=0;j<argc;j++) {
				for (p=argv[j];*p;p++) nvWrite(i++,*p);
				nvWrite(i++,' ');
				}
			}
		else {
			printf("%s: bad option\n",argv[j][1]);
			printf("usage: [-c] [-v msg]\n");
			exit(1);
			}
		}
	else {
		printf("%s: bad argument\n",argv[j]);
		printf("usage: [-c] [-v msg]\n");
		exit(1);
		}
	}

printf("reading: ");
for (i=0;;i++) {
	c = nvRead(i);
	if (c == 0 || c == 0xff) break;
	putchar(c);
	}
putchar('\n');
}
#endif

/*************************************************************
*  int nvCheck(char *msg)
*/
nvCheck(msg)
char *msg;
{
Ulong r;
int len,i,famcode;
Func *f;
int needcpy;

*msg = 0;
if (nvInfo.nvbase) return(1);

#if 0
/* check to see if I am executing out of flash */
/* do this by checking if I am in the same 8MB as nvInfo.start */
/* if so, I must copy some routines to RAM before executing them */
if ((((Ulong)nvCheck)&~(0x800000-1)) == nvInfo.start&~(0x800000-1)) needcpy=1;
else needcpy = 0;
#else
needcpy=1;
#endif

/* first, find out what nvType we have */
if (needcpy) {
	len = ((Ulong)nvType_end)-((Ulong)nvType);
	f = (Func *)malloc(len + jalBytes(nvType,len/4));
	if (!f) {
		strcpy(msg,"malloc failure 1");
		return(0);
		}
	codecpy(f,nvType,len/4);
	flush_cache(DCACHE);
	flush_cache(ICACHE);
	famcode = (* f)();
	free(f);
	}
else famcode = nvType();

/* look up the famcode in the nvTypes table */
for (i=0;nvTypes[i].driver;i++) {
	if (nvTypes[i].famcode == famcode) break;
	}
if (!nvTypes[i].driver) { /* famcode not found */
	nvInfo.nvbase = 0;
	if (msg && famcode != 0) sprintf(msg,"%04x: Bad flashcode",famcode);
	return(0);
	}
nvInfo.name = nvTypes[i].name;

/* now, install the correct driver */
if (needcpy) {
	len = ((Ulong)nvTypes[i].edriver)-((Ulong)nvTypes[i].driver);
	f = (Func *)malloc(len + jalBytes(nvTypes[i].driver,len/4));
	if (!f) {
		strcpy(msg,"malloc failure 2");
		return(0);
		}
	codecpy(f,nvTypes[i].driver,len/4);
	flush_cache(DCACHE);
	flush_cache(ICACHE);
	nvInfo.driver = f;
	}
else nvInfo.driver = nvTypes[i].driver;

nvInfo.size = nvTypes[i].size;
nvInfo.type = &nvTypes[i];

/* return if this device doesn't support sector erase */
if (!nvTypes[i].se) {
	nvInfo.nvbase = 0;
	if (msg) strcpy(msg,"Function not supported");
	return(0);
	}

/* calculate the correct value for nvbase */
if (nvInfo.nvbase == 0) {
	len = nvInfo.size*nvInfo.width;
	len -= nvTypes[i].se*nvInfo.width;
#if 0 /* we aught to check if there's enough room */
	if (len <= PMONsize) {
		nvInfo.nvbase = 0;
		if (msg) strcpy(msg,"Insufficient space");
		return(0);
		}
#endif
	nvInfo.nvbase = nvInfo.start + len;
	len -= nvTypes[i].se*nvInfo.width;
	nvInfo.dvrsa = nvInfo.start + len;
	}
/*printf("famcode=%04x ",famcode); /*  */
if (msg) strcpy(msg,nvInfo.name);
return(1);
}

/*************************************************************
*  nvWrite(adr,val)
*/
nvWrite(adr,val)
Ulong adr;
int val;
{
if (!nvInfo.nvbase) return;
if (adr == 0) nvClear();
(* nvInfo.driver)(NV_WRITE,nvInfo.nvbase+(adr*nvInfo.gap),val);
}

/*************************************************************
*  nvRead(adr)
*/
nvRead(adr)
Ulong adr;
{
if (!nvInfo.nvbase) return(0);
return inb(nvInfo.nvbase+(adr*nvInfo.gap));
}

/*************************************************************
*  nvClear()
*/
nvClear()
{
int i;

if (!nvInfo.nvbase) return;
if (nvInfo.start == nvInfo.nvbase) (* nvInfo.driver)(NV_ERASE,nvInfo.start);
else {
	for (i=0;i<nvInfo.width;i++) {
		(* nvInfo.driver)(NV_SERASE,nvInfo.nvbase+i);
		}
	}
}

/*************************************************************
*  nvType()
*	determine family code
*	what we do know at start:
*		start address of device
*		gap
*		width
*/
nvType()
{
Ulong adrA,adrB,adr0,adr1;
Ulong msk;
int sh,b1,b2,h0,h1;

sh = nvInfo.width/2;
adrA = (nvInfo.start|(0x5555<<sh));
adrB = (nvInfo.start|(0x2aaa<<sh));
adr0 = (nvInfo.start|(0x0000<<sh));
adr1 = (nvInfo.start|(0x0001<<sh));
b1 = inb(adr0);
b2 = inb(adr1);
h0 =  (b1<<8)|b2; /* read the existing value */

outb(adrA,0xaa); outb(adrB,0x55); outb(adrA,0x90); 
b1 = inb(adr0);
b2 = inb(adr1);
outb(adrA,0xaa); outb(adrB,0x55); outb(adrA,0xf0); 
h1 =  (b1<<8)|b2; /* read the famcode */

if (h0 == h1) return(0); /* must be eprom */
return(h1);
}
nvType_end() {}

/*************************************************************
*  flash28(op,adr,val) 
*/
flash28(op,adr,val) 
int op,val;
Ulong adr;
{
Uchar pvd;
Ulong X,cs;
int plscnt,dev,sh,i;
		 
printf("flash28: op=%d adr=%08x val=%02x\n",op,adr,val&0xff);
switch (op) {
	case NV_WRITE : 
		printf("flash28w: NOT TESTED\n");
		cs = nvInfo.nvbase&~3;
		sh = nvInfo.width/2;
		dev = adr&((1<<sh)-1);
		X = cs|dev; /* any address within the device */
		printf("nvbase=%08x X=%08x\n",nvInfo.nvbase,X);
		for (plscnt=0;plscnt<25;plscnt++) {
			outb(X,0x40); /* prog */
			outb(adr,val);
			for (i=500;i>0;i++) wbflushtmp1 = 0; /* 10us delay */
			outb(X,0xc0); /* prog verify */
			for (i=300;i>0;i++) wbflushtmp1 = 0; /* 6us delay */
			pvd = inb(X); 
			if (val == pvd) break;
			}
		printf("pvd=%02x plscnt=%d\n",pvd,plscnt);
		/* outb(X,0xff); outb(X,0xff); outb(X,0x00); /* reset-rd */
		if (plscnt >= 25) return(0);
		return(1);
	case NV_ERASE :
		printf("flash28e: NOT IMPLEMENTED\n");
		break;
	case NV_SERASE :
		printf("flash28se: NOT supported by device\n");
		break;
	}
}
flash28end() {}

#define P29MAX	10 /* max polling count for 29F devices */

int diagcnt;

/*************************************************************
*  flash29(op,adr,val) 
*/
flash29(op,adr,val) 
int op;
Ulong adr;
Uchar val;
{
Ulong adrA,adrB,adr0,adr1;
Uchar epd;
int sh,cnt,i;

switch (op) {
	case NV_WRITE : 
		sh = nvInfo.width/2;
		adrA = ((adr&~(0x1ffff<<sh))|(0x5555<<sh));
		adrB = ((adr&~(0x1ffff<<sh))|(0x2aaa<<sh));
#if 0
		printf("flash29w: adr=%08x val=%02x adrA=%08x adrB=%08x\n",
			adr,val,adrA,adrB);
#endif
		outb(adrA,0xaa); outb(adrB,0x55); outb(adrA,0xa0); 
		outb(adr,val);
		for (i=0;i<3000;i++) wbflushtmp1 = 0; /* min 100us delay */
		for (cnt=0;;cnt++) {
			epd = inb(adr);
			if ((epd&0x80) == (val&0x80) && epd == val) break; 
			if (epd&(1<<5)) break;
			}
#if 0
		if (epd != val) printf("flash29w: ## TIMEOUT\n");
#endif
		if (epd != val) return(0);
		return(1);
		break;
	case NV_SERASE : 
		sh = nvInfo.width/2;
		adrA = ((adr&~(0x1ffff<<sh))|(0x5555<<sh));
		adrB = ((adr&~(0x1ffff<<sh))|(0x2aaa<<sh));
#if 0
		printf("flash29se: adr=%08x adrA=%08x adrB=%08x\n",
			adr,adrA,adrB);
#endif
		/* unlock, setup */
		outb(adrA,0xaa); outb(adrB,0x55); outb(adrA,0x80); 
		/* unlock, sectEra */
		outb(adrA,0xaa); outb(adrB,0x55); outb(adr,0x30); 
		for (i=0;i<3000;i++) wbflushtmp1 = 0; /* min 100us delay */
		val = 0xff;
		for (cnt=0;cnt<5000000;cnt++) { /* cnt=1.3M is normal */
			epd = inb(adr);
			if ((epd&0x80) == (val&0x80) && epd == val) break; 
			if (epd&(1<<5)) break;
			}
#if 0
		if (epd != val) printf("flash29se: ## TIMEOUT\n");
		printf("flash29se: epd=%02x cnt=%d\n",epd,cnt);
#endif
		if (epd != val) return(0);
		return(1);
		break;
	case NV_ERASE :
		printf("flash29e: NOT IMPLEMENTED\n");
		break;
	default: printf("flash29: %d: bad opcode\n",op);
	}
}
flash29end() {}

#define XL28_TIMEOUT 100000

/*************************************************************
*  xl28(op,adr,val) 
*/
xl28(op,adr,val) 
int op,val;
Ulong adr;
{
int i,n;
Ulong a;

switch (op) {
	case NV_WRITE :
		outb(adr,val);
		for (i=0;i<XL28_TIMEOUT && inb(adr) != val;i++) wbflushtmp1=0;
		if (i >= XL28_TIMEOUT) return(0);
		break;
	case NV_ERASE :
	case NV_SERASE :
		/* clear nvram */
		val = 0xff;
		for (n=0;n<nvInfo.size;n++) {
			if (nvRead(n) != val) {
				a = nvInfo.nvbase+(n*nvInfo.gap);
				outb(a,val);
				for (i=0;i<XL28_TIMEOUT && inb(adr) != val;i++) 
					wbflushtmp1=0;
				if (i >= XL28_TIMEOUT) return(0);
				}
			}
		break;
	}

}
xl28end() {}


/*************************************************************
*  do_shrc()
*/
do_shrc()
{
int c,n;
char ch,buf[80];

c = 'n';
ioctl(STDIN,FIONREAD,&n);
if (n != 0) {
	for (;;) {
		ioctl(STDIN,FIONREAD,&n);
		if (n == 0) break;
		read(STDIN,&ch,1);
		}
	for (;;) {
		printf("Skip NVRAM read? (y/n)? >");
		gets(buf);
		c = *buf;
		if (c == 'n' || c == 'y') break;
		if (c == 'N' || c == 'Y') break;
		}
	}
if (c == 'Y' || c == 'y') return;
shrc(0);
shrc_needed = 1; 
}


/*************************************************************
*  shrc(n)
*	n=0 do part1
*	n=1 do part2
*/
shrc(n)
int n;
{
int c;
char buf[LINESZ];
unsigned long adr;

adr = 0;
for (;;) {
	c = getln(&adr,buf);
	if (c == 0) break;
	if (strequ(buf,"p2")) {
		if (n == 0) break;
		if (n == 1) n++;
		}
	else if (n == 2 || n == 0) do_cmd(buf);
	}
}

/*************************************************************
*  getln(adr,p)
*/
getln(adr,p)
unsigned long *adr;
unsigned char *p;
{
unsigned int c = 0;
long addr;

addr = *adr;
for (;;) {
	c = nvRead(addr);
	if (c == 0xff) c = 0;
	if (c == 0) break;
	addr++;
	if (c == '\n') break;
	*p++ = c;
	}
*adr = addr;

*p = 0;
return(c);
}

/*************************************************************
*  writeFlash(adr,val)
*	This is used by self-updating flashes (xflash.c)
*/
writeFlash(adr,val)
Ulong adr;
Uchar val;
{
int se,width;
Ulong msk;
char nvtype[20];

if (!nvInfo.type) return(0);

se = nvInfo.type->se;
width = nvInfo.width;
msk = ((se*width)-1) & ~(width-1);

if ((adr&msk) == 0) (* nvInfo.driver)(NV_SERASE,adr);
(* nvInfo.driver)(NV_WRITE,adr,val);
return(0);
}

#if 0
/*************************************************************
*/
printNvInfo()
{

printf("name=%s start=%08x width=%d size=0x%x gap=%d nvbase=%08x driver=%08x\n",
	nvInfo.name, nvInfo.start, nvInfo.width, nvInfo.size, nvInfo.gap,
	nvInfo.nvbase,nvInfo.driver);

if (nvInfo.type)
	printf("famcode=%04x name=%s se=0x%x size=0x%x driver=%08x\n", 
		nvInfo.type->famcode, nvInfo.type->name, nvInfo.type->se, 
		nvInfo.type->size,nvInfo.type->driver);
}
#endif
