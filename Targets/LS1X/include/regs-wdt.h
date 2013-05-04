#ifndef __ASM_MACH_LOONGSON1_REGS_WDT_H
#define __ASM_MACH_LOONGSON1_REGS_WDT_H

#if defined(LS1ASOC)
#define LS1X_WDT_BASE			0xbfe7c060
#elif defined(LS1BSOC) || defined(LS1CSOC)
#define LS1X_WDT_BASE			0xbfe5c060
#endif

#define LS1X_WDT_EN			(LS1X_WDT_BASE + 0x0)
#define LS1X_WDT_TIMER		(LS1X_WDT_BASE + 0x4)
#define LS1X_WDT_SET		(LS1X_WDT_BASE + 0x8)

#endif /* __ASM_MACH_LOONGSON1_REGS_WDT_H */
