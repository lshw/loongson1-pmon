/*
 * GZIP Compression functions for kernel crash dumps.
 *
 * Created by: Matt Robinson (yakker@sourceforge.net)
 * Copyright 2001 Matt D. Robinson.  All rights reserved.
 *
 * This code is released under version 2 of the GNU GPL.
 */
#include <stdio.h>
#include <linux/zlib.h>
typedef unsigned int u32;
typedef unsigned char u8;
#define	ENOMEM		12	/* Out of memory */

static void *deflate_workspace;
static void *inflate_workspace;

char *new;
int newsize;
static u32
compress_gzip(const u8 *old, u32 oldsize)
{
	/* error code and dump stream */
	int err;
	char *buf;
	int bufsize;
	z_stream dump_stream;
	buf=new=malloc(oldsize);
	bufsize=oldsize;
	
	dump_stream.workspace = deflate_workspace;
	
	if ((err = zlib_deflateInit(&dump_stream,Z_BEST_COMPRESSION)) != Z_OK) {
		printf("compress_gzip(): zlib_deflateInit() " "failed (%d)!\n", err);
		return 0;
	}

	dump_stream.next_in = (u8 *) old;
	dump_stream.avail_in = oldsize;
while(1)
{
	dump_stream.next_out = buf;
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
new=realloc(new,bufsize+oldsize);
buf=new+dump_stream.total_out;
bufsize+=oldsize;
}
	/* let's end the deflated compression stream */
	if ((err = zlib_deflateEnd(&dump_stream)) != Z_OK) {
		printf("compress_gzip(): zlib_deflateEnd() "
			"failed (%d)!\n", err);
	}
	newsize=dump_stream.total_out;

	return dump_stream.total_out;
}
#define MYDBG printf("debug:%s,%d\n",__FUNCTION__,__LINE__);
static u32
compress_gunzip(const u8 *old, u32 oldsize)
{
	/* error code and dump stream */
	int err;
	char *buf;
	int bufsize;
	z_stream dump_stream;
	buf=new=malloc(oldsize);
	bufsize=oldsize;
	
	dump_stream.workspace = inflate_workspace;
	
	if ((err = zlib_inflateInit(&dump_stream)) != Z_OK) {
		/* fall back to RLE compression */
		printf("compress_gunzip(): zlib_inflateInit() "
			"failed (%d)!\n", err);
		return 0;
	}

	dump_stream.next_in = (u8 *) old;
	dump_stream.avail_in = oldsize;


while(1){	
	dump_stream.next_out = buf;
	dump_stream.avail_out = oldsize;

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
new=realloc(new,bufsize+oldsize);
buf=new+dump_stream.total_out;
bufsize+=oldsize;
}

	/* let's end the deflated compression stream */
	if ((err = zlib_inflateEnd(&dump_stream)) != Z_OK) {
		printf("compress_gunzip(): zlib_inflateEnd() "
			"failed (%d)!\n", err);
	}
	newsize=dump_stream.total_out;

	/* return the compressed byte total (if it's smaller) */
	return dump_stream.total_out;
}


/* setup the gzip compression functionality */
int main(int argc,char **argv)
{
u32 fsize;
char *buf;
FILE *fin;
FILE *fout;
if(argc<3){printf("usage:gzip filein fileout [-d]\n");return -1;}

deflate_workspace = malloc(zlib_deflate_workspacesize());
if (!deflate_workspace) {
		printf("compress_gzip_init(): Failed to "
			"alloc %d bytes for deflate workspace\n",
			zlib_deflate_workspacesize());
		goto out;
	}
inflate_workspace = malloc(zlib_inflate_workspacesize());

if (!inflate_workspace) {
		printf("compress_gunzip_init(): Failed to "
			"alloc %d bytes for deflate workspace\n",
			zlib_inflate_workspacesize());
		goto out;
	}
fin=fopen(argv[1],"rb");
fseek(fin,0,SEEK_END);
fsize=ftell(fin);
fseek(fin,0,SEEK_SET);
buf=malloc(fsize);
fread(buf,fsize,1,fin);
fclose(fin);

if(argc>3)
compress_gunzip(buf,fsize);
else
compress_gzip(buf,fsize);
fout=fopen(argv[2],"wb");
fwrite(new,newsize,1,fout);
fclose(fout);
free(new);
free(buf);
out:
if(deflate_workspace)free(deflate_workspace);
if(inflate_workspace)free(inflate_workspace);
return 0;
}

