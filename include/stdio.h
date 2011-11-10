/* $Id: stdio.h,v 1.1.1.1 2006/09/14 01:59:06 root Exp $ */
#ifndef _STDIO_
#define _STDIO_

#if !defined(_ANSI_SOURCE) && !defined(__STRICT_ANSI__)
#include <sys/types.h>
#endif

#include <sys/cdefs.h>

#include <machine/ansi.h>
#include "mod_usb_kbd.h"
#ifdef  _BSD_SIZE_T_
typedef _BSD_SIZE_T_    size_t;
#undef  _BSD_SIZE_T_
#endif

#ifndef NULL
#define NULL    0
#endif

typedef struct FILE {
	int fd;
	int valid;
	int ungetcflag;
	int ungetchar;
} FILE;

__BEGIN_DECLS
int	fclose	__P((FILE *));
FILE	*fopen	__P((const char *, const char *));

int	fgetc	__P((FILE *));
char	*fgets	__P((char *, int , FILE *));
size_t	fread	__P((void *, size_t, size_t, FILE *));
int	fscanf	__P((FILE *, const char *, ...));
int	getc	__P((FILE *));
int	getchar	__P((void));
char	*gets	__P((char *));
int	scanf	__P((const char *, ...));
int	sscanf	__P((const char *, const char *, ...));
int	ungetc	__P((int, FILE *));

long	ftell	__P((FILE *));
int	fprintf	__P((FILE *, const char *, ...));
int	fputc	__P((int , FILE *));
int	fputs	__P((const char *, FILE *));
size_t	fwrite	__P((const void *, size_t, size_t, FILE *));
int	printf	__P((const char *, ...));
int	putc	__P((int, FILE *));
int	putchar	__P((int));
int	puts	__P((const char *));
int	sprintf	__P((char *, const char *, ...));

int	vfprintf __P((FILE *, const char *, _BSD_VA_LIST_));
int	vprintf	__P((const char *, _BSD_VA_LIST_));
int	vsprintf __P((char *, const char *, _BSD_VA_LIST_));
void	perror __P((const char *));
__END_DECLS

void	clearerr __P((FILE *));
int	feof	__P((FILE *));
int	ferror	__P((FILE *));
int	fileno	__P((FILE *));
int	fflush	__P((FILE *));
int	fseek	__P((FILE *, long int, int));

extern FILE _iob[];
extern int vga_available;
extern int kbd_available;
extern int usb_kbd_available;

#if NMOD_USB_KBD >0 
#define stdin	(kbd_available|usb_kbd_available?(&_iob[3]):(&_iob[0]))
#else
#define stdin	(kbd_available?(&_iob[3]):(&_iob[0]))
#endif

#define stdout	(vga_available?(&_iob[4]):(&_iob[1]))
#define stderr	(vga_available?(&_iob[4]):(&_iob[2]))
#define kbdin	(&_iob[3])
#define vgaout	(&_iob[4])
#define serialin (&_iob[0])
#define serialout (&_iob[1])

#ifdef OPEN_MAX
#undef OPEN_MAX
#endif
#define OPEN_MAX 32

#define MAXLN 256

#define EOF  (-1)

typedef int iFunc __P((void));
typedef int *Addr;
#endif /* _STDIO_ */
