#define WANGCAI_DEBUG
#ifdef WANGCAI_DEBUG
#endif
/*
 * cramfs.c
 *
 * Copyright (C) 1999 Linus Torvalds
 *
 * Copyright (C) 2000-2002 Transmeta Corporation
 *
 * Copyright (C) 2003 Kai-Uwe Bloem,
 * Auerswald GmbH & Co KG, <linux-development@auerswald.de>
 * - adapted from the www.tuxbox.org u-boot tree, added "ls" command
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License (Version 2) as
 * published by the Free Software Foundation.
 *
 * Compressed ROM filesystem for Linux.
 *
 * TODO:
 * add support for resolving symbolic links
 */

/*
 * These are the VFS interfaces to the compressed ROM filesystem.
 * The actual compression is based on zlib, see the other files.
 */

//#include <common.h>
#include "include/malloc.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <file.h>
//#include <asm/byteorder.h>
#include "include/stat.h"
//#include <jffs2/jffs2.h>
//#include <jffs2/load_kernel.h>
#include "include/cramfs_fs.h"

/* These two macros may change in future, to provide better st_ino
   semantics. */
#define MAX_CRAMFS_DEV_NAME 256
//#define CRAMINO(x)	(CRAMFS_GET_OFFSET(x) ? CRAMFS_GET_OFFSET(x)<<2 : 1)
//#define OFFSET(x)	((x)->i_ino)


/****uncompress.c****/
/*
 * uncompress.c
 *
 * Copyright (C) 1999 Linus Torvalds
 * Copyright (C) 2000-2002 Transmeta Corporation
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License (Version 2) as
 * published by the Free Software Foundation.
 *
 * cramfs interfaces to the uncompression library. There's really just
 * three entrypoints:
 *
 *  - cramfs_uncompress_init() - called to initialize the thing.
 *  - cramfs_uncompress_exit() - tell me when you're done
 *  - cramfs_uncompress_block() - uncompress a block.
 *
 * NOTE NOTE NOTE! The uncompression is entirely single-threaded. We
 * only have one stream, and we'll initialize it only once even if it
 * then is used by multiple filesystems.
 */

//#include <common.h>
//#include <malloc.h>
#include "include/zlib.h"

static z_stream stream;

void *zalloc(void *, unsigned, unsigned);
void zfree(void *, void *, unsigned);

#define	ZALLOC_ALIGNMENT	16
void *zalloc(void *x, unsigned items, unsigned size)
{
	void *p;

	size *= items;
	size = (size + ZALLOC_ALIGNMENT - 1) & ~(ZALLOC_ALIGNMENT - 1);

	p = malloc (size);

	return (p);
}

void zfree(void *x, void *addr, unsigned nb)
{
	free (addr);
}

/* Returns length of decompressed data. */
int cramfs_uncompress_block (void *dst, void *src, int srclen)
{
	int err;

	inflateReset (&stream);

	stream.next_in = src;
	stream.avail_in = srclen;

	stream.next_out = dst;
	stream.avail_out = 4096 * 2;

#ifdef WANGCAI_DEBUG
//	printf("inflate begin:src:%xh,dst:%xh,srclen:%d ", src, dst, srclen);
#endif
	err = inflate (&stream, Z_FINISH);
#ifdef WANGCAI_DEBUG
//	printf("cramfs_uncompress_block :inflate returned successfully\n");
#endif

	if (err != Z_STREAM_END)
		goto err;
	return stream.total_out;

      err:
	/*printf ("Error %d while decompressing!\n", err); */
	/*printf ("%p(%d)->%p\n", src, srclen, dst); */
	return -1;
}

