/*
 * $Id: cmdlinepart.c,v 1.19 2005/11/07 11:14:19 gleixner Exp $
 *
 * Read flash partition table from command line
 *
 * Copyright 2002 SYSGO Real-Time Solutions GmbH
 *
 * The format for the command line is as follows:
 *
 * mtdparts=<mtddef>[;<mtddef]
 * <mtddef>  := <mtd-id>:<partdef>[,<partdef>]
 * <partdef> := <size>[@offset][<name>][ro]
 * <mtd-id>  := unique name used in mapping driver/device (mtd->name)
 * <size>    := standard linux memsize OR "-" to denote all remaining space
 * <name>    := '(' NAME ')'
 *
 * Examples:
 *
 * 1 NOR Flash, with 1 single writable partition:
 * edb7312-nor:-
 *
 * 1 NOR Flash with 2 partitions, 1 NAND with one
 * edb7312-nor:256k(ARMboot)ro,-(root);edb7312-nand:-(home)
 */

#include <pmon.h>

#include <linux/mtd/mtd.h>

/* special size referring to all the remaining space in a partition */
#define SIZE_REMAINING UINT_MAX

/* mtdpart_setup() parses into here */
extern struct mtd_info *ls1x_mtd ;


unsigned long long memparse (char *ptr, char **retptr)
{
      unsigned long long ret = strtoul (ptr, retptr, 0);

	switch (**retptr) {
	case 'G':
	case 'g':
		ret <<= 10;
	case 'M':
	case 'm':
		ret <<= 10;
	case 'K':
	case 'k':
		ret <<= 10;
		(*retptr)++;
	default:
		break;
	}
	return ret;
}

/*
 * Parse one partition definition for an MTD. Since there can be many
 * comma separated partition definitions, this function calls itself
 * recursively until no more partition definitions are found. Nice side
 * effect: the memory to keep the mtd_partition structs and the names
 * is allocated upon the last definition being found. At that point the
 * syntax has been verified ok.
 */
unsigned long mtd_offset;
unsigned int this_part=0;
static  int newpart(char *s,char **retptr)
{
	unsigned long size;
	char *name,realname[200];
	int name_len;
	char delim;

	/* fetch the partition size */
	if (*s == '-')
	{	/* assign all remaining space to this partition */
		size = SIZE_REMAINING;
		s++;
	} else {
		size = memparse(s, &s);
		if (size < PAGE_SIZE)
		{
			printf("partition size too small (%lx)\n", size);
			return NULL;
		}
	}

	/* fetch partition name and flags */
	delim = 0;
        /* check for offset */
        if (*s == '@')
	{
                s++;
                mtd_offset = memparse(s, &s);
        }
        /* now look for name */
	if (*s == '(')
	{
		delim = ')';
	}

		memset(realname,0,sizeof(realname));
	if (delim)
	{
		char *p;

	    	name = ++s;
		if ((p = strchr(name, delim)) == 0)
		{
			printf("no closing %c found in partition name\n", delim);
			return NULL;
		}
		name_len = p - name;
	strncpy(realname,name,name_len);
		s = p + 1;
	}
	else
		sprintf(realname,"mtd%d",this_part);

	if(size == SIZE_REMAINING)
		size=ls1x_mtd->size - mtd_offset;
	this_part ++;
	add_mtd_device(ls1x_mtd,mtd_offset,size,realname);

	if (*s == ',')
	{
	        *retptr = s;
		mtd_offset += size;
		return 1;
		/* more partitions follow, parse them */
	}
	/* enter this partition (offset will be calculated later if it is zero at this point) */


	/* return partition table */
	return NULL;

}

/*
 * Parse the command line.
 */
int mtdpart_setup_real(char *s)
{
 
	    	int mtd_id_len;
		char *p, *mtd_id;

	    	mtd_id = s;
		/* fetch <mtd-id> */
		if (!(p = strchr(s, ':')))
		{
			printf("no mtd-id\n");
			return 0;
		}
		mtd_id_len = p - mtd_id;

		/*
		 * parse one mtd. have it reserve memory for the
		 * struct cmdline_mtd_partition and the mtd-id string.
		 */
		while(
		newpart(p + 1,&s)!=NULL) {
		 p=s ;
		}	
	       	return 0;
}

