/* $Id: ramfiles.c,v 1.1.1.1 2006/09/14 01:59:08 root Exp $ */
#include <termio.h>
#include <pmon.h>
#include "logfile.h"

/*************************************************************
 *  _mfile[]
 *      A stub to satisfy the linker when building PMON. This is
 *      necessary because the same read() and write() routines are
 *      used by both PMON and the client, and the client supports
 *      ram-based files.
 */

Ramfile         _mfile[] =
{
#if NLOGFILE > 0
   {"logger", 0, 0x3000, 0x1000, 0x0000},
#endif
   {0}};
