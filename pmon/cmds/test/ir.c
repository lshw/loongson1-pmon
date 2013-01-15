#include <pmon.h>
#include <cpu.h>
#include <string.h>
#include "target/fcr.h"
#include "ir.h"

#define PWM_CNTR	0xbfe5c000
#define PWM_HRC	0xbfe5c004
#define PWM_LRC	0xbfe5c008
#define PWM_CTRL	0xbfe5c00c

#define EN				1
#define OE				(1<<3)
#define SINGLE		(1<<4)
#define INTE			(1<<5)
#define INT			(1<<6)
#define CNTR_RST		(1<<7)

#define write_reg(addr, val)	*(volatile unsigned int *)(addr) = val
#define read_reg(addr)			*(volatile unsigned int *)(addr)

#define REG_GPIO_CFG0		0xbfd010c0		//GPIO 配置寄存器 0
#define REG_GPIO_CFG1		0xbfd010c4		//GPIO 配置寄存器 1
#define REG_GPIO_OE0			0xbfd010d0		//GPIO 配置寄存器输出使能 0
#define REG_GPIO_OE1			0xbfd010d4		//GPIO 配置寄存器输出使能 1
#define REG_GPIO_IN0			0xbfd010e0		//GPIO 配置寄存器输入寄存器 0
#define REG_GPIO_IN1			0xbfd010e4		//GPIO 配置寄存器输入寄存器 1
#define REG_GPIO_OUT0		0xbfd010f0		//GPIO 配置寄存器输出寄存器 0
#define REG_GPIO_OUT1		0xbfd010f4		//GPIO 配置寄存器输出寄存器 1

#define LS1B_BOARD_INTREG_BASE 0xbfd01040
struct ls1b_board_intc_regs
{
	volatile unsigned int int_isr;
	volatile unsigned int int_en;
	volatile unsigned int int_set;
	volatile unsigned int int_clr;		/* offset 0x10*/
	volatile unsigned int int_pol;
   	volatile unsigned int int_edge;		/* offset 0 */
}; 

static struct ls1b_board_intc_regs volatile *ls1b_board_int0_regs
	= (struct ls1b_board_intc_regs volatile *)(LS1B_BOARD_INTREG_BASE);

static int ls1b_ir_pinstate(void)
{
	unsigned int ret;
	ret = *(volatile unsigned int *)(REG_GPIO_IN1); 
	ret = ((ret >> (GPIO_IR - 32)) & 0x01);
	return ret;
}

static void ls1b_ir_irq_enable(int flag)
{
	/* 使能GPIO并初始化中断为边沿触发方式 */
	if (flag){
		*(volatile unsigned int *)(REG_GPIO_CFG1) |= (1 << (GPIO_IR - 32));
		*(volatile unsigned int *)(REG_GPIO_OE1) |= (1 << (GPIO_IR - 32));

		(ls1b_board_int0_regs + 3) -> int_edge	|= (1 << (GPIO_IR - 32));
		(ls1b_board_int0_regs + 3) -> int_pol	&= ~(1 << (GPIO_IR - 32));
		(ls1b_board_int0_regs + 3) -> int_clr	|= (1 << (GPIO_IR - 32));
		(ls1b_board_int0_regs + 3) -> int_set	&= ~(1 << (GPIO_IR - 32));
		(ls1b_board_int0_regs + 3) -> int_en	|= (1 << (GPIO_IR - 32));
	}
	/* 关闭GPIO和中断 */
	else{
		*(volatile unsigned int *)(REG_GPIO_CFG1) &= ~(1 << (GPIO_IR - 32));
		(ls1b_board_int0_regs + 3) -> int_en	&= ~(1 << (GPIO_IR - 32));
	}
}