int cramfs_uncompress_init (void)
{
	int err;

	stream.zalloc = zalloc;
	stream.zfree = zfree;
	stream.next_in = 0;
	stream.avail_in = 0;

//#if defined(CONFIG_HW_WATCHDOG) || defined(CONFIG_WATCHDOG)
//	stream.outcb = (cb_func) WATCHDOG_RESET;
//#else
//	stream.outcb = Z_NULL;
//#endif /* CONFIG_HW_WATCHDOG */

	err = inflateInit (&stream);
	if (err != Z_OK) {
		printf ("Error: inflateInit2() returned %d\n", err);
		return -1;
	}

	return 0;
}

int cramfs_uncompress_exit (void)
{
	inflateEnd (&stream);
	return 0;
}

/****uncompress.c****/


typedef struct file_info_more {
	int flen;
	char *data;
}file_info_more;
struct cramfs_super super;
/* CPU address space offset calculation macro, struct part_info offset is
 * device address space offset, so we need to shift it by a device start address. */
extern int devio_open(int, const char *, int, int);
extern int devio_close(int);
extern int devio_read(int, void *, size_t);
extern int devio_lseek(int, off_t, int);

static int read_inode_data(int fd, char *inode_data, struct cramfs_inode *file_inode);

static int cramfs_read_super (struct cramfs_super *superblock)
{
	unsigned long root_offset;

	/* Read the first block and get the superblock from it */
	memcpy (&super, superblock, sizeof (super));

	/* Do sanity checks on the superblock */
	if (super.magic != CRAMFS_32 (CRAMFS_MAGIC)) {
		/* check at 512 byte offset */
		memcpy (&super, superblock + 512, sizeof (super));
		if (super.magic != CRAMFS_32 (CRAMFS_MAGIC)) {
			printf ("cramfs: wrong magic\n");
			return -1;
		}
	}

	/* flags is reused several times, so swab it once */
	super.flags = CRAMFS_32 (super.flags);
	super.size = CRAMFS_32 (super.size);

	/* get feature flags first */
	if (super.flags & ~CRAMFS_SUPPORTED_FLAGS) {
		printf ("cramfs: unsupported filesystem features\n");
		return -1;
	}

	/* Check that the root inode is in a sane state */
	if (!S_ISDIR (CRAMFS_16 (super.root.mode))) {
		printf ("cramfs: root is not a directory\n");
		return -1;
	}
	root_offset = CRAMFS_GET_OFFSET (&(super.root)) << 2;
	if (root_offset == 0) {
		printf ("cramfs: empty filesystem");
	} else if (!(super.flags & CRAMFS_FLAG_SHIFTED_ROOT_OFFSET) &&
		   ((root_offset != sizeof (struct cramfs_super)) &&
		    (root_offset != 512 + sizeof (struct cramfs_super)))) {
		printf ("cramfs: bad root offset %lu\n", root_offset);
		return -1;
	}

	return 0;
}

static unsigned long cramfs_resolve (int fd, struct cramfs_inode *dir_inode, int raw,
				     char *filename)//open a cramfs file recursely.
{
	unsigned long inodeoffset = 0, nextoffset;
	struct cramfs_inode *inode_data = malloc(dir_inode->size);
	int retval;

	if (!S_ISDIR(CRAMFS_16 (dir_inode->mode)))
		return -1;

//	devio_lseek(fd, CRAMFS_GET_OFFSET(dir_inode) << 2, 0);
//	retval = devio_read(fd, inode_data, file_inode->size);
	retval = read_inode_data(fd, (char *)inode_data, dir_inode);
	if (retval < 0) {
		return retval;
	}
	while (inodeoffset < CRAMFS_24(dir_inode->size)) {
		struct cramfs_inode *inode;
		char *name;
		int namelen;

		inode = inode_data + inodeoffset;//offset of the root directory's data,there are all of inodes in root.

		/*
		 * Namelengths on disk are shifted by two
		 * and the name padded out to 4-byte boundaries
		 * with zeroes.
		 */
		namelen = CRAMFS_GET_NAMELEN (inode) << 2;
		name = (char *) inode + sizeof (struct cramfs_inode);

		nextoffset =
			inodeoffset + sizeof (struct cramfs_inode) + namelen;

		for (;;) {
			if (!namelen)
				return -1;
			if (name[namelen - 1])
				break;
			namelen--;
		}

		if (!strncmp (filename, name, namelen)) {
			char *p = strtok (NULL, "/");

			if (raw && (p == NULL || *p == '\0')) {
				free(inode_data);
				return (CRAMFS_GET_OFFSET(dir_inode) << 2) + inodeoffset;
			}

			if (S_ISDIR (CRAMFS_16 (inode->mode))) {
				free(inode_data);
				return cramfs_resolve (fd, inode, raw, p);
			} else if (S_ISREG (CRAMFS_16 (inode->mode))) {
				return (CRAMFS_GET_OFFSET(dir_inode) << 2) + inodeoffset;
			} else {
				free(inode_data);
				printf ("%*.*s: unsupported file type (%x)\n",
					namelen, namelen, name,
					CRAMFS_16 (inode->mode));
				return 0;
			}
		}

		inodeoffset = nextoffset;
	}

	free(inode_data);
	printf ("can't find corresponding entry\n");
	return 0;
}

