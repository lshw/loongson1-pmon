#include <stdio.h>
#include<linux/types.h>
#include<linux/io.h>
#include"kbd.h"

/*
 * Translation of escaped scancodes to keycodes.
 * This is now user-settable.
 * The keycodes 1-88,96-111,119 are fairly standard, and
 * should probably not be changed - changing might confuse X.
 * X also interprets scancode 0x5d (KEY_Begin).
 *
 * For 1-88 keycode equals scancode.
 */

#define E0_KPENTER 96
#define E0_RCTRL   97
#define E0_KPSLASH 98
#define E0_PRSCR   99
#define E0_RALT    100
#define E0_BREAK   101		/* (control-pause) */
#define E0_HOME    102
#define E0_UP      103
#define E0_PGUP    104
#define E0_LEFT    105
#define E0_RIGHT   106
#define E0_END     107
#define E0_DOWN    108
#define E0_PGDN    109
#define E0_INS     110
#define E0_DEL     111

/* BTC */
#define E0_MACRO   112
/* LK450 */
#define E0_F13     113
#define E0_F14     114
#define E0_HELP    115
#define E0_DO      116
#define E0_F17     117
#define E0_KPMINPLUS 118
/*
 * My OmniKey generates e0 4c for  the "OMNI" key and the
 * right alt key does nada. [kkoller@nyx10.cs.du.edu]
 */
#define E0_OK	124
/*
 * New microsoft keyboard is rumoured to have
 * e0 5b (left window button), e0 5c (right window button),
 * e0 5d (menu button). [or: LBANNER, RBANNER, RMENU]
 * [or: Windows_L, Windows_R, TaskMan]
 */
#define E0_MSLW	125
#define E0_MSRW	126
#define E0_MSTM	127

char inputbuf[300];
static unsigned char e0_keys[128] = {
	0, 0, 0, 0, 0, 0, 0, 0,	/* 0x00-0x07 */
	0, 0, 0, 0, 0, 0, 0, 0,	/* 0x08-0x0f */
	0, 0, 0, 0, 0, 0, 0, 0,	/* 0x10-0x17 */
	0, 0, 0, 0, E0_KPENTER, E0_RCTRL, 0, 0,	/* 0x18-0x1f */
	0, 0, 0, 0, 0, 0, 0, 0,	/* 0x20-0x27 */
	0, 0, 0, 0, 0, 0, 0, 0,	/* 0x28-0x2f */
	0, 0, 0, 0, 0, E0_KPSLASH, 0, E0_PRSCR,	/* 0x30-0x37 */
	E0_RALT, 0, 0, 0, 0, E0_F13, E0_F14, E0_HELP,	/* 0x38-0x3f */
	E0_DO, E0_F17, 0, 0, 0, 0, E0_BREAK, E0_HOME,	/* 0x40-0x47 */
	E0_UP, E0_PGUP, 0, E0_LEFT, E0_OK, E0_RIGHT, E0_KPMINPLUS, E0_END,	/* 0x48-0x4f */
	E0_DOWN, E0_PGDN, E0_INS, E0_DEL, 0, 0, 0, 0,	/* 0x50-0x57 */
	0, 0, 0, E0_MSLW, E0_MSRW, E0_MSTM, 0, 0,	/* 0x58-0x5f */
	0, 0, 0, 0, 0, 0, 0, 0,	/* 0x60-0x67 */
	0, 0, 0, 0, 0, 0, 0, E0_MACRO,	/* 0x68-0x6f */
	0, 0, 0, 0, 0, 0, 0, 0,	/* 0x70-0x77 */
	0, 0, 0, 0, 0, 0, 0, 0	/* 0x78-0x7f */
};
static void kbd_wait(void);
void kbd_poll(void);
char kbd_getch(void);
static int kbd_read_data(void);
static void kbd_write_command_w(int data);
static int kbd_wait_for_input(void);
static void kbd_write_output_w(int data);
static int kbd_write_command_w_and_wait(int data);
static int kbd_write_output_w_and_wait(int data);
static void kbd_clear_input(void);
int kbd_initialize(void);
int kbd_translate(unsigned char scancode, unsigned char *keycode);
static unsigned char handle_kbd_event(void);
static int do_acknowledge(unsigned char scancode);
static inline void handle_keyboard_event(unsigned char scancode);
void handle_scancode(unsigned char scancode, int down);
static void do_self(unsigned char value, char up_flag);
static void do_cons(unsigned char value, char up_flag);
static void do_fn(unsigned char value, char up_flag);
static void do_pad(unsigned char value, char up_flag);
static void do_cur(unsigned char value, char up_flag);
static void do_shift(unsigned char value, char up_flag);
static void do_null(void);
static void enter(void);
static void do_spec(unsigned char value, char up_flag);
static void do_dead(unsigned char value, char up_flag);
unsigned char kbd_code;
unsigned int esc_seq;
unsigned int esc_code;
extern void delay __P((int));

