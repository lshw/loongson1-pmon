/*
 * Copyright (c) 2007 SUNWAH HI-TECH  (www.sw-linux.com.cn)
 * Wing Sun	<chunyang.sun@sw-linux.com>
 * Weiping Zhu<weiping.zhu@sw-linux.com>
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by Opsycon AB, Sweden.
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 */
#ifndef __BOOT_CFG_H__
#define __BOOT_CFG_H__

#include <pmon.h>
#include <exec.h>

#define MAXARGS 256

#define MAX_TIME_OUT 1000

#define OPTION_LEN	50
#define VALUE_LEN	1024
#define GLOBAL_VALUE_LEN 256

#define MENU_TITLE_BUF_LEN 79

typedef struct menu_option
{
	char option [OPTION_LEN + 1];
	int set_type;				/* 0-unset£¬1-set */
	int use_default;			/* use defualt value, 0-if value don't used, use default, 1- must set */
	char value 	[GLOBAL_VALUE_LEN + 1];
}MenuOptions;

typedef struct _Menu_Item_
{
	char title 	[MENU_TITLE_BUF_LEN + 1];	//Title of menu item, display on screen.
	char * kernel ;	//kernel file to load.
	char * args	;	//arguments for kernel.
	char * initrd ;	//initrd file for kernel, maybe empty.
	char * root;	//ROOT device from args.
}Menu_Item;

int OpenLoadConfig __P((const char* filename));
int menu_list_read __P((ExecId, int, int));
char* trim __P((char *line));
int GetTitle __P((const char *str,char * title, int title_buf_len));
int GetOption __P((char *str, char *option, int option_len, char *value, int val_len));
int boot_load __P((int index));
void set_option_value __P((const char* option, const char* value));
const char* get_option_value __P((const char* option));
void remove_comment __P((char *str));

#ifndef __BOOT_CFG_C__
extern Menu_Item menu_items[MAXARGS];//Storage All menu information.
extern int menus_num ;
extern MenuOptions menu_options[];
extern int options_num ;
#endif 

#define issep(ch) ((ch) == ' ' || (ch) == '\t')

#endif
