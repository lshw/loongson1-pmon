#ifndef _JBT6K74_H
#define _JBT6K74_H

enum jbt_state {
	JBT_STATE_DEEP_STANDBY,
	JBT_STATE_SLEEP,
	JBT_STATE_NORMAL,
};

int jbt6k74_init(void);
int jbt6k74_display_onoff(int on);
int jbt6k74_enter_state(enum jbt_state new_state);

#endif