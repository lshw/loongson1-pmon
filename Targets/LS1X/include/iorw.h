//sw
/* write/read MMIO register */
#ifndef	IORW
#define IORW

/*
typedef unsigned long  u32;
typedef unsigned short u16;
typedef unsigned char  u8;
typedef signed long  s32;
typedef signed short s16;
typedef signed char  s8;
typedef int bool;
typedef unsigned long dma_addr_t;
*/

#define writeb(val, addr) (*(volatile unsigned char *)(addr) = (val))
#define writew(val, addr) (*(volatile unsigned short *)(addr) = (val))
#define writel(val, addr) (*(volatile unsigned long *)(addr) = (val))
#define readb(addr) (*(volatile unsigned char *)(addr))
#define readw(addr) (*(volatile unsigned short *)(addr))
#define readl(addr) (*(volatile unsigned long *)(addr))

#endif
