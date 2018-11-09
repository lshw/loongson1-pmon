#ifdef SIM
#include "stdio.h"
#else 
typedef int size_t;
extern void tgt_putchar(char c);
#endif
#include "zlib_pmon.bin.c"

#define __init
#define KERN_ERR

#include "memop.c"
#include <linux/zlib.h>
typedef unsigned int u32;
typedef unsigned char u8;

static void *inflate_workspace;

char printerr(char *msg,int value)
{
 static char buf[0x100];
stringserial(msg);
buf[0]="(";
buf[1]=0;
if(value<0)btoa(buf+1,value,-10);
else btoa(buf+1,value,10);
strcat(buf,")!\n");
stringserial(buf);
}

int 
strlen(const char *p)
{
	int n;

	if (!p)
		return (0);
	for (n = 0; *p; p++)
		n++;
	return (n);
}
/**
 * strcat - Append one %NUL-terminated string to another
 * @dest: The string to be appended to
 * @src: The string to append to it
 */
char * strcat(char * dest, const char * src)
{
	char *tmp = dest;

	while (*dest)
		dest++;
	while ((*dest++ = *src++) != '\0')
		;

	return tmp;
}


/*
 *  char *btoa(dst,value,base) 
 *      converts value to ascii, result in dst
 */
char *
btoa(char *dst, unsigned int value, int base)
{
	char buf[34], digit;
	int i, j, rem, neg;

	if (value == 0) {
		dst[0] = '0';
		dst[1] = 0;
		return (dst);
	}

	neg = 0;
	if (base == -10) {
		base = 10;
		if (value & (1L << 31)) {
			value = (~value) + 1;
			neg = 1;
		}
	}

	for (i = 0; value != 0; i++) {
		rem = value % base;
		value /= base;
		if (rem >= 0 && rem <= 9)
			digit = rem + '0';
		else if (rem >= 10 && rem <= 36)
			digit = (rem - 10) + 'a';
		buf[i] = digit;
	}

	buf[i] = 0;
	if (neg)
		strcat (buf, "-");

	/* reverse the string */
	for (i = 0, j = strlen (buf) - 1; j >= 0; i++, j--)
		dst[i] = buf[j];
	dst[i] = 0;
	return (dst);
}

static int
compress_gunzip(const u8 *old, u32 oldsize,u8 *new)
{
	/* error code and dump stream */
	int err=0;
	z_stream dump_stream;
	
	dump_stream.workspace = inflate_workspace;
	
	if ((err = zlib_inflateInit(&dump_stream)) != Z_OK) {
		/* fall back to RLE compression */
		printerr("compress_gunzip(): zlib_inflateInit() "
			"failed",err);
		return err;
	}

	dump_stream.next_in = (u8 *) old;
	dump_stream.avail_in = oldsize;


	dump_stream.next_out = new;
while(1){	
	dump_stream.avail_out =0x8000;

	err = zlib_inflate(&dump_stream, Z_FINISH);
	if(err==Z_STREAM_END)break;
	else if(err<0 && (err!=Z_BUF_ERROR))
	{
		/* zero is return code here */
		(void)zlib_inflateEnd(&dump_stream);
		printerr("compress_gunzip(): zlib_inflate() failed",err);
		return err;
	}
	tgt_putchar('.');
	
}

	/* let's end the deflated compression stream */
	if ((err = zlib_inflateEnd(&dump_stream)) != Z_OK) {
		printerr("compress_gunzip(): zlib_inflateEnd() "
			"failed",err);
	}

	/* return the compressed byte total (if it's smaller) */
	return err;
}


extern char     end[];
static int __init run_unzip(char *start,int to)
{
int err;
again:
inflate_workspace = (void *)end;
err=compress_gunzip(start,sizeof(biosdata),to);
if(err<0){stringserial("retry\n");goto again;}
}

#ifdef SIM
int main(int argc,char **argv)
{
	fpw=fopen("tmp.txt","wb");
	run_unzip(biosdata,0x80010000);
	fclose(fpw);
}
#else
#include "initmips.c"
int read,write,open,close,printf,vsprintf,getenv,tgt_reboot,CpuTertiaryCacheSize;
#endif

