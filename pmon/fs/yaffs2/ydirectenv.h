/*
 * YAFFS: Yet another Flash File System . A NAND-flash specific file system.
 *
 * Copyright (C) 2002-2007 Aleph One Ltd.
 *   for Toby Churchill Ltd and Brightstar Engineering
 *
 * Created by Charles Manning <charles@aleph1.co.uk>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License version 2.1 as
 * published by the Free Software Foundation.
 *
 * Note: Only YAFFS headers are LGPL, YAFFS C code is covered by GPL.
 */

/*
 * ydirectenv.h: Environment wrappers for YAFFS direct.
 */

#ifndef __YDIRECTENV_H__
#define __YDIRECTENV_H__

/* Direct interface */

#include <include/linux/mtd/mtd.h>
#ifndef CONFIG_YAFFS_DIRECT
#define CONFIG_YAFFS_DIRECT
#endif



#define YCHAR char
#define YUCHAR unsigned char
#define _Y(x) x
#define yaffs_strcpy(a,b)    strcpy(a,b)
#define yaffs_strncpy(a,b,c) strncpy(a,b,c)
#define yaffs_strncmp(a,b,c) strncmp(a,b,c)
#define yaffs_strlen(s)	     strlen(s)
#define yaffs_sprintf	     sprintf
#define yaffs_toupper(a)     toupper(a)

#ifdef NO_Y_INLINE
#define Y_INLINE
#else
#define Y_INLINE inline
#endif

#define YMALLOC(x) yaffs_malloc(x)
#define YFREE(x)   free(x)
#define YMALLOC_ALT(x) yaffs_malloc(x)
#define YFREE_ALT(x)   free(x)
#define YMALLOC_DMA(x) yaffs_malloc(x)


#define YYIELD()  do {} while(0)

#define TENDSTR "\n"
#define TSTR(x) x
#define TOUT(p) printf p


#define YAFFS_LOSTNFOUND_NAME		"lost+found"
#define YAFFS_LOSTNFOUND_PREFIX		"obj"

//#include "yaffscfg.h"

#define Y_CURRENT_TIME yaffsfs_CurrentTime()
#define Y_TIME_CONVERT(x) x

#define YAFFS_ROOT_MODE				0666
#define YAFFS_LOSTNFOUND_MODE		0666

#define yaffs_SumCompare(x,y) ((x) == (y))
#define yaffs_strcmp(a,b) strcmp(a,b)
 
#define YAFFSFS_N_HANDLES 200

#if 0
#define YAFFS_TRACE_OS			0x00000002
#define YAFFS_TRACE_ALLOCATE		0x00000004
#define YAFFS_TRACE_SCAN		0x00000008
#define YAFFS_TRACE_BAD_BLOCKS		0x00000010
#define YAFFS_TRACE_ERASE		0x00000020
#define YAFFS_TRACE_GC			0x00000040
#define YAFFS_TRACE_WRITE		0x00000080
#define YAFFS_TRACE_TRACING		0x00000100
#define YAFFS_TRACE_DELETION		0x00000200
#define YAFFS_TRACE_BUFFERS		0x00000400
#define YAFFS_TRACE_NANDACCESS		0x00000800
#define YAFFS_TRACE_GC_DETAIL		0x00001000
#define YAFFS_TRACE_SCAN_DEBUG		0x00002000
#define YAFFS_TRACE_MTD			0x00004000
#define YAFFS_TRACE_CHECKPOINT		0x00008000

#define YAFFS_TRACE_VERIFY		0x00010000
#define YAFFS_TRACE_VERIFY_NAND		0x00020000
#define YAFFS_TRACE_VERIFY_FULL		0x00040000
#define YAFFS_TRACE_VERIFY_ALL		0x000F0000


#define YAFFS_TRACE_ERROR		0x40000000
#define YAFFS_TRACE_BUG			0x80000000
#define YAFFS_TRACE_ALWAYS		0xF0000000
#endif

#if 1
#define YAFFS_TRACE_OS			0x00000000
#define YAFFS_TRACE_ALLOCATE		0x00000000
#define YAFFS_TRACE_SCAN		0x00000000
#define YAFFS_TRACE_BAD_BLOCKS		0x00000000
#define YAFFS_TRACE_ERASE		0x00000000
#define YAFFS_TRACE_GC			0x00000000
#define YAFFS_TRACE_WRITE		0x00000000
#define YAFFS_TRACE_TRACING		0x00000000
#define YAFFS_TRACE_DELETION		0x00000000
#define YAFFS_TRACE_BUFFERS		0x00000000
#define YAFFS_TRACE_NANDACCESS		0x00000000
#define YAFFS_TRACE_GC_DETAIL		0x00000000
#define YAFFS_TRACE_SCAN_DEBUG		0x00000000
#define YAFFS_TRACE_MTD			0x00000000
#define YAFFS_TRACE_CHECKPOINT		0x00008000

#define YAFFS_TRACE_VERIFY		0x00000000
#define YAFFS_TRACE_VERIFY_NAND		0x00000000
#define YAFFS_TRACE_VERIFY_FULL		0x00000000
#define YAFFS_TRACE_VERIFY_ALL		0x00000000


#define YAFFS_TRACE_ERROR		0x00000000
#define YAFFS_TRACE_BUG			0x00000000
#define YAFFS_TRACE_ALWAYS		0x00000000
#endif

#define T(mask,p) do{ if((mask) & (yaffs_traceMask | YAFFS_TRACE_ALWAYS)) TOUT(p);} while(0)
#define YBUG() T(YAFFS_TRACE_BUG,(TSTR("==>> yaffs bug: " __FILE__ " %d" TENDSTR),__LINE__))




#define prefetch(x) 1

