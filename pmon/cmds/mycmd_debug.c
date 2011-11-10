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

#include <errno.h>
#define nr_printf printf
#define nr_gets gets
#define nr_strtol strtoul
#define nr_strtoll strtoull
#if __mips >= 3
#define nr_str2addr str2addr
#else
#define nr_str2addr strtoul
#endif


#define ASID_INC	0x1
#define ASID_MASK	0xff

/*
 * PAGE_SHIFT determines the page size
 */
#ifdef CONFIG_PAGE_SIZE_4KB
#define PAGE_SHIFT	12
#endif
#ifdef CONFIG_PAGE_SIZE_16KB
#define PAGE_SHIFT	14
#endif
#ifdef CONFIG_PAGE_SIZE_64KB
#define PAGE_SHIFT	16
#endif
#define PAGE_SIZE	(1UL << PAGE_SHIFT)
#define PAGE_MASK	(~(PAGE_SIZE-1))
#define printk printf
#define CKSEG0 0xffffffff80000000ULL

static char *msk2str(unsigned int mask)
{
static char str[20];
	switch (mask) {
	case PM_4K:	return "4kb";
	case PM_16K:	return "16kb";
	case PM_64K:	return "64kb";
	case PM_256K:	return "256kb";
	case PM_1M:	return "1Mb";
	case PM_4M:	return "4Mb";
	case PM_16M:	return "16Mb";
	case PM_64M:	return "64Mb";
	case PM_256M:	return "256Mb";
	}
	sprintf(str,"%08x",mask);
	return str;
}

#define BARRIER()					\
	__asm__ __volatile__(				\
		".set\tnoreorder\n\t"			\
		"nop;nop;nop;nop;nop;nop;nop\n\t"	\
		".set\treorder");

void dump_tlb(int first, int last)
{
	unsigned long s_entryhi, entryhi, entrylo0, entrylo1, asid;
	unsigned int s_index, pagemask, c0, c1, i;

	s_entryhi = read_c0_entryhi();
	s_index = read_c0_index();
	asid = s_entryhi & 0xff;

	for (i = first; i <= last; i++) {
		write_c0_index(i);
		BARRIER();
		tlb_read();
		BARRIER();
		pagemask = read_c0_pagemask();
		entryhi  = read_c0_entryhi();
		entrylo0 = read_c0_entrylo0();
		entrylo1 = read_c0_entrylo1();

		/* Unused entries have a virtual address of CKSEG0.  */
		if ((entryhi & ~0x1ffffUL) != CKSEG0
		    && (entryhi & 0xff) == asid) {
			/*
			 * Only print entries in use
			 */
			printk("Index: %2d pgmask=%s ", i, msk2str(pagemask));

			c0 = (entrylo0 >> 3) & 7;
			c1 = (entrylo1 >> 3) & 7;

			printk("va=%011lx asid=%02lx\n",
			       (entryhi & ~0x1fffUL),
			       entryhi & 0xff);
			printk("\t[pa=%011lx c=%d d=%d v=%d g=%ld] ",
			       (entrylo0 << 6) & PAGE_MASK, c0,
			       (entrylo0 & 4) ? 1 : 0,
			       (entrylo0 & 2) ? 1 : 0,
			       (entrylo0 & 1));
			printk("[pa=%011lx c=%d d=%d v=%d g=%ld]\n",
			       (entrylo1 << 6) & PAGE_MASK, c1,
			       (entrylo1 & 4) ? 1 : 0,
			       (entrylo1 & 2) ? 1 : 0,
			       (entrylo1 & 1));
		}
	}
	printk("\n");

	write_c0_entryhi(s_entryhi);
	write_c0_index(s_index);
}


int tlbdump(int argc,char **argv)
{
	dump_tlb(0,63);
return 0;
}

/*
 * 0x40000000 maps to pci 0x40000000 by tlb,1G
 */

