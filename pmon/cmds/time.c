/* $Id: time.c,v 1.1.1.1 2006/09/14 01:59:08 root Exp $ */
/*
 * Copyright (c) 2001-2002 Opsycon AB  (www.opsycon.se / www.opsycon.com)
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
#ifdef _KERNEL
#undef _KERNEL
#endif

#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <time.h>
#include <stdlib.h>

#include <pmon.h>

int cmd_date __P((int, char *[]));
time_t sys_time __P((time_t *));

/*
 * Copyright (c) 1987 Regents of the University of California.
 * All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * Arthur Olson.
 *
 * Redistribution and use in source and binary forms are permitted
 * provided that the above copyright notice and this paragraph are
 * duplicated in all such forms and that any documentation,
 * advertising materials, and other materials related to such
 * distribution and use acknowledge that the software was developed
 * by the University of California, Berkeley.  The name of the
 * University may not be used to endorse or promote products derived
 * from this software without specific prior written permission.
 * THIS SOFTWARE IS PROVIDED ``AS IS'' AND WITHOUT ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED
 * WARRANTIES OF MERCHANTIBILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 */

#define TZNAME_MAX	32
#define	TZ_MAX_CHARS	50	/* Maximum number of abbreviation characters */
#define	TZ_MAX_TYPES	10	/* Maximum number of local time types */
#define	TZ_MAX_TIMES	370

#define	SECS_PER_MIN	60
#define	MINS_PER_HOUR	60
#define	HOURS_PER_DAY	24
#define	DAYS_PER_WEEK	7
#define	DAYS_PER_NYEAR	365
#define	DAYS_PER_LYEAR	366
#define	SECS_PER_HOUR	(SECS_PER_MIN * MINS_PER_HOUR)
#define	SECS_PER_DAY	((long) SECS_PER_HOUR * HOURS_PER_DAY)
#define	MONS_PER_YEAR	12

#define	TM_SUNDAY	0
#define	TM_MONDAY	1
#define	TM_TUESDAY	2
#define	TM_WEDNESDAY	3
#define	TM_THURSDAY	4
#define	TM_FRIDAY	5
#define	TM_SATURDAY	6

#define	TM_JANUARY	0
#define	TM_FEBRUARY	1
#define	TM_MARCH	2
#define	TM_APRIL	3
#define	TM_MAY		4
#define	TM_JUNE		5
#define	TM_JULY		6
#define	TM_AUGUST	7
#define	TM_SEPTEMBER	8
#define	TM_OCTOBER	9
#define	TM_NOVEMBER	10
#define	TM_DECEMBER	11

#define	TM_YEAR_BASE	1900

#define	EPOCH_YEAR	1970
#define	EPOCH_WDAY	TM_THURSDAY

#define	isleap(y) (((y) % 4) == 0 && (((y) % 100) != 0 || ((y) % 400) == 0))

static const int	mon_lengths[2][MONS_PER_YEAR] = {
	{31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31},
	{31, 29, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31}
};

static const int	year_lengths[2] = {
	DAYS_PER_NYEAR, DAYS_PER_LYEAR
};

struct rule {
	int		r_type;		/* type of rule--see below */
	int		r_day;		/* day number of rule */
	int		r_week;		/* week number of rule */
	int		r_mon;		/* month number of rule */
	long		r_time;		/* transition time of rule */
};

#define RT_NONE			0
#define	RT_JULIAN_DAY		1	/* Jn - Julian day */
#define	RT_DAY_OF_YEAR		2	/* n - day of year */
#define	RT_MONTH_NTH_DAY_OF_WEEK	3	/* Mm.n.d - month, week, day of week */

static struct tm *offtime __P((const time_t *clock, long offset));
static void tzreset __P((void));

time_t
sys_time(time_t *p)
{
    time_t t;

	t = tgt_gettime ();  
	if(p != NULL) {
		*p = t;
	}
	return(t);
}