typedef void (*k_hand) (unsigned char value, char up_flag);
typedef void (k_handfn) (unsigned char value, char up_flag);

static k_handfn
    do_self, do_fn, do_spec, do_pad, do_dead, do_cons, do_cur, do_shift;
/*
	do_meta, do_ascii, do_lock, do_lowercase, do_slock, do_dead2,
	do_ignore;
*/
static k_hand key_handler[16] = {
	do_self, do_fn, do_spec, do_pad, do_dead, do_cons, do_cur, do_shift,
//      do_meta, do_ascii, do_lock, do_lowercase, do_slock, do_dead2,
//      do_ignore, do_ignore
};

typedef void (*void_fnp) (void);
typedef void (void_fn) (void);

static void_fn do_null, enter;
	/*show_ptregs, send_intr, lastcons, caps_toggle,
	   num, hold, scroll_forw, scroll_back, boot_it, caps_on, compose,
	   SAK, decr_console, incr_console, spawn_console, bare_num; */

static void_fnp spec_fn_table[16] = {
	do_null, enter,		/*show_ptregs,  show_mem,
				   show_state,  send_intr,      lastcons,       caps_toggle,
				   num,         hold,           scroll_forw,    scroll_back,
				   boot_it,     caps_on,        compose,        SAK,
				   decr_console,        incr_console,   spawn_console,  bare_num */
};

int shift_state = 0;

/*
 * Wait for keyboard controller input buffer to drain.
 *
 * Don't use 'jiffies' so that we don't depend on
 * interrupts..
 *
 * Quote from PS/2 System Reference Manual:
 *
 * "Address hex 0060 and address hex 0064 should be written only when
 * the input-buffer-full bit and output-buffer-full bit in the
 * Controller Status register are set 0."
 */

static void kbd_wait(void)
{
	unsigned long timeout = KBC_TIMEOUT;

	do {
		/*
		 * "handle_kbd_event()" will handle any incoming events
		 * while we wait - keypresses or mouse movement.
		 */
		unsigned char status = handle_kbd_event();

		if (!(status & KBD_STAT_IBF))
			return;
		delay(1000);
		timeout--;
	} while (timeout);
}

#define SEND_ESC_SEQ(c)	\
	esc_code = c;	\
	esc_seq = 1;

void kbd_poll()
{
	if (esc_seq) {
		switch (esc_seq) {
		case 1:
			kbd_code = 0x1b;
			esc_seq++;
			break;
		case 2:
			kbd_code = '[';
			esc_seq++;
			break;
		case 3:
			kbd_code = esc_code;
			esc_seq = 0;
		}
	} else {
		while (kbd_read_status() & KBD_STAT_OBF)
			handle_kbd_event();
	}
}