int tlb_init(int tlbs,int cachetype)
{
	int idx, pid;
	unsigned int phyaddr,viraddr;
	int eflag=0;
	int i;
	


	pid = read_c0_entryhi() & ASID_MASK;
	
	viraddr=0x00000000;
	phyaddr=0x00000000;

for(i=0;i<tlbs;i++)
{

	write_c0_pagemask(PM_16M);
	pid = read_c0_entryhi() & ASID_MASK;
	write_c0_entryhi(viraddr | (pid));
	idx = i;
	write_c0_index(i);
	write_c0_entrylo0((phyaddr >> 6)|7|(cachetype<<3)); //uncached,global
	write_c0_entrylo1(((phyaddr+(16<<20)) >> 6)|7|(cachetype<<3));
	write_c0_entryhi(viraddr | (pid));
	tlb_write_indexed();
	write_c0_entryhi(pid);
	viraddr += 32<<20;
	phyaddr += 32<<20;
	if(viraddr>=0x80000000)break;
}

return 0;
}
static int tlbset(int argc,char **argv)
{
	int idx, pid;
	unsigned int phyaddr,viraddr;
	int eflag=0;
	
	if(argc==4)
	{
		if(!strcmp("-x",argv[3]))eflag=1;
		else return -1;
	}
	else if(argc!=3)return -1;
	
	viraddr=strtoul(argv[1],0,0);
	phyaddr=strtoul(argv[2],0,0);
	write_c0_pagemask(PM_DEFAULT_MASK);


	pid = read_c0_entryhi() & ASID_MASK;
	

	viraddr &= (PAGE_MASK << 1);
	write_c0_entryhi(viraddr | (pid));
	tlb_probe();
	idx = read_c0_index();
	printf("viraddr=%08x,phyaddr=%08x,pid=%x,idx=%x\n",viraddr,phyaddr,pid,idx);
	write_c0_entrylo0((phyaddr >> 6)|0x1f);
	write_c0_entrylo1(((phyaddr+PAGE_SIZE) >> 6)|0x1f);
	write_c0_entryhi(viraddr | (pid));

	if(idx < 0) {
		tlb_write_random();
	} else {
		tlb_write_indexed();
	}
	write_c0_entryhi(pid);
    if(eflag)    __asm__ __volatile__ ("mtc0 %0,$22;"::"r"(0x4));
return 0;
}

static int tlbtest(int argc,char **argv)
{
	int idx, pid;
	unsigned int phyaddr,viraddr;
	int i;
	int eflag=0;
	unsigned int lo0,lo1,hi;
	
	
	if(argc!=3)return -1;

	viraddr=strtoul(argv[1],0,0);
	phyaddr=strtoul(argv[2],0,0);

for(i=0;i<64;i++)
{

	write_c0_pagemask(PM_DEFAULT_MASK);
	pid = read_c0_entryhi() & ASID_MASK;
	viraddr &= (PAGE_MASK << 1);
	write_c0_entryhi(viraddr | (pid));
	idx = i;
	write_c0_index(i);
	write_c0_entrylo0((phyaddr >> 6)|0x1f);
	write_c0_entrylo1(((phyaddr+PAGE_SIZE) >> 6)|0x1f);
	write_c0_entryhi(viraddr | (pid));
	if(idx < 0) {
		tlb_write_random();
	} else {
		tlb_write_indexed();
	}
	write_c0_entryhi(pid);
	tlb_read();

	lo0=read_c0_entrylo0();
	lo1=read_c0_entrylo1();
	hi=read_c0_entryhi();
	if(lo0!=((phyaddr >> 6)|0x1f))printf("idx %d:lo0 %x!=%x\n",i,lo0,(phyaddr >> 6)|0x1f);
	if(lo1!=(((phyaddr+PAGE_SIZE) >> 6)|0x1f))printf("idx %d:lo0 %x!=%x\n",i,lo1,((phyaddr+PAGE_SIZE) >> 6)|0x1f);
	if(hi!=(viraddr | (pid)))printf("idx %d:vadd %lx!=%lx\n",i,hi,viraddr | (pid));
	phyaddr=phyaddr+PAGE_SIZE*2;
	viraddr=viraddr+PAGE_SIZE*2;
}

return 0;
}

