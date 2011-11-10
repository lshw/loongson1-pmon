/* $Id: strequ.c,v 1.1.1.1 2006/09/14 01:59:06 root Exp $ */

#include <string.h>

/** int strequ(s1,s2) return 1 if s1 matches s2 else returns 0 */

int 
strequ (str1, str2)
     const char *str1, *str2;
{

    if (!str1 || !str2)
	return (0);

    while (*str1)
	if (*str1++ != *str2++)
	    return (0);
    if (*str2 == 0)
	return (1);
    return (0);
}
