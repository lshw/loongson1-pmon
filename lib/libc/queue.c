/* $Id: queue.c,v 1.1.1.1 2006/09/14 01:59:06 root Exp $ */

/*
 * Copyright (c) 2000-2002 Opsycon AB  (www.opsycon.se)
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
 *	This product includes software developed by Opsycon AB.
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

#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>

#include <queue.h>

extern int reschedule(void);

/**  Queue *Qcreate(size) creates a queue of specified size */

Queue *
Qcreate(size_t size)
{
	Queue *q;

	if (size < 0 || (size & (size - 1))) {
		printf ("Qcreate: size %d not a power of two\n");
		return 0;
	}

	q = (Queue *) malloc (sizeof (Queue) + ((size - 1) * sizeof (Msg)));
	if (!q)
		return (0);
	q->limit = size - 1;
	q->first = q->count = 0;
	return (q);
}

/**  Qput(q,msg) -- adds msg to queue */

void
Qput(Queue *q, Msg msg)
{
	while (Qfull (q))
		reschedule ();

	q->dat[(q->first + q->count) & q->limit] = msg;
	q->count++;
}

/**  Msg Qget(q) -- removes a msg from a queue */

Msg 
Qget(Queue *q)
{
	Msg msg;

	while (Qempty(q))
		reschedule ();

	msg = q->dat[q->first];
	q->first = (q->first + 1) & q->limit;
	q->count--;
	return (msg);
}

/** Qfull(q) returns 1 if queue is full (macro) */
/** Qempty(q) returns 1 if queue is empty (macro) */

/**  Qinquiry(q,op) -- inquire about queue SIZE, USED, SPACE */

int
Qinquiry(Queue *q, int op)
{
	switch (op) {
	case Q_SIZE:		/* what's the capacity of the queue? */
		return Qsize (q);
	case Q_USED:		/* how much space is used? */
		return Qused (q);
	case Q_SPACE:		/* how much space is left? */
		return Qspace (q);
	}
	return -1;
}

/**  Msg Qread(q,n) -- read msg n from queue (non destructive) */

Msg 
Qread(Queue *q, int n)
{
	if (n >= q->count)
		return 0;
	return q->dat[(q->first + n) & q->limit];
}
