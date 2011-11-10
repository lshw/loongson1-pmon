#include <termio.h>
#include <pmon.h>
#include <stdio.h>
#include <linux/types.h>
#include <linux/io.h>
#include "mod_framebuffer.h"

#define index_cursor_pos_hi 0x0e
#define index_cursor_pos_lo 0x0f

#define crt_index 0x3d4
#define crt_data 0x3d5
extern int vga_available;
extern unsigned char kbd_code;
#if NMOD_USB_KBD != 0
extern unsigned char usb_kbd_code; 
#endif
extern unsigned char usb_kbd_code; 
#ifndef VGA_MEM_BASE
#ifdef BONITOEL
	unsigned char * vgabh=(unsigned char *)0xb00b8000;
#endif
	
#ifdef CONFIG_PCI0_GAINT_MEM
	unsigned char * vgabh=(unsigned char *)0xbeeb8000;
#endif

#ifdef CONFIG_PCI0_HUGE_MEM
	unsigned char * vgabh=(unsigned char *)0xb48b8000;
#endif

#ifdef CONFIG_PCI0_LARGE_MEM
	unsigned char * vgabh=(unsigned char *)0xb50b8000;
#endif

#if !(defined(BONITOEL)|defined(CONFIG_PCI0_GAINT_MEM)|defined(CONFIG_PCI0_HUGE_MEM)|defined(CONFIG_PCI0_LARGE_MEM)) 
	unsigned char * vgabh=(unsigned char *)0xb00b8000;
#endif
#else
	unsigned char * vgabh=(unsigned char *)(VGA_MEM_BASE+0x000b8000);
#endif


int rowcount=25;
int colcount=80;

void vga_roll(void);
unsigned char get_crt_reg(unsigned char index);
void set_crt_reg(unsigned char index,unsigned char data);
void set_cursor(unsigned char x,unsigned char y);
int write_at_cursor(char val);
int backspace_cursor(void);
void vga_set_enter(void);

void vga_roll()
{
	int i;
	for (i=0;i<(rowcount-1)*colcount;i++ )
		*(unsigned char *)(vgabh+2*i)=*(unsigned char *)(vgabh+2*(i+colcount));
	for (;i<rowcount*colcount;i++ )
		*(unsigned char *)(vgabh+2*i)=0;

}

unsigned char get_crt_reg(unsigned char index)
{
	linux_outb(index,crt_index);
	return linux_inb(crt_data);
}

void set_crt_reg(unsigned char index,unsigned char data)
{
	linux_outb(index,crt_index);
	linux_outb(data,crt_data);
}
//int write(unsigned char x,unsigned char y,char val)
void set_cursor(unsigned char x,unsigned char y)
{
	set_crt_reg(index_cursor_pos_lo,(x+(y*colcount))&0xff);
	set_crt_reg(index_cursor_pos_hi,(x+(y*colcount))>>8);	
//	printf("set cursor x=0x%x,y=0x%x \n",x,y);
}

int write_at_cursor(char val)
{
//	int	pos_row,pos_col;
	unsigned char hi,lo;
	unsigned short cursor;

	if(val == '\n') {
		vga_set_enter();
		return 0;
	}
	if(val == '\b') {
		backspace_cursor();
		return 0;
	}
	if(val == '\r') {
		return 0;
	}
	hi=get_crt_reg(index_cursor_pos_hi);
	lo=get_crt_reg(index_cursor_pos_lo);
	cursor=(hi<<8)|lo;
//	printf("cursor hi =0x%x,lo =0x%x,cursor=0x%x \n",hi,lo,cursor);
	*(unsigned char *)(vgabh+2*cursor)=val;
	*(unsigned char *)(vgabh+2*cursor+1)=0x07;
	if (cursor==rowcount*colcount-1)	{
		vga_roll();
		cursor-=colcount;
	}
	cursor=cursor+1;
	set_crt_reg(index_cursor_pos_lo,(cursor&0xff));
	set_crt_reg(index_cursor_pos_hi,(cursor>>8));
//	printf("write value=0x%x done! cursor=0x%x \n",val,cursor);
	return 0;
}

