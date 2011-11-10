#include <pmon.h>
#include <cpu.h>

#define TSC2007_CMD(cmd,pdn,m) (((cmd) << 4) | ((pdn) << 2) | ((m) << 1))
/* converter functions */

#define	MEAS_TEMP0 0
#define	MEAS_AUX 2
#define	MEAS_TEMP1 4 
#define	ACTIVATE_X_DRIVERS 8 
#define	ACTIVATE_Y_DRIVERS 9 
#define	ACTIVATE_YnX_DRIVERS 10 
#define	MEAS_XPOS 12
#define	MEAS_YPOS 13
#define	MEAS_Z1 14
#define	MEAS_Z2 15

#define	PD_POWERDOWN_ENABLEPENIRQ 0 /* ADC off & penirq */ 
#define	PD_ADCON_DISABLEPENIRQ 1 /* ADC on & no penirq */ 

/* resolution modes */

#define	M_12BIT 0
#define	M_8BIT 1

void tsc2003_test(void)
{
	unsigned char command_byte;
	command_byte = TSC2007_CMD (MEAS_TEMP0, PD_POWERDOWN_ENABLEPENIRQ, M_12BIT);
	i2c_send_b((0x48<<1) | 1, 1, &command_byte, 1);
}

static const Cmd Cmds[] =
{
	{"MyCmds"},
	{"tsc2003","", 0, "test tsc2003", tsc2003_test, 0, 99, CMD_REPEAT},
	{0, 0}
};

static void init_cmd __P((void)) __attribute__ ((constructor));

static void
init_cmd()
{
	cmdlist_expand(Cmds, 1);
}
