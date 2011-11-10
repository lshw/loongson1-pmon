/*
 * ./modf.c : stdlib function
 * Copyright (c) 1992 Algorithmics Ltd.
 */

#include <math.h>

#define BITSPERBYTE 8
#define BITS(type)  (BITSPERBYTE * (int)sizeof(type))

#define LONGBITS    BITS(long)
#define DOUBLEBITS  BITS(double)

#define _IEEE       1
#define _DEXPLEN    11
#define _FEXPLEN    8
#define _HIDDENBIT  1

#define DSIGNIF     (DOUBLEBITS - _DEXPLEN + _HIDDENBIT - 1)

#define DMAXPOWTWO  ((double)(1L << (LONGBITS -2))*(1L << (DSIGNIF - LONGBITS +1)))

#if _IEEE == 1

#if defined (__NO_FLOAT)

double
modf (double d, double *ipart)
{
    abort ();
}

#elif defined (__SOFT_FLOAT) || defined(__SINGLE_FLOAT)

#include <ieee754.h>

double
modf (double d, double *ipart)
{
    ieee754dp sd, sipart, srv;

    sd.d = d;
    srv = ieee754dp_modf(sd, &sipart);
    *ipart = sipart.d;
    return srv.d;
}

#else

#include <math.h>

double modf(double d, double *ipart)
{
    double absval;
    
    absval = fabs(d);

    if( absval >= DMAXPOWTWO) {
	*ipart = d;
	return 0.0;
    } else {
	double ival = absval;
	
	ival += DMAXPOWTWO;
	ival -= DMAXPOWTWO;
	
	while(ival > absval)
	  ival -= 1.0;
	if(d < 0)
	  ival = -ival;
	*ipart = ival;
	return d - ival;
    }
}

#endif /* __SOFT_FLOAT */

#endif /* _IEEE == 1 */
