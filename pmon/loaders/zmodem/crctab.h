/*
 * updcrc macro derived from article Copyright (C) 1986 Stephen Satchell. 
 *  NOTE: First srgument must be in range 0 to 255.
 *        Second argument is referenced twice.
 * 
 * Programmers may incorporate any or all code into their programs, 
 * giving proper credit within the source. Publication of the 
 * source routines is permitted so long as proper credit is given 
 * to Stephen Satchell, Satchell Evaluations
 *
 * wow ! a whole macro ! lets copyright it.....
 */

extern unsigned short int  crc16tab[0x100];
extern unsigned long crc32tab[0x100];

#define UPDCRC16(cp, crc) (crc16tab[((crc >> 8) & 255)] ^ (crc << 8) ^ cp)

#define UPDCRC32(b, c)    (crc32tab[((int)c ^ b) & 0xff] ^ ((c >> 8) & 0x00FFFFFF))