static int cramfs_uncompress(struct cramfs_inode *file_inode, char *max_file, char *inode_data)
{
	char *cram_data = inode_data;
	struct cramfs_inode *p_inode = file_inode;
	struct cramfs_inode *inode = p_inode;
	unsigned long loadoffset = (unsigned long)max_file;
	unsigned long *block_ptrs = (unsigned long *)cram_data;
	unsigned long curr_block = (CRAMFS_GET_OFFSET(p_inode) + (((CRAMFS_24 (inode->size)) +
				      4095) >> 12)) << 2;
	int size, total_size = 0;
	int i;
/*
#ifdef WANGCAI_DEBUG
	for (i = 0; i < ((CRAMFS_24 (inode->size) + 4095) >> 12); i++) {
		printf("block_ptrs[%d]: %d\n", i, block_ptrs[i]);
	}
#endif
*/
	cramfs_uncompress_init ();

	for (i = 0; i < ((CRAMFS_24 (inode->size) + 4095) >> 12); i++) {
		size = cramfs_uncompress_block ((void *)loadoffset,
						(void *) (cram_data + curr_block - (CRAMFS_GET_OFFSET(p_inode) << 2)),
						(CRAMFS_32 (block_ptrs[i]) -
						 curr_block));

#ifdef WANGCAI_DEBUG
//	printf("block_ptrs[%d]: %d ", i, block_ptrs[i]);
//	printf("curr_block: %d ", curr_block);
//	printf(" %dth uncompress size: %d\n", i, size);
#endif

		if (size < 0) {
#ifdef WANGCAI_DEBUG
	printf("error, uncompress return\n");
	printf("error block_ptrs[%d]: %d\n", i, block_ptrs[i]);
	printf("error curr_block: %d\n", curr_block);
#endif
			return size;
		}
		loadoffset += size;
		total_size += size;
		curr_block = CRAMFS_32 (block_ptrs[i]);
	}

	cramfs_uncompress_exit ();
	return total_size;
}

/*
int cramfs_load (char *loadoffset, struct part_info *info, char *filename)
{
	unsigned long offset;

	if (cramfs_read_super (info))
		return -1;

	offset = cramfs_resolve (super.root, 0, strtok (filename, "/"));

	if (offset <= 0)
		return offset;

	return cramfs_uncompress (PART_OFFSET(info), offset,
				  (unsigned long) loadoffset);
}
*/

/*
static int cramfs_list_inode (struct part_info *info, unsigned long offset)
{
	struct cramfs_inode *inode = (struct cramfs_inode *)
		(PART_OFFSET(info) + offset);
	char *name, str[20];
	int namelen, nextoff;

	/*
	 * Namelengths on disk are shifted by two
	 * and the name padded out to 4-byte boundaries
	 * with zeroes.
	 */
