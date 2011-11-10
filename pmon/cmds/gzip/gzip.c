/*
 * GZIP Compression functions for kernel crash dumps.
 *
 * Created by: Matt Robinson (yakker@sourceforge.net)
 * Copyright 2001 Matt D. Robinson.  All rights reserved.
 *
 * This code is released under version 2 of the GNU GPL.
 */
#include <stdio.h>
#include <termio.h>
#include <string.h>
#include <setjmp.h>
#include <sys/endian.h>
#include <ctype.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#ifdef _KERNEL
#undef _KERNEL
#include <sys/ioctl.h>
#define _KERNEL
#else
#include <sys/ioctl.h>
#endif

#include <machine/cpu.h>

#include <pmon.h>
#include <dev/pci/pcivar.h>
#include <dev/pci/pcidevs.h>
#include <flash.h>

/* header files */
#include <linux/zlib.h>
#include <stdio.h>
typedef unsigned int u32;
typedef unsigned char u8;
#define	ENOMEM		12	/* Out of memory */
#include "random.c"

static void *deflate_workspace;
static void *inflate_workspace;

static u32
compress_gzip(const u8 *old, u32 oldsize, u8 *new)
{
	/* error code and dump stream */
	int err;
	z_stream dump_stream;
	
	dump_stream.workspace = deflate_workspace;
	
	if ((err = zlib_deflateInit(&dump_stream,Z_BEST_COMPRESSION)) != Z_OK) {
		printf("compress_gzip(): zlib_deflateInit() " "failed (%d)!\n", err);
		return 0;
	}

	dump_stream.next_in = (u8 *) old;
	dump_stream.avail_in = oldsize;

	dump_stream.next_out = new;
while(1)
{
	dump_stream.avail_out = oldsize;

	/* deflate the page -- check for error */
	err = zlib_deflate(&dump_stream, Z_FINISH);
	if(err==Z_STREAM_END)break;
	if(err<0  && (err!=Z_BUF_ERROR)){
		/* zero is return code here */
		printf("compress_gzip(): zlib_deflate() failed (%d),total_out=%x,next_out=%x!\n",
			err,dump_stream.total_out,dump_stream.next_out);
		(void)zlib_deflateEnd(&dump_stream);
		return 0;
	}
}
	/* let's end the deflated compression stream */
	if ((err = zlib_deflateEnd(&dump_stream)) != Z_OK) {
		printf("compress_gzip(): zlib_deflateEnd() "
			"failed (%d)!\n", err);
	}

	return dump_stream.total_out;
}

static u32
compress_gunzip(const u8 *old, u32 oldsize,u8 *new)
{
	/* error code and dump stream */
	int err;
	z_stream dump_stream;
	
	dump_stream.workspace = inflate_workspace;
	
	if ((err = zlib_inflateInit(&dump_stream)) != Z_OK) {
		/* fall back to RLE compression */
		printf("compress_gunzip(): zlib_inflateInit() "
			"failed (%d)!\n", err);
		return 0;
	}

	dump_stream.next_in = (u8 *) old;
	dump_stream.avail_in = oldsize;


	dump_stream.next_out = new;
while(1){	
	dump_stream.avail_out =2*oldsize;

	err = zlib_inflate(&dump_stream, Z_FINISH);
	if(err==Z_STREAM_END)break;
	else if(err<0 && (err!=Z_BUF_ERROR))
	{
		/* zero is return code here */
		(void)zlib_inflateEnd(&dump_stream);
		printf("compress_gunzip(): zlib_inflate() failed (%d)!\n",
			err);
		return 0;
	}
}

	/* let's end the deflated compression stream */
	if ((err = zlib_inflateEnd(&dump_stream)) != Z_OK) {
		printf("compress_gunzip(): zlib_inflateEnd() "
			"failed (%d)!\n", err);
	}

	/* return the compressed byte total (if it's smaller) */
	return dump_stream.total_out;
}

#define FFLAG 1
#define LFLAG 2
#define RFLAG 4

/* setup the gzip compression functionality */
int gzip(int argc,char **argv)
{
u32 fsize,newsize,newsize1,faddr;
char *buf,*buf1,*buf2;
unsigned int *src,*dst;
unsigned int *pint;
int i,err;
char c,flag;
static int first=1;
err=0;
fsize=0x100000;
faddr= strtoul(getenv("heaptop"),0,16);
if(faddr<(unsigned int)heaptop)faddr=(unsigned int)heaptop;
flag=0;

optind = err = 0;
while ((c = getopt (argc, argv, "f:s:lr")) != EOF) {
		switch (c) {
			case 'f':
				flag |= FFLAG; 
				faddr=strtoul(optarg,0,0);
				break;
			case 's':
				fsize=strtoul(optarg,0,0);
				break;
			case 'l':
				flag |= LFLAG;break;
			case 'r':
				flag|=RFLAG;
				break;
			default:
				err++;
				break;
		}
	}




deflate_workspace = malloc(zlib_deflate_workspacesize());
if (!deflate_workspace) {
		printf("compress_gzip_init(): Failed to "
			"alloc %d bytes for deflate workspace\n",
			zlib_deflate_workspacesize());
		err= -ENOMEM;
		goto out;
	}
inflate_workspace = malloc(zlib_inflate_workspacesize());

if (!inflate_workspace) {
		printf("compress_gunzip_init(): Failed to "
			"alloc %d bytes for deflate workspace\n",
			zlib_inflate_workspacesize());
		err= -ENOMEM;
		goto out;
	}

buf=(void *)faddr;

		if(!(flag&&FFLAG)){Rand_seed(CPU_GetCOUNT(),~CPU_GetCOUNT());}
		
do{
	if(!(flag&FFLAG)&&((flag&RFLAG)||!first))
	{
		for(i=0,pint=(void *)buf;i<fsize/4;i++)
		{
			pint[i]=Rand();
		}
		first=1;
	}
	buf1=buf+fsize;
	newsize=compress_gzip(buf,fsize,buf1);
	buf2=(void *)((int)(buf1+newsize+3)&~3);
	newsize1=compress_gunzip(buf1,newsize,buf2);
	printf("fsize=%x,newsize=%x,newsize1=%x\n",fsize,newsize,newsize1);
	if(newsize1!=fsize){printf("size error\n");}
	for(src=(void *)buf,dst=(void *)buf2;src<(u32 *)buf1;src++,dst++)
	{
		if(*src!=*dst)
		{
			printf("src=%08x dst=%08x %08x!=%08x\n",src,dst,*src,*dst);
		}
	}
}while(flag&LFLAG);
out:
if(deflate_workspace)free(deflate_workspace);
if(inflate_workspace)free(inflate_workspace);
return 0;
}

/* module initialization */

static const Cmd Cmds[] =
{
	{"MyCmds"},
	{"gzip","[-f fileaddr] [-s filesize] -l -r",0,"gzip test",gzip,0,99,CMD_REPEAT},
	{0, 0}
};


static void init_cmd __P((void)) __attribute__ ((constructor));

static void
init_cmd()
{
	cmdlist_expand(Cmds, 1);
}