static struct tm *offtime(const time_t *clock, long offset)
{
    struct tm *	tmp;
    long	days;
    long	rem;
    int		y;
    int		yleap;
    const int *		ip;
    static struct tm	tm;

    tmp = &tm;
    days = *clock / SECS_PER_DAY;
    rem = *clock % SECS_PER_DAY;
    rem += offset;
    while (rem < 0) {
	rem += SECS_PER_DAY;
	--days;
    }
    while (rem >= SECS_PER_DAY) {
	rem -= SECS_PER_DAY;
	++days;
    }
    tmp->tm_hour = (int) (rem / SECS_PER_HOUR);
    rem = rem % SECS_PER_HOUR;
    tmp->tm_min = (int) (rem / SECS_PER_MIN);
    tmp->tm_sec = (int) (rem % SECS_PER_MIN);
    tmp->tm_wday = (int) ((EPOCH_WDAY + days) % DAYS_PER_WEEK);
    if (tmp->tm_wday < 0)
      tmp->tm_wday += DAYS_PER_WEEK;
    y = EPOCH_YEAR;
    if (days >= 0)
      for ( ; ; ) {
	  yleap = isleap(y);
	  if (days < (long) year_lengths[yleap])
	    break;
	  ++y;
	  days = days - (long) year_lengths[yleap];
      }
    else do {
	--y;
	yleap = isleap(y);
	days = days + (long) year_lengths[yleap];
    } while (days < 0);
    tmp->tm_year = y - TM_YEAR_BASE;
    tmp->tm_yday = (int) days;
    ip = mon_lengths[yleap];
    for (tmp->tm_mon = 0; days >= (long) ip[tmp->tm_mon]; ++(tmp->tm_mon))
      days = days - (long) ip[tmp->tm_mon];
    tmp->tm_mday = (int) (days + 1);
    tmp->tm_isdst = 0;
    tmp->tm_zone = "";
    tmp->tm_gmtoff = offset;
    return tmp;
}

struct tm *gmtime(const time_t *clock)
{
    struct tm *res;

    res = offtime(clock, 0L);
    res->tm_zone = "GMT";	/* UCT ? */
    return res;
}


/*
** Given a pointer into a time zone string, scan until a character that is not
** a valid character in a zone name is found.  Return a pointer to that
** character.
*/

static const char *getzname(register const char *strp)
{
    char	c;

    while ((c = *strp) != '\0' && !isdigit(c) && c != ',' && c != '-' &&
	   c != '+')
      ++strp;
    return strp;
}

/*
** Given a pointer into a time zone string, extract a number from that string.
** Check that the number is within a specified range; if it is not, return
** NULL.
** Otherwise, return a pointer to the first character not part of the number.
*/

static const char *getnum(const char * strp,
			  int * const nump,const int min,const int max)
{
    char	c;
    int	num;

    if (strp == NULL || !isdigit(*strp))
      return NULL;
    num = 0;
    while ((c = *strp) != '\0' && isdigit(c)) {
	num = num * 10 + (c - '0');
	if (num > max)
	  return NULL;		/* illegal value */
	++strp;
    }
    if (num < min)
      return NULL;		/* illegal value */
    *nump = num;
    return strp;
}

/*
** Given a pointer into a time zone string, extract a number of seconds,
** in hh[:mm[:ss]] form, from the string.
** If any error occurs, return NULL.
** Otherwise, return a pointer to the first character not part of the number
** of seconds.
*/

static const char *getsecs(const char *	strp,long * const secsp)
{
    int	num;

    strp = getnum(strp, &num, 0, HOURS_PER_DAY);
    if (strp == NULL)
      return NULL;
    *secsp = num * SECS_PER_HOUR;
    if (*strp == ':') {
	++strp;
	strp = getnum(strp, &num, 0, MINS_PER_HOUR - 1);
	if (strp == NULL)
	  return NULL;
	*secsp += num * SECS_PER_MIN;
	if (*strp == ':') {
	    ++strp;
	    strp = getnum(strp, &num, 0, SECS_PER_MIN - 1);
	    if (strp == NULL)
	      return NULL;
	    *secsp += num;
	}
    }
    return strp;
}

