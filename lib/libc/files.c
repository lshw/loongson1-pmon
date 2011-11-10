/* $Id: files.c,v 1.1.1.1 2006/09/14 01:59:06 root Exp $ */
#include <termio.h>

/*************************************************************
 *  _mfile[]
 *      A stub to satisfy the linker when building PMON. This is
 *      necessary because the same read() and write() routines are
 *      used by both PMON and the client, and the client supports
 *      ram-based files.
 */

Ramfile         _mfile[] =
{
    {0}};
