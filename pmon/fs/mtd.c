/*
 * Copyright (c) 2000 Opsycon AB  (www.opsycon.se)
 * Copyright (c) 2002 Patrik Lindergren (www.lindergren.com)
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
 *	This product includes software developed by Opsycon AB.
 *	This product includes software developed by Patrik Lindergren.
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
/* $Id: ramfile.c,v 1.1.1.1 2006/09/14 01:59:08 root Exp $ */

#include <stdio.h>
#include <string.h>
#include <termio.h>
#include <fcntl.h>
#include <file.h>
#include <ctype.h>
#include <mtdfile.h>
#include <unistd.h>
#include <stdlib.h>
#undef _KERNEL
#include <errno.h>
#include "logfile.h"
#include <pmon.h>
#include <linux/mtd/mtd.h>
#include "../sys/dev/nand/yaf-nand/nand.h"


LIST_HEAD(mtdfiles, mtdfile) mtdfiles = LIST_HEAD_INITIALIZER(mtdfiles);

static int mtdidx = 0;
static struct mtdfile *goodpart0 = 0;
extern unsigned char use_yaf;		//lxy

extern int nand_write_skip_bad(nand_info_t *nand, 
	loff_t offset, size_t *length, 	u_char *buffer, int withoob);

static int mtdfile_open(int , const char *, int, int);
static int mtdfile_close(int);
static int mtdfile_read(int, void *, size_t);
static int mtdfile_write(int, const void *, size_t);
static off_t mtdfile_lseek(int, off_t, int);

static int file_to_mtd_pos(int fd, int *plen);

static int
   mtdfile_open (fd, fname, mode, perms)
   int	       fd;
   const char *fname;
   int         mode;
   int	       perms;
{
	mtdfile *p;
	mtdpriv *priv;
	char    *dname,namebuf[64];
	int idx=-1;
	int found = 0;
	int open_size=0;
	int open_offset=0;
	char *poffset=0,*psize;
      
	strncpy(namebuf,fname,63);
	dname = namebuf;
	if (strncmp (dname, "/dev/", 5) == 0)
		dname += 5;
	if (strncmp (dname, "mtd", 3) == 0)
		dname += 3;
	else return -1;
	
	if(dname[0]=='*') idx=-1;
	else if(dname[0]=='/')
	{
	dname++;
	 if(dname[0])
	 {
	if((poffset=strchr(dname,'@'))) *poffset++=0;
		LIST_FOREACH(p, &mtdfiles, i_next) {
		if(!strcmp(p->name,dname))
		goto foundit;
		}
	 }
	}
	else if(isdigit(dname[0])){
	if((poffset=strchr(dname,'@'))) *poffset++=0;
	idx=strtoul(dname,0,0);
	}
	else idx=-1;


		
		LIST_FOREACH(p, &mtdfiles, i_next) {
			if(idx==-1)
			 {
			 if(p->part_offset||(p->part_size!=p->mtd->size))
			  printf("mtd%d: flash:%d size:%d writesize:%d partoffset=0x%x,partsize=%d %s\n",p->index,p->mtd->index,p->mtd->size,p->mtd->writesize,p->part_offset,p->part_size,p->name);
			 else
			  printf("mtd%d: flash:%d size:%d writesize:%d %s\n",p->index,p->mtd->index,p->mtd->size,p->mtd->writesize,p->name);
			}
			else if(p->index==idx) {
				found = 1;
				break;
			}	
		}
		
		if(!found) {
			return(-1);
		}
foundit:
	
	if(poffset)
	{
	 if((psize=strchr(poffset,',')))
	 {
	  *psize++=0;
	  open_size=strtoul(psize,0,0);
	 }
	 open_offset=strtoul(poffset,0,0);
	}
	
	p->refs++;
	priv=malloc(sizeof(struct mtdpriv));
	priv->file=p;
	priv->open_size=open_size?open_size:p->part_size;
	priv->open_offset=open_offset;

	_file[fd].posn = 0;
	_file[fd].data = (void *)priv;
	
	return (fd);
}

static int mtdfile_close(int fd)
{
	mtdpriv *priv;

	priv = (mtdpriv *)_file[fd].data;
	priv->file->refs--;
	free(priv);
	   
	return(0);
}

static int mtdfile_read(int fd, void *buf, size_t n)
{
	mtdfile *p;
	mtdpriv *priv;
	int maxlen, newpos;
	size_t retlen = 0, left = n;
	unsigned int block_inc;

	priv = (mtdpriv *)_file[fd].data;
	p = priv->file;

	if (_file[fd].posn + n > priv->open_size)
		n = priv->open_size - _file[fd].posn;
	
	if (p->mtd->type == MTD_NANDFLASH) {
		block_inc = nand_read_skip_bad(p->mtd, _file[fd].posn+priv->open_offset+p->part_offset, &n, buf);
		if (block_inc < 0)
			printf ("lxy: error happen, while programing flash......\n");
		else if (block_inc > 0)
			_file[fd].posn += block_inc * p->mtd->erasesize;
	} else if (p->mtd->type == MTD_NORFLASH) {
		while(left) {
			newpos = file_to_mtd_pos(fd, &maxlen);
			p->mtd->read(p->mtd, newpos, min(left, maxlen), &retlen, buf);
			if(retlen<=0)break;
			_file[fd].posn += retlen;
			buf += retlen;
			left -= retlen;
		}
		return (n-left);
	}

	_file[fd].posn += n;
      
	return (n);
}

