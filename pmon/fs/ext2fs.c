#include <string.h>
#include <pmon.h>
#include <file.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/unistd.h>
#include "ext2fs.h"
#include <diskfs.h>
//#define DEBUG_IDE
extern int   devio_open (int, const char *, int, int);
extern int   devio_close (int);
extern int   devio_read (int, void *, size_t);
extern int   devio_write (int, const void *, size_t);
extern off_t devio_lseek (int, off_t, int);


static int ext2_read_file(int ,void *,size_t ,size_t ,struct ext2_inode * );
static int ReadFromIndexBlock(int ,__u32,__u32,__u8 **,	size_t *,size_t *,__u32 *);
static inline ext2_dirent *ext2_next_entry(ext2_dirent *);
static int ext2_entrycmp(char * ,void *  , int );
static int ext2_get_inode(int ,unsigned long ,struct ext2_inode ** );
static int   ext2_load_linux(int ,int,const unsigned char *);
static int   ext2_load_file_content(int ,struct ext2_inode * ,unsigned char * );
static int   read_super_block(int,int);
int   ext2_open (int, const char *, int, int);
int   ext2_close (int);
int   ext2_read (int, void *, size_t);
int   ext2_write (int, const void *, size_t);
off_t ext2_lseek (int, off_t, int);

__u32 RAW_BLOCK_SIZE=1024;
unsigned long SECTORS_PER_BLOCK=2;	
unsigned long GROUPDESCS_PER_BLOCK=32;	
unsigned long INODES_PER_GROUP=0;
unsigned long EXT2_INODE_SIZE=128;
off_t start_sec;
#define EXT3_SUPER_MAGIC    0xEF53
#define START_PARTION start_sec*512
#define RAW_SECTOR_SIZE 512
#define EXT2_GROUP_DESC_SIZE 32	
static struct ext2_inode INODE_STRUCT;
static struct ext2_inode *File_inode=&INODE_STRUCT;
static int read_super_block(int fd,int index)
{
	__u8 *diskbuf,*leadbuf;
	struct ext2_super_block *ext2_sb;
	int i,find_linux_partion;	
	int err;
	find_linux_partion=0;
	diskbuf=leadbuf=0;
	err=-1;
	int k;

	if((leadbuf=(__u8 *)malloc(RAW_SECTOR_SIZE))==NULL)
	{
		printf("Can't alloc memory for the super block!\n");
		goto out;
	}
	if((diskbuf=(__u8 *)malloc(16*RAW_SECTOR_SIZE))==NULL)
	{
		printf("Can't alloc memory for the super block!\n");
		goto out;
	}
	if(index)
	{
	devio_lseek(fd,0,0);
	if((devio_read(fd,leadbuf,RAW_SECTOR_SIZE))!=RAW_SECTOR_SIZE){		
		printf("Can't read the leading block from disk!\n");
		goto out;
	}
	//search the partion table to find the linux partion wiht id=83
	for(i=446;i<512;i+=0x10){
		if(leadbuf[i+4]==0x83)
		{	
			start_sec=*(unsigned short *)(leadbuf+i+8+2);
			start_sec<<=16;
			start_sec+=*(unsigned short *)(leadbuf+i+8);
			devio_lseek(fd,start_sec*RAW_SECTOR_SIZE,0);
			if((devio_read(fd,diskbuf,16*RAW_SECTOR_SIZE))!=16*RAW_SECTOR_SIZE)
			{
				printf("Read the super block error!\n");
				goto out;
			}
			ext2_sb=(struct ext2_super_block *)(diskbuf+1024);
			if(ext2_sb->s_rev_level==0) //old version
			{
			EXT2_INODE_SIZE=128;
			}
			else
			{
			EXT2_INODE_SIZE=ext2_sb->s_inode_size;
			}
			INODES_PER_GROUP = ext2_sb->s_inodes_per_group;
			RAW_BLOCK_SIZE = BLOCK_1KB << ext2_sb->s_log_block_size;

			SECTORS_PER_BLOCK = RAW_BLOCK_SIZE / RAW_SECTOR_SIZE;
			GROUPDESCS_PER_BLOCK = RAW_BLOCK_SIZE / EXT2_GROUP_DESC_SIZE;
			if(ext2_sb->s_magic==EXT3_SUPER_MAGIC)
			{
			find_linux_partion++;
			if(index==find_linux_partion){
				err=0;
				goto out;
			}
			}
		}
	}
	}

	{
			devio_lseek(fd,0,0);
			start_sec=0;
			if((devio_read(fd,diskbuf,16*RAW_SECTOR_SIZE))!=16*RAW_SECTOR_SIZE)
			{
				printf("Read the super block error!\n");
				goto out;
			}
#if 0
			for (k=0; k<RAW_SECTOR_SIZE; k++)
			{
				printf ("%#x\t", diskbuf[k]);
				if (k%16 == 0)
					printf ("\n");
			}
			printf ("\n");
#endif
	
			ext2_sb=(struct ext2_super_block *)(diskbuf+1024);

			INODES_PER_GROUP = ext2_sb->s_inodes_per_group;
			RAW_BLOCK_SIZE = BLOCK_1KB << ext2_sb->s_log_block_size;

			SECTORS_PER_BLOCK = RAW_BLOCK_SIZE / RAW_SECTOR_SIZE;
			GROUPDESCS_PER_BLOCK = RAW_BLOCK_SIZE / EXT2_GROUP_DESC_SIZE;
			if(ext2_sb->s_magic==EXT3_SUPER_MAGIC)
			{
				err=0;
				goto out;
			}
	}
	printf("There is no %d linux partion\n",index);
out:
	if(leadbuf)free(leadbuf);
	if(diskbuf)free(diskbuf);
	return err;
}