int backspace_cursor()
{
//	int	pos_row,pos_col;
	unsigned char hi,lo;
	unsigned short cursor;
	hi=get_crt_reg(index_cursor_pos_hi);
	lo=get_crt_reg(index_cursor_pos_lo);
	cursor=(hi<<8)|lo;
//	printf("cursor hi =0x%x,lo =0x%x,cursor=0x%x \n",hi,lo,cursor);
	cursor--;
#if 0  //qiaochong
	*(unsigned char *)(vgabh+2*cursor)=' ';
	*(unsigned char *)(vgabh+2*cursor+1)=0x07;
#endif
	set_crt_reg(index_cursor_pos_lo,(cursor&0xff));
	set_crt_reg(index_cursor_pos_hi,(cursor>>8));
//	printf("write value=0x%x done! cursor=0x%x \n",val,cursor);
	return 0;
}


void vga_set_enter()
{
	unsigned char hi,lo;
	unsigned short cursor;
	hi=get_crt_reg(index_cursor_pos_hi);
	lo=get_crt_reg(index_cursor_pos_lo);
	cursor=(hi<<8)|lo;
	cursor+=colcount;
	if (cursor>=rowcount*colcount) {
		vga_roll();
		cursor-=colcount;
	}
	cursor=(cursor/colcount)*colcount;
	set_crt_reg(index_cursor_pos_lo,(cursor&0xff));
	set_crt_reg(index_cursor_pos_hi,(cursor>>8));
}

int
vgaterm (int op, struct DevEntry *dev, unsigned long param, int data)
{
	unsigned char code;
	switch (op) {
		case OP_INIT:
		case OP_XBAUD:
		case OP_BAUD:
			return 0;

		case OP_TXRDY:
			return 1;

		case OP_TX:
			if(vga_available)
				write_at_cursor(data&0xff);
			break;

		case OP_RXRDY:
			if ( 
#if NMOD_USB_KBD != 0
			     (usb_kbd_available && usb_kbd_code) || 
#endif
			     (kbd_available && kbd_code) ) {
				return 1;
			}else {
				return 0;
			}

		case OP_RX:
#if NMOD_USB_KBD != 0
			if(usb_kbd_available && usb_kbd_code){
				code = usb_kbd_code;
				usb_kbd_code = 0;
				return code;
			}
#endif
			if(kbd_available && kbd_code){
				code = kbd_code;
				kbd_code = 0;
				return code;
			}else{
				return 0;			
			}

		case OP_RXSTOP:
			break;
	}
	return 0;
}

#if NMOD_FRAMEBUFFER > 0
int 
fbterm (int op, struct DevEntry *dev, unsigned long param, int data)
{

	unsigned char code;
	switch (op) {
		case OP_OPEN:
			return 0;
                case OP_INIT:
                case OP_XBAUD:
                case OP_BAUD:
                        return 0;
                                                                                                                                                            
                case OP_TXRDY:
                        return 1;
                                                                                                                                                            
                case OP_TX:
			//*(unsigned char*)0xbfd003f8 = data&0xff;
                        if(vga_available)
				video_putc(data&0xff);
                                //fb_putchar(data&0xff);
                        break;
                                                                                                                                                            
                case OP_RXRDY:
			if ( 
#if NMOD_USB_KBD != 0
			     (usb_kbd_available && usb_kbd_code) || 
#endif
			     (kbd_available && kbd_code) ) {
				return 1;
			}else {
				return 0;
			}
                case OP_RX:
#if NMOD_USB_KBD != 0
			if(usb_kbd_available && usb_kbd_code){
				code = usb_kbd_code;
				usb_kbd_code = 0;
				return code;
			}
#endif
			if(kbd_available && kbd_code){
				code = kbd_code;
				kbd_code = 0;
				return code;
			}else{
				return 0;			
			}

                case OP_RXSTOP:
                        break;
        }
        return 0;
}

#endif
