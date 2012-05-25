/* type */
#ifndef _TYPES_
#define _TYPES_

typedef unsigned int u32;
typedef unsigned short u16;
typedef unsigned char u8;

#define K1BASE 0xa0000000
#define KSEG1(addr) ((void *)(K1BASE | (u32)(addr)))

#define KSEG1_STORE8(addr, value) *(volatile u8 *)(KSEG1(addr)) = (value)
#define KSEG1_STORE16(addr, value) *(volatile u16 *)(KSEG1(addr)) = (value)
#define KSEG1_STORE32(addr, value) *(volatile u32 *)(KSEG1(addr)) = (value)

#define KSEG1_LOAD8(addr) *(volatile u8 *)(KSEG1(addr))
#define KSEG1_LOAD16(addr) *(volatile u16 *)(KSEG1(addr))
#define KSEG1_LOAD32(addr) *(volatile u32 *)(KSEG1(addr))

#define STORE8(addr, value) *(volatile u8 *)(addr) = value
#define STORE16(addr, value) *(volatile u16 *)(addr) = value
#define STORE32(addr, value) *(volatile u32 *)(addr) = value

#define LOAD8(addr) *(volatile u8 *)(addr)
#define LOAD16(addr) *(volatile u16 *)(addr)
#define LOAD32(addr) *(volatile u32 *)(addr)

#define PHY(addr) \
    (((u32)addr >= 0x80000000 && (u32)addr < 0xa0000000)? \
    (u32)addr-0x80000000:(u32)addr >= 0xa0000000? \
    (u32)addr-0xa0000000:(u32)addr) 

#endif /* _TYPES_ */
 
