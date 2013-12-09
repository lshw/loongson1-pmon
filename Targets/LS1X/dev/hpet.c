#include <machine/pio.h>
#include <stdio.h>
#include <pmon.h>
#include <cpu.h>


#define	GS232_HPET_BASE	0xbfe6c000
#define GCAP_L	(GS232_HPET_BASE+0x0)
#define GCAP_H	(GS232_HPET_BASE+0x4)
#define GCON	(GS232_HPET_BASE+0x10)
#define GINTS	(GS232_HPET_BASE+0x20)
#define MCOUNT	(GS232_HPET_BASE+0xf0)
#define T0_CON	(GS232_HPET_BASE+0x100)
#define T0_CMP	(GS232_HPET_BASE+0x108)
#define T1_CON	(GS232_HPET_BASE+0x120)
#define T1_CMP	(GS232_HPET_BASE+0x128)
#define T2_CON	(GS232_HPET_BASE+0x140)
#define T2_CMP	(GS232_HPET_BASE+0x148)




void hpet_init(void)
{
	writel(0x0,MCOUNT);
	
	writel(0x1ffffff,T0_CMP);
	writel(0x2fffffff,T1_CMP);
	writel(0x3fffffff,T2_CMP);

	writel((readl(T0_CON) | 0x4),T0_CON);
	writel((readl(T1_CON) | 0x4),T1_CON);
	writel((readl(T2_CON) | 0x4),T2_CON);

	writel((readl(GCON) | 0x1),GCON);
}

void hpet_intr(void)
{
	printf("===hpet int sts: %x\n",readl(GINTS));
	writel(0x7,GINTS);
}

int check_intr(void)
{
	if((readl(0xbfd01040) & 0x2) != 0)
	{
		printf("==get hpet intr in confreg: %08x\n",readl(0xbfd01040));
		writel(0x2,0xbfd01058);
		return 1;
	}

	return 0;
}

void get_hpet_sts(void)
{
	long tmp = readl(MCOUNT);
	
	printf("===t0_cmp: %x\n",readl(T0_CMP));

	printf("===hpet int sts: %x\n",readl(GINTS));
	printf("===hpet MCOUNT: %x\n",tmp);
}

void hpet_test(void)
{
	hpet_init();

	while(1)
		get_hpet_sts();
	while(!check_intr())
		;
//	hpet_intr();

}

#if 0
static const Cmd Cmds[] =
{
	{"MyCmds"},
	{"hpet_test","",0,"hpet_test",hpet_test,0,99,CMD_REPEAT},
	{0, 0}
};

static void init_cmd __P((void)) __attribute__ ((constructor));

static void
init_cmd()
{
	cmdlist_expand(Cmds, 1);
}
#endif

