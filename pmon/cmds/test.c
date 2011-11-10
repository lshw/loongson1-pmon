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

/* header files */
#include <linux/zlib.h>
#include <stdio.h>
#include <stdarg.h>
static int cmd_test(int ac,char **av)
{
long tests;
int i;
printf("test %s\n",av[1]);
tests=strtoul(av[1],0,0);
for(i=0;i<31;i++)
{
	if(!tests&i)break;
	switch(i)
	{
		case 0:break;
	}
}

return 0;
}

//-------------------------------------------------------------------------------------------
static const Cmd Cmds[] =
{
	{"MyCmds"},
	{"test","val",0,"hardware test",cmd_test,0,99,CMD_REPEAT},
	{0, 0}
};

static void init_cmd __P((void)) __attribute__ ((constructor));
static void init_cmd()
{
	cmdlist_expand(Cmds, 1);
}
