#ifndef __LS1X_UART_H_
#define __LS1X_UART_H_

#define LS1X_UART0_BASE			0xbfe40000
#define LS1X_UART1_BASE			0xbfe44000
#define LS1X_UART2_BASE			0xbfe48000
#define LS1X_UART3_BASE			0xbfe4c000
#ifdef LS1BSOC	/* Loongson 1B最多可扩展12个3线UART串口 1A只能有4个 */
#define LS1X_UART4_BASE			0xbfe6c000
#define LS1X_UART5_BASE			0xbfe7c000
#define LS1X_UART6_BASE			0xbfe41000
#define LS1X_UART7_BASE			0xbfe42000
#define LS1X_UART8_BASE			0xbfe43000
#define LS1X_UART9_BASE			0xbfe45000
#define LS1X_UART10_BASE		0xbfe46000
#define LS1X_UART11_BASE		0xbfe47000
#endif

#define UART16550_PARITY_NONE   0
#define UART16550_PARITY_ODD    0x08
#define UART16550_PARITY_EVEN   0x18
#define UART16550_PARITY_MARK   0x28
#define UART16550_PARITY_SPACE  0x38

#define UART16550_DATA_5BIT     0x0
#define UART16550_DATA_6BIT     0x1
#define UART16550_DATA_7BIT     0x2
#define UART16550_DATA_8BIT     0x3

#define UART16550_STOP_1BIT     0x0
#define UART16550_STOP_2BIT     0x4

void ls1x_uart_init(unsigned long base, u8 data, u8 parity, u8 stop);

#endif