/*
 * p is at least 6 bytes before the end of page
 */
static inline ext2_dirent *ext2_next_entry(ext2_dirent *p)
{
	return (ext2_dirent *)((char*)p + le16_to_cpu(p->rec_len));
}

/* ext2 entry name is not null terminated,so we could not use strcmp 
 * return 0 if the first 'len' characters of 'entry' match those of 's'
 */
static int ext2_entrycmp(char * s,void * entry , int len)
{
	int i;
	if (strlen(s)!=len) return -1;
	for(i=0;i<len;i++)
		if(*(char *)(s+i)!=*(char *)((char *)entry+i)) return -1;
	return 0;
}

/* read inode 'ino' */
static int ext2_get_inode(int fd,unsigned long ino,struct ext2_inode ** ext2_raw_inode_ptr)
{
	unsigned long offset,block,block_group,group_desc,desc;
	
	struct ext2_group_desc * ext2_gd;
	unsigned char * bh;
	off_t temp;	
	/* in which block group*/
	block_group = (ino-1) / INODES_PER_GROUP;
	/* in which block */
	group_desc = block_group / GROUPDESCS_PER_BLOCK ;
	/* introduction block(?), super block , then group descriptors 
	 * introduction block maybe the same as super block 
	 */
	block = 1024/RAW_BLOCK_SIZE + 1 + group_desc;
	/* which descriptor,inside the block */
	desc = block_group % GROUPDESCS_PER_BLOCK;
#ifdef DEBUG_IDE
	printf("ext2_get_inode:ino=%d,block group=%d,block=%d,desc=%d\n",ino,block_group,block,desc);
#endif
	bh = (unsigned char *)malloc(RAW_BLOCK_SIZE);
	temp=(off_t)block*RAW_BLOCK_SIZE+start_sec*512;
	devio_lseek(fd,temp,0);
	if(	RAW_BLOCK_SIZE!=devio_read(fd,bh,RAW_BLOCK_SIZE)){ free(bh);return -1;}
	ext2_gd = (struct ext2_group_desc *) (bh+desc*EXT2_GROUP_DESC_SIZE);
#ifdef DEBUG_IDE
	printf("ext2_group_desc -> bg_free_blocks_count=%d\n",ext2_gd->bg_free_blocks_count);
	printf("ext2_group_desc -> bg_free_inodes_count=%d\n",ext2_gd->bg_free_inodes_count);
	printf("ext2_group_desc -> bg_inode_table=%d\n",ext2_gd->bg_inode_table);
	printf("ext2_group_desc -> bg_block_bitmap=%d\n",ext2_gd->bg_block_bitmap);
	printf("ext2_group_desc -> bg_inode_bitmap=%d\n",ext2_gd->bg_inode_bitmap);
#endif
	offset = ((ino-1)%INODES_PER_GROUP) * EXT2_INODE_SIZE;
	block  = ext2_gd->bg_inode_table + (offset/RAW_BLOCK_SIZE);
	offset = offset % RAW_BLOCK_SIZE;
#ifdef DEBUG_IDE
	printf("ext2_get_inode: offset is %d,block is %d\n",offset,block);
#endif
	bh = (unsigned char * )malloc(RAW_BLOCK_SIZE);
	temp=(off_t)block*RAW_BLOCK_SIZE+start_sec*512;

#ifdef DEBUG_IDE
	printf("the RAW_BLOCK_SIZE is %x\n",RAW_BLOCK_SIZE);
	printf("In ext2fs.c    The seek offset is %llx\n",temp);
	
#endif	
	devio_lseek(fd,temp,0);
	if (RAW_BLOCK_SIZE!=devio_read(fd,bh,RAW_BLOCK_SIZE)) {free(bh);return -1;}
	*ext2_raw_inode_ptr = (struct ext2_inode *) (bh + offset);
#ifdef DEBUG_IDE
	printf("inode->i_block[0]=%d,the inode->i_size=%d \n",(*ext2_raw_inode_ptr)->i_block[0],(*ext2_raw_inode_ptr)->i_size);
#endif
	return 0;
}