static int kbd_read_data(void)
{
	int retval = KBD_NO_DATA;
	unsigned char status;

	status = kbd_read_status();
	if (status & KBD_STAT_OBF) {
		unsigned char data = kbd_read_input();

		retval = data;
		if (status & (KBD_STAT_GTO | KBD_STAT_PERR))
			retval = KBD_BAD_DATA;
	}
	return retval;
}

static void kbd_write_command_w(int data)
{
	kbd_wait();
	kbd_write_command(data);
}

static int kbd_wait_for_input(void)
{
	long timeout = KBD_INIT_TIMEOUT;

	do {
		int retval = kbd_read_data();
		if (retval >= 0)
			return retval;
		delay(1000);
	} while (--timeout);
	return -1;
}

static void kbd_write_output_w(int data)
{
	kbd_wait();
	kbd_write_output(data);
}

static int kbd_write_command_w_and_wait(int data)
{
	kbd_write_command_w(data);
	return kbd_wait_for_input();
}

static int kbd_write_output_w_and_wait(int data)
{
	kbd_write_output_w(data);
	return kbd_wait_for_input();
}

static void kbd_clear_input(void)
{
	int maxread = 100;	/* Random number */

	do {
		if (kbd_read_data() == KBD_NO_DATA)
			break;
	} while (--maxread);
}

const char *kbd_error_msgs[] = {
	"Keyboard succesfully initialized.",
	"Keyboard failed self test",
	"Keyboard reset failed, no ACK",
	"Keyboard reset failed, no POR",
	"Disable keyboard: no ACK",
	"Enable keyboard: no ACK",
	"Set rate: no ACK",
	"Set rate: no 2nd ACK"
};

#ifdef LOONGSON2F_7INCH
/********************************************************/
#include "include/kb3310.h"
/* we just ensure there is code in EC, and the keyboard will work. */
static int kb3310_test(void)
{
	unsigned char val;
	val = rdec(0x00);
	if(val == 0xff)
		return 1;
	
	return 0;
}
#endif

int kbd_initialize(void)
{
	int status;
	int count;

#ifdef LOONGSON2F_7INCH
	status = kb3310_test();
	if(status != 0){
		printf("Waring!! You should burn the flash rom first for kbd initial.\n");
	}
#endif

	/* Flush the buffer */
	kbd_clear_input();

	/*
	 * Test the keyboard interface.
	 * This seems to be the only way to get it going.
	 * If the test is successful a x55 is placed in the input buffer.
	 */
	kbd_write_command_w(KBD_CCMD_SELF_TEST);

	if (kbd_wait_for_input() != 0x55) {
		printf("Self test cmd failed,ignored!\n");
		//return 1;
	}

	/*
	 * Perform a keyboard interface test.  This causes the controller
	 * to test the keyboard clock and data lines.  The results of the
	 * test are placed in the input buffer.
	 */
	kbd_write_command_w(KBD_CCMD_KBD_TEST);
	if (kbd_wait_for_input() != 0x00) {
		printf("KBD_TEST cmd failed,ignored!\n");
		//return 1;
	}

	/*
	 * Enable the keyboard by allowing the keyboard clock to run.
	 */
	kbd_write_command_w(KBD_CCMD_KBD_ENABLE);

	/*
	 * Reset keyboard. If the read times out
	 * then the assumption is that no keyboard is
	 * plugged into the machine.
	 * This defaults the keyboard to scan-code set 2.
	 *
	 * Set up to try again if the keyboard asks for RESEND.
	 */
	do {
		kbd_clear_input();
		kbd_write_output_w(KBD_CMD_RESET);
		status = kbd_wait_for_input();
		if (status == KBD_REPLY_ACK)
			break;
#ifdef KBD_CHECK_FAST
		if (status != KBD_REPLY_RESEND)
			return 2;
#else
		if (status != KBD_REPLY_RESEND)
			break;
#endif
	} while (1);

	if (kbd_wait_for_input() != KBD_REPLY_POR) {
		//printf("NO POR, ignored!\n");
		//return 3;
	}

	/*
	 * Set keyboard controller mode. During this, the keyboard should be
	 * in the disabled state.
	 *
	 * Set up to try again if the keyboard asks for RESEND.
	 */
#ifndef FCRSOC
	count = 0;
	do {
		kbd_write_output_w(KBD_CMD_DISABLE);
		status = kbd_wait_for_input();
		if (status == KBD_REPLY_ACK)
			break;
		if (status != KBD_REPLY_RESEND)
			if (++count > 1)
				break;
			//return 4;
	} while (1);
#endif

	kbd_write_command_w(KBD_CCMD_WRITE_MODE);
	kbd_write_output_w(KBD_MODE_KBD_INT
			   | KBD_MODE_SYS
			   | KBD_MODE_DISABLE_MOUSE | KBD_MODE_KCC);
#if 1
	/* ibm powerpc portables need this to use scan-code set 1 -- Cort */
	if (!(kbd_write_command_w_and_wait(KBD_CCMD_READ_MODE) & KBD_MODE_KCC)) {
		/*
		 * If the controller does not support conversion,
		 * Set the keyboard to scan-code set 1.
		 */
		kbd_write_output_w(0xF0);
		kbd_wait_for_input();
		kbd_write_output_w(0x01);
		kbd_wait_for_input();
	}
#endif
	if (kbd_write_output_w_and_wait(KBD_CMD_ENABLE) != KBD_REPLY_ACK) {
		//return 5;
	}

	/*
	 * Finally, set the typematic rate to maximum.
	 */
	if (kbd_write_output_w_and_wait(KBD_CMD_SET_RATE) != KBD_REPLY_ACK) {
		//return 6;
	}
	if (kbd_write_output_w_and_wait(0x00) != KBD_REPLY_ACK) {
		//return 7;
	}

	return 0;
}

