#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/param.h>
#include <sys/types.h>
#include <sys/errno.h>
#include <sys/endian.h>

#include <pmon.h>
#include <exec.h>
#include <pmon/loaders/loadfn.h>

#ifdef __mips__
#include <machine/cpu.h>
#endif


int cmd_initrd (int ac, char *av[])
{
	int flags = 0;
	int ret;
	unsigned int rd_start;
	
	if (ac != 2 && ac!=3)
		return -1;

	if(ac==3)rd_start=strtoul(av[2],0,0);
	else rd_start = 0x84000000;

	flags |= RFLAG;

	ret = boot_initrd(av[1],rd_start, flags);
	if (ret == 0)
	{
		return EXIT_SUCCESS;
	}
	
	return EXIT_FAILURE;
}

/*
 *  Command table registration
 *  ==========================
 */

static const Cmd GrubCmd[] =
{
	{"grub like command"},
	{"initrd",	"initrd/initramfs path",
			0,
			"load initrd/initramfs image",
			cmd_initrd, 2, 3, CMD_REPEAT},
	{0, 0}
};

static void init_cmd __P((void)) __attribute__ ((constructor));

static void
init_cmd()
{
	cmdlist_expand(GrubCmd, 1);
}

