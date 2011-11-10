#ifdef _KERNEL
#undef _KERNEL
#include <sys/param.h>
#include <sys/socket.h>
#include <sys/errno.h>
#include <net/if.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <sys/protosw.h>
#include <sys/ioctl.h>
#define _KERNEL
#else
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <netinet/in.h>
#endif
#include <net/route.h>

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <pmon.h>

#include "../netio/bootparams.h"
#include "../netio/bootp.h"

static int	testbootp(int argc, char* argv[])
{
	struct bootparams bootp;
	bzero(&bootp, sizeof(struct bootparams));
	bootp.need = BOOT_ADDR;

	printf("bootp starts...\n");
	boot_bootp(&bootp, "rtl0");
	printf("bootp finished...\n");
	return 0;
}



/*
 *  *  Command table registration
 *   *  ==========================
 *    */

static const Cmd Cmds[] =
{
	{"Network"},
	{"bootp",    "", 0,
		"bootp",
		testbootp, 1, 99, 0},
	{0, 0}
};

static void init_cmd __P((void)) __attribute__ ((constructor));

static void
init_cmd()
{
	cmdlist_expand(Cmds, 1);
}



