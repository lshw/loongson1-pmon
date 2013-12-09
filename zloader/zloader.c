#ifdef SIM
#include "stdio.h"
#else 
typedef int size_t;
extern void tgt_putchar(char c);
void tgt_puts(char *str);
#endif
#include "pmon.bin.c"

#define __init
#define KERN_ERR

#ifndef SIM
#include "memop.c"
char *membase = (char *)(0xffffffff80000000 + (((MEMSIZE < 256 ? MEMSIZE : 256) - 4) << 20));
static char *sbrk(int size)
{
	char *p = membase;
	membase += size;
	return p;
}

#include "malloc.c"
#endif
/*
 * gzip declarations
 */
#define OF(args)  args

#ifndef memzero
#define memzero(s, n)     memset ((s), 0, (n))
#endif

typedef unsigned char  uch;
typedef unsigned short ush;
typedef unsigned long  ulg;

#define INBUFSIZ 4096
#define WSIZE 0x8000    /* window size--must be a power of two, and */
			/*  at least 32K for zip's deflate method */

static uch *inbuf;
static uch *window;

static unsigned insize;  /* valid bytes in inbuf */
static unsigned inptr;   /* index of next byte to be processed in inbuf */
static unsigned outcnt;  /* bytes in output buffer */
static int exit_code;
static long bytes_out;
static int crd_outfd;

static void *dest;

#define get_byte()  (inbuf[inptr++])

#ifdef SIM
FILE *fpw;
#endif

/* Diagnostic functions (stubbed out) */
#define Assert(cond,msg)
#define Trace(x)
#define Tracev(x)
#define Tracevv(x)
#define Tracec(c,x)
#define Tracecv(c,x)

#define STATIC static

static void flush_window(void);
static void error(char *m);
static void gzip_mark(void **);
static void gzip_release(void **);

#include "inflate.c"

static void __init gzip_mark(void **ptr)
{
}

static void __init gzip_release(void **ptr)
{
}

/*
 * Write the output window window[0..outcnt-1] and update crc and bytes_out.
 * (Used for the decompressed data only.)
 */
static void __init flush_window(void)
{
    ulg c = crc;         /* temporary variable */
    unsigned n;
    uch *in, ch;
#ifdef SIM 
	fwrite(window, 1, outcnt, fpw);
#else
	memcpy((void *)dest, window, outcnt);
#endif
	dest = dest + outcnt;
    in = window;
    for (n = 0; n < outcnt; n++) {
	    ch = *in++;
	    c = crc_32_tab[((int)c ^ ch) & 0xff] ^ (c >> 8);
    }
    crc = c;
    bytes_out += (ulg)outcnt;
    outcnt = 0;
}

static void __init error(char *x)
{
	tgt_puts(x);
	exit_code = 1;
}

static int __init run_unzip(char *start, long to)
{
	int result;
	insize = 0;		/* valid bytes in inbuf */
	inptr = 0;		/* index of next byte to be processed in inbuf */
	outcnt = 0;		/* bytes in output buffer */
	exit_code = 0;
	bytes_out = 0;
	crc = (ulg)0xffffffffL; /* shift register contents */

	inbuf = start;
	dest = (void *)to;

	window = malloc(WSIZE);
	if (window == 0) {
		tgt_puts("RAMDISK: Couldn't allocate gzip window\n");
		free(inbuf);
		return -1;
	}
	makecrc();
	result = gunzip();
	free(window);
	return result;
}

#ifdef SIM
int main(int argc, char **argv)
{
	fpw = fopen("tmp.txt", "wb");
	run_unzip(biosdata, 0x80010000);
	fclose(fpw);
}
#else
#include "initmips.c"
int read, write, open, close, getenv, tgt_reboot, CpuTertiaryCacheSize, tgt_reboot;
#endif

