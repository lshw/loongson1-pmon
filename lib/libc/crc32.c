/*      $Id: crc32.c,v 1.2 2006/09/09 10:15:07 pefo Exp $    */

/*
 * Copyright (c) 1983 Regents of the University of California.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

/*
 *  CRC32 support functions.
 */

#include <sys/types.h>
#include <crc32.h>

/*
 *  CRC generator shortcut table.
 */
static const u_int crctab[] = {
	0x00000000, 0x1db71064, 0x3b6e20c8, 0x26d930ac,
	0x76dc4190, 0x6b6b51f4, 0x4db26158, 0x5005713c,
	0xedb88320, 0xf00f9344, 0xd6d6a3e8, 0xcb61b38c,
	0x9b64c2b0, 0x86d3d2d4, 0xa00ae278, 0xbdbdf21c
};

/*
 *  Generate CRC32 for data block 'data' with size 'length'.
 *
 *  Returns generated CRC32.
 */
u_int
crc32_generate(void *data, size_t length)
{
	return crc32_generate_seg(data, length, 0xffffffff);
}

u_int
crc32_generate_seg(void *data, size_t length, u_int crc)
{
	const u_char *p = data;
	const u_int *crctabp = crctab;
	int i;

	if (length < 1)
		return -1;

	i = length;
	while (i-- > 0) {
		crc ^= *p++;
		crc = (crc >> 4) ^ crctabp[crc & 0xf];
		crc = (crc >> 4) ^ crctabp[crc & 0xf];
	}
	return crc;
}

/*
 *  Check CRC32 for data block 'data' with size 'length'.
 *  Last two bytes is the CRC32.
 *
 *  Returns -1 if data out of range.
 *  Returns 0 if CRC32 is checked OK.
 *  Returns > 0 if CRC32 is checked BAD.
 */
u_int
crc32_check(void *data, size_t length)
{
	const u_char *p = data;
	int result;

	if (length < 5)
		return -1;

	result = crc32_generate(data, length - 4);
	p += length - 4;
	result ^= p[0] | p[1] << 8 | p[2] << 16 | p[3] << 24;
	return result;
}