static int ext2_load_file_content(int fd,struct ext2_inode * inode,unsigned char * bh)  
{
	return ext2_read_file(fd,bh,inode->i_size,0,inode);
}
/* load linux kernel from ext2 partition
 * return 0 if success,else -1
 */
static int ext2_load_linux(int fd,int index, const unsigned char *path)
{
	struct ext2_inode * ext2_raw_inode;
	ext2_dirent * de;
	unsigned char * bh;
	int i;
	unsigned int inode;
	int find=1;
	unsigned char s[EXT2_NAME_LEN];
	unsigned char pathname[EXT2_NAME_LEN],*pathnameptr;
	unsigned char *directoryname;
	int showdir,lookupdir;
	showdir=0;
	lookupdir=0;
	bh=0;
	if(read_super_block(fd,index))
		return -1;
	
	if((path[0]==0)||(path[strlen(path)-1]=='/'))lookupdir=1;
	
	strncpy(pathname,path,sizeof(pathname));
	pathnameptr=pathname;
	for(inode=EXT2_ROOT_INO;find;)
	{
		for(i=0;pathnameptr[i]&&(pathnameptr[i]!='/');i++);
		pathnameptr[i]=0;
		directoryname=(unsigned char *)pathnameptr;
		pathnameptr=(unsigned char *)(pathnameptr+i+1);
		if(!strlen(directoryname)&&lookupdir)showdir=1;
		if (ext2_get_inode(fd,inode,&ext2_raw_inode)) {
			printf("load EXT2_ROOT_INO error");
			return -1;
		}
		if(!bh)bh=(unsigned char *)malloc(RAW_BLOCK_SIZE+ext2_raw_inode->i_size);
		if(bh==NULL){
			printf("Error in allocting memory for file content!\n");
			return -1;
		}
		ext2_load_file_content(fd,ext2_raw_inode,bh);
		de = (ext2_dirent *) bh;
		find=0;
		
		for ( ; ((unsigned char *) de < bh+ext2_raw_inode->i_size)&&(de->rec_len>0)&&(de->name_len>0) ; de = ext2_next_entry(de)){
			strncpy(s,de->name,de->name_len);
			s[de->name_len]='\0';//*(de->name+de->name_len)='\0';
#ifdef DEBUG_IDE			
			printf("entry:name=%s,inode=%d,rec_len=%d,name_len=%d,file_type=%d\n",s,de->inode,de->rec_len,de->name_len,de->file_type);
#endif
		    if(showdir){
				printf("%s%s",s,((de->file_type)&2)?"/ ":" ");
			}
			if (!ext2_entrycmp(directoryname, de->name, de->name_len)){ 
				if(de->file_type==EXT2_FT_REG_FILE||de->file_type==EXT2_FT_UNKNOWN)
				{
					if (ext2_get_inode(fd,de->inode,&File_inode)) {
						printf("load EXT2_ROOT_INO error");
						free(bh);
						return -1;
					}
					#define S_IFREG  0100000
					if(File_inode->i_mode&S_IFREG)
					{
					free(bh);
					return 0;
					}
				}
				find=1;
				inode=de->inode;
				break;
			}
		}
		if(!find)
		{
			free(bh);
		if(!lookupdir)printf("Not find the file or directory!\n");
		else printf("\n");
			return -1;
		}
	}
	return -1;
}



