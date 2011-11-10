/*
 * stdarg.h : MIPS stdarg handling
 * Copyright (c) 1992 Algorithmics Ltd.
 */

#ifndef __MIPS_STDARG_H
#define __MIPS_STDARG_H

/*
 * ANSI style varargs for MIPS
 */

#include <mips/ansi.h>

#ifdef _VA_LIST_
typedef _VA_LIST_ va_list;
#undef _VA_LIST_
#endif

#if __mips64 && !defined(__mips_algabi)
/* Cygnus GCC passes args as 64-bits regs */
#define __VA_REG	long long
#else
/* Algor GCC passes args as 32-bits regs for compatibility */
#define __VA_REG	int
#endif

#ifndef _VA_MIPS_H_ENUM
#define _VA_MIPS_H_ENUM
/* values returned by __builtin_classify_type */
enum {
  __no_type_class = -1,
  __void_type_class,
  __integer_type_class,
  __char_type_class,
  __enumeral_type_class,
  __boolean_type_class,
  __pointer_type_class,
  __reference_type_class,
  __offset_type_class,
  __real_type_class,
  __complex_type_class,
  __function_type_class,
  __method_type_class,
  __record_type_class,
  __union_type_class,
  __array_type_class,
  __string_type_class,
  __set_type_class,
  __file_type_class,
  __lang_type_class
};
#endif

/* Amount of space required in an argument list for an arg of type TYPE.
   TYPE may alternatively be an expression whose type is used.  */

#define __va_rounded_size(TYPE)  \
  (((sizeof (TYPE) + sizeof (__VA_REG) - 1) / sizeof (__VA_REG)) 	\
   * sizeof (__VA_REG))

/* Alignment of an arg of given type (char,short promote to reg), - 1. */

#if __mips64 && __mips_cygabi
#define __va_alignoff(TYPE)  7
#else
#define __va_alignoff(TYPE)  						\
  (((__alignof(TYPE) <= __alignof(__VA_REG)) 				\
    ?__alignof(__VA_REG) 						\
    : __alignof(TYPE)) - 1)
#endif

/* When big-endian, small structures are shifted left in register so
   as to be in low memory when saved, but scalar data smaller than a
   register is not. */

#define __va_real_size(TYPE) \
    ((sizeof(TYPE) <= sizeof(__VA_REG)	 				\
      && __builtin_classify_type(*(TYPE *)0) != __record_type_class	\
      && __builtin_classify_type(*(TYPE *)0) != __union_type_class)	\
     ? sizeof(TYPE) : __va_rounded_size(TYPE))

#if defined(__VARARGS_H)
/* old-style vararg */
#define va_alist	__builtin_va_alist
#define va_dcl		__VA_REG __builtin_va_alist; ...
#define va_start(AP)	AP = (va_list)&__builtin_va_alist;
#elif __GNUC_MAJOR__ > 2 || __GNUC_MINOR__ >= 7
/* new-style stdarg, modern gcc */
#define va_start(AP, LASTARG) 						\
 (AP = ((va_list) __builtin_next_arg(LASTARG)))
#else
/* new-style stdarg, older gcc */
#define va_start(AP, LASTARG) 						\
 (AP = ((va_list) __builtin_next_arg()))
#endif

#define va_end(AP)	((void)0)

/* Copy va_list into another variable of this type.  */
#define __va_copy(dest, src) (dest) = (src)

#ifdef __MIPSEB__
/* big-endian: args smaller than register in higher memory address */
#define va_arg(AP, TYPE) 						      \
 (AP = 									      \
   (va_list) (((long)AP + __va_alignoff(TYPE)) & ~__va_alignoff(TYPE))        \
      + __va_rounded_size(TYPE), 					      \
   *((TYPE *) (void *) ((char *)AP - __va_real_size(TYPE))))
#else
/* little-endian: args smaller than register in lower memory address */
#define va_arg(AP, TYPE) 						      \
 (AP = 									      \
   (va_list) (((long)AP + __va_alignoff(TYPE)) & ~__va_alignoff(TYPE))        \
      + __va_rounded_size(TYPE), 					      \
   *((TYPE *) (void *) ((char *)AP - __va_rounded_size(TYPE))))
#endif

#endif /* __MIPS_STDARG_H */