/*
	namelen = CRAMFS_GET_NAMELEN (inode) << 2;
	name = (char *) inode + sizeof (struct cramfs_inode);
	nextoff = namelen;

	for (;;) {
		if (!namelen)
			return namelen;
		if (name[namelen - 1])
			break;
		namelen--;
	}

	printf (" %s %8d %*.*s", mkmodestr (CRAMFS_16 (inode->mode), str),
		CRAMFS_24 (inode->size), namelen, namelen, name);

	if ((CRAMFS_16 (inode->mode) & S_IFMT) == S_IFLNK) {
		/* symbolic link.
		 * Unpack the link target, trusting in the inode's size field.
		 */
/*
		unsigned long size = CRAMFS_24 (inode->size);
		char *link = malloc (size);

		if (link != NULL && cramfs_uncompress (PART_OFFSET(info), offset,
						       (unsigned long) link)
		    == size)
			printf (" -> %*.*s\n", (int) size, (int) size, link);
		else
			printf (" [Error reading link]\n");
		if (link)
			free (link);
	} else
		printf ("\n");

	return nextoff;
}

int cramfs_ls (struct part_info *info, char *filename)
{
	struct cramfs_inode *inode;
	unsigned long inodeoffset = 0, nextoffset;
	unsigned long offset, size;

	if (cramfs_read_super (info))
		return -1;

	if (strlen (filename) == 0 || !strcmp (filename, "/")) {
		/* Root directory. Use root inode in super block */
/*
		offset = CRAMFS_GET_OFFSET (&(super.root)) << 2;
		size = CRAMFS_24 (super.root.size);
	} else {
		/* Resolve the path */
/*
		offset = cramfs_resolve (PART_OFFSET(info),
					 CRAMFS_GET_OFFSET (&(super.root)) <<
					 2, CRAMFS_24 (super.root.size), 1,
					 strtok (filename, "/"));

		if (offset <= 0)
			return offset;

		/* Resolving was successful. Examine the inode */
/*
		inode = (struct cramfs_inode *) (PART_OFFSET(info) + offset);
		if (!S_ISDIR (CRAMFS_16 (inode->mode))) {
			/* It's not a directory - list it, and that's that */
/*
			return (cramfs_list_inode (info, offset) > 0);
		}

		/* It's a directory. List files within */
/*
		offset = CRAMFS_GET_OFFSET (inode) << 2;
		size = CRAMFS_24 (inode->size);
	}

	/* List the given directory */
/*
	while (inodeoffset < size) {
		inode = (struct cramfs_inode *) (PART_OFFSET(info) + offset +
						 inodeoffset);

		nextoffset = cramfs_list_inode (info, offset + inodeoffset);
		if (nextoffset == 0)
			break;
		inodeoffset += sizeof (struct cramfs_inode) + nextoffset;
	}

	return 1;
}

int cramfs_info (struct part_info *info)
{
	if (cramfs_read_super (info))
		return 0;

	printf ("size: 0x%x (%u)\n", super.size, super.size);

	if (super.flags != 0) {
		printf ("flags:\n");
		if (super.flags & CRAMFS_FLAG_FSID_VERSION_2)
			printf ("\tFSID version 2\n");
		if (super.flags & CRAMFS_FLAG_SORTED_DIRS)
			printf ("\tsorted dirs\n");
		if (super.flags & CRAMFS_FLAG_HOLES)
			printf ("\tholes\n");
		if (super.flags & CRAMFS_FLAG_SHIFTED_ROOT_OFFSET)
			printf ("\tshifted root offset\n");
	}

	printf ("fsid:\n\tcrc: 0x%x\n\tedition: 0x%x\n",
		super.fsid.crc, super.fsid.edition);
	printf ("name: %16s\n", super.name);

	return 1;
}
*/

