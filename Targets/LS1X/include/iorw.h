/* write/read MMIO register */
#ifndef IORW
#define IORW

#define writeb(val, addr) (*(volatile unsigned char *)(addr) = (val))
#define writew(val, addr) (*(volatile unsigned short *)(addr) = (val))
#define writel(val, addr) (*(volatile unsigned long *)(addr) = (val))
#define readb(addr) (*(volatile unsigned char *)(addr))
#define readw(addr) (*(volatile unsigned short *)(addr))
#define readl(addr) (*(volatile unsigned long *)(addr))

#endif
