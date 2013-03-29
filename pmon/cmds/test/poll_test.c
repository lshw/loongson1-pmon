#include <pmon.h>
#include <cpu.h>

/* 轮询测试, pmon中cpu运行于32位模式下，是关中断运行.该程序用于测试某个gpio引脚是否被拉低 */

static void init_cmd __P((void)) __attribute__ ((constructor));

int button_poll(void *unused)
{
	int ret;
/*	int cause;
	volatile int *p=0xbfe0011c;
	asm("mfc0 %0,$13":"=r"(cause));
	if(cause&(1<<10))tgt_poweroff();*/
	ret = !gpio_get_value(38);
	if(ret) {
		printf("+++++++++++++++++++^^+++++++%x\n", ret);
	}
	return 0;
}

static void init_cmd()
{
	ls1x_gpio_direction_input(38);
	/* level, func, arg */
	tgt_poll_register(1, button_poll, 0);
//	cmdlist_expand(Cmds, 1);
}