/*
int cramfs_check (struct part_info *info)
{
	struct cramfs_super *sb;

	if (info->dev->id->type != MTD_DEV_TYPE_NOR)
		return 0;

	sb = (struct cramfs_super *) PART_OFFSET(info);
	if (sb->magic != CRAMFS_32 (CRAMFS_MAGIC)) {
*/
		/* check at 512 byte offset */
/*		sb = (struct cramfs_super *) (PART_OFFSET(info) + 512);
		if (sb->magic != CRAMFS_32 (CRAMFS_MAGIC))
			return 0;
	}
	return 1;
}
*/
/*
static int cramfs_open_init(int fd, const char *devname, int flags, int mode)
{
	static mark_init = 0;
	int dev_open_ret;
	char *superblock;

	dev_open_ret = devio_open(fd, devname, flags, mode);
	if (dev_open_ret < 0) {
		printf("NOTE in cramfs_open:open dev error");
		return dev_open_ret;
	}

	devio_lseek(fd, 0, 0);
	superblock = (char *)malloc(sizeof(cramfs_super));
	devio_read(fd, superblock, sizeof(*superblock));
}*/

static int read_inode_data(int fd, char *inode_data, struct cramfs_inode *file_inode)
{
	devio_lseek(fd, CRAMFS_GET_OFFSET(file_inode) << 2, 0);
	return devio_read(fd, inode_data, file_inode->size);
}
#define NOTE_CRAMFS_OPEN(x) printf("NOTE in cramfs_open: %s", x)
int cramfs_open(int fd, const char *path, int flags, int mode)
{
	int i;
	const char *pathw_dev;
	char path_pointer[MAX_CRAMFS_DEV_NAME];
	char devname[MAX_CRAMFS_DEV_NAME];
	int retval;
	char *superblock;
	unsigned long offset;

	struct cramfs_inode *file_inode;
	char *inode_data;
	char *max_file;
	struct file_info_more *file_data_uncompressed = malloc(sizeof(file_info_more));

	if (strncmp(path, "/dev/", 5) == 0)
		path += 5;

	if (strncmp(path, "cramfs@", 7) == 0) {
		pathw_dev = path + 7;
	}
	else
		return -1;

	for (i = 0; pathw_dev[i] && pathw_dev[i] != '/'; i++)
		devname[i] = pathw_dev[i];
	devname[i] = '\0';
	if (!pathw_dev[i])
		return -1;
	else {
		strncpy(path_pointer, &pathw_dev[i], strlen(&pathw_dev[i]));//path pointer like "/home/zhouyanghong/```"
	}
	
	printf("devname:%s\n", devname);
	retval = devio_open(fd, devname, flags, mode);
	if (retval < 0) {
//		printf("NOTE in cramfs_open:open dev error\n");
		NOTE_CRAMFS_OPEN("open device error\n");
		printf("devname:%s\n", devname);
		return retval;
	}

	superblock = (char *)malloc(sizeof(struct cramfs_super) + 512);
	devio_lseek(fd, 0, 0);
	retval = devio_read(fd, superblock, sizeof(struct cramfs_super) + 512);
	if (retval < 0) {
//		printf("NOTE in cramfs_open:read dev super error\n");
		NOTE_CRAMFS_OPEN("read superblock error\n");
		return retval;
	}

	if(cramfs_read_super((struct cramfs_super *)superblock) < 0)
		return -1;
	offset = cramfs_resolve(fd, &super.root, 0, strtok (path_pointer, "/"));

	if (offset <= 0)
		return -1;

	free(superblock);

#ifdef WANGCAI_DEBUG
	printf("resolve the file ok\n");
#endif
	file_inode = (struct cramfs_inode *)malloc(sizeof(struct cramfs_inode));
	devio_lseek(fd, (int)offset, 0);
	retval = devio_read(fd, file_inode, sizeof(struct cramfs_inode));
	if (retval < 0) {
//		printf("NOTE in cramfs_read:read file inode error\n");
		NOTE_CRAMFS_OPEN("read file inode error\n");
		free(file_inode);
		return retval;
	}
#ifdef WANGCAI_DEBUG
	//	printf("NOTE in cramfs_read: read file inode ok\n"); 
	NOTE_CRAMFS_OPEN("read file inode ok\n");
#endif
	inode_data = (char *)malloc(file_inode->size);
	retval = read_inode_data(fd, inode_data, file_inode);
	if (retval < 0) {
	//	printf("NODE in cramfs_read:read inode_data error\n");
		NOTE_CRAMFS_OPEN("read inode_dara error\n");
		free(file_inode);
		free(inode_data);
		return -1;
	}
#ifdef WANGCAI_DEBUG
	NOTE_CRAMFS_OPEN("read inode_data ok\n");
	printf("file's size: %dk\n", file_inode->size/1024);
	printf("file's data offset: %xh\n", (file_inode->offset << 2));
#endif
	max_file = (char *)malloc(16 << 20);
	free(max_file);
	max_file = (char *)malloc(16 << 20);
	retval = cramfs_uncompress(file_inode, max_file, inode_data);
	if (retval < 0){
//		printf("NOTE in cramfs_read:read uncompress inode_data error\n");
		NOTE_CRAMFS_OPEN("uncompress inode_data error\n");
		free(file_inode);
		free(inode_data);
		free(max_file);
		return -1;
	}
#ifdef WANGCAI_DEBUG
	printf("uncompress file data ok, file size after uncompressed:%dk, file data entry:%xh\n", retval/1024, max_file);
	printf("open cramfs file ok\n");
#endif
	file_data_uncompressed->flen = retval;//retval is the file size after uncompressed.
	file_data_uncompressed->data = max_file;
	_file[fd].data = (void *)(file_data_uncompressed);
	free(file_inode);
	free(inode_data);
	
	return fd;
}//cramfs_open end.

