/* $Id: fcntl.h,v 1.1.1.1 2006/09/14 01:59:06 root Exp $ */

#ifndef _FCNTL_H_
#define _FCNTL_H_

#if 0
#define	O_RDONLY 	0
#define	O_WRONLY 	1
#define	O_RDWR	 	2
#endif

__BEGIN_DECLS
int	open __P((const char *, int, ...)); 
int	ioctl __P((int fd, unsigned long op, ...));
__END_DECLS

#include <sys/fcntl.h>
#endif /* _FCNTL_ */