int kbd_translate(unsigned char scancode, unsigned char *keycode)
{
	static int prev_scancode;

	/* special prefix scancodes.. */
	if (scancode == 0xe0 || scancode == 0xe1) {
		prev_scancode = scancode;
		return 0;
	}

	/* 0xFF is sent by a few keyboards, ignore it. 0x00 is error */
	if (scancode == 0x00 || scancode == 0xff) {
		prev_scancode = 0;
		return 0;
	}

	scancode &= 0x7f;

	if (prev_scancode) {
		if (prev_scancode != 0xe0) {
			return 0;
		} else {
			prev_scancode = 0;
			/*
			 *  The keyboard maintains its own internal caps lock and
			 *  num lock statuses. In caps lock mode E0 AA precedes make
			 *  code and E0 2A follows break code. In num lock mode,
			 *  E0 2A precedes make code and E0 AA follows break code.
			 *  We do our own book-keeping, so we will just ignore these.
			 */
			/*
			 *  For my keyboard there is no caps lock mode, but there are
			 *  both Shift-L and Shift-R modes. The former mode generates
			 *  E0 2A / E0 AA pairs, the latter E0 B6 / E0 36 pairs.
			 *  So, we should also ignore the latter. - aeb@cwi.nl
			 */
			if (scancode == 0x2a || scancode == 0x36)
				return 0;

			if (e0_keys[scancode])
				*keycode = e0_keys[scancode];
			else {
				return 0;
			}
		}
	} else {
		*keycode = scancode;
	}
	return 1;
}

static inline void handle_mouse_event(unsigned char scancode);
/*
 * This reads the keyboard status port, and does the
 * appropriate action.
 *
 * It requires that we hold the keyboard controller
 * spinlock.
 */
