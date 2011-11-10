/* $Id: queue.h,v 1.1.1.1 2006/09/14 01:59:06 root Exp $ */

/*
 * Copyright (c) 2000-2002 Opsycon AB  (www.opsycon.se)
 * Copyright (c) 2000 Rtmx, Inc   (www.rtmx.com)
 * Copyright (c) 2001 ipUnplugged AB (www.ipunplugged.com)
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
 *	This product includes software developed for Rtmx, Inc by
 *	Opsycon Open System Consulting AB, Sweden.
 *	This product includes software developed by Opsycon AB.
 *	This product includes software developed by ipUnplugged AB.
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

#ifndef _QUEUE_H_
#define _QUEUE_H_

typedef unsigned char Msg;

typedef struct Queue {
	unsigned short first;
	unsigned short count;
	unsigned short limit;
	Msg dat[1];
	} Queue;

/* Qinquiry operations */
#define Q_SIZE 1	/* Queue capacity */
#define Q_USED 2	/* space used */
#define Q_SPACE 3	/* space remaining */

Queue	*Qcreate __P((size_t));
void	Qput __P((Queue *, Msg));
Msg	Qget __P((Queue *));
int	Qinquiry __P((Queue *, int));
Msg	Qread __P((Queue *, int));

#define Qfull(x)	((x)->count == (x)->limit)
#define Qempty(x)	((x)->count == 0)
#define Qsize(x)	((x)->limit)
#define Qused(x)	((x)->count)
#define Qspace(x)	((x)->limit - (x)->count)

#endif /* _QUEUE_ */