/*
** Given a pointer into a time zone string, extract an offset, in
** [+-]hh[:mm[:ss]] form, from the string.
** If any error occurs, return NULL.
** Otherwise, return a pointer to the first character not part of the time.
*/

static const char *getoffset(const char *strp,long * const offsetp)
{
    int	neg;

    if (*strp == '-') {
	neg = 1;
	++strp;
    } else if (isdigit(*strp) || *strp++ == '+')
      neg = 0;
    else	return NULL;	/* illegal offset */
    strp = getsecs(strp, offsetp);
    if (strp == NULL)
      return NULL;		/* illegal time */
    if (neg)
      *offsetp = -*offsetp;
    return strp;
}

/*
** Given a pointer into a time zone string, extract a rule in the form
** date[/time].  See POSIX section 8 for the format of "date" and "time".
** If a valid rule is not found, return NULL.
** Otherwise, return a pointer to the first character not part of the rule.
*/

static const char *getrule(const char *strp,struct rule * const	rulep)
{
    if (*strp == 'J') {
	/*
	 ** Julian day.
	 */
	rulep->r_type = RT_JULIAN_DAY;
	++strp;
	strp = getnum(strp, &rulep->r_day, 1, DAYS_PER_NYEAR);
    } else if (*strp == 'M') {
	/*
	 ** Month, week, day.
	 */
	rulep->r_type = RT_MONTH_NTH_DAY_OF_WEEK;
	++strp;
	strp = getnum(strp, &rulep->r_mon, 1, MONS_PER_YEAR);
	if (strp == NULL)
	  return NULL;
	if (*strp++ != '.')
	  return NULL;
	strp = getnum(strp, &rulep->r_week, 1, 5);
	if (strp == NULL)
	  return NULL;
	if (*strp++ != '.')
	  return NULL;
	strp = getnum(strp, &rulep->r_day, 0, DAYS_PER_WEEK - 1);
    } else if (isdigit(*strp)) {
	/*
	 ** Day of year.
	 */
	rulep->r_type = RT_DAY_OF_YEAR;
	strp = getnum(strp, &rulep->r_day, 0, DAYS_PER_LYEAR - 1);
    } else
      return NULL;	/* invalid format */
    if (strp == NULL)
      return NULL;
    if (*strp == '/') {
	/*
	 ** Time specified.
	 */
	++strp;
	strp = getsecs(strp, &rulep->r_time);
    } else	
      rulep->r_time = 2 * SECS_PER_HOUR; /* default = 2:00:00 */
    return strp;
}

/*
** Given the Epoch-relative time of January 1, 00:00:00 GMT, in a year, the
** year, a rule, and the offset from GMT at the time that rule takes effect,
** calculate the Epoch-relative time that rule takes effect.
*/