static int mtdfile_write(int fd, const void *data, size_t n)
{
	mtdfile *p;
	mtdpriv *priv;
	struct erase_info erase;
	unsigned int start_addr;
	int maxlen;
	size_t retlen = 0, left = n;
	void *buf = (void *)data;

	unsigned int block_inc;
	extern unsigned char yaf_use;	//lxy
	extern unsigned char yaf_w;
	size_t n1 = n;

	if (yaf_use)		//lxy: yaffs2'img need a block of 128Kb, also with 4Kb oob
		n -= 4*1024;

	priv = (mtdpriv *)_file[fd].data;
	p = priv->file;

	if (_file[fd].posn + n > priv->open_size)
		n = priv->open_size - _file[fd].posn;

	if (p->mtd->type == MTD_NANDFLASH) {
		start_addr = _file[fd].posn + priv->open_offset + p->part_offset;
		n = (n + p->mtd->writesize - 1) & ~(p->mtd->writesize - 1);

		erase.mtd = p->mtd;
		erase.callback = 0;
		erase.addr = (start_addr + p->mtd->erasesize - 1) & ~(p->mtd->erasesize - 1);
		erase.len = (n + p->mtd->erasesize - 1) & ~(p->mtd->erasesize - 1);
		erase.priv = 0;

		if (yaf_w) {		//lxy: check bad block && erase
			if(erase.addr>=start_addr && erase.addr<start_addr+erase.len) {
				//lxy: if erase return !=0, then it is bad block, so we skip it
				while (p->mtd->erase(p->mtd,&erase) != 0) {
					erase.addr += p->mtd->erasesize;
					start_addr += p->mtd->erasesize;
					_file[fd].posn += p->mtd->erasesize;			
				}
			}
		} else {	//lxy: only check bad block
			while (p->mtd->block_isbad(p->mtd, start_addr & ~(p->mtd->erasesize - 1))) {
				printf ("\nSkip bad block 0x%08llx\n", start_addr & ~(p->mtd->erasesize - 1));
				erase.addr += p->mtd->erasesize;
				start_addr += p->mtd->erasesize;
				_file[fd].posn += p->mtd->erasesize;
			}
		}

		if (yaf_use) {	//lxy: program data
			block_inc = nand_write_skip_bad(p->mtd, start_addr, &n1, buf, 1);
			if (block_inc<0)
				printf ("lxy: error while programing nandflash..........\n");
			else if (block_inc>0) {
				_file[fd].posn += block_inc * p->mtd->erasesize;
			}
		} else {
			p->mtd->write(p->mtd, start_addr, n, &n, buf);
		}
		
		_file[fd].posn += n;

		return (n1);		//lxy
	} else if (p->mtd->type == MTD_NORFLASH) {
		while (left) {
			start_addr = file_to_mtd_pos(fd, &maxlen);
			maxlen = min(left, maxlen);
//			maxlen = (maxlen+p->mtd->writesize-1)&~(p->mtd->writesize-1);		//lxy disable this for norflash

			erase.mtd = p->mtd;
			erase.callback = 0;
			erase.addr = (start_addr + p->mtd->erasesize - 1) & ~(p->mtd->erasesize - 1);
			erase.len = (maxlen + p->mtd->erasesize - 1) & ~(p->mtd->erasesize - 1);
			erase.priv = 0;

			if(erase.addr>=start_addr && erase.addr<start_addr+maxlen) {
				p->mtd->erase(p->mtd, &erase);
			}

			p->mtd->write(p->mtd, start_addr, maxlen, &retlen, buf);

			if (retlen <= 0)
				break;
			_file[fd].posn += retlen;
			buf += retlen;
			if (left > retlen)
				left -= retlen;
			else
				left = 0;
		}
		return (n-left);
	}
}

/*************************************************************
 *  mtdfile_lseek(fd,offset,whence)
 */
static off_t
mtdfile_lseek (fd, offset, whence)
	int             fd, whence;
	off_t            offset;
{
	mtdfile        *p;
	mtdpriv *priv;

	priv = (mtdpriv *)_file[fd].data;
	p = priv->file;


	switch (whence) {
		case SEEK_SET:
			_file[fd].posn = offset;
			break;
		case SEEK_CUR:
			_file[fd].posn += offset;
			break;
		case SEEK_END:
			_file[fd].posn = priv->open_size + offset;
			break;
		default:
			errno = EINVAL;
			return (-1);
	}
	return (_file[fd].posn);
}