//load /dev/fs/ext2@wd0/boot/vmlinux
//path here is wd0/boot/vmlinux
int ext2_open(int fd,const char *path,int flags,int mode)
{	
	int i,index;
	char strbuf[EXT2_NAME_LEN],*str;
	strncpy(strbuf,path,sizeof(strbuf));
	for(i=0;strbuf[i]&&(strbuf[i]!='/');i++);
	if(!strbuf[i]){
		printf("the DEV Name  is expected!\n");
		return -1;		
	}
	strbuf[i]=0;
	{char *p;
	 p=&strbuf[strlen(strbuf)-1];
	if((p[0]>='a') && (p[0]<='z'))
	{index=p[0]-'a'+1; 
	 p[0]=0;
	}
	else if(p[0]=='A'||!strcmp(strbuf,"fd0"))index=0;
	else index=1;
	}

	if(devio_open(fd,strbuf,flags,mode)<0)//extract the device name
	return -1;
#ifdef DEBUG_IDE
	printf("Open the device %s ok\n",strbuf);
#endif
	str=strbuf+i+1;
//	for(i=1;i<5;i++){
		if(!(ext2_load_linux(fd,index,str)))
			return fd;	
	if((str[0]!=0)&&(str[strlen(str)-1]!='/'))	printf("we can't locate root directory in super block!\n");
//	}
	return -1;
}
int ext2_close(int fd)
{
	_file[fd].posn = 0;
	return devio_close(fd);
}
static int ReadFromIndexBlock(int fd,__u32 start_block,__u32 end_block,__u8 **ReadBuf,	size_t *size,size_t *position,__u32 *addr_start)
//ReadBuf  :point to the file content
//size:the real number still need to be read
//position:the file point where the read start
//return :0--successful
//	  others--error
{
	__u32 remain_count;
	int re;
	off_t remain_size,addr_abosolute_start;
	if(start_block>end_block)
		return 0;
#ifdef DEBUG_IDE
	printf("I want to read data size :%u, start_block is %d,end_block is %u\n",*size,start_block,end_block);
#endif
//------------------------------------------------------------------------------------	
	//Read the unaligned data within a block. 
	remain_size=RAW_BLOCK_SIZE-*position%RAW_BLOCK_SIZE;
	if(remain_size>*size)
	{
		remain_size=*size;
		remain_count=0;
	}
	else
	if(remain_size==RAW_BLOCK_SIZE)
	{
		remain_size=0;
		remain_count=0;
	}
	else
		remain_count=1;
	addr_start+=start_block;
	//start_block starts with 0. 0-11:direct others 0-1023.???????
	addr_abosolute_start=(off_t)*addr_start*RAW_BLOCK_SIZE+*position%RAW_BLOCK_SIZE;
	if(remain_count)
	{
		devio_lseek(fd,addr_abosolute_start+START_PARTION,0);
		re=devio_read(fd,(__u8 *)*ReadBuf,remain_size);
		if(re!=remain_size){
			printf("Can't Read Data From the Disk \n");
			return -1;}
		start_block+=remain_count;
		addr_start+=remain_count;
		remain_count=0;	
		*ReadBuf+=remain_size;
		*position+=remain_size;
		*size-=remain_size;
		addr_abosolute_start=(off_t)*addr_start*RAW_BLOCK_SIZE+*position%RAW_BLOCK_SIZE;
		remain_size=0;
	}
//------------------------------------------------------------------------------------
//Read the BLOCK aligned data
	while(*size&&(remain_size<*size)&&(remain_count+start_block<=end_block))
	{
		if(*(addr_start+remain_count+1)==*(addr_start+remain_count)+1)
		//continuous block ,so we just remember the blocks number .
		{
			if(remain_size+RAW_BLOCK_SIZE>*size)
				remain_size=*size;
			else
			{
				remain_count++;
				remain_size+=RAW_BLOCK_SIZE;
			}
		}
		else
		{
#ifdef DEBUG_IDE
	printf("The remain size is %u ,size is%u\n",remain_size,*size);
	printf("Block begin at %u,end at %u\n",*addr_start,*(addr_start+remain_count));
#endif
			if(!remain_size)
			//if not continuous,we just read one block
			{
			
				if(*size<RAW_BLOCK_SIZE)
					remain_size=*size;
				else
				{
					remain_size=RAW_BLOCK_SIZE;
					remain_count=1;
				}
			}
			if(*addr_start==0 && remain_count == 1)
			{
			memset((__u8 *)*ReadBuf,0,remain_size);
			re=remain_size;
			}
			else
			{
			devio_lseek(fd,addr_abosolute_start+START_PARTION,0);
			re=devio_read(fd,(__u8 *)*ReadBuf,remain_size);
			}
			if(re!=remain_size)
			{
				printf("We can't read data from disk!\n");
				return -1;
			}
			start_block+=remain_count;
			addr_start+=remain_count;
			remain_count=0;
			*ReadBuf+=remain_size;
			*position+=remain_size;
			*size-=remain_size;
			if((*position%RAW_BLOCK_SIZE)&&*size)
			{
				printf("Oh,My God!When I read in the aligned data,I met one unaligned position\n");
				return -1;
				}
			addr_abosolute_start=(off_t)*addr_start*RAW_BLOCK_SIZE;
			remain_size=0;
		}
	}
	if(!*size) return 0;//No data need to read.Wonderful!
	if(remain_size)
	{
		if(remain_size>*size)
			remain_size=*size;
		devio_lseek(fd,addr_abosolute_start+START_PARTION,0);
#ifdef DEBUG_IDE
	printf("The remain size is %u,size is %u\n",remain_size,*size);
	printf("Additional Block begin at %d,end at %d\n",*addr_start,*(addr_start+remain_count));
#endif
		re=devio_read(fd,(__u8 *)*ReadBuf,remain_size);
		*ReadBuf+=remain_size;
		if(re!=remain_size)
		{
			printf("We can't read data from disk!\n");
			return -1;
		}
		*position+=remain_size;
		*size-=remain_size;
	}
	return 0;
}
	
