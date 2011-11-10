/*	$Id: sizemem.c,v 1.1.1.1 2006/09/14 01:59:06 root Exp $ */

#ifdef NEED_SIZEMEM
#define MAXMEM	(128*1024*1024)		/* max memory 128 Mb */
#define INCR	(256*1024)		/* 256 Kb increments */
#define MAXSEGS (MAXMEM / INCR)

#define MARKER	((unsigned char)0x55)
#define NMARKER	((unsigned char)0xaa)

#define START	((unsigned char)0x12)
#define NSTART	((unsigned char)0x34)

#ifdef PMCC
main ()
{

    printf ("This board has %d Kbytes of RAM\n", sizemem (0xa0020000) / 1024);
}
#endif

#ifdef R4000
# define BW		16	/* 64-bit bus, bank interleaved */
#else
# define BW		8	/* 32-bit bus, bank interleaved */
#endif

sizemem (base, maxsize)
    volatile unsigned char  *base;
    int maxsize;
{
    unsigned char   old[MAXSEGS][2];
    volatile unsigned char  *p;
    int             i, max, berr;

    /* round base up to INCR boundary */
    base = (unsigned char *) (((int)base + INCR - 1) & ~(INCR - 1));
    max = (maxsize - (int)base) / INCR;
    if (max > MAXSEGS)
      max = MAXSEGS;
    
    /* disable bus errors */
    berr = sbdberrenb (0);

/* save contents */
    for (i = 0, p = base; i < max; i++, p += INCR) {
	old[i][0] = p[0];
	old[i][1] = p[BW];
    }

/* set boundaries */
    for (i = 0, p = base; i < max; i++, p += INCR) {
	p[0]	= MARKER;
	p[BW]	= NMARKER;
    }

/* clear first location (in case addresses wrap) */
    base[0] 	= START;
    base[BW]	= NSTART;
    wbflush ();

/* search for wrap or garbage */
    (void) sbdberrenb (0); 	/* reset error counter */
    for (i = 1, p = base + INCR;
	 i < max && p[0] == MARKER && p[BW] == NMARKER && sbdberrcnt () == 0;
	 i++, p += INCR)
	continue;

    if (i < max && p[0] == START && p[BW] == NSTART)
      /* wraparound: work out distance from base */
      max = p - base;
    else
      /* garbage: dropped off end of memory */
      max = (int)p;

/* restore contents */
    for (; i >= 0; i--, p -= INCR) {
	p[0]	= old[i][0];
	p[BW]	= old[i][1];
    }

    (void) sbdberrenb (berr);
    return (max);
}
#endif
