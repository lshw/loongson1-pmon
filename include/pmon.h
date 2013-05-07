/*	$Id: pmon.h,v 1.1.1.1 2006/09/14 01:59:06 root Exp $ */

/*
 * Copyright (c) 2001 Opsycon AB  (www.opsycon.se)
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by Opsycon AB, Sweden.
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 */
/*
 * PMON Written for LSI LOGIC Corp. by Phil Bunce. Released to public
 * domain by LSI LOGIC.
 *              Phil Bunce, 100 Dolores St. Ste 242, Carmel CA 93923
 *
 * PMON Ported/rewritten for PowerPC and MIPS by Opsycon AB. New version
 * released under the BSD copyright above.
 */

#ifndef _PMON_H_
#define _PMON_H_

#include <sys/types.h>
#include <sys/cdefs.h>
#include <pmon_arch.h>

#ifndef _LIBC
#include <progress.h>
#include <target/pmon_target.h>
#endif

/* flush_cache types */
#define ICACHE  1
#define DCACHE  2
#define IADDR   4

#define MAX_BPT	 32		/* max number of user breakpoints */
#define LINESZ	256		/* max command line length */
#define MAX_AC	100		/* max number of args to commands */
#define PATSZ    80		/* max pattern length for search & fill */

#define NO_BPT		0	/* must be 0 but no brk at addr 0... */

/* trace_mode states */
#define TRACE_NO	0	/* no trace/go , must be 0 */
#define TRACE_TB	1	/* trace & break */
#define TRACE_TG	2	/* trace & go */
#define TRACE_GB	3	/* go & break */
#define TRACE_DC	4	/* debug mode continue */
#define TRACE_DS	5	/* debug mode step */
#define TRACE_TN	6	/* multistep trace */

/* regsize states */
#define REGSZ_32	0
#define REGSZ_64	1
#define	REGSZ_BOTH	0
#define	REGSZ_DOUBLE	1
#define	REGSZ_SINGLE	2
#define	REGSZ_NONE	3

/* command base values */
#define	INBASE_AUTO	0
#define	INBASE_OCT	1
#define	INBASE_DEC	2
#define	INBASE_HEX	3
#define	INBASE_STRING	"auto 8 10 16"
#ifndef INBASE_DEFAULT
#define	INBASE_DEFAULT	"16"
#endif

/* Exception types decoded by machdep exception decoder */
#define	EXC_BAD		0	/* Undecodeable */
#define	EXC_INT		1	/* HW interrupt */
#define	EXC_BPT		2	/* Breakpoint */
#define	EXC_TRC		3	/* Trace (not all arches) */
#define	EXC_WTCH	4	/* Watch (not all arches) */
#define	EXC_RES	5

#define CNTRL(x) (x & 0x1f)

typedef int     Func __P((void));

typedef struct Demo {
    const char     *name;
    Func           *addr;
} Demo;

typedef struct Optdesc {
    const char     *name;
    const char     *desc;
} Optdesc;

typedef struct Cmd {
	const char     *name;
	const char     *opts;
	const Optdesc  *optdesc;
	const char     *desc;
	int	       (*func) __P((int, char *[]));
	int             minac;
	int             maxac;
	int             flag;
#define	CMD_REPEAT	1	/* Command is repeatable */
#define	CMD_HIDE	2	/* Command is hidden */
#define	CMD_ALIAS	4	/* Alias for another command name */
} Cmd;

void cmdlist_expand __P((const Cmd *, int));

typedef struct Bps {
    unsigned long   addr;
    unsigned long   value;
    char           *cmdstr;
} Bps;

typedef struct Stopentry {
    unsigned long   addr;
    unsigned long   value;
    char            name[10];
    char            sense;
} Stopentry;

/* external data declarations */
extern const Cmd *CmdList[];
extern char     date[];
extern char     vers[];
extern register_t initial_sr;

extern struct trapframe *cpuinfotab[];


struct trapframe;

/*
 *  Declarations should be placed here when they are needed
 *  in a program module other than the one they live in.
 */
long load_elf __P((int, char *, int *, int));

/* stty.c */
int		getbaudrate __P((char *));

