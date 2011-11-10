/*
	PMON Graphic Library
	Designed by Zhouhe(÷‹∫’) USTC 04011
*/
#define WA_LEFT 31
#define WA_CENTRE 1
#define WA_RIGHT 0
void w_present(void);
void w_init(void);
void w_enterconsole();
void w_leaveconsole();

void w_setpage(int);
int w_getpage();

void w_window(int x,int y,int w,int h,char *text);
int w_button(int x,int y,int w,char *caption);
void w_text(int x,int y,int xalign,char *ostr);
int w_select(int x,int y,int w,char *caption,char **options,int *current);
int w_switch(int x,int y,char *caption,int *base,int mask);
int w_input(int x,int y,int w,char *caption,char * text,int buflen);
void w_bigtext(int x,int y,int w,int h,char *text);
int w_selectinput(int x,int y,int w,char *caption,char **options,char *current,int buflen);
int w_biginput(int x,int y,int w,int h,char *caption,char * text,int buflen);

int w_focused();
int w_entered();
void w_setfocusid(int);
int w_getfocusid();

void w_setcolor(char windowcolor,char buttonunused,char buttonused);
void w_defaultcolor();

int w_keydown(int kin);
int w_hitanykey();