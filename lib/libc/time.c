/* $Id: time.c,v 1.1.1.1 2006/09/14 01:59:06 root Exp $ */
#ifdef LR33000
#include "mips.h"

unsigned long   _acc, _time, _tenths, _clkinit;
long            _tstart;
int             clkfreq = 25;	/* clock freq in MHz */

#define TIMER	(*((volatile long *)M_TIC2))
#define STARTCLK 	if (_clkinit == 0) { 		\
				_tstart = TIMER; 	\
				onintr(0,&clkdat); 	\
				startclk(); 		\
				_clkinit = 1; 		\
				}

extern int      clkdat;

/*************************************************************
 *  long time(tloc)
 *      Returns the current time in seconds
 */
long 
time (tloc)
     long           *tloc;
{
    unsigned long   t;

    STARTCLK
	t = _time;
    if (tloc != 0)
	*tloc = t;
    return (t);
}

/*************************************************************
 *  int stime(tp)
 *      Set the current time, tp is in seconds.
 */
int 
stime (tp)
     long           *tp;
{
    STARTCLK
	_time = *tp;
    return (0);
}

/*************************************************************
 *  unsigned long clock()
 *      Returns the current time in microseconds
 */
unsigned long 
clock ()
{
    STARTCLK
	return ((_acc * 100000) + ((_tstart - TIMER) / clkfreq));
}

/*************************************************************
 *  unsigned long cycles()
 *      Returns the current count of CPU cycles (clocks)
 */
unsigned long 
cycles ()
{
    STARTCLK
	return ((_acc * 100000 * clkfreq) + (_tstart - TIMER));
}
#endif