static time_t transtime(
			const time_t				janfirst,
			const int				year,
			const struct rule * const		rulep,
			const long				offset)
{
    int	leapyear;
    time_t	value;
    int	i;
    int		d, m1, yy0, yy1, yy2, dow;

    leapyear = isleap(year);
    switch (rulep->r_type) {
    default:
	return -1;
    case RT_JULIAN_DAY:
	/*
	 ** Jn - Julian day, 1 == January 1, 60 == March 1 even in leap
	 ** years.
	 ** In non-leap years, or if the day number is 59 or less, just
	 ** add SECSPERDAY times the day number-1 to the time of
	 ** January 1, midnight, to get the day.
	 */
	value = janfirst + (rulep->r_day - 1) * SECS_PER_DAY;
	if (leapyear && rulep->r_day >= 60)
	  value += SECS_PER_DAY;
	break;

    case RT_DAY_OF_YEAR:
	/*
	 ** n - day of year.
	 ** Just add SECSPERDAY times the day number to the time of
	 ** January 1, midnight, to get the day.
	 */
	value = janfirst + rulep->r_day * SECS_PER_DAY;
	break;

    case RT_MONTH_NTH_DAY_OF_WEEK:
	/*
	 ** Mm.n.d - nth "dth day" of month m.
	 */
	value = janfirst;
	for (i = 0; i < rulep->r_mon - 1; ++i)
	  value += mon_lengths[leapyear][i] * SECS_PER_DAY;

	/*
	 ** Use Zeller's Congruence to get day-of-week of first day of
	 ** month.
	 */
	m1 = (rulep->r_mon + 9) % 12 + 1;
	yy0 = (rulep->r_mon <= 2) ? (year - 1) : year;
	yy1 = yy0 / 100;
	yy2 = yy0 % 100;
	dow = ((26 * m1 - 2) / 10 +
	       1 + yy2 + yy2 / 4 + yy1 / 4 - 2 * yy1) % 7;
	if (dow < 0)
	  dow += DAYS_PER_WEEK;

	/*
	 ** "dow" is the day-of-week of the first day of the month.  Get
	 ** the day-of-month (zero-origin) of the first "dow" day of the
	 ** month.
	 */
	d = rulep->r_day - dow;
	if (d < 0)
	  d += DAYS_PER_WEEK;
	for (i = 1; i < rulep->r_week; ++i) {
	    if (d + DAYS_PER_WEEK >=
		mon_lengths[leapyear][rulep->r_mon - 1])
	      break;
	    d += DAYS_PER_WEEK;
	}

	/*
	 ** "d" is the day-of-month (zero-origin) of the day we want.
	 */
	value += d * SECS_PER_DAY;
	break;
    }

    /*
     ** "value" is the Epoch-relative time of 00:00:00 GMT on the day in
     ** question.  To get the Epoch-relative time of the specified local
     ** time on that day, add the transition time and the current offset
     ** from GMT.
     */
    return value + rulep->r_time + offset;
}

static char		tz_stdnam[TZNAME_MAX] = "UCT";
static char		tz_dstnam[TZNAME_MAX] = "UCT";
static long 		tz_stdoffset = 0;
static long 		tz_dstoffset = 0;
static struct rule 	tz_dststart;
static struct rule 	tz_dstend;
static char		tz_hasdst;
char 			*tzname[2] = { tz_stdnam, tz_dstnam};
/*
** Given a POSIX section 8-style TZ string, fill in the rule tables as
** appropriate.
*/

static int tzparse(const char *name)
{
    const char *		stdname;
    const char *		dstname;
    int				stdlen;
    int				dstlen;
    
    tzname[0] = tz_stdnam;
    tzname[1] = tz_dstnam;
    memset(&tz_dststart,0,sizeof(tz_dststart));
    memset(&tz_dstend,0,sizeof(tz_dstend));
    
    stdname = name;
    name = getzname(name);
    stdlen = name - stdname;
    if (stdlen < 3 || stdlen >= TZNAME_MAX)
      return -1;
    strncpy(tz_stdnam,stdname,stdlen);
    if (*name == '\0')
      return -1;
    
    name = getoffset(name, &tz_stdoffset);
    if (name == NULL)
      return -1;
    if (*name != '\0') {
	dstname = name;
	name = getzname(name);
	dstlen = name - dstname; /* length of DST zone name */
	if (dstlen < 3 || dstlen >= TZNAME_MAX)
	  return -1;
	strncpy(tz_dstnam,dstname,dstlen);
	
	if (*name != '\0' && *name != ',' ) {
	    name = getoffset(name, &tz_dstoffset);
	    if (name == NULL)
	      return -1;
	} else	
	  tz_dstoffset = tz_stdoffset - SECS_PER_HOUR;
	tz_hasdst = 1;
	
	if (*name == ',') {
	    ++name;
	    if ((name = getrule(name, &tz_dststart)) == NULL)
	      return -1;
	    if (*name++ != ',')
	      return -1;
	    if ((name = getrule(name, &tz_dstend)) == NULL)
	      return -1;
	} 
	if (*name != '\0')
	  return -1;
    } else {
	strcpy(tz_dstnam,"   ");
	tz_dstoffset = 0; 
	tz_hasdst = 0;
    }
    return 0;
}