static unsigned char handle_kbd_event(void)
{
	unsigned char status = kbd_read_status();
	unsigned int work = 10000;

	while ((--work > 0) && (status & KBD_STAT_OBF)) {
		unsigned char scancode;

		scancode = kbd_read_input();

		/* Error bytes must be ignored to make the 
		   Synaptics touchpads compaq use work */
#if 1
		/* Ignore error bytes */
		if (!(status & (KBD_STAT_GTO | KBD_STAT_PERR)))
#endif
		{
			if (status & KBD_STAT_MOUSE_OBF) 
				handle_mouse_event(scancode);
			else
				handle_keyboard_event(scancode);
		}

		status = kbd_read_status();
	}

	if (!work)
		printf("controller jammed (0x%02X).\n", status);

	return status;
}

static volatile unsigned char reply_expected;
static volatile unsigned char acknowledge;
static volatile unsigned char resend;

static int do_acknowledge(unsigned char scancode)
{
	if (reply_expected) {
		/* Unfortunately, we must recognise these codes only if we know they
		 * are known to be valid (i.e., after sending a command), because there
		 * are some brain-damaged keyboards (yes, FOCUS 9000 again) which have
		 * keys with such codes :(
		 */
		if (scancode == KBD_REPLY_ACK) {
			acknowledge = 1;
			reply_expected = 0;
			return 0;
		} else if (scancode == KBD_REPLY_RESEND) {
			resend = 1;
			reply_expected = 0;
			return 0;
		}
	}
	return 1;
}

static inline void handle_keyboard_event(unsigned char scancode)
{
	if (do_acknowledge(scancode))
		handle_scancode(scancode, !(scancode & 0x80));
}

int kbd_test=0;
int kbd_test_hit=0;
void handle_scancode(unsigned char scancode, int down)
{
	unsigned char keycode;
	char up_flag = down ? 0 : 0200;
	if(kbd_test){ printf("%02x\n",scancode);kbd_test_hit=1;return;}

	/*
	 *  Convert scancode to keycode
	 */
	if (!kbd_translate(scancode, &keycode))
		goto out;

	/*
	 *  Repeat a key only if the input buffers are empty or the
	 *  characters get echoed locally. This makes key repeat usable
	 *  with slow applications and under heavy loads.
	 */
	if (1) {
		u_short keysym;
		u_char type;
		
		ushort *key_map = key_maps[shift_state];
		if (key_map != NULL) {
			keysym = key_map[keycode];
			type = KTYP(keysym);
	//		printf("keycode = %d\n",keycode);
	//		printf("[kbd] keysym %x\n", keysym);

			if (type >= 0xf0) {
				type -= 0xf0;
				if (type == KT_LETTER) {
					type = KT_LATIN;
				}
				if (*key_handler[type]) {
					(*key_handler[type]) (keysym & 0xff,
							      up_flag);
				}
			}
		}
	}
      out:;
}

static void do_self(unsigned char value, char up_flag)
{
	if (up_flag)
		return;		/* no action, if this is a key release */
	kbd_code = value;
}

static void do_cons(unsigned char value, char up_flag)
{
	printf("do_cons value=%x\n", value);

}

static void do_fn(unsigned char value, char up_flag)
{
	if (!up_flag) {
		switch (value) {
		case 0x14:
			SEND_ESC_SEQ('H');
			break;
		case 0x16:
			SEND_ESC_SEQ('G'); /*Delete Key*/
			break;
		case 0x17:
			SEND_ESC_SEQ('F');
			break;
		}
	}
}

static void do_pad(unsigned char value, char up_flag)
{
	if (!up_flag) {
		switch (value) {
		case 0x10:
			SEND_ESC_SEQ('G');
			break;
		case 0xe:
			kbd_code = 10;
		default:
			break;
		}
	}
	printf("do_pad value=%x\n", value);
}

static void do_cur(unsigned char value, char up_flag)
{
	if (!up_flag) {
		switch (value) {
		case 0:
			SEND_ESC_SEQ('B');
			break;
		case 1:
			SEND_ESC_SEQ('D');
			break;
		case 2:
			SEND_ESC_SEQ('C');
			break;
		case 3:
			SEND_ESC_SEQ('A');
			break;
		}
	}
}