extern void CPU_TLBClear();
extern void CPU_TLBInit(unsigned int address,unsigned int steps);
static int tlbclear(int argc,char **argv)
{
CPU_TLBClear();
return 0;
}

static int tlbinit(int argc,char **argv)
{
	unsigned int addr,size;
	if(argc!=3)return -1;
if(!strcmp(argv[0],"tlbinit"))
{
	addr=strtoul(argv[1],0,0);
	size=strtoul(argv[2],0,0);
CPU_TLBInit(addr,size);
}
else
{
 tlb_init(strtoul(argv[1],0,0),strtoul(argv[2],0,0));
}
return 0;
}



static int cmd_cacheflush(int argc,char **argv)
{
unsigned long long addr;
unsigned int size,rw;
if(argc!=4)return -1;
addr=strtoull(argv[1],0,0);
size=strtoul(argv[2],0,0);
rw=strtoul(argv[3],0,0);

mycacheflush(addr,size,rw);
return 0;
}

static int cmd_cflush(int argc,char **argv)
{
unsigned long addr;
unsigned int size,rw;
if(argc!=4)return -1;
addr=strtoul(argv[1],0,0);
size=strtoul(argv[2],0,0);
rw=strtoul(argv[3],0,0);

CPU_IOFlushDCache(addr,size,rw);
return 0;
}

static int cmd_testcpu(int argc,char **argv)
{
int count=strtoul(argv[1],0,0);
asm(" .set noreorder\n"
"move $4,%0\n"
"li $2,0x80000000\n"
"1:\n"
".rept 300\n"
"ld $3,($2)\n"
"add.s $f4,$f2,$f0\n"
"mul.s $f10,$f8,$f6\n"
"srl $5,$4,10\n"
".endr\n"
"bnez $4,1b\n"
"addiu $4,-1\n"
".set reorder\n"
:
:"r"(count)
);
return 0;
}


static	unsigned long long lrdata;
static int lwl(int argc,char **argv)
{
	unsigned long long addr;
	if(argc!=2)return -1;
	addr=nr_str2addr(argv[1],0,0);
	  asm("lw  $2,%0;\n"
"		   lwl $2,(%1);\n"
"		   sw $2,%0\n"
		  ::"m"(lrdata),"r"(addr)
		  :"$2"
		 );
	  nr_printf("data=%016llx\n",lrdata);
	  return 0;
}
static int lwr(int argc,char **argv)
{
	unsigned long long addr;
	if(argc!=2)return -1;
	addr=nr_str2addr(argv[1],0,0);
	  asm("lw $2,%0;\n"
"		   lwr $2,(%1);\n"
"		   sw $2,%0\n"
		  ::"m"(lrdata),"r"(addr)
		  :"$2"
		 );
	  nr_printf("data=%016llx\n",lrdata);
	  return 0;
}

static int swl(int argc,char **argv)
{
	unsigned long long addr;
	if(argc!=2)return -1;
	addr=nr_str2addr(argv[1],0,0);
	  asm("lw  $2,%0;\n"
"		   swl $2,(%1);\n"
		  ::"m"(lrdata),"r"(addr)
		  :"$2"
		 );
	  return 0;
}
static int swr(int argc,char **argv)
{
	unsigned long long addr;
	if(argc!=2)return -1;
	addr=nr_str2addr(argv[1],0,0);
	  asm("lw $2,(%0);\n"
"		   swr $2,(%1);\n"
		  ::"r"(&lrdata),"r"(addr)
		  :"$2"
		 );
	  return 0;
}

#if __mips >= 3
static int ldl(int argc,char **argv)
{
	unsigned long long addr;
	if(argc!=2)return -1;
	addr=nr_str2addr(argv[1],0,0);
	  asm("ld $2,(%0);\n"
"		   ldl $2,(%1);\n"
"		   sd $2,(%0)\n"
		  ::"r"(&lrdata),"r"(addr)
		  :"$2"
		 );
	  nr_printf("data=%016llx\n",lrdata);
}
static int ldr(int argc,char **argv)
{
	unsigned long long addr;
	if(argc!=2)return -1;
	addr=nr_str2addr(argv[1],0,0);
	  asm("ld $2,(%0);\n"
"		   ldr $2,(%1);\n"
"		   sd $2,(%0)\n"
		  ::"r"(&lrdata),"r"(addr)
		  :"$2"
		 );
	  nr_printf("data=%016llx\n",lrdata);
}
#endif
static int linit(int argc,char **argv)
{
	if(argc>1)
	lrdata=strtoull(argv[1],0,0);
	  nr_printf("data=%016llx\n",lrdata);
	return 0;
}

