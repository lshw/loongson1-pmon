#include "mod_framebuffer.h"
#define TYPE_NONE 0
#define TYPE_CMD  1
#define TYPE_MENU 2
#define TYPE_MSG  3
#define TYPE_EDITMENU 4
#define TYPE_CHOICE 5

#if NMOD_FRAMEBUFFER ==0
#define POP_W	50
//#define POP_W	0
#define POP_H	20
//#define POP_H	0
//#define POP_X	16
//#define POP_Y	8
#define POP_X	0
#define POP_Y	1
#define MSG_W	50
#define MSG_H	20
#define MSG_X	5
#define MSG_Y	8
#define INFO_Y  19
#define INFO_W  50
#else
#define POP_W	50
//#define POP_W	0
#define POP_H	20
//#define POP_H	0
//#define POP_X	16
//#define POP_Y	5
#define POP_X	0
#define POP_Y	1
#define MSG_W	50
#define MSG_H	20
#define MSG_X	5
#define MSG_Y	8
#define INFO_Y  25//19	//lxy
#define INFO_W  50
#endif

struct setupMenuitem{
char y;
char x;
char dnext;
char rnext;
char type;
char *msg;
int (*action)(int msg);
void *arg;
};

struct setupMenu{
int (*action)(int msg);
int width,height;
struct setupMenuitem *items;
};

void __console_alloc(void);
void __console_free(void);
void do_menu(struct setupMenu *newmenu);
int setup_quit(int);
