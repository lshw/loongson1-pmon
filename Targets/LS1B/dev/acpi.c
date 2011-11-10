#include <pmon.h>
#include <linux/types.h>
#define udelay delay

#define writel(val, addr) (*(volatile u32*)(addr) = (val))
#define readl(addr) (*(volatile u32*)(addr))

#define APB_BASE	0xbfe40000
#define ACPI_BASE	0x3c000+APB_BASE
#define GEN_PMCON_1 ACPI_BASE+0x30
#define GEN_PMCON_2 ACPI_BASE+0x34
#define GEN_PMCON_3 ACPI_BASE+0x38
#define PM1_STS		ACPI_BASE+0x0
#define PM1_EN		ACPI_BASE+0x4
#define PM1_CNT		ACPI_BASE+0x8
#define PM1_TMR		ACPI_BASE+0xc
#define PROC_CNT	ACPI_BASE+0x10

#define GPE_STS		ACPI_BASE+0x20
#define GPE_EN		ACPI_BASE+0x24


static unsigned long wakeup_state_address = 0xa01FFC00;


int acpi_str(int argc,char **argv)
{
	u32 reg;
	unsigned long flags = 0;
	asm volatile(  
	"move	$8, %0\n\t"
	"la $9,	1f\n\t"
	"sw	$9, ($8)\n\t"
	"sw	$29, 4($8)\n\t"
	"sw	$30, 8($8)\n\t"
	"sw	$28, 12($8)\n\t"
	"sw	$16, 16($8)\n\t"
	"sw	$17, 20($8)\n\t"
    "sw	$18, 24($8)\n\t"
    "sw	$19, 28($8)\n\t"
    "sw	$20, 32($8)\n\t"
    "sw	$21, 36($8)\n\t"
	"sw	$22, 40($8)\n\t"
    "sw	$23, 44($8)\n\t"

    "sw	$26, 48($8)\n\t"//k0
    "sw	$27, 52($8)\n\t"//k1

    "sw	$2, 56($8)\n\t"//v0
    "sw	$3, 60($8)\n\t"//v1

	"mfc0	$9, $12\n\t"
	"mfc0	$10, $4\n\t"
	"sw	$9, 64($8)\n\t"
	"sw	$10, 68($8)\n\t"
	"1:"
	:
	:"r" (wakeup_state_address)
	:"$8","$31","$9"
	);

		reg = readl((void *)PM1_CNT);
		reg &= (7<<10);
		if(reg!=0)
			goto finish;
		/* Save wakeup_state to low memory*/
		printf("ACPI COPY CONTEXT\n");
		int i;
		volatile unsigned int *tmp1;
		volatile unsigned int tmp2;
		for(i=0;i<18;i++){
			tmp1 =  (unsigned int *)(wakeup_state_address+4*i);
			tmp2 =  *tmp1;
			printf("%x: %x\n ",tmp1, tmp2);
		}

	//local_irq_save(flags);
	/* Clear WAK_STS */
	reg = readl((void *)PM1_STS);
	reg |= (1<<15);
	writel(reg, (void *)PM1_STS);
	/* clear pm1_sts*/
	writel(0xffffffff,(void *)PM1_STS);
	/* get PM1A control */
	reg = readl((void *)PM1_CNT);
	/* Clear the SLP_EN and SLP_TYP fields */
	reg &= ~(15<<10);
	writel(reg, (void *)PM1_CNT);
	/* Insert the SLP_TYP bits */
	reg |= ((3+2)<<10);
	/* Write #1: write the SLP_TYP data to the PM1 Control registers */
	writel(reg, (void *)PM1_CNT);
	/* Insert the sleep enable (SLP_EN) bit */
	//reg |= (1<<13);
	/* Flush caches, as per ACPI specification */
	flushcache();
	/* Write #2: Write both SLP_TYP + SLP_EN */
	//writel(reg, (void *)PM1_CNT);
//	((void (*)(void))ioremap_nocache(0x1fc00480, 4)) ();
	printf("ACPI ENTER\n");
	#if 1
	asm volatile(
		"li	$26,	0xbfc00480\n\t"
		"jr	$26\n\t"
		"nop"
	);
	#endif

	/* Wait until we enter sleep state */
	do{}while(1);
	/*Hibernation should stop here and won't be executed any more*/

	/*=====================WAKE UP===========================*/
finish:
	//local_irq_restore(flags);
	printf("ACPI FINISH\n");
		writel(-1, (void *)PM1_STS);
		writel(-1, (void *)GPE_STS);
		writel(0, (void *)PM1_CNT);
	return 0;
}

static const Cmd Cmds[] =
{
	{"MyCmds"},
	{"acpi_str","",0,"acpi_str",acpi_str,0,99,CMD_REPEAT},
	{0, 0}
};

static void init_cmd __P((void)) __attribute__ ((constructor));

static void
init_cmd()
{
	cmdlist_expand(Cmds, 1);
}