struct list_head {
	struct list_head *next, *prev;
};

#ifdef LIST_HEAD
#undef  LIST_HEAD
#endif
#define LIST_HEAD_INIT(name) { &(name), &(name) }

#define LIST_HEAD(name) \
	struct list_head name = LIST_HEAD_INIT(name)

#define INIT_LIST_HEAD(ptr) do { \
	(ptr)->next = (ptr); (ptr)->prev = (ptr); \
} while (0)


static __inline__ void __list_add(struct list_head *new,
				  struct list_head *prev,
				  struct list_head *next)
{
	next->prev = new;
	new->next = next;
	new->prev = prev;
	prev->next = new;
}

/**
 * list_add - add a new entry
 * @new: new entry to be added
 * @head: list head to add it after
 *
 * Insert a new entry after the specified head.
 * This is good for implementing stacks.
 */
static __inline__ void list_add(struct list_head *new, struct list_head *head)
{
	__list_add(new, head, head->next);
}

/**
 * list_add_tail - add a new entry
 * @new: new entry to be added
 * @head: list head to add it before
 *
 * Insert a new entry before the specified head.
 * This is useful for implementing queues.
 */
static __inline__ void list_add_tail(struct list_head *new,
				     struct list_head *head)
{
	__list_add(new, head->prev, head);
}

/*
 * Delete a list entry by making the prev/next entries
 * point to each other.
 *
 * This is only for internal list manipulation where we know
 * the prev/next entries already!
 */
static __inline__ void __list_del(struct list_head *prev,
				  struct list_head *next)
{
	next->prev = prev;
	prev->next = next;
}

/**
 * list_del - deletes entry from list.
 * @entry: the element to delete from the list.
 * Note: list_empty on entry does not return true after this, the entry is
 * in an undefined state.
 */
static __inline__ void list_del(struct list_head *entry)
{
	__list_del(entry->prev, entry->next);
}

/**
 * list_del_init - deletes entry from list and reinitialize it.
 * @entry: the element to delete from the list.
 */
static __inline__ void list_del_init(struct list_head *entry)
{
	__list_del(entry->prev, entry->next);
	INIT_LIST_HEAD(entry);
}

/**
 * list_empty - tests whether a list is empty
 * @head: the list to test.
 */
static __inline__ int list_empty(struct list_head *head)
{
	return head->next == head;
}

/**
 * list_splice - join two lists
 * @list: the new list to add.
 * @head: the place to add it in the first list.
 */
static __inline__ void list_splice(struct list_head *list,
				   struct list_head *head)
{
	struct list_head *first = list->next;

	if (first != list) {
		struct list_head *last = list->prev;
		struct list_head *at = head->next;

		first->prev = head;
		head->next = first;

		last->next = at;
		at->prev = last;
	}
}

/**
 * list_entry - get the struct for this entry
 * @ptr:	the &struct list_head pointer.
 * @type:	the type of the struct this is embedded in.
 * @member:	the name of the list_struct within the struct.
 */
#define list_entry(ptr, type, member) \
	((type *)((char *)(ptr)-(unsigned long)(&((type *)0)->member)))

/**
 * list_for_each	-	iterate over a list
 * @pos:	the &struct list_head to use as a loop counter.
 * @head:	the head for your list.
 */
#define list_for_each(pos, head) \
	for (pos = (head)->next, prefetch(pos->next); pos != (head); \
		pos = pos->next, prefetch(pos->next))

/**
 * list_for_each_safe	-	iterate over a list safe against removal
 *                              of list entry
 * @pos:	the &struct list_head to use as a loop counter.
 * @n:		another &struct list_head to use as temporary storage
 * @head:	the head for your list.
 */
#define list_for_each_safe(pos, n, head) \
	for (pos = (head)->next, n = pos->next; pos != (head); \
		pos = n, n = pos->next)

/*
 * File types
 */
#define DT_UNKNOWN	0
#define DT_FIFO		1
#define DT_CHR		2
#define DT_DIR		4
#define DT_BLK		6
#define DT_REG		8
#define DT_LNK		10
#define DT_SOCK		12
#define DT_WHT		14


//#endif

/*
 * Attribute flags.  These should be or-ed together to figure out what
 * has been changed!
 */
#define ATTR_MODE	1
#define ATTR_UID	2
#define ATTR_GID	4
#define ATTR_SIZE	8
#define ATTR_ATIME	16
#define ATTR_MTIME	32
#define ATTR_CTIME	64
#define ATTR_ATIME_SET	128
#define ATTR_MTIME_SET	256
#define ATTR_FORCE	512	/* Not a change, but a change it */
#define ATTR_ATTR_FLAG	1024

struct iattr {
	unsigned int ia_valid;
	unsigned ia_mode;
	unsigned ia_uid;
	unsigned ia_gid;
	unsigned ia_size;
	unsigned ia_atime;
	unsigned ia_mtime;
	unsigned ia_ctime;
	unsigned int ia_attr_flags;
};

#define KERN_DEBUG






typedef unsigned char __u8;
typedef unsigned short __u16;
typedef unsigned int __u32;
typedef signed int __s32;
typedef signed short __s16;


extern unsigned int yaffs_traceMask;
extern unsigned int yaffs_wr_attempts;


int yaffs_Initialise(unsigned nBlocks);
void *yaffs_malloc(size_t size);
void yaffs_free(void *ptr);

typedef struct {
	const char *prefix;
	struct yaffs_DeviceStruct *dev;
} yaffsfs_DeviceConfiguration;


void yaffsfs_Lock(void);
void yaffsfs_Unlock(void);

__u32 yaffsfs_CurrentTime(void);

void yaffsfs_SetError(int err);
int yaffsfs_GetError(void);


#endif