/* sbrk.c */
int		chg_heaptop __P((char *, char *));

/* init_main.c */
void		init_net __P((int));

/* miscmds.c */
void		flush_cache __P((int, void *));

/* memcmds.c */
void		store_dword __P((void *, int64_t));
void		store_word __P((void *, int32_t));
void		store_half __P((void *, int16_t));
void		store_byte __P((void *, int8_t));


/* debugger.c */
void		rm_bpts __P((void));
void		flush_validpc __P((void));
int		chg_validpc __P((char *, char *));
void		clrbpts __P((void));
u_int32_t	getpchist __P((int));

extern int      trace_mode;
extern Bps      Bpt[];
extern Bps      BptTmp;
extern Bps      BptTrc;
extern Bps      BptTrcb;

/* hist.c */
void		get_cmd __P((char *));
void		histinit __P((void));
void		get_line __P((char *, int));
char           *gethistn __P((int));

extern int      histno;		/* current history number */

/* main.c */
int		main __P((void));
void		dbginit __P((char *));
int		do_cmd __P((char *));
int		no_cmd __P((int, char *[]));
void		closelst __P((int));
void		console_state(int);

extern int      memorysize;
extern int	repeating_cmd;
extern char     prnbuf[];
extern char     line[LINESZ + 1];

/* more.c */
int		chg_moresz __P((char *, char *));
void		dotik __P((int, int));
int		more __P((char *, int *, int));

extern unsigned int moresz;

/* rsa.c */
int		get_rsa __P((u_int32_t *, char *));
int		get_rsa_reg __P((register_t *, char *));

/* set.c */
void		envbuild __P((char **, char *));
void		envinit __P((void));
void		envsize __P((int *, int *));
void		mapenv __P((void (*) __P((char *, char *))));
int		matchenv __P((char *));
int		setenv __P((char *, char *));
int		do_setenv __P((char *, char *, int));

/* sym.c */
void		syminit __P((void));
void		clrsyms __P((void));
int		newsym __P((char *, u_int32_t));
void		defsyms __P((u_int32_t, u_int32_t, u_int32_t));
int		sym2adr __P((register_t *, char *));
char 		*adr2sym __P((char *, unsigned long));
int		adr2symoff __P((char *, unsigned int, int));

/*-----*/
int		atob __P((u_int32_t *, char *, int));
int		llatob __P((u_int64_t *, char *, int));
int		gethex __P((int32_t *, char *, int32_t));
char		*btoa __P((char *, u_int32_t, int32_t));
char		*llbtoa __P((char *, u_int64_t, int32_t));

void		movequad __P((void *, void *));

/*
 *  Architecture dependent functions
 */
void		md_cacheon (void);
int		md_cachestat (void);
const char     *md_cpuname (void);
int		md_cputype (void);
void		md_dumpexc (struct trapframe *);
int		md_getpipefreq (int);
void		md_fprestore (void *);
void		md_fpsave (void *);
void		md_setpc (struct trapframe *, register_t);
void	       *md_getpc (struct trapframe *);
void		md_clreg (struct trapframe *);
void		md_setsp (struct trapframe *, register_t);
void		md_setsr (struct trapframe *, register_t);
void		md_setlr (struct trapframe *, register_t);
register_t	md_getsr (struct trapframe *);
register_t	md_getlink (struct trapframe *);
register_t	md_adjstack (struct trapframe *, register_t);
void	       *md_branch_target (void *);
void		md_setargs (struct trapframe *, register_t,register_t,
			    register_t, register_t);
void		md_setentry (struct trapframe *, register_t);
int		md_valid_load_addr (paddr_t, paddr_t);
int		md_exc_type (struct trapframe *);
void	       *md_get_excpc (struct trapframe *);
const char     *md_getexcname (struct trapframe *);
int		md_is_call (void *p);
int		md_is_jr (void *p);
int		md_is_writeable (void *p);
int		md_is_branch (void *p);
int		md_is_cond_branch (void *p);
void		md_settrace (void);
int		md_ator (register_t *, char *, int);
void           *md_disasm (char *, void *);