/*
static ls1b_ir_irq_handler(unsigned int ls1b_ir_interval)
{	
	if (ls1b_ir_interval > 800 && ls1b_ir_interval < 15000) {
		if (ls1b_ir_interval > 11000) {
				ls1b_ir_state = LS1B_IR_STATE_RECEIVESTARTCODE;
				ls1b_ir_key_code_tmp = 0;
				ls1b_ir_databit_count = 0;
				ls1b_ir_systembit_count =0;				
		} else if (ls1b_ir_state == LS1B_IR_STATE_RECEIVESTARTCODE) {
			if (ls1b_ir_systembit_count >= SYSTEMCODE_BIT_NUM - 1) {
				ls1b_ir_state = LS1B_IR_STATE_RECEIVESYSTEMCODE;
				ls1b_ir_systembit_count = 0;
			} else if ((ls1b_ir_interval > 800 && ls1b_ir_interval < 1300) || (ls1b_ir_interval > 1900 && ls1b_ir_interval < 2400)) {
				ls1b_ir_systembit_count ++;
			} else goto receive_errerbit;
		} else if (ls1b_ir_state == LS1B_IR_STATE_RECEIVESYSTEMCODE) {
			if (ls1b_ir_databit_count < 8) {
				if (ls1b_ir_interval > 1900 && ls1b_ir_interval < 2400) {
					ls1b_ir_key_code_tmp |= (1 << ls1b_ir_databit_count);
					ls1b_ir_databit_count++;
				} else if (ls1b_ir_interval > 800 && ls1b_ir_interval < 1300) {
					ls1b_ir_databit_count++;
				} else goto receive_errerbit;
			} else if ((ls1b_ir_interval > 800 && ls1b_ir_interval < 1300) || (ls1b_ir_interval > 1900 && ls1b_ir_interval < 2400)) {
				ls1b_ir_state = LS1B_IR_STATE_IDLE;
				ls1b_ir_key_code = ls1b_ir_key_code_tmp;
				ls1b_ir_key_code_tmp = 0;
				ls1b_ir_databit_count = 0;
				ls1b_ir_systembit_count =0;	
				printf("IR:Receive key code:%d.\n",ls1b_ir_key_code);
			} else goto receive_errerbit;	
		}
		ls1b_ir_interval = 0;
		(ls1b_board_int0_regs + 3) -> int_clr |= (1 << (GPIO_IR - 32));
		return;
	}
	
receive_errerbit:
	ls1b_ir_state = LS1B_IR_STATE_IDLE;
	ls1b_ir_key_code_tmp = 0;
	ls1b_ir_databit_count = 0;
	ls1b_ir_systembit_count =0;
	ls1b_ir_interval = 0;
	(ls1b_board_int0_regs + 3) -> int_clr	|= (1 << (GPIO_IR - 32));
}
*/

