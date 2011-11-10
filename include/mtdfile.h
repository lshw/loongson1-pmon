#include <linux/mtd/mtd.h>
#include <sys/queue.h>

typedef struct mtdfile {
	struct mtd_info *mtd;
	int refs;
	int fd;
	int flags;
	int index;
	unsigned int part_size;
	unsigned int part_offset;
#define MTDFILE_STATIC  0x0000
#define MTDFILE_DYNAMIC 0x0001
	LIST_ENTRY(mtdfile)	i_next;
	char name[1];
} mtdfile;

typedef struct mtdpriv {
struct mtdfile *file;
	unsigned int open_offset;
	unsigned int open_size;
} mtdpriv;

