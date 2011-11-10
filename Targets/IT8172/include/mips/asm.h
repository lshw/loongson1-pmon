/*
 * asm.h: various macros to help assembly languages writers
 * Copyright (c) 1993	Algorithmics Ltd
 */

#define LEAF(name) \
  	.text; \
  	.globl	name; \
  	.ent	name; \
name:

#define XLEAF(name) \
  	.text; \
  	.globl	name; \
  	.aent	name; \
name:

#define WLEAF(name) \
  	.text; \
  	.weakext name; \
  	.ent	name; \
name:

#define SLEAF(name) \
  	.text; \
  	.ent	name; \
name:

#define END(name) \
  	.size name,.-name; \
  	.end	name

#define SEND(name) END(name)
#define WEND(name) END(name)

#define EXPORT(name) \
  	.globl name; \
  	.type name,@object; \
name:

#define EXPORTS(name,sz) \
  	.globl name; \
  	.type name,@object; \
  	.size name,sz; \
name:

#define WEXPORT(name,sz) \
  	.weakext name; \
  	.type name,@object; \
  	.size name,sz; \
name:

#define	IMPORT(name, size) \
	.extern	name,size

#define BSS(name,size) \
	.comm	name,size

#define LBSS(name,size) \
  	.lcomm	name,size
