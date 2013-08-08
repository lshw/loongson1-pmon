#include <pmon.h>
#include <cpu.h>
#include <string.h>

#include <sys/device.h>

int usb_probe(int argc,char **argv)
{
//	struct device ehci_dev;
	
//	usb_init(&ehci_dev);
	return 0;
}

static const Cmd Cmds[] =
{
	{"MyCmds"},
	{"usb_probe","", 0, "usb probe", usb_probe, 0, 99, CMD_REPEAT},
	{0, 0}
};

static void init_cmd __P((void)) __attribute__ ((constructor));

static void
init_cmd()
{
	cmdlist_expand(Cmds, 1);
}