static void do_shift(unsigned char value, char up_flag)
{
	if (up_flag) {
		shift_state = 0;
	} else if (value == 0) {	//key shift
		shift_state = 1;
	} else if (value == 2) {	//key ctrl
		shift_state = 2;
#ifdef SPANISH
	} else {	//key AltGr spanish
		shift_state = 3;
#endif
	}
//      printf("do_shift value=0x%x,shift_state=0x%x\n",value,shift_state);
}

static void do_null()
{
//      compute_shiftstate();
}

static void enter(void)
{
//      extern void vga_set_enter();
//      vga_set_enter();
	kbd_code = '\n';
}

static void do_spec(unsigned char value, char up_flag)
{
	if (up_flag)
		return;
//      printf("do_spec value=%x\n",value);
//#define SIZE(x) (sizeof(x)/sizeof((x)[0]))
//      if (value >= SIZE(spec_fn_table))       return;
	if (spec_fn_table[value])
		spec_fn_table[value] ();
}

static void do_dead(unsigned char value, char up_flag)
{
//      printf("do_dead value=%x\n",value);
}

#define KBD_CMD_SET_LEDS    0xED	/* Set keyboard leds */
#define mdelay delay1

static int send_data(unsigned char data)
{
	int retries = 3;

	do {
		unsigned long timeout = KBD_TIMEOUT;

		acknowledge = 0;	/* Set by interrupt routine on receipt of ACK. */
		resend = 0;
		reply_expected = 1;
		kbd_write_output_w(data);
		for (;;) {
			if (acknowledge)
				return 1;
			if (resend)
				break;
			mdelay(1);
			if (!--timeout) {
				printf
				    ("keyboard: Timeout - AT keyboard not present?(%02x)\n",
				     data);
				return 0;
			}
		}
	} while (retries-- > 0);
	printf("keyboard: Too many NACKs -- noisy kbd cable?\n");
	return 0;
}

void pckbd_leds(unsigned char leds)
{
	if (kbd_available && (!send_data(KBD_CMD_SET_LEDS) || !send_data(leds))) {
		send_data(KBD_CMD_ENABLE);	/* re-enable kbd if any errors */
		kbd_available = 0;
	}
}

//--------------------------------------mouse-------------------------------
#ifdef CONFIG_PSMOUSE
#endif
static int aux_reconnect = 0;


#define AUX_RECONNECT1 0xaa	/* scancode1 when ps2 device is plugged (back) in */
#define AUX_RECONNECT2 0x00	/* scancode2 when ps2 device is plugged (back) in */
 
static int aux_count;
/* used when we send commands to the mouse that expect an ACK. */
static unsigned char mouse_reply_expected;

#define AUX_INTS_OFF (KBD_MODE_KCC | KBD_MODE_DISABLE_MOUSE | KBD_MODE_SYS | KBD_MODE_KBD_INT)
#define AUX_INTS_ON  (KBD_MODE_KCC | KBD_MODE_SYS | KBD_MODE_MOUSE_INT | KBD_MODE_KBD_INT)

#define MAX_RETRIES	60		/* some aux operations take long time*/


static void __aux_write_ack(int val);
static inline void handle_mouse_event(unsigned char scancode)
{
	static unsigned char prev_code;
	if (mouse_reply_expected) {
		if (scancode == AUX_ACK) {
			mouse_reply_expected--;
			return;
		}
		mouse_reply_expected = 0;
	}
	else if(scancode == AUX_RECONNECT2 && prev_code == AUX_RECONNECT1
		&& aux_reconnect) {
		printf ("PS/2 mouse reconnect detected\n");
		__aux_write_ack(AUX_ENABLE_DEV);  /* ping the mouse :) */
		return;
	}

	prev_code = scancode;
	if (getenv("aux")) {
		printf("mouse:%x\n",scancode);
	}
}