void	       *md_getcpuinfoptr (void);
int		md_getreg (register_t *, char *);
int		md_getregaddr (register_t **, char *);
int		md_disp_as_reg (register_t *, char *, int *);
int		md_registers (int, char *[]);
int		md_disassemble (int, char *[]);
int		md_stacktrace (int, char *[]);

void		*md_dumpframe (void *);
void		md_do_stacktrace (void *, int, int, char *);

void		md_init_cmd_debug (void);
void		sym_init_cmd_debug (void);

struct fl_map;

/*
 *  Target dependent functions
 */
void		clrhndlrs (void);
void		cpu_initclocks (void);
void		enablertclock (void);
void		startrtclock (int);
register_t	tgt_clienttos (void);
void		tgt_clkpoll (void);
int		tgt_clockram_read(char *, int, int);
int		tgt_clockram_write(char *, int, int);
void		tgt_cmd_vers (void);
int		tgt_pllfreq (void);
int		tgt_cpufreq (void);
void		tgt_devconfig (void);
void		tgt_devinit (void);
void		tgt_display (char *, int);
register_t	tgt_enable (int);
int		tgt_ethaddr (char *);
void		tgt_flashinfo (void *, size_t *);
void		tgt_flashinit (void);
struct fl_map  *tgt_flashmap (void);
void		tgt_flashprogram (void *, int, void *, int);
int		tgt_flashsetpageno (int);
void		tgt_flashwrite_disable (void);
int		tgt_flashwrite_enable (void);
int		tgt_getmachtype (void);
time_t		tgt_gettime (void);
void		tgt_logo (void);
void		tgt_machprint (void);
void		tgt_machreset (void);
void		tgt_mapenv (int (*) __P((char *, char *)));
void		tgt_memprint (void);
void		tgt_netinit (void);
void		tgt_netpoll (void);
void		tgt_netreset (void);
int		tgt_onesecond(int);
int		tgt_pipefreq (void);
void		tgt_poll (void);
void	       *tgt_poll_register (int, int(*) __P((void *)), void *);
void		tgt_putchar (int);
void		tgt_reboot (void);
void		tgt_rtinit (void);
int		tgt_setenv (char *, char *);
void		tgt_settime (time_t);
int		tgt_unsetenv (char *);
void		initstack (int, char **, int);
/*
 *  SMP related functions
 */
#if defined(SMP)
int		tgt_smpstartup (void);
int		tgt_smpwhoami (void);
int		tgt_smpready (void);
int		tgt_smpidle (void);
int		tgt_smpschedule (int);
int		tgt_smplock (void);
void		tgt_smpunlock (void);
int		tgt_smpfork (size_t, char *);
int		tgt_semlock (int);
void		tgt_semunlock (int);
int		whatcpu;		/* Cpu selected for debug */

#else
#define	whatcpu	0
#define tgt_smplock()	(1)
#define	tgt_smpunlock()
#endif

/*
 *  Kernel linkage
 */
void		netpoll __P((void));
int		spawn __P((char *, int(*) __P((int, char *[])), int, char *[]));


#define getfield(w,s,p)	((((unsigned long)w)&(((1<<s)-1)<<p))>>p)
#define load_byte(adr)	(*(u_int8_t *)(adr))
#define load_half(adr)	(*(u_int16_t *)(adr))
#define load_word(adr)	(*(u_int32_t *)(adr))
#define load_dword(adr)	(*(u_int64_t *)(adr))
#define load_reg(adr)	(*(register_t *)(adr))

/* macros to increment and decrement x, modulus mod */
#define incmod(x,mod)	(((x+1) > mod)?0:x+1)
#define decmod(x,mod)	(((x-1) < 0)?mod:x-1)
unsigned long ulmin __P((unsigned long, unsigned long));
long lmin(long, long);
int min(int, int);
#define maincpu (!cpuid)
extern int cpuid;
#define I2C_SINGLE 0 
#define I2C_BLOCK 1
#define I2C_SMB_BLOCK 2

int tgt_i2cread(int type,unsigned char *addr,int addrlen,unsigned char *buf,int count);
int tgt_i2cwrite(int type,unsigned char *addr,int addrlen,unsigned char *buf,int count);
int tgt_printf (const char *fmt, ...);
#endif /* _PMON_H_ */
