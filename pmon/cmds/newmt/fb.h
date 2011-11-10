#ifndef __NEW_FB_H__
#define __NEW_FB_H__


#include "mod_framebuffer.h"
#include "mod_x86emu_int10.h"
#include "mod_x86emu.h"

void video_cls(void);
void video_drawstring (int xx, int yy, unsigned char *s);
void video_console_print(int console_col, int console_row, unsigned char *s);
void video_set_background(unsigned char r, unsigned char g, unsigned char b);
void video_set_bg(unsigned char r, unsigned char g, unsigned char b);

void stop_record(void);
void begin_record(void);

#endif


