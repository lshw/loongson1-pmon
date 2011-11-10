
#ifndef __FCR_SOC_DFME_H_
#define __FCR_SOC_DFME_H_
#define MAC_REG_BASE	0xBF005200  //zgj 0x1f002000

#define __ioswab8(x) (x)
#define __ioswab16(x) (x)
#define __ioswab32(x) (x)

static inline unsigned char dmfe_inb(unsigned long port)
{
    return __ioswab8(*(volatile unsigned char *)(port | K1BASE));
}

static inline unsigned short dmfe_inw(unsigned long port)
{
    return __ioswab16(*(volatile unsigned short *)( port | K1BASE));
}

static inline unsigned int dmfe_inl(unsigned long port)
{
    return __ioswab32(*(volatile unsigned long *)( port | K1BASE));
}

#define dmfe_outb(val,port) \
do {\
*(volatile unsigned char *)((port) | K1BASE) = __ioswab8(val);  \
} while(0)

#define dmfe_outw(val,port)							\
do {									\
	*(volatile unsigned short *)((port) | K1BASE) = __ioswab16(val);	\
} while(0)

#define dmfe_outl(val,port)							\
do {									\
	*(volatile unsigned long *)((port) | K1BASE) = __ioswab32(val);\
} while(0)

#define dmfe_readb(addr)             (*(volatile unsigned char *)(addr))
#define dmfe_readw(addr)             __ioswab16((*(volatile unsigned short *)(addr)))
#define dmfe_readl(addr)             __ioswab32((*(volatile unsigned int *)(addr)))

#endif /* __LX_IO_H_ */

