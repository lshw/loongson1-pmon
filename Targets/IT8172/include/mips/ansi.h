/*
 * ansi.h : machine dependent ANSI type definitions
 * Copyright (c) 1993 Algorithmics Ltd.
 */

#ifndef __MIPS_ANSI_H_
#define __MIPS_ANSI_H_
#define _ANSI_H_

/*
 * Types which are fundamental to the implementation and may appear in
 * more than one standard header are defined here.  Standard headers
 * then use:
 *	#ifdef	_SIZE_T_
 *	typedef	_SIZE_T_ size_t;
 *	#undef	_SIZE_T_
 *	#endif
 */

/* GNU specific, but gets round builtin_... compatibility problems */
#define _SIZE_T_	__typeof(sizeof(int))	/* works with builtin_... */
#define _PTRDIFF_T_	long
#define _CLOCK_T_	unsigned long
#define _TIME_T_	long
#define _WCHAR_T_	unsigned short	/* ISO 10646 PLANE 0 (UNICODE) */
#define _VA_LIST_	void *

#endif /* !__MIPS_ANSI_H */
