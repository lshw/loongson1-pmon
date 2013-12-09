#include <machine/pio.h>
#include <stdio.h>
#include <pmon.h>
#include <cpu.h>

#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <file.h>
#include <mtdfile.h>

unsigned char yaf_use = 0;		//lxy
unsigned char yaf_w = 1;

static int devcp(int argc, char **argv)
{
	char *buf;
	int fp0, fp1;
	int n, i;
	int bs = 0x20000;
	int seek = 0, skip = 0;
	char *fsrc = 0, *fdst = 0;
	unsigned int count = -1, nowcount = 0;
	char pstr[80] = "";
	int quiet = 0;

	yaf_use = 0;	//lxy
	yaf_w = 1;

#if NGZIP > 0
	int unzip=0;
#endif
	if (argc < 3)
		return -1;
	fsrc = argv[1];
	fdst = argv[2];
	if (!fsrc||!fdst)
		return -1;
	fp0 = open(fsrc, O_RDONLY);
	fp1 = open(fdst, O_RDWR|O_CREAT|O_TRUNC);

	if (!strncmp(_file[fp1].fs->devname, "mtd", 3)) {
		mtdpriv *priv;
		mtdfile *p;
		priv = (mtdpriv *)_file[fp1].data;
		p = priv->file;
		if (p->mtd->type == MTD_NANDFLASH) {
			bs = p->mtd->erasesize;
		}
	}

	for (i=3; i<argc; i++) {
		if(!strncmp(argv[i],"bs=",3))
		 	bs=strtoul(&argv[i][3],0,0);
		else if(!strncmp(argv[i],"count=",6))
		 	count=strtoul(&argv[i][6],0,0);
		else  if(!strncmp(argv[i],"skip=",5))
		 	skip=strtoul(&argv[i][5],0,0);
		else if(!strncmp(argv[i],"seek=",5))
		 	seek=strtoul(&argv[i][5],0,0);
		else if(!strncmp(argv[i],"quiet=",6))
			quiet=strtoul(&argv[i][6],0,0);
#if NGZIP > 0
		else if(!strcmp(argv[i],"unzip=1"))
			unzip=1;
#endif
		else if (!strcmp(argv[i], "yaf"))		//lxy
		{
			yaf_use = 1;
			bs += (4 * 1024);
		}
		else if (!strcmp(argv[i], "nw"))	//lxy
			yaf_w = 0;
	}

	buf = malloc(bs);
//	buf = (char *)(((long)malloc(bs + 32) + 32) & ~(32-1) | 0xa0000000);
//	buf = 0xa1000000;
	if (!buf) {
		printf("malloc failed!,please set heaptop bigger\n");
		return -1;
	}

	if (!fp0||!fp1) {
		printf("open file error!\n");
		free(buf);
		return -1;
	}
	lseek(fp0, skip*bs, SEEK_SET);
	lseek(fp1, seek*bs, SEEK_SET);
#if NGZIP > 0
	if (unzip)
		if (gz_open(fp0) == -1)
			unzip = 0;
#endif
	while(count--) {
		int rcount=0;
		char *pnow=buf;
#if NGZIP > 0
		if (unzip) 
			while (rcount<bs) {
				n = gz_read(fp0, pnow, bs);
				if (n <= 0)
					break; 
				rcount += n;
				pnow += n;
			}
		else
#endif
			while (rcount<bs) {
				n = read(fp0, pnow, bs-rcount);
				if (n <= 0)
					break;
				rcount += n;
				pnow += n;
			}
		nowcount += rcount;
		if (!(strstr(argv[1],"/dev/tty") || strstr(argv[2],"/dev/tty") || quiet)) {
			int i;
			for (i=0; i<strlen(pstr); i++)
				printf("\b \b");
			sprintf(pstr, "%d", nowcount);
			printf("%s", pstr);
		}
		if (write(fp1, buf, rcount) < rcount || rcount <bs)
			break;
	}
	free(buf);
#if NGZIP > 0
	if (unzip)
		gz_close(fp0);
#endif
	close(fp0);
	close(fp1);
	if (nowcount == 0)	//lxy
		return -1;
	return 0;
}

static const Cmd Cmds[] = {
	{"MyCmds"},
	{"devcp", "srcfile dstfile [bs=0x20000] [count=-1] [seek=0] [skip=0] [quiet=0] [yaf] [nw]", 
	  0, "copy form src to dst", devcp, 0, 99, CMD_REPEAT},
	{0, 0}
};

static void init_cmd __P((void)) __attribute__ ((constructor));

static void init_cmd(void)
{
	cmdlist_expand(Cmds, 1);
}