int add_mtd_device(struct mtd_info *mtd, int offset, int size, char *name)
{
	struct mtdfile *rmp;
	int len = sizeof(struct mtdfile);

	printf("Creat MTD partitions on \"%s\": name=\"%s\" size=%dByte\n", mtd->name, name, size);

	if (name)
		len += strlen(name);

	rmp = (struct mtdfile *)malloc(len);
	if (rmp == NULL) {
		fprintf(stderr, "Out of space adding mtdfile");
		return(NULL);
	}

	bzero(rmp, len);
	rmp->mtd = mtd;
	rmp->index = mtdidx++;
	rmp->part_offset = offset;
	if (size)
		rmp->part_size = size;
	else
		rmp->part_size = mtd->size - offset;
	if (name)
		strcpy(rmp->name, name);
	if (!mtdfiles.lh_first) {
		rmp->i_next.le_next = 0;
		rmp->i_next.le_prev = &rmp->i_next;
		mtdfiles.lh_first = rmp;
	}
	else {
		*mtdfiles.lh_first->i_next.le_prev=mtdfiles.lh_first;
		LIST_INSERT_BEFORE(mtdfiles.lh_first, rmp, i_next);
		*mtdfiles.lh_first->i_next.le_prev = 0;
	}
/*add into badblock table */
	if (!strcmp(name,"g0")) {
		goodpart0 = rmp;
	}
	return 0;
}

static int file_to_mtd_pos(int fd, int *plen)
{
	struct mtdfile *goodpart;
	mtdfile *p;
	mtdpriv *priv;
	int add,file_start,offset,pos;

	priv = (mtdpriv *)_file[fd].data;
	p = priv->file;

	offset = 0;
	file_start = p->part_offset + priv->open_offset;
	pos = _file[fd].posn;

	if (getenv("goodpart")) {
		goodpart=goodpart0;
		add=goodpart0->part_offset;
		while(goodpart) {
			if(file_start<goodpart->part_offset)
			offset += add;
			if(file_start+pos+offset>=goodpart->part_offset && file_start+pos+offset<goodpart->part_offset+goodpart->part_size)break;


			if(goodpart->i_next.le_next)add=goodpart->i_next.le_next->part_offset-goodpart->part_offset-goodpart->part_size;
			goodpart=goodpart->i_next.le_next;
		}
		if(plen)*plen=(goodpart->part_offset+goodpart->part_size)-(file_start+pos+offset);
	} else {
		if (plen)
			*plen = (p->part_offset+p->part_size)-(file_start+pos+offset);
	}

	return file_start + pos + offset;
}

int del_mtd_device (struct mtd_info *mtd)
{
	struct mtdfile *rmp;

	LIST_FOREACH(rmp, &mtdfiles, i_next) {
		if(rmp->mtd==mtd) {
			if(rmp->refs == 0) {
				LIST_REMOVE(rmp, i_next);
				free(rmp);
				return(0);
			} else {
				return(-1);
			}
		}
	}
	return(-1);
}

static FileSystem mtdfs =
{
	"mtd", FS_MEM,
	mtdfile_open,
	mtdfile_read,
	mtdfile_write,
	mtdfile_lseek,
	mtdfile_close,
	NULL
};

static void init_fs __P((void)) __attribute__ ((constructor));

static void init_fs(void)
{
	/*
	 * Install ram based file system.
	 */
	filefs_init(&mtdfs);

}

static int mtd_erase(int argc, char **argv)
{
	int fd;
	mtdfile	*p;
	mtdpriv *priv;
	struct erase_info erase;
	unsigned int start_addr;
	int erase_block, count;
	
	if (argc < 2)
		return -1;
	
	fd = open (argv[1],O_RDWR);
	priv = (mtdpriv *)_file[fd].data;
	p = priv->file;
	erase_block = p->part_size / p->mtd->erasesize;	
	start_addr = _file[fd].posn + priv->open_offset + p->part_offset;

	erase.mtd = p->mtd;
	erase.callback = 0;
	erase.addr = (start_addr + p->mtd->erasesize - 1) & ~(p->mtd->erasesize - 1);
	erase.len = p->mtd->erasesize;
	erase.priv = 0;

	printf("mtd_erase working: \n0x%08x  ", erase.addr);
	for (count=0; count<erase_block; count++) {
		if (p->mtd->erase(p->mtd,&erase) != 0) {
			erase.addr += p->mtd->erasesize;
			printf("0x%08x  ");
			continue;
		} else {
			erase.addr += p->mtd->erasesize;
		}
		printf("\b\b\b\b\b\b\b\b\b\b%08x  ", erase.addr);
	}
	close (fd);
	printf("\nmtd_erase work done!\n");

	return 0;
}

static const Cmd Cmds[] =
{
	{"MyCmds"},
	{"mtd_erase", "mtd_erase [device]", 0, "yaffs write and read", mtd_erase, 1, 99, CMD_REPEAT},
	{0, 0}
};

static void init_cmd __P((void)) __attribute__ ((constructor));

void init_cmd(void)
{
	cmdlist_expand(Cmds, 1);
}