#if 1
double sin(double);
static void testfloat()
{
volatile static double  x=1.12,y=1.34,z;
z=sin(x);
printf("sin(1.12)=%d\n",(int)(z*1000));
printf("sin(1.12)=%f\n",z);
z=sin(x)*y;
printf("sin(1.12)*1.34=%d\n",(int)(z*1000));
z=x*y;
printf("1.12*1.34=%d\n",(int)(z*1000));
}
#endif
static int mytest(int argc,char **argv)
{
#if 1
tgt_fpuenable();
testfloat();
#endif

	return 0;
}
#if __mips >= 3
static int cache_type=1;/*0:D,1:S*/

static int dumpcache(int argc,char **argv)
{
unsigned long taglo,taghi;
int way;
unsigned long long phytag;
unsigned long addr;
long len;

if(argc!=3)return -1;
addr=strtoul(argv[1],0,0);
len=strtoul(argv[2],0,0);

for(;len>0;len-=32,addr+=32)
{

 for(way=0;way<=3;way++)
 {
  
if(argv[0][0]=='s')
{
asm("cache %3,(%2);mfc0 %0,$28;mfc0 %1,$29":"=r"(taglo),"=r"(taghi):"r"(addr|way),"i"(7));
 phytag= ((((unsigned long long)taglo>>13)&0xfffff)<<17)|((((unsigned long long)taghi&0xf))<<36);
 printf("\n%08x:state=%d,phyaddr=%010llx,taglo=%08x,taghi=%08x\n",addr|way,(taglo>>10)&3,phytag,taglo,taghi);
}
else
{
asm("cache %3,(%2);mfc0 %0,$28;mfc0 %1,$29":"=r"(taglo),"=r"(taghi):"r"(addr|way),"i"(5));
 phytag= ((((unsigned long long)taglo>>8)&0xffffff)<<12)|((((unsigned long long)taghi&0xf))<<36);
 printf("\n%08x:state=%d,phyaddr=%010llx,way=%d,mode=%d,taglo=%08x,taghi=%08x\n",addr|way,(taglo>>6)&3,phytag,(taglo>>4)&3,(taghi>>29)&7,taglo,taghi);
 }
}

}

return 0;
}

#endif
#if PCI_IDSEL_CS5536 != 0
#include <target/cs5536.h>
static int cs5536_gpio(int argc,char **argv)
{
	unsigned long val;
	unsigned long tag;
	unsigned long base;
	unsigned long reg;
	typedef struct gpio_struc {char *s;unsigned int a;} gpio_struc;
	
	int i,j;
	gpio_struc gpio_params[]={
	{"out_val",GPIOL_OUT_VAL},
	{"out",GPIOL_OUT_EN},
	{"out_inv",GPIOL_OUT_INVRT_EN},
	{"out_od",GPIOL_OUT_OD_EN},
	{"out_aux1",GPIOL_OUT_AUX1_SEL},
	{"out_aux2",GPIOL_OUT_AUX2_SEL},
	{"pu",GPIOL_PU_EN},
	{"pd",GPIOL_PD_EN},
	{"in",GPIOL_IN_EN},
	{"in_readback",GPIOL_IN_READBACK},
	{"in_inv",GPIOL_IN_INVRT_EN},
	{"in_aux1",GPIOL_IN_AUX1_SEL},
	};

	tag = _pci_make_tag(0, 14, 0);
	base = _pci_conf_read(tag, 0x14);
	base |= 0xbfd00000;
	base &= ~3;


	reg=strtoul(argv[1],0,0);
	printf("base=%x,reg=%x\n",base,reg);
	if(reg>15){reg -= 16;base += 0x80; }
	if(argc>2)
	for(i=2;i<argc;i++)
	{

	for(j=0;j<sizeof(gpio_params)/sizeof(gpio_params[0]);j++)
	 
	 if(!strncmp(argv[i],gpio_params[j].s,strlen(gpio_params[j].s)))
	 {
	  if(*(argv[i]+strlen(gpio_params[j].s))==':')
	  {
	  int val=(strtoul(argv[i]+strlen(gpio_params[j].s)+1,0,0)!=0);
	  //printf("%s%d,%x\n",gpio_params[j].s,val,base+gpio_params[j].a);
		
	  if(val) *(volatile unsigned int *)(base+gpio_params[j].a) = (1<<reg);
	  else *(volatile unsigned int *)(base+gpio_params[j].a) = (1<<(16+reg));
	  break;
	  }
	  else if(*(argv[i]+strlen(gpio_params[j].s))=='?')
	  {
	   printf("%s=%d\n",gpio_params[j].s,(*(volatile unsigned int *)(base+gpio_params[j].a)>>reg)&1);
	  break;
	  }

	 }
	}
	else
	{
	for(j=0;j<sizeof(gpio_params)/sizeof(gpio_params[0]);j++)
	   printf("%s=%d\n",gpio_params[j].s,(*(volatile unsigned int *)(base+gpio_params[j].a)>>reg)&1);
	}

	return 0;
}
#endif

