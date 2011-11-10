#ifndef	_PPC_H
#define	_PPC_H

// #include <machine/types.h>
#define S8		char
#define U8		unsigned char
#define U16		unsigned short
#define U32		unsigned int
#define U64		long double

//----------------------------------------------------------------------------
// PPC Assembly Functions
//----------------------------------------------------------------------------
#define SYNC	__asm __volatile("sync")
#define ISYNC	__asm __volatile("isync")
#define EIEIO	__asm __volatile("eieio")
#define NOP		__asm __volatile("nop")

// Macros for doing byte swapping loads and stores
#define LWBRX(dest, addr)		__asm __volatile("lwbrx %0,0,%1"\
								: "=r"(dest) : "r"(addr))
#define LHBRX(dest, addr)		__asm __volatile("lhbrx %0,0,%1"\
								: "=r"(dest) : "r"(addr))
#define STWBRX(value, dest)		__asm __volatile("stwbrx %0,0,%1"\
								: : "r"(value), "r"(dest))
#define STHBRX(value, dest)		__asm __volatile("sthbrx %0,0,%1"\
								: : "r"(value), "r"(dest))

// Macros for doing direct loads and stores
#define LBZ(dest, addr)			__asm __volatile("lbz %0,0(%1)"\
								: "=r"(dest) : "r"(addr))
#define LHZ(dest, addr)			__asm __volatile("lhz %0,0(%1)"\
								: "=r"(dest) : "r"(addr))
#define LWZ(dest, addr)			__asm __volatile("lwz %0,0(%1)"\
								: "=r"(dest) : "r"(addr))
#define STB(value, dest)		__asm __volatile("stb %0,0(%1)"\
								: : "r"(value), "r"(dest))
#define STH(value, dest)		__asm __volatile("sth %0,0(%1)"\
								: : "r"(value), "r"(dest))
#define STW(value, dest)		__asm __volatile("stw %0,0(%1)"\
								: : "r"(value), "r"(dest))

// Macros to access DEC reg -- Supervisor mode only
#define GET_DEC_REG(dest)		__asm __volatile("mfdec %0"\
								: "=r"(dest) : )
#define SET_DEC_REG(value)		__asm __volatile("mtdec %0"\
								: : "r"(value))

// Macros to access TB reg -- Must be in supervisor mode to set
#define GET_TBL_REG(dest)		__asm __volatile("mftb %0"\
								: "=r"(dest) : )
#define GET_TBU_REG(dest)		__asm __volatile("mftbu %0"\
								: "=r"(dest) : )
#define SET_TBL_REG(value)		__asm __volatile("mttb %0"\
								: : "r"(value))
#define SET_TBU_REG(value)		__asm __volatile("mttbu %0"\
								: : "r"(value))

// Count number of leading zeros of a 32 bit value
#define CNTLZW(result, data)	__asm __volatile("cntlzw %0, %1"\
								: "=r" (result) : "r" (data))
// Store Regs in memory
#define STMW(reg, addr)			__asm __volatile("stmw %0,0(%1)"\
								: : "r"(reg), "r"(addr))

// Access MSR Register -- Supervisor mode only
#define MFMSR(dest)				__asm __volatile("mfmsr %0"\
								: "=r"(dest) : )
#define MTMSR(value)			__asm __volatile("mtmsr %0"\
								: : "r"(value))

#define	ENABLE_EXTERNAL_INTR(msr_reg)	MFMSR(msr_reg); \
								MTMSR((msr_reg | 0x8000))

#define	DISABLE_EXTERNAL_INTR(msr_reg)	MFMSR(msr_reg); \
								MTMSR((msr_reg & 0xFFFF7FFF))

// Access HID1 Register -- Supervisor mode only
#define GETHID1(dest)				__asm __volatile("mfspr %0, 1009"\
								: "=r"(dest) : )

#endif	// PPC