static void kb_wait(void)
{
	unsigned long timeout = KBC_TIMEOUT;

	do {
		/*
		 * "handle_kbd_event()" will handle any incoming events
		 * while we wait - keypresses or mouse movement.
		 */
		unsigned char status = handle_kbd_event();

		if (! (status & KBD_STAT_IBF))
			return;
		mdelay(1);
		timeout--;
	} while (timeout);
#ifdef KBD_REPORT_TIMEOUTS
	printf("Keyboard timed out[1]\n");
#endif
}

/*
 * Check if this is a dual port controller.
 */
static int detect_auxiliary_port(void)
{
	unsigned long flags;
	int loops = 10;
	int retval = 0;


	/* Put the value 0x5A in the output buffer using the "Write
	 * Auxiliary Device Output Buffer" command (0xD3). Poll the
	 * Status Register for a while to see if the value really
	 * turns up in the Data Register. If the KBD_STAT_MOUSE_OBF
	 * bit is also set to 1 in the Status Register, we assume this
	 * controller has an Auxiliary Port (a.k.a. Mouse Port).
	 */
	kb_wait();
	kbd_write_command(KBD_CCMD_WRITE_AUX_OBUF);

	kb_wait();
	kbd_write_output(0x5a); /* 0x5a is a random dummy value. */

	do {
		unsigned char status = kbd_read_status();

		if (status & KBD_STAT_OBF) {
			(void) kbd_read_input();
			if (status & KBD_STAT_MOUSE_OBF) {
				printf("Detected PS/2 Mouse Port.\n");
				retval = 1;
			}
			break;
		}
		mdelay(1);
	} while (--loops);

	return retval;
}

/*
 * Send a byte to the mouse.
 */
static void aux_write_dev(int val)
{
	unsigned long flags;

	kb_wait();
	kbd_write_command(KBD_CCMD_WRITE_MOUSE);
	kb_wait();
	kbd_write_output(val);
}

/*
 * Send a byte to the mouse & handle returned ack
 */
static void __aux_write_ack(int val)
{
	kb_wait();
	kbd_write_command(KBD_CCMD_WRITE_MOUSE);
	kb_wait();
	kbd_write_output(val);
	/* we expect an ACK in response. */
	mouse_reply_expected++;
	kb_wait();
}

static void aux_write_ack(int val)
{
	unsigned long flags;

	__aux_write_ack(val);
}

static void kbd_write_cmd(int cmd)
{
	unsigned long flags;

	kb_wait();
	kbd_write_command(KBD_CCMD_WRITE_MODE);
	kb_wait();
	kbd_write_output(cmd);
}


int psaux_init(void)
{
	int retval;

	if (!detect_auxiliary_port())
		return -1;

	kbd_write_command_w(KBD_CCMD_MOUSE_ENABLE); /* Enable Aux. */
	aux_write_ack(AUX_SET_SAMPLE);
	aux_write_ack(100);			/* 100 samples/sec */
	aux_write_ack(AUX_SET_RES);
	aux_write_ack(3);			/* 8 counts per mm */
	aux_write_ack(AUX_SET_SCALE21);		/* 2:1 scaling */
	kbd_write_command(KBD_CCMD_MOUSE_DISABLE); /* Disable aux device. */
	kbd_write_cmd(AUX_INTS_OFF); /* Disable controller ints. */

//---------------------------------------
	kbd_write_command_w(KBD_CCMD_MOUSE_ENABLE);	/* Enable the
							   auxiliary port on
							   controller. */
	aux_write_ack(AUX_ENABLE_DEV); /* Enable aux device */
	kbd_write_cmd(AUX_INTS_ON); /* Enable controller ints */
	
	mdelay(2);			/* Ensure we follow the kbc access delay rules.. */

	send_data(KBD_CMD_ENABLE);	/* try to workaround toshiba4030cdt problem */
	return 0;
}