/*
 * reset tz state to the default initial state
*/
static void tzreset()
{
    tzparse("UTC0");
}

void tzset(void)
{
    const char *tz = getenv("TZ");
    static char	*tz_envval;
    
    if(tz && *tz) {
		if(tz_envval) {
		    if(strcmp(tz_envval,tz) == 0)
		      return;
		    tz_envval = realloc(tz_envval,strlen(tz)+1);
		    strcpy(tz_envval,tz);
		} else {
		    tz_envval = malloc(strlen(tz)+1);
		    strcpy(tz_envval,tz);
		}
	if(tzparse(tz) == 0)
	  return;
    }
    if(tz_envval) {
	free(tz_envval);
	tz_envval = 0;
	tzreset();
    }
}

struct tm *localtime(const time_t *tp)
{
    struct tm	*tm;
    time_t	tgmt = *tp;
    
    int		dst;
    
    tzset();
    
    if(tz_hasdst) {
	if(tz_dststart.r_type != RT_NONE) {
	    time_t	tjan = 0;
	    time_t	tyr = tgmt;
	    time_t 	yl;
	    time_t	stim;
	    time_t	etim;
	    int		yr;
	    
	    for(yr = EPOCH_YEAR;
		tyr >= (yl=year_lengths[isleap(yr)] * SECS_PER_DAY); yr++) {
		
		tyr -= yl;
		tjan +=  yl;
	    }
	    stim = transtime(tjan,yr,&tz_dststart,0);
	    etim = transtime(tjan,yr,&tz_dstend,0);
	    if(stim < etim) {
		if(tgmt >= stim && tgmt < etim)
		  dst = 1;
		else
		  dst = 0;
	    } else {
		if(tgmt >= etim && tgmt < stim)
		  dst = 0;
		else
		  dst = 1;
	    }
	} else
	  dst = -1;
    } else
      dst = 0;
    
    tm = offtime(tp,dst==1?tz_dstoffset:tz_stdoffset);
//  tm = offtime(tp,dst==1?-tz_dstoffset:-tz_stdoffset);
    tm->tm_isdst = dst;
    tm->tm_zone = 0;
    return tm;
}

time_t  gmmktime(const struct tm *tm)
{
    int yr;
    int mn;
    time_t secs;

    if (tm->tm_sec > 61 ||
        tm->tm_min > 59 ||
        tm->tm_hour > 23 ||
        tm->tm_mday > 31 ||
        tm->tm_mon > 12 ||
        tm->tm_year < 70) {
        return (time_t)-1;
    }

    /*
     * Sum up seconds from beginning of year
     */
    secs = tm->tm_sec;
    secs += tm->tm_min * SECS_PER_MIN;
    secs += tm->tm_hour * SECS_PER_HOUR;
    secs += (tm->tm_mday-1) * SECS_PER_DAY;

    for (mn = 0; mn < tm->tm_mon; mn++)
      secs += mon_lengths[isleap(tm->tm_year+1900)][mn] * SECS_PER_DAY;

    for(yr=1970; yr < tm->tm_year + 1900; yr++)
      secs += year_lengths[isleap(yr)]*SECS_PER_DAY;

    return secs;
}