//----------------------------------
static const Cmd Cmds[] =
{
	{"MyCmds"},
#if __mips >= 3
	{"scachedump",	"", 0, "access Scache tag",dumpcache, 0, 99, CMD_REPEAT},
	{"dcachedump",	"", 0, "access Dcache tag",dumpcache, 0, 99, CMD_REPEAT},
#endif
	{"tlbset","viraddr phyaddr [-x]",0,"tlbset viraddr phyaddr [-x]",tlbset,0,99,CMD_REPEAT},
	{"tlbtest","viraddr phyaddr ",0,"tlbset viraddr phyaddr ",tlbtest,0,99,CMD_REPEAT},
	{"tlbclear","",0,"tlbclear",tlbclear,0,99,CMD_REPEAT},
	{"tlbdump","",0,"tlbdump",tlbdump,0,99,CMD_REPEAT},
	{"tlbinit","addr size",0,"tlbinit phaddr=vaddr",tlbinit,0,99,CMD_REPEAT},
	{"tlbinit1","tlbs type",0,"tlbinit fill all tlb from 0 with cachecoherence type.",tlbinit,0,99,CMD_REPEAT},
	{"cacheflush","addr size rw",0,"cacheflush addr size rw",cmd_cacheflush,0,99,CMD_REPEAT},
	{"cflush","addr size rw",0,"cflush addr size rw",cmd_cflush,0,99,CMD_REPEAT},
	{"testcpu","",0,"testcpu",cmd_testcpu,2,2,CMD_REPEAT},
	{"lwl","n",0,"lwl n",lwl,2,2,CMD_REPEAT},
	{"lwr","n",0,"lwr n",lwr,2,2,CMD_REPEAT},
	{"swl","n",0,"swl n",swl,2,2,CMD_REPEAT},
	{"swr","n",0,"swr n",swr,2,2,CMD_REPEAT},
#if __mips >= 3
	{"ldl","n",0,"ldl n",ldl,2,2,CMD_REPEAT},
	{"ldr","n",0,"ldr n",ldr,2,2,CMD_REPEAT},
#endif
	{"linit","",0,"linit",linit,1,2,CMD_REPEAT},
	{"mytest","",0,"mytest",mytest,1,1,CMD_REPEAT},
#if PCI_IDSEL_CS5536 != 0
	{"cs5536_gpio","reg out:? in:? out_aux1:? out_aux2:? in_aux1:? pu:? val:?",0,"set cs5536 gpio",cs5536_gpio,2,99,CMD_REPEAT},
#endif
	{0, 0}
};


static void init_cmd __P((void)) __attribute__ ((constructor));

static void
init_cmd()
{
	cmdlist_expand(Cmds, 1);
}