int cramfs_close(int fd)
{
	if (_file[fd].data == NULL)
		return -1;
	_file[fd].posn = 0;
	if (((struct file_info_more *)(_file[fd].data))->data == NULL || ((struct file_info_more *)(_file[fd].data))->flen == 0)
		return -1;
	printf("note in cramfs_close before free:flen is:%dk, data entry is:%x\n", ((struct file_info_more *)(_file[fd].data))->flen/1024, ((struct file_info_more *)(_file[fd].data))->data);
	((struct file_info_more *)_file[fd].data)->flen = 0;
	free(((struct file_info_more *)_file[fd].data)->data);
	free(((struct file_info_more *)(_file[fd].data)));
	return devio_close(fd);
}

int cramfs_write(int fd, const void *start, size_t size)
{
	return 0;
}
#define NOTE_CRAMFS_READ(x) printf("NOTE in cramfs_read: %s",x);
int cramfs_read(int fd, void *read_start, size_t size)
{
	size_t real_size = size;
	printf("read begin:size is %d bytes, to where? %x, posn is %x\n", size, read_start, _file[fd].posn);
	if (_file[fd].posn == ((struct file_info_more *)_file[fd].data)->flen)
		return 0;
	if (_file[fd].data == NULL)
		return -1;
	if(_file[fd].posn + size > ((struct file_info_more *)_file[fd].data)->flen) {
		real_size = ((struct file_info_more *)_file[fd].data)->flen - _file[fd].posn;
	}
	memcpy(read_start, &(((struct file_info_more *)_file[fd].data)->data[_file[fd].posn]), real_size);
	_file[fd].posn += real_size;
	printf("READ END\n");
	return real_size;
}

off_t cramfs_lseek(int fd, off_t offset, int where)
{
	_file[fd].posn = offset;
	return offset;
}

static FileSystem cramfile = {
	"cramfs", FS_FILE,
	cramfs_open,
	cramfs_read,
	cramfs_write,
	cramfs_lseek,
	cramfs_close,
	NULL
};
static void init_cramfs(void) __attribute__ ((constructor));
static void init_cramfs(void)
{
	filefs_init(&cramfile);
}