time_t	mktime(struct tm *tm)
{
    int yr;
    int mn;
    time_t secs;
    struct	tm *ntm;

    if (tm->tm_sec > 61 || 
	tm->tm_min > 59 || 
	tm->tm_hour > 23 || 
	tm->tm_mday > 31 || 
	tm->tm_mon > 12 ||
	tm->tm_year < 70) {
	return (time_t)-1;
    }
    
    /*
     * Sum up seconds from beginning of year
     */
    secs = tm->tm_sec;
    secs += tm->tm_min * SECS_PER_MIN;
    secs += tm->tm_hour * SECS_PER_HOUR;
    secs += (tm->tm_mday-1) * SECS_PER_DAY;
    
    for (mn = 0; mn < tm->tm_mon; mn++)
      secs += mon_lengths[isleap(tm->tm_year+1900)][mn] * SECS_PER_DAY;
    
    for(yr=1970; yr < tm->tm_year + 1900; yr++)
      secs += year_lengths[isleap(yr)]*SECS_PER_DAY;
    
    if(tm->tm_isdst == 0) {
	secs -= tz_stdoffset;
	ntm = localtime(&secs);
    } else if(tm->tm_isdst > 0) {
	tm->tm_isdst = 1;
	secs -= tz_dstoffset;
	ntm = localtime(&secs);
    } else {
	/* try to determine if daylight saving was set at this time 
	 */
	secs -= tz_stdoffset;
	ntm = localtime(&secs);
	if(ntm->tm_mday != tm->tm_mday ||
	   ntm->tm_hour != tm->tm_hour ||
	   ntm->tm_min  != tm->tm_min ||
	   ntm->tm_sec  != tm->tm_sec) {
	    secs += tz_stdoffset;
	    secs -= tz_dstoffset;
	}
    }
    /* work secs back into a tm to get all the fields correct 
     */
    if(ntm->tm_year != tm->tm_year ||
       ntm->tm_mon  != tm->tm_mon ||
       ntm->tm_mday != tm->tm_mday ||
       ntm->tm_hour != tm->tm_hour ||
       ntm->tm_min  != tm->tm_min ||
       ntm->tm_sec  != tm->tm_sec ||
       (tm->tm_isdst >= 0 && ntm->tm_isdst != tm->tm_isdst))
       return -1;
    
    *tm = *ntm;
    return secs;
}

#ifdef SIMPLE 

static int month[12] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};

static int
yeartoday (int year)
{
    return (year % 4) ? 365 : 366;
}


time_t
mktime (struct tm *tm)
{
    time_t n;
    int i, days = 0;

    if (tm->tm_sec > 61 || 
	tm->tm_min > 59 || 
	tm->tm_hour > 23 || 
	tm->tm_mday > 31 || 
	tm->tm_mon > 12 ||
	tm->tm_year < 70) {
	return (time_t)-1;
    }

    n = tm->tm_sec + 60 * tm->tm_min + 3600 * tm->tm_hours;
    n += (tm->tm_mday - 1) * 3600 * 24;

    month[1] = (yeartoday(tm->tm_year) == 366) ? 29 : 28;
    for (i = tm->tm_mon - 2; i >= 0; i--)
	days += month[i];

    for (i = 70; i < tm->tm_year; i++)
	days += yeartoday(i);
    n += days * 3600 * 24;

    return (n);
}


struct tm *gmtime (const time_t *t)
{
    static struct tm tm;
    time_t n;

    n = *t % (3600 * 24);	/* hrs+mins+secs */
    tm.tm_sec = n % 60;
    n /= 60;
    tm.tm_min = n % 60;
    tm.tm_hour = n / 60;
    
    n = *t / (3600 * 24);	/* days since 1/1/1970*/
    tm.tm_wday = (n + 4) % 7;	/* 1/1/70 was Thursday */
    
    for (j = 1970, i = yeartoday(j); n >= i; j++, i = yeartoday(j))
	n -= i;

    tm.tm_yday = n;
    tm.tm_year = j - 1900;

    month[1] = (i == 366) ? 29 : 28;
    for (i = 0; n >= month[i]; i++)
	n -= month[i];
    tm.tm_mon = i + 1;
    tm.tm_day = n + 1;