int ext2_read(int fd,void *read_start,size_t size)
{
	int real_size;
	
	memset(read_start, 0, size);
	if ((_file[fd].posn + size) > File_inode->i_size) {
		size = File_inode->i_size - _file[fd].posn;
	}
	real_size=ext2_read_file(fd,read_start,size,_file[fd].posn,File_inode);
	if ((_file[fd].posn + real_size) > File_inode->i_size) {
		real_size = File_inode->i_size - _file[fd].posn; 
		_file[fd].posn = File_inode->i_size;
	} else {
		_file[fd].posn+=real_size;	
	}
	return real_size;
}
static int ext2_read_file(int fd,void *read_start,size_t size,size_t pos,struct ext2_inode * inode)
{	
	__u8 *buff,*index_buff,*start=(__u8 *)read_start;
	size_t read_size=size,position=pos;
	__u32 *addr_start,*d_addr_start,start_block;
	int re,i;
	start_block=position/RAW_BLOCK_SIZE;
	addr_start=inode->i_block;	
#ifdef DEBUG_IDE
	printf("the pos is %llx,the size is %llx\n",position,read_size);
#endif
	re=ReadFromIndexBlock(fd,start_block,11,&start,&read_size,&position,addr_start);
#ifdef DEBUG_IDE
	printf("The addr_start is %x,RAW_BLOCK_SIZE is %x,start_sec is %x\n",*addr_start,RAW_BLOCK_SIZE,start_sec);
#endif
	if(re)
	{
		printf("Error in Reading from direct block\n");
		return 0;
	}
	if(!read_size)
	{
#ifdef DEBUG_IDE
		for(i=0;i<size;i+=RAW_BLOCK_SIZE/4)
			printf("%4x",(__u8)*((__u8 *)read_start+i));
		printf("\n");
#endif
		return (int)size;
	}
//////////////////////Read Index block///////////////////////////////////
	start_block=position/RAW_BLOCK_SIZE-12;
	buff=(__u8 *)malloc(RAW_BLOCK_SIZE);
	if(!buff)
	{
		printf("Can't alloc memory!\n");
		return 0;
	}
	addr_start=&(inode->i_block[12]);
	devio_lseek(fd,(off_t)*addr_start*RAW_BLOCK_SIZE+START_PARTION, 0);
	re=devio_read(fd,buff,RAW_BLOCK_SIZE);
	if(re!=RAW_BLOCK_SIZE)
	{
		printf("Read the iblock[12] error!\n");
		return 0;
	}
	addr_start=(__u32 *)buff;
	re=ReadFromIndexBlock(fd,start_block,RAW_BLOCK_SIZE/4-1,&start,&read_size,&position,addr_start);
	if(re) {
		free((char*)buff);	/*spark add*/
		return 0;
	}
	if(!read_size)
	{
#ifdef DEBUG_IDE
		for(i=0;i<size;i+=RAW_BLOCK_SIZE/4)
			printf("%4x",(__u8)*((__u8 *)read_start+i));
		printf("\n");
#endif
		free((char*)buff);              /* spark add */
		return (int)size;
	}
/////////////////////////////////////////Read Double index block////////////////
	addr_start=&(inode->i_block[13]);
	devio_lseek(fd,(off_t)*addr_start*RAW_BLOCK_SIZE+START_PARTION,0);
	re=devio_read(fd,buff,RAW_BLOCK_SIZE);
	if(re!=RAW_BLOCK_SIZE)
	{
		printf("Read the iblock[13] error!\n");
		free((char*)buff);	/* spark add */
		return 0;
	}
	d_addr_start=(__u32 *)buff;
	index_buff=(__u8 *)malloc(RAW_BLOCK_SIZE);
	if(!index_buff)
	{
		printf("Can't alloc memory!\n");
		return 0;
	}
	for(i=0;i<RAW_BLOCK_SIZE/4;i++)
	{	
		devio_lseek(fd,(off_t)*(d_addr_start+i)*RAW_BLOCK_SIZE+START_PARTION,0);
		re=devio_read(fd,index_buff,RAW_BLOCK_SIZE);
		if(re!=RAW_BLOCK_SIZE)
		{
			printf("Can't read index block!\n");
			return 0;
		}
		addr_start=(__u32 *)index_buff;
		start_block=position/RAW_BLOCK_SIZE-12-RAW_BLOCK_SIZE/4*(i+1);
		re=ReadFromIndexBlock(fd,start_block,RAW_BLOCK_SIZE/4-1,&start,&read_size,&position,addr_start);
		if(re)
		{
			printf("Can't read the double index block!\n");
			free((char*)buff);
			free((char*)index_buff);	/* spark add */
			return 0;
		}
		if(!read_size) {
			free((char*)buff);		/* spark add */
			free((char*)index_buff);
			return (int)size;
		}
	}
	printf("I can't read so big files,give me a email!\n");
	return 0;
}
int ext2_write(int fd,const void *start,size_t size)
{
	return 0;
}
off_t ext2_lseek(int fd,off_t offset,int where)
{
		_file[fd].posn=offset;
	return offset;
}
static DiskFileSystem diskfile={
	"ext2",
	ext2_open,
	ext2_read,
	ext2_write,
	ext2_lseek,
	ext2_close,
	NULL
};
static void init_diskfs(void) __attribute__ ((constructor));
static void init_diskfs()
{
	diskfs_init(&diskfile);
}
