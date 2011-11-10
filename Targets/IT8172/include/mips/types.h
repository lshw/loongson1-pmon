/*
 * machine/types.h : SDE basic type definitions
 *
 * Copyright (c) 1998-1999, Algorithmics Ltd.  All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify 
 * it under the terms of the "Free MIPS" License Agreement, a copy of 
 * which is available at:
 *
 *  http://www.algor.co.uk/ftp/pub/doc/freemips-license.txt
 *
 * You may not, however, modify or remove any part of this copyright 
 * message if this program is redistributed or reused in whole or in
 * part.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * "Free MIPS" License for more details.  
 */

#ifndef __MACHINE_TYPES_H_

#define __MACHINE_TYPES_H_

/*
 * Basic integral types of exactly the implied number of bits.  Omit the
 * typedef if not possible for a machine/compiler combination.
 */
#if __GNUC__ > 2 || (__GNUC__ == 2 && __GNUC_MINOR__ >= 7)
typedef signed   int   int8_t    __attribute__((__mode__(__QI__)));
typedef unsigned int   u_int8_t  __attribute__((__mode__(__QI__)));
typedef          int   int16_t   __attribute__((__mode__(__HI__)));
typedef unsigned int   u_int16_t __attribute__((__mode__(__HI__)));
typedef          int   int32_t   __attribute__((__mode__(__SI__)));
typedef unsigned int   u_int32_t __attribute__((__mode__(__SI__)));
typedef          int   int64_t   __attribute__((__mode__(__DI__)));
typedef unsigned int   u_int64_t __attribute__((__mode__(__DI__)));
#else
typedef signed   char  int8_t;
typedef unsigned char  u_int8_t;
typedef          short int16_t;
typedef unsigned short u_int16_t;
typedef          int   int32_t;
typedef unsigned int   u_int32_t;
#ifdef __GNUC__
__extension__ typedef          long long  int64_t;
__extension__ typedef unsigned long long  u_int64_t;
#endif
#endif

typedef unsigned char  u_char;
typedef unsigned short u_short;
typedef unsigned int   u_int;
typedef unsigned long  u_long;

/*
 * Basic integral types of at least the implied number of bits.  Omit the
 * typedef if not possible for a machine/compiler combination.
 */
typedef int8_t              int8m_t;
typedef u_int8_t            u_int8m_t;
typedef int16_t             int16m_t;
typedef u_int16_t           u_int16m_t;
typedef int32_t             int32m_t;
typedef u_int32_t           u_int32m_t;
#ifdef __GNUC__
typedef int64_t             int64m_t;
typedef u_int64_t	    u_int64m_t;
#endif

/* The longest/most efficient integer register type. */
typedef int32_t			register_t;

/* Address types */
typedef char *			caddr_t;
typedef unsigned long		vm_offset_t;
typedef __typeof(sizeof(int))	vm_size_t;

#endif /* __MACHINE_TYPES_H_ */
