/*	$Id: hello.c,v 1.1.1.1 2006/09/14 01:59:08 root Exp $ */

/*
 * Copyright (c) 2002 Opsycon AB  (www.opsycon.se)
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

/*
 *   This is a simple program that demonstrate the linkage
 *   between PMON2000 and a loaded program.
 */

typedef long long off_t;

struct callvectors {
	int     (*open) (char *, int, int);
	int     (*close) (int);
	int     (*read) (int, void *, int);
	int     (*write) (int, void *, int);
	off_t   (*lseek) (int, off_t, int);
	int     (*printf) (const char *, ...);
	void    (*cacheflush) (void);
	char    *(*gets) (char *);
};

struct callvectors *callvec;

#define	printf (*callvec->printf)
#define	gets   (*callvec->gets)

void __gccmain(void);
void __gccmain(void){}
#define linux_outb(val,port) *(volatile unsigned char *)(0xbfd00000+port)=val
#define linux_outb_p(val,port) (*(volatile unsigned char *)0xbfc00000,linux_outb(val,port),*(volatile unsigned char *)0xbfc00000)
#define linux_inb(port) *(volatile unsigned char *)(0xbfd00000+port)
#define linux_inb_p(port) *(volatile unsigned char *)(0xbfd00000+port)

static inline unsigned char CMOS_READ(unsigned char addr)
{
        unsigned char val;
        linux_outb_p(addr, 0x70);
        val = linux_inb_p(0x71);
        return val;
}
                                                                               
static inline void CMOS_WRITE(unsigned char val, unsigned char addr)
{
        linux_outb_p(addr, 0x70);
        linux_outb_p(val, 0x71);
}
void  init_8259A(int auto_eoi)
{


	linux_outb(0xff, 0x21);	/* mask all of 8259A-1 */
	linux_outb(0xff, 0xA1);	/* mask all of 8259A-2 */

	/*
	 * linux_outb_p - this has to work on a wide range of PC hardware.
	 */
	linux_outb_p(0x11, 0x20);	/* ICW1: select 8259A-1 init */
	linux_outb_p(0x00, 0x21);	/* ICW2: 8259A-1 IR0-7 mapped to 0x00-0x07 */
	linux_outb_p(0x04, 0x21);	/* 8259A-1 (the master) has a slave on IR2 */
	if (auto_eoi)
		linux_outb_p(0x03, 0x21);	/* master does Auto EOI */
	else
		linux_outb_p(0x01, 0x21);	/* master expects normal EOI */

	linux_outb_p(0x11, 0xA0);	/* ICW1: select 8259A-2 init */
	linux_outb_p(0x08, 0xA1);	/* ICW2: 8259A-2 IR0-7 mapped to 0x08-0x0f */
	linux_outb_p(0x02, 0xA1);	/* 8259A-2 is a slave on master's IR2 */
	linux_outb_p(0x01, 0xA1);	/* (slave's support for AEOI in flat mode
				    is to be investigated) */

}
int
main(int argc, char **argv, char **env, struct callvectors *cv)
{
	char str[256];
	char **ev;
	int i;
	unsigned stat;

	callvec = cv;

	printf("\n\nHello! This is the 'hello' program!\n\n");
	init_8259A(0);
	*(volatile char *)0xbfd00021=0;
	*(volatile char *)0xbfd000a1=0;
//	linux_outb(0x60,0x64);
//	linux_outb(0x2,0x60);

//	asm("mfc0 $2,$12;\r\nor $2,0xf01;\r\n mtc0 $2,$12\r\n":::"$2");
	asm("mfc0 %0,$12;":"=r"(stat));
	stat|=0x1|(1<<13);
	asm("mtc0 %0,$12;"::"r"(stat));
	asm("mfc0 %0,$12;":"=r"(stat));
	printf("stat=%x\n",stat);
	printf("time is %d:%d:%d\n",CMOS_READ(0x4),CMOS_READ(0x2),CMOS_READ(0x0));
#if 0
	CMOS_WRITE(CMOS_READ(0x4),5);
	CMOS_WRITE(CMOS_READ(0x2),3);
	CMOS_WRITE(CMOS_READ(0x0),1);
	CMOS_WRITE(CMOS_READ(0xb)|(7<<4),0xb);
#endif

	gets(str);
	return 0;
	printf("It was invoked with:\n");
	for (i = 0; i < argc; i++) {
		printf("Arg %2d: %s\n", i, argv[i]);
	}
	printf("\nEnvironment setup = \n");
	ev = env;
	while(*ev) {
		printf("\t%s\n", *ev);
		ev++;
	}
	printf("\n\n");

	printf("Type '<ctl>C' to stop the program\n");
	gets(str);
	printf("Ehhh.. you typed '%s'\n", str);
	return(0);
}
