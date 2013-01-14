#ifdef	LS1A_CORE
	#define GPIO_IR 59
#else
	#define GPIO_IR 61
#endif

#define SYSTEMCODE_BIT_NUM 16

#define LS1B_IR_STATE_IDLE					0
#define LS1B_IR_STATE_RECEIVESTARTCODE		1
#define LS1B_IR_STATE_RECEIVESYSTEMCODE		2
#define LS1B_IR_STATE_RECEIVEDATACODE		3

static unsigned int ls1b_ir_state = LS1B_IR_STATE_IDLE;
static unsigned int ls1b_ir_systembit_count = 0;
static unsigned int ls1b_ir_databit_count = 0;
static unsigned int ls1b_ir_key_code_tmp = 0;
static unsigned int ls1b_ir_key_code = 0;

