    tm.tm_gmtoff = 0;
    tm.tm_isdst = 0;
    tm.tm_zone = "GMT";
    return &tm;
}

#endif

static const char * const dayname[] = {
    "Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat" };

static const char * const monthname[] = {
    "Jan", "Feb", "Mar", "Apr", "May", "Jun",
    "Jul", "Aug", "Sep", "Oct", "Nov", "Dec" };

static int 
get2 (char *s)
{
    if (s[0] < '0' || s[0] > '9' || s[1] < '0' || s[1] > '9')
      return -1;
    return (s[0] - '0') * 10 + (s[1] - '0');
}


int
cmd_date(int argc, char **argv)
{
    struct tm tm;
    time_t t;

    t = tgt_gettime ();
    tm = *localtime (&t);

    if (argc == 2) {
	char *s;
	int hadsecs = 0;
	int n;
	
	s = argv[1];
	n = strlen (s);
	
	if (n >= 3 && s[n-3] == '.') { /* seconds */
	    if ((tm.tm_sec = get2 (&s[n-2])) < 0 || tm.tm_sec >= 60) {
		printf ("bad seconds\n");
		return (1);
	    }
	    hadsecs = 1;
	    n -= 3;
	}

	if (n >= 2) {		/* minutes */
	    if ((tm.tm_min = get2 (&s[n-2])) < 0 || tm.tm_min >= 60) {
		printf ("bad minutes\n");
		return (1);
	    }
	    n -= 2;
	    if (!hadsecs)
	      tm.tm_sec = 0;
	}

	if (n >= 2) {		/* hours */
	    if ((tm.tm_hour = get2 (&s[n-2])) < 0 || tm.tm_hour >= 24) {
		printf ("bad hours\n");
		return (1);
	    }
	    n -= 2;
	}

	if (n >= 2) {		/* days */
	    if ((tm.tm_mday = get2 (&s[n-2])) < 0 || tm.tm_mday > 31) {
	      /* test could be more thorough... */
		printf ("bad day\n");
		return (1);
	    }
	    n -= 2;
	}

	if (n >= 2) {		/* months */
	    if ((tm.tm_mon = get2 (&s[n-2])) < 1 || tm.tm_mon > 12) {
		printf ("bad month\n");
		return (1);
	    }
	    tm.tm_mon -= 1;	/* zero based */
	    n -= 2;
	}

	if (n >= 2) {		/* years */
	    if ((tm.tm_year = get2 (&s[n-2])) < 0) {
		printf ("bad year\n");
		return (1);
	    }
	    if(n >= 4 && get2(&s[n-4]) == 20) {
		tm.tm_year += 100;
		n -= 2;
	    }
	    else if(n >= 4 && get2(&s[n-4]) == 19) {
		n -= 2;
	    }
	    else if(tm.tm_year < 70) {
		tm.tm_year += 100;
	    }
	    n -= 2;
	}
	if (n != 0) {
	    printf ("%s: bad date syntax\n", argv[1]);
	    return (1); 
	}

	t = mktime (&tm);
	if (t != -1)
	    tgt_settime (t);
    }

    if (t != -1) {
	printf ("%s %s %2d %02d:%02d:%02d %d\n",
		dayname[tm.tm_wday], monthname[tm.tm_mon], tm.tm_mday,
		tm.tm_hour, tm.tm_min, tm.tm_sec, TM_YEAR_BASE+tm.tm_year);
    }
    else {
	printf ("invalid date\n");
    }
    return (0);
}

/*
 *  Command table registration
 *  ==========================
 */

static const Cmd Cmds[] =
{
	{"Shell"},
	{"date",	"[yyyymmddhhmm.ss]",
			0,
			"get/set date and time",
			cmd_date, 1, 2, CMD_REPEAT},
	{0, 0}
};

static void init_cmd __P((void)) __attribute__ ((constructor));

void
init_cmd()
{
	cmdlist_expand(Cmds, 1);
}

