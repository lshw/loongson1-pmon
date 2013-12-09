static struct parttype {
	char *name;
	unsigned char type;
} known_parttype[]= {
	"FAT12",1,
	"FAT16",4,
	"W95 FAT32",0xb,
	"W95 FAT32 (LBA)",0xc,
	"W95 FAT16 (LBA)",0xe,
	"W95 Ext'd (LBA)",0xf,
	"Linux",0x83,
	"Linux Ext'd" , 0x85,
	"Dos Ext'd" , 5,
	"Linux swap / Solaris" , 0x82,
	"Linux raid" , 0xfd,	/* autodetect RAID partition */
	"Freebsd" , 0xa5,    /* FreeBSD Partition ID */
	"Openbsd" , 0xa6,    /* OpenBSD Partition ID */
	"Netbsd" , 0xa9,   /* NetBSD Partition ID */
	"Minix" , 0x81,  /* Minix Partition ID */
};

struct partition {
    unsigned char boot_ind;     /* 0x80 - active */
    unsigned char head;     /* starting head */
    unsigned char sector;       /* starting sector */
    unsigned char cyl;      /* starting cylinder */
    unsigned char sys_ind;      /* What partition type */
    unsigned char end_head;     /* end head */
    unsigned char end_sector;   /* end sector */
    unsigned char end_cyl;      /* end cylinder */
    unsigned int start_sect;    /* starting sector counting from 0 */
    unsigned int nr_sects;      /* nr of sectors in partition */
} __attribute__((packed));

size_t fread (void *src, size_t size, size_t count, FILE *fp);
size_t fwrite (const void *dst, size_t size, size_t count, FILE *fp);
static char diskname[0x40];

static int __disksyscall1(int type, long long addr, union commondata *mydata)
{
	char fname[0x40];
	FILE *fp;
	if (strncmp(diskname, "/dev/", 5))
		sprintf(fname, "/dev/disk/%s", diskname);
	else
		strcpy(fname, diskname);
	fp = fopen(fname, "r+");
	if (!fp) {
		printf("open %s error!\n", fname);
		return -1;
	}
	fseek(fp,addr,SEEK_SET);
	switch (type) {
		case 1:fread(&mydata->data1,1,1,fp);break;
		case 2:fread(&mydata->data2,2,1,fp);break;
		case 4:fread(&mydata->data4,4,1,fp);break;
		case 8:fread(&mydata->data8,8,1,fp);break;
	}
	fclose(fp);
	return 0;
}

static int __disksyscall2(int type, long long addr, union commondata *mydata)
{
	char fname[0x40];
	FILE *fp;
	if (strncmp(diskname, "/dev/", 5))
		sprintf(fname, "/dev/disk/%s", diskname);
	else
		strcpy(fname, diskname);
	fp = fopen(fname, "r+");
	if (!fp) {
		printf("open %s error!\n",fname);
		return -1;
	}
	fseek(fp, addr, SEEK_SET);
	switch (type) {
		case 1:fwrite(&mydata->data1,1,1,fp);break;
		case 2:fwrite(&mydata->data2,2,1,fp);break;
		case 4:fwrite(&mydata->data4,4,1,fp);break;
		case 8:fwrite(&mydata->data8,8,1,fp);break;
	}
	fclose(fp);
	return 0;
}

int mydisks(int ac, char **av)
{
	if (ac > 2)
		return -1;
	if (ac == 2) {
		if(nr_strtol(av[1], 0, 0) == -1) {
			syscall1=__syscall1;
			syscall2=__syscall2;
			diskname[0] = 0;
		}
		else {
			strncpy(diskname, av[1], 0x40);
			diskname[0x3f] = 0;
		}
	}

	if (diskname[0]) {
		syscall1=(void *)__disksyscall1;
		syscall2=(void *)__disksyscall2;
		printf("disk select %s\n",diskname);
	}
	else
		printf("select normal memory access\n");

	return (0);
}

static int testide(int ac,char **av)
{
	FILE *fp;
	char *buf;
	long bufsize;
	unsigned long count,x,tmp;
	bufsize=(ac==1)?0x100000:strtoul(av[1],0,0);
	buf=heaptop;
	x=count=0;
	tmp=bufsize>>24;
	fp=fopen("/dev/disk/wd0","r");
	if(!fp){printf("open error!\n");return -1;}
	while(!feof(fp))
	{
		fread(buf,bufsize,1,fp);
		x +=bufsize;
		if((x&0xffffff)==0){count+=tmp?tmp:16;printf("%d\n",count);}
	}
	fclose(fp);
	return 0;
}

static int fdisk(int argc,char **argv)
{
	int j,n,type_counts;
	struct partition *p0;
	FILE *fp;
	char device[0x40];
	int buf[0x10];
		if(strncmp(argv[1],"/dev/",5)) sprintf(device,"/dev/disk/%s",argv[1]);
		else strcpy(device,diskname);
	sprintf(device,"/dev/disk/%s",(argc==1)?"wd0":argv[1]);
	type_counts=sizeof(known_parttype)/sizeof(struct parttype);
	fp=fopen(device,"rb");
	if(!fp)return -1;
	fseek(fp,446,0);
	fread(buf,0x40,1,fp);
	fclose(fp);

	printf("Device Boot %-16s%-16s%-16sId System\n","Start","End","Sectors");
	for(n=0,p0=(void *)buf;n<4;n++,p0++) {
		if(!p0->sys_ind)continue;

		for(j=0;j<type_counts;j++)
		if(known_parttype[j].type==p0->sys_ind)break;

		printf("%-6d %-4c %-16d%-16d%-16d%x %s\n",n,(p0->boot_ind==0x80)?'*':' ',p0->start_sect,p0->start_sect+p0->nr_sects,p0->nr_sects,\
		p0->sys_ind,j<type_counts?known_parttype[j].name:"unknown");
	}
	return 0;
}

static const Cmd disk_cmds[] = {
	{"MyCmds"},
	{"testide","[onecesize]",0,"test ide dma",testide,0,99,CMD_REPEAT},
	{"fdisk","diskname",0,"dump disk partation",fdisk,0,99,CMD_REPEAT},
	{"disks",	"disk", 0, "select disk", mydisks, 0, 99, CMD_REPEAT},
	{0, 0}
};

static void disk_init_cmd __P((void)) __attribute__ ((constructor));

static void disk_init_cmd(void)
{
	cmdlist_expand(disk_cmds, 1);
}
