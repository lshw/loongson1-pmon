/*
 * This file is used for IT8172
 *
 */

#define SBD_DEBUG 1
#ifndef SBD_DEBUG
#define SBD_DISPLAY(text, code)		/* No display function */
#else
void hexserial __P((int));
#define SBD_DISPLAY(text, code)	tgt_display(text, code)
#endif
extern char *heaptop;

/*
 *  Name of envvar that has to be set to enable expert mode.
 */
#define EXPERT  "Not defined"

/*
 *  Target arch specifics
 */
//#define	HAVE_QUAD			/* Native 64 bit integers */

/*
 *  Boot loader parameters.
 */

#define	TGT_BOOT_ADR	0x80400000	/* Load 4 meg up. */
#define	TGT_BOOT_SIZ	0x00002000	/* Suck in 8k */
#define	TGT_BOOT_OFF	0x00000400	/* Start reading from byte 1024 */

/*
 *  Target dependent CLIENTPC settings
 */
#define CLIENTPC 0x80100000
#define SETCLIENTPC "80100000"