#define PLL_FREQ_REG(x) *(volatile unsigned int *)(0xbfe78030+x)
void test_ir(int argc,char **argv)
{
	unsigned int pll,ctrl,clk, md_pipefreq;
	unsigned int ls1b_ir_interval = 0;
	
	#ifdef CONFIG_CHINESE
	printf("红外接收测试\n说明：\n");
	printf("1.用红外发射器或红外遥控器，对准开发板上的红外接收头，\n按发射器上的任意按键发送红外编码信号，\n接收头接收到信号后系统将对其解码，并把解码结果显示出来。\n");
	printf("2.如果没有显示解码结果，请检查红外接收头是否正常\n");
	printf("3.按任意键退出测试程序。\n");
	#else
	printf("press any key to out of ir_test\n");
	#endif
	
	/* 使能GPIO中断，用于红外接收引脚，边沿触发方式 */
	ls1b_ir_irq_enable(1);
	/* 设置PWM寄存器，工作于定时器模式 */
	/* 注意PWM寄存器的设置顺序，PWM_HRC和PWM_LRC寄存器会偶然清零 */
	write_reg(PWM_CTRL, 0);
	write_reg(PWM_CNTR, 0);
	/* 计算APB总线频率，为内存频率的1/2 由于PWM挂在APB总线上，所以其频率为APB总线频率 */
	pll = PLL_FREQ_REG(0);
	ctrl = PLL_FREQ_REG(4);
	clk = (12+(pll&0x3f))*APB_CLK/2 + ((pll>>8)&0x3ff)*APB_CLK/2/1024;
	md_pipefreq = ((ctrl&0xc00)==0xc00) ? APB_CLK : (ctrl&(1<<19)) ? clk/((ctrl>>14)&0x1f) : clk/2;
	/* 设置定时中断间隔为100usec(设置为1usec或10usec精度可能不够) */
	write_reg(PWM_HRC, md_pipefreq/2/10000);
	write_reg(PWM_LRC, md_pipefreq/2/10000);
	write_reg(PWM_CNTR, 0);
	write_reg(PWM_CTRL, 0x29);
	while(1){
		if (INT & read_reg(PWM_CTRL)){
			write_reg(PWM_CTRL, 0);
			write_reg(PWM_CNTR, 0);
			write_reg(PWM_CTRL, 0x29);
			ls1b_ir_interval += 100;	//用于时间计数
		}
		/* 判断是否按任意键退出 */
//		if (get_uart_char(COM1_BASE_ADDR)){
		if (getchar()){
		#ifdef CONFIG_CHINESE
			printf("\n退出红外测试测试程序\n");
		#else
			printf("\nout of Ir test\n");
		#endif
			break;
		}
		/* 判断是否发生中断 */
		if((ls1b_board_int0_regs + 3)->int_isr == 0){
			continue;
		}
		if (ls1b_ir_pinstate() != 0){
			continue;
		}
		if (ls1b_ir_interval > 800 && ls1b_ir_interval < 15000) {
			if (ls1b_ir_interval > 11000) {
					ls1b_ir_state = LS1B_IR_STATE_RECEIVESTARTCODE;
					ls1b_ir_key_code_tmp = 0;
					ls1b_ir_databit_count = 0;
					ls1b_ir_systembit_count =0;				
			}
			else if (ls1b_ir_state == LS1B_IR_STATE_RECEIVESTARTCODE) {
				if (ls1b_ir_systembit_count >= SYSTEMCODE_BIT_NUM - 1) {
					ls1b_ir_state = LS1B_IR_STATE_RECEIVESYSTEMCODE;
					ls1b_ir_systembit_count = 0;
				}
				else if ((ls1b_ir_interval > 800 && ls1b_ir_interval < 1300) || (ls1b_ir_interval > 1900 && ls1b_ir_interval < 2400)) {
					ls1b_ir_systembit_count ++;
				}
				else goto receive_errerbit;
			}
			else if (ls1b_ir_state == LS1B_IR_STATE_RECEIVESYSTEMCODE) {
				if (ls1b_ir_databit_count < 8) {
					if (ls1b_ir_interval > 1900 && ls1b_ir_interval < 2400) {
						ls1b_ir_key_code_tmp |= (1 << ls1b_ir_databit_count);
						ls1b_ir_databit_count++;
					}
					else if (ls1b_ir_interval > 800 && ls1b_ir_interval < 1300) {
						ls1b_ir_databit_count++;
					}
					else goto receive_errerbit;
				}
				else if ((ls1b_ir_interval > 800 && ls1b_ir_interval < 1300) || (ls1b_ir_interval > 1900 && ls1b_ir_interval < 2400)) {
					ls1b_ir_state = LS1B_IR_STATE_IDLE;
					ls1b_ir_key_code = ls1b_ir_key_code_tmp;
					ls1b_ir_key_code_tmp = 0;
					ls1b_ir_databit_count = 0;
					ls1b_ir_systembit_count = 0;
					#ifdef CONFIG_CHINESE
					printf("接收到信号：%d\n", ls1b_ir_key_code);
					#else
					printf("IR:Receive key code:%d.\n", ls1b_ir_key_code);
					#endif
				}
				else goto receive_errerbit;	
			}
			ls1b_ir_interval = 0;
			(ls1b_board_int0_regs + 3) -> int_clr |= (1 << (GPIO_IR - 32));
			continue;
		}
	receive_errerbit:
		ls1b_ir_state = LS1B_IR_STATE_IDLE;
		ls1b_ir_key_code_tmp = 0;
		ls1b_ir_databit_count = 0;
		ls1b_ir_systembit_count =0;
		ls1b_ir_interval = 0;
		(ls1b_board_int0_regs + 3) -> int_clr	|= (1 << (GPIO_IR - 32));
	}
	ls1b_ir_irq_enable(0);
	/* 清零PWM寄存器 */
	write_reg(PWM_CTRL, 0);
	write_reg(PWM_CNTR, 0);
	write_reg(PWM_HRC, 0);
	write_reg(PWM_LRC, 0);
}

static const Cmd Cmds[] =
{
	{"MyCmds"},
	{"test_Ir","", 0, "test_Ir", test_ir, 0, 99, CMD_REPEAT},
	{0, 0}
};

static void init_cmd __P((void)) __attribute__ ((constructor));

static void
init_cmd()
{
	cmdlist_expand(Cmds, 1);
}

