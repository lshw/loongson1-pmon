#ifndef __LINUXIO_H_
#define __LINUXIO_H_

#ifndef mips_io_port_base
#if defined(FCRSOC)||defined(BONITOEL)
#define mips_io_port_base 0xbfd00000
#elif defined(LS1ASOC)
#define mips_io_port_base 0xbc000000
#else
#ifdef CONFIG_PCI0_GAINT_MEM
#define mips_io_port_base 0xbea00000
#else
#define mips_io_port_base 0xb0100000
#endif
#endif
#endif

#define __SLOW_DOWN_IO \
	__asm__ __volatile__( \
		"sb\t$0,0x80(%0)" \
		: : "r" (mips_io_port_base));

#define SLOW_DOWN_IO {__SLOW_DOWN_IO; __SLOW_DOWN_IO; __SLOW_DOWN_IO; \
		__SLOW_DOWN_IO; __SLOW_DOWN_IO; __SLOW_DOWN_IO; __SLOW_DOWN_IO;  \
		__SLOW_DOWN_IO; }

static inline unsigned char linux_inb(unsigned long port)
{
        return (*(volatile unsigned char *)(mips_io_port_base + port));
}

static inline unsigned short linux_inw(unsigned long port)
{
        return (*(volatile unsigned short *)(mips_io_port_base + port));
}

static inline unsigned int linux_inl(unsigned long port)
{
        return (*(volatile unsigned long *)(mips_io_port_base + port));
}

#define linux_outb(val,port)\
do {\
*(volatile unsigned char *)(mips_io_port_base + (port)) = (val);  \
} while(0)

#define linux_outw(val,port)							\
do {									\
	*(volatile unsigned short *)(mips_io_port_base + (port)) = (val);	\
} while(0)

#define linux_outl(val,port)							\
do {									\
	*(volatile unsigned long *)(mips_io_port_base + (port)) = (val);\
} while(0)

#define linux_outb_p(val,port)                                                \
do {                                                                    \
        *(volatile unsigned char *)(mips_io_port_base + (port)) = (val);           \
        SLOW_DOWN_IO;                                                   \
} while(0)

static inline unsigned char linux_inb_p(unsigned long port)
{
	unsigned char __val;

        __val = *(volatile unsigned char *)(mips_io_port_base + port);
        SLOW_DOWN_IO;

        return __val;
}

#endif /* __LINUXIO_H_ */
