/* $Id: memtst.c,v 1.1.1.1 2006/09/14 01:59:08 root Exp $ */

/*
 * Copyright (c) 2001 Opsycon AB  (www.opsycon.se / www.opsycon.com)
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by Opsycon AB, Sweden.
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <termio.h>
#ifdef _KERNEL
#undef _KERNEL
#endif
#include <sys/ioctl.h>
#include <pmon.h>

#define ADDRESS_MARK           0x00000003

int cmd_memtst __P((int, char *[]));
static int do_mt __P((u_int32_t *, u_int32_t *, int));
int memory_selfchk __P((int, char *[]));

const Optdesc         mt_opts[] = {
	{"-c", "continuous test"},
	{"-v", "verbose"},
	{0}
};

static const Cmd MtestCmd[] =
{
	{"Memory"},
	{"mt", "[-cv] [start [end]]", mt_opts, "simple memory test", cmd_memtst, 1, 4, CMD_REPEAT},
	{"memory_selfchk", "", 0, "memory_selfchk", memory_selfchk, 1, 4, CMD_REPEAT},
	{0, 0}
};

static void init_cmd_mtest __P((void)) __attribute__ ((constructor));

static void
init_cmd_mtest()
{
	cmdlist_expand(MtestCmd, 1);
}

extern char *heaptop;
int cmd_memtst(int ac, char **av)
{
	int cflag, vflag, err;
	u_int32_t saddr, eaddr;
	int c;
	char cmd[100];

	cflag = 0;
	vflag = 0;

	saddr = heaptop;
	saddr |= 0xa0000000;
#if defined(__mips__)
//	eaddr = (memorysize & 0x3fffffff) + ((int)heaptop & 0xc0000000);
	eaddr = 0xa2000000;
#else
	eaddr = memorysize;
#endif

	optind = 0;
	while((c = getopt(ac, av, "cv")) != EOF) {
		switch(c) {
		case 'v':
			vflag = 1;
			break;
		case 'c':
			cflag = 1;
			break;
		default:
			return(-1);
		}
	}

	if(optind < ac) {
		if (!get_rsa(&saddr, av[optind++])) {
			return(-1);
		}
		if(optind < ac) {
			if(!get_rsa(&eaddr, av[optind++])) {
				return (-1);
			}
		}
	}
	
	/* 起始地址和结束地址都设置为4字节对齐 */
	saddr = saddr & ~0x3;
	eaddr &= ~0x3;

	if(eaddr < saddr) {
	#ifdef CONFIG_CHINESE
		printf("结束地址值小于起始地址值!\n");
	#else
		printf("end address less that start address!\n");
	#endif
		return(-1);
	}

	ioctl (STDIN, CBREAK, NULL);

#ifdef CONFIG_CHINESE
	printf ("测试从 %08x 到 %08x 的地址  %s\n", saddr, eaddr, (cflag) ? "连续测试" : "");
#else
	printf ("Testing %08x to %08x %s\n", saddr, eaddr, (cflag) ? "continuous" : "");
#endif

	if (cflag) {
		while(!(err = do_mt((u_int32_t *)saddr, (u_int32_t *)eaddr, vflag)));
	}
	else {
		err = do_mt((u_int32_t *)saddr, (u_int32_t *)eaddr, vflag);
	}

	if (err) {
	#ifdef CONFIG_CHINESE
		printf ("发现错误！ %d\n", err);
//		sprintf(cmd, "set memorytest \"%s\"", "测试失败！");
	#else
		printf ("There were %d errors.\n", err);
//		sprintf(cmd, "set memorytest \"%s\"", "failed !");
	#endif
//		do_cmd(cmd);
		return (1);
	}
#ifdef CONFIG_CHINESE
	printf ("测试通过\n");
//	sprintf(cmd, "set memorytest \"%s\"", "测试通过");
#else
	printf ("Test passed ok.\n");
//	sprintf(cmd, "set memorytest \"%s\"", "pass");
#endif
//	do_cmd(cmd);
	return (0);
}

static int
do_mt(u_int32_t *saddr, u_int32_t *eaddr, int vflag)
{
	int i, j, err, temp, siz;
	unsigned int w, *p, r;

	err = 0;
	siz = eaddr - saddr;

	/* walking ones test */
	if(vflag) {
	#ifdef CONFIG_CHINESE
		printf("Walking ones ..... 请等待\n");
	#else
		printf("Walking ones - %p  ", saddr);
	#endif
	}
	else {
	#ifdef CONFIG_CHINESE
		printf("测试中...  ");
	#else
		printf("Testing...  ");
	#endif
	}
	for (p = saddr, i = 0; i < siz; i++, p++) {
		w = 1;
		for (j = 0; j < 32; j++) {
			*p = w;
			temp = 0;
			r = *p;
			if (r != w) {
				err++;
			#ifdef CONFIG_CHINESE
				printf ("\b\n错误: 地址=%08x 读取值=%08x 正确值=%08x  ", p, r, w);
			#else
				printf ("\b\nerror: addr=%08x read=%08x expected=%08x  ", p, r, w);
			#endif
			}
			w <<= 1;
//			dotik (1, 0);
		}
//		if(vflag && ((int)p & 0xffff) == 0) {
//			printf("\b\b\b\b\b\b\b\b\b\b\b\b%p  ", p);
//		}
	}
	if(vflag) {
	#ifdef CONFIG_CHINESE
		printf("\b\b\b\b\b\b\b\b\b\b\b\b完成.          \n");
	#else
		printf("\b\b\b\b\b\b\b\b\b\b\b\bDone.          \n");
	#endif
	}

	/* store the address in each address */
	if(vflag) {
	#ifdef CONFIG_CHINESE
		printf("地址测试 ..... 请等待\n", saddr);
	#else
		printf("Address test - %p  ", saddr);
	#endif
	}
	for (p = saddr, i = 0; i < siz; i++, p++) {
		*p = (unsigned int)p;
//		dotik (1, 0);
//		if(vflag && ((int)p & 0xffff) == 0) {
//			printf("\b\b\b\b\b\b\b\b\b\b\b\b%p  ", p);
//		}
	}

	/* check that each address contains its address */
	for (p = saddr, i = 0; i < siz; i++, p++) {
		r = *p;
		if (r != (unsigned int)p) {
			err++;
		#ifdef CONFIG_CHINESE
			printf ("\b\n错误: 地址=%08x 读取值=%08x 正确值=%08x  ", p, r, w);
		#else
			printf ("\b\nerror: adr=%08x read=%08x expected=%08x  ", p, r, p);
		#endif
		}
//		dotik (1, 0);
//		if(vflag && ((int)p & 0xffff) == 0) {
//			printf("\b\b\b\b\b\b\b\b\b\b\b\b%p  ", p);
//		}
	}
	if(vflag) {
	#ifdef CONFIG_CHINESE
		printf("\b\b\b\b\b\b\b\b\b\b\b\b完成.          \n");
	#else
		printf("\b\b\b\b\b\b\b\b\b\b\b\bDone.          \n");
	#endif
	}
	return (err);
}


/*--------------------------------------------------------------------------------------------------------*/
#define ROMLOG_PRINT	printf
#define ROMLOG_PUTHEX
#define REFCLK          330000000
#define TIMEOUT         200     /* ms */

#define LONG_WIDTH             sizeof(u_int32_t)
#define BEGIN_LOW_ADDR_TYPE    0
#define BEGIN_HIGH_ADDR_TYPE   1
#define BLOCK_LEVEL_SIZE       0x00100000
#define BLOCK_LEVEL_SIZE_MARK  0x000FFFFF
#define ADDRESS_MARK           0x00000003
#define WATCH_DOG_ADDRESS_MARK 0x00000FFF

#define SELFCHK_SUCCESS    0
#define SELFCHK_FAILED     -1
#define SELFCHK_CANCELLED  1

#define CTRL(x)                (x & 0x1f)

typedef void (*SELFCHK_WATCHDOG_FUNC)(void);

void ppc_cache_enable(void)
{
	return;
}

void memery_selfcheck_watchdog(void)
{
	return;
}

static void selfchk_udelay(u_int32_t usec)
{
	return;
}

static int wait_key_cmd(void)
{
	int     c;

	/* Wait 0-9 press, ignore other key press. */
	do {
//		c = ROMLOG_GETC();
//		c = get_uart_char(10);
//		c = get_uart_char(0);
		c = get_uart_char(8);
		selfchk_udelay(10);
		memery_selfcheck_watchdog();
		switch (c) {
			case '0':
				return c;
			case '1':
				return c;
			case '2':
				return c;
			case '3':
				return c;
			case '4':
				return c;
			case '5':
				return c;
			case '6':
				return c;
			case '7':
				return c;
			case '8':
				return c;
			case '9':
				return c;
			default:
				break;
		}
	} while (1);

	return (c);
}


static int is_stop_memery_selfchk(void)
{
	int key;

//	key = ROMLOG_GETC();
//	key = get_uart_char(10);
//	key = get_uart_char(0);
	key = get_uart_char(8);

	if (key ==  CTRL('C')) {
		ROMLOG_PRINT("Cancelled by user\r\n");   
		return TRUE;
	}

	return FALSE;
}

/* Wait usr press CTRL+M to enter memory selfchk. */
static int memery_selfchk_wait_cmd(void)
{
    int c;

    u_int32_t timeout =  TIMEOUT * 100;

    /* Wait CTRL+M press OR timeout, ignore other key press. */
    do {
        c = ROMLOG_GETC( );
        timeout--;
        selfchk_udelay(10);
    } while (((c == -1) || (c != CTRL('M'))) && (timeout > 0));

    return (c == CTRL('M'));
}

static void memery_selfchk_print(u_int32_t addr, u_int32_t corrent, u_int32_t actual) 
{
    ROMLOG_PRINT("[FAILED]Address: ");
    ROMLOG_PUTHEX(addr);
    ROMLOG_PRINT(", Correct Value: ");
    ROMLOG_PUTHEX(corrent);
    ROMLOG_PRINT(", Actual Value: ");
    ROMLOG_PUTHEX(actual);
    ROMLOG_PRINT("\r\n");
    memery_selfcheck_watchdog();
}


static void memery_selfchk_watchdog(u_int32_t addr, SELFCHK_WATCHDOG_FUNC mem_watchdog)
{
	return;
}

/**
 * memery_selfchk_moving_inversions - moving inversions(倒置) 算法
 * 
 * @mem_start: 内存自检起始地址
 * @mem_size: 内存自检大小
 * @mem_data: 内存检测的数据
 * @mem_watchdog: 内存检测时的喂狗函数
 *
 * 用moving inversions算法进行内存检测
 *
 * CTRL+C退出返回SELFCHK_CANCELLED，自检成功返回SELFCHK_SUCCESS，
 * 否则返回SELFCHK_FAILED
 */
static int memery_selfchk_moving_inversions(u_int32_t mem_start,
                                                   u_int32_t mem_size,
                                                   u_int32_t mem_data,
                                                   u_int32_t check_width,
                                                   SELFCHK_WATCHDOG_FUNC mem_watchdog)
{
    int ret;
    int offset;
    u_int32_t start, end;
    u_int32_t inver_data;
    u_int32_t read_buf;

    if ((mem_start & ADDRESS_MARK) != 0) {
        ROMLOG_PUTHEX(mem_start);
        ROMLOG_PRINT(" :the address is not alignment doubleword.\r\n");
        
        return SELFCHK_FAILED;
    }

    ret = SELFCHK_SUCCESS;
    inver_data = ~(mem_data);
    offset = 0;

    /* 向内存写数据 */
    while (offset < mem_size) {
        start = mem_start + offset;
        
        *((volatile u_int32_t *)start) = mem_data;
		
		/* 判断是否按下CTRL+C退出返回 */
        if (is_stop_memery_selfchk() == TRUE) {
            return SELFCHK_CANCELLED;
        }
        
        memery_selfchk_watchdog(start, mem_watchdog);
        
		/* 步进 */
        offset += check_width;
    }

    /* 内存段的低地址开始读数据，检测后，写入其反码 */
    offset = 0;
    while (offset < mem_size) {
        start = mem_start + offset;
        
        read_buf = *((volatile u_int32_t *)start);
        if (read_buf != mem_data) {
            ret = SELFCHK_FAILED;
			/* 打印信息 */
            memery_selfchk_print(start, mem_data, read_buf);
        }

        *((volatile u_int32_t *)start) = inver_data;
        
        if (is_stop_memery_selfchk() == TRUE) {
            return SELFCHK_CANCELLED;
        }

        memery_selfchk_watchdog(start, mem_watchdog);
        
        offset += check_width;
    }

    /* 内存段的高地址开始读数据，检测后，写入其反码 */
    offset = mem_size - check_width;
    while (offset >= 0) {
        end = mem_start + offset;
        
        read_buf = *((volatile u_int32_t *)end);
        if (read_buf != inver_data) {
            ret = SELFCHK_FAILED;
            memery_selfchk_print(end, inver_data, read_buf);
        }

        *((volatile u_int32_t *)end) = mem_data;
        
        if (is_stop_memery_selfchk() == TRUE) {
            return SELFCHK_CANCELLED;
        }

        memery_selfchk_watchdog(end , mem_watchdog);

        offset -= check_width;
    }

    offset = mem_size - check_width;
    while (offset >= 0) {
        end = mem_start + offset;
        
        read_buf = *((volatile u_int32_t *)end);
        if (read_buf != mem_data) {
            ret = SELFCHK_FAILED;
            memery_selfchk_print(end, mem_data, read_buf);
        }
        
        if (is_stop_memery_selfchk() == TRUE) {
            return SELFCHK_CANCELLED;
        }
        
        memery_selfchk_watchdog(end , mem_watchdog);

        offset -= check_width;
    }

    return ret;
}


/**
 * memery_selfchk_modulo_x - Modulo-X算法
 * 
 * @mem_start: 内存自检起始地址
 * @mem_size: 内存自检大小
 * @mem_data: 内存检测的数据
 * @mem_watchdog: 内存检测时的喂狗函数
 *
 * 用Modulo-X算法进行内存检测
 *
 * CTRL+C退出返回SELFCHK_CANCELLED，自检成功返回SELFCHK_SUCCESS，
 * 否则返回SELFCHK_FAILED
 */
static int memery_selfchk_modulo_x(u_int32_t mem_start,
                                          u_int32_t mem_size,
                                          u_int32_t mem_data,
                                          SELFCHK_WATCHDOG_FUNC mem_watchdog)
{
    int ret;
    int offset;
    u_int32_t inver_data;
    u_int32_t start;
    u_int32_t read_buf;

    if ((mem_start & ADDRESS_MARK) != 0) {
        ROMLOG_PUTHEX(mem_start);
        ROMLOG_PRINT(" : the address is not alignment doubleword.\r\n");
        return SELFCHK_FAILED;
    }

    ret = SELFCHK_SUCCESS;
    offset = 0;
    inver_data = ~(mem_data);

    /* 向每个20*（长整型数据长度）（ppc中长整型数据的长度为4bytes）的整数倍的地址
     * 写入一定格式的数据（数据类型为长整型），向其他地址写入上述数据的反码。
     */
    while (offset < mem_size) {
        start = mem_start + offset;
        if (start % 20 == 0) {
            *((volatile u_int32_t *)start) = mem_data;
        } else {
            *((volatile u_int32_t *)start) = inver_data;
        }

        if (is_stop_memery_selfchk() == TRUE) {
            return SELFCHK_CANCELLED;
        }
        
        memery_selfchk_watchdog(start, mem_watchdog);

        offset += LONG_WIDTH;
    }

    /* 检查所有 20*长整型数据长度的整数倍的地址中的数据，是否和写入的值相等 */
    offset = 0;
    while (offset < mem_size) {
        start = mem_start + offset;
        if (start % 20 == 0) {
            read_buf = *((volatile u_int32_t *)start);
            if (read_buf != mem_data) {
                ret = SELFCHK_FAILED;
                memery_selfchk_print(start, mem_data, read_buf);
            }
        }

        if (is_stop_memery_selfchk() == TRUE) {
            return SELFCHK_CANCELLED;
        }
        
        memery_selfchk_watchdog(start, mem_watchdog);

        offset += LONG_WIDTH;
    }

    return ret;
}


static int memery_selfchk_read_after_write(u_int32_t mem_start,
                                                   u_int32_t mem_size,
                                                   u_int32_t type,
                                                   u_int32_t read_data,
                                                   u_int32_t write_data,
                                                   SELFCHK_WATCHDOG_FUNC mem_watchdog)
{
    int ret;
    int offset;
    u_int32_t start, end;
    u_int32_t read_buf;

    ret = SELFCHK_SUCCESS;

    /* 读出一个数据，再写入另一个数据 */
    switch (type) {
    case BEGIN_LOW_ADDR_TYPE:
        offset = 0;
        while (offset < mem_size) {
            start = mem_start + offset;
            read_buf = *((volatile u_int32_t *)start);
            if (read_buf != read_data) {
                ret = SELFCHK_FAILED;
                memery_selfchk_print(start, read_data, read_buf);
            }
            
            *((volatile u_int32_t *)start) = write_data;
            
            if (is_stop_memery_selfchk() == TRUE) {
                return SELFCHK_CANCELLED;
            }

            memery_selfchk_watchdog(start, mem_watchdog);

            offset += LONG_WIDTH;
        }
        break;
    case BEGIN_HIGH_ADDR_TYPE:
        offset= mem_size - LONG_WIDTH;
        while (offset >= 0) {
            end = mem_start + offset;
            read_buf = *((volatile u_int32_t *)end);
            if (read_buf != read_data) {
                ret = SELFCHK_FAILED;
                memery_selfchk_print(end, read_data, read_buf);
            }
            
            *((volatile u_int32_t *)end) = write_data;
            
            if (is_stop_memery_selfchk() == TRUE) {
                return SELFCHK_CANCELLED;
            }

            memery_selfchk_watchdog(end, mem_watchdog);

            offset -= LONG_WIDTH;
        }
        break;
     default:
        ret = SELFCHK_FAILED;
        break;
     }

    return ret;
}


/**
 * memery_selfchk_mats - MATS算法
 * 
 * @mem_start: 内存自检起始地址
 * @mem_size: 内存自检大小
 * @write_data0: 进行内存自检的数据0
 * @write_data1: 进行内存自检的数据1
 * @mem_watchdog: 内存检测时的喂狗函数
 *
 * 用MATS算法进行内存检测
 *
 * CTRL+C退出返回SELFCHK_CANCELLED，自检成功返回SELFCHK_SUCCESS，
 * 否则返回SELFCHK_FAILED
 */
static int memery_selfchk_mats(u_int32_t mem_start, 
                                      u_int32_t mem_size,
                                      u_int32_t write_data0,
                                      u_int32_t write_data1,
                                      SELFCHK_WATCHDOG_FUNC mem_watchdog)
{
    int ret;
    int offset;
    u_int32_t start, end;
    u_int32_t read_buf;

    if ((mem_start & ADDRESS_MARK) != 0) {
        ROMLOG_PUTHEX(mem_start);
        ROMLOG_PRINT(" : the address is not alignment doubleword.\r\n");
        return SELFCHK_FAILED;
    }

    ret = SELFCHK_SUCCESS;
    offset = 0;

    /* 被测试单元全空间写 0 */
    while (offset < mem_size) {
        start = mem_start + offset;
        
        *((volatile u_int32_t *)start) = write_data0;

        if (is_stop_memery_selfchk() == TRUE) {
            return SELFCHK_CANCELLED;
        }
        
        memery_selfchk_watchdog(start, mem_watchdog);

        offset += LONG_WIDTH;
    }

    /* 从地址低位操作，读0，改写为1，从低位地址向高位地址操作，直到所有的内存测试空间都写为1。 */
    ret = memery_selfchk_read_after_write(mem_start, 
        mem_size, BEGIN_LOW_ADDR_TYPE, write_data0, write_data1, mem_watchdog);
    if (ret == SELFCHK_CANCELLED) {
        return ret;
    }
 
    /* 从地址高位操作，读1，改写为0，从高位地址向低位地址操作，直到所有的内存测试空间都写为0 */
    ret = memery_selfchk_read_after_write(mem_start, 
        mem_size, BEGIN_HIGH_ADDR_TYPE, write_data1, write_data0, mem_watchdog);
    if (ret == SELFCHK_CANCELLED) {
        return ret;
    }
 
    /* 读测试空间的值0进行比较 */
    offset = mem_size- LONG_WIDTH;
    while (offset >= 0) {
        end = mem_start + offset;
        
        read_buf = *((volatile u_int32_t *)end);
        if (read_buf != write_data0) {
            ret = SELFCHK_FAILED;
            memery_selfchk_print(end, write_data0, read_buf);
        }

        if (is_stop_memery_selfchk() == TRUE) {
            return SELFCHK_CANCELLED;
        }
        
        memery_selfchk_watchdog(end, mem_watchdog);

        offset -= LONG_WIDTH;
    }

    return ret;
}


/**
 * memery_selfchk_march_c - March C算法
 * 
 * @mem_start: 内存自检起始地址
 * @mem_size: 内存自检大小
 * @write_data0: 进行内存自检的数据0
 * @write_data1: 进行内存自检的数据1
 * @mem_watchdog: 内存检测时的喂狗函数
 *
 * 用March C算法进行内存检测
 *
 * CTRL+C退出返回SELFCHK_CANCELLED，自检成功返回SELFCHK_SUCCESS，
 * 否则返回SELFCHK_FAILED
 */
static int memery_selfchk_march_c(u_int32_t mem_start, 
                                         u_int32_t mem_size, 
                                         u_int32_t write_data0,
                                         u_int32_t write_data1,
                                         SELFCHK_WATCHDOG_FUNC mem_watchdog)
{
    int ret;
    int offset;
    u_int32_t start;
    u_int32_t read_buf;

    if ((mem_start & ADDRESS_MARK) != 0) {
        ROMLOG_PUTHEX(mem_start);
        ROMLOG_PRINT(" : the address is not alignment doubleword.\r\n");
        return SELFCHK_FAILED;
    }

    ret = SELFCHK_SUCCESS;
    offset = 0;

    /* 被测试单元全空间写 0 */
    while (offset < mem_size) {
        start = mem_start + offset;
        
        *((volatile u_int32_t *)start)= write_data0;

        if (is_stop_memery_selfchk() == TRUE) {
            return SELFCHK_CANCELLED;
        }

        memery_selfchk_watchdog(start, mem_watchdog);
        
        offset += LONG_WIDTH;
    }
    
    /* 从地址低位操作，读0，写为1，增加地址直到全空间都写为1 */
    ret = memery_selfchk_read_after_write(mem_start, 
        mem_size, BEGIN_LOW_ADDR_TYPE, write_data0, write_data1, mem_watchdog);
    if (ret == SELFCHK_CANCELLED) {
        return ret;
    } 
    
    /* 从地址低位操作，读1，写为0，增加地址直到全空间都写为0 */
    ret = memery_selfchk_read_after_write(mem_start, 
        mem_size, BEGIN_LOW_ADDR_TYPE, write_data1, write_data0, mem_watchdog);
    if (ret == SELFCHK_CANCELLED) {
        return ret;
    } 

    /* 从地址高位操作，读0，写为1，降低地址直到全空间都写为1 */
    ret = memery_selfchk_read_after_write(mem_start, 
        mem_size, BEGIN_HIGH_ADDR_TYPE, write_data0, write_data1, mem_watchdog);
    if (ret == SELFCHK_CANCELLED) {
        return ret;
    } 
    
    /* 从地址高位操作，读1，写为0，减低地址直到全空间都写为0 */
    ret = memery_selfchk_read_after_write(mem_start, 
        mem_size, BEGIN_HIGH_ADDR_TYPE, write_data1, write_data0, mem_watchdog);
    if (ret == SELFCHK_CANCELLED) {
        return ret;
    } 
    
    /* 从地址低位操作，读0，比较，增加地址直到比较完毕 */
    offset = 0;
    while (offset < mem_size) {
        start = mem_start + offset;
        
        read_buf = *((volatile u_int32_t *)start);
        if (read_buf != write_data0) {
            ret = SELFCHK_FAILED;
            memery_selfchk_print(start, write_data0, read_buf);
        }

        if (is_stop_memery_selfchk() == TRUE) {
            return SELFCHK_CANCELLED;
        }

        memery_selfchk_watchdog(start, mem_watchdog);
        
        offset += LONG_WIDTH;
    }
    
    return ret;
}


/**
 * memery_selfchk_march_g - March G算法
 * 
 * @mem_start: 内存自检起始地址
 * @mem_size: 内存自检大小
 * @write_data0: 进行内存自检的数据0
 * @write_data1: 进行内存自检的数据1
 * @mem_watchdog: 内存检测时的喂狗函数
 *
 * 用March G算法进行内存检测
 *
 * CTRL+C退出返回SELFCHK_CANCELLED，自检成功返回SELFCHK_SUCCESS，
 * 否则返回SELFCHK_FAILED
 */
static int memery_selfchk_march_g(u_int32_t mem_start, 
                                         u_int32_t mem_size,
                                         u_int32_t write_data0,
                                         u_int32_t write_data1,
                                         SELFCHK_WATCHDOG_FUNC mem_watchdog)
{
    int ret;
    int offset;
    u_int32_t start, end;
    u_int32_t read_buf;

    if ((mem_start & ADDRESS_MARK) != 0) {
        ROMLOG_PUTHEX(mem_start);
        ROMLOG_PRINT(" : the address is not alignment doubleword.\n");
        return SELFCHK_FAILED;
    }

    ret = SELFCHK_SUCCESS;
    offset = 0;

    /* 被测试单元全空间写 0 */
    while (offset < mem_size) {
        start = mem_start + offset;
        *((volatile u_int32_t *)start) = write_data0;

        if (is_stop_memery_selfchk() == TRUE) {
            return SELFCHK_CANCELLED;
        }
        
        memery_selfchk_watchdog(start, mem_watchdog);
        
        offset += LONG_WIDTH;
    }
    
    /* 从地址低位操作，读0，写为1，读取1，再写0，读取0，再写1。增加地址直到全空间都写为1 */
    offset = 0;
    while (offset < mem_size) {
        start = mem_start + offset;
        read_buf = *((volatile u_int32_t *)start);
        if (read_buf != write_data0) {
            ret = SELFCHK_FAILED;
            memery_selfchk_print(start, write_data0, read_buf);
        }
        
        *((volatile u_int32_t *)start) = write_data1;
        read_buf = *((volatile u_int32_t *)start);
        if (read_buf != write_data1) {
            ret = SELFCHK_FAILED;
            memery_selfchk_print(start, write_data1, read_buf);
        }

        *((volatile u_int32_t *)start) = write_data0;
        read_buf = *((volatile u_int32_t *)start);
        if (read_buf != write_data0) {
            ret = SELFCHK_FAILED;
            memery_selfchk_print(start, write_data0, read_buf);
        }
        
        *((volatile u_int32_t *)start) = write_data1;

        if (is_stop_memery_selfchk() == TRUE) {
            return SELFCHK_CANCELLED;
        }
        
        memery_selfchk_watchdog(start, mem_watchdog);
        
        offset += LONG_WIDTH;
    }

    /* 从地址低位操作，读1，写0，再写1，增加地址直到全空间都写为1 */
    offset = 0;
    while (offset < mem_size) {
        start = mem_start + offset;
        
        read_buf = *((volatile u_int32_t *)start);
        if (read_buf != write_data1) {
            ret = SELFCHK_FAILED;
            memery_selfchk_print(start, write_data1, read_buf);
        }
        
        *((volatile u_int32_t *)start) = write_data0;

        *((volatile u_int32_t *)start) = write_data1;

        if (is_stop_memery_selfchk() == TRUE) {
            return SELFCHK_CANCELLED;
        }

        memery_selfchk_watchdog(start, mem_watchdog);
        
        offset += LONG_WIDTH;
    }
    
    /* 从地址高位操作，读1，写0，写1，再写0，降低地址直到全空间都写为0 */
    offset = mem_size - LONG_WIDTH;
    while (offset >= 0) {
        end = mem_start + offset;
        
        read_buf = *((volatile u_int32_t *)end);
        if (read_buf != write_data1) {
            ret = SELFCHK_FAILED;
            memery_selfchk_print(end, write_data1, read_buf);
        }
        
        *((volatile u_int32_t *)end) = write_data0;
        
        *((volatile u_int32_t *)end) = write_data1;
        
        *((volatile u_int32_t *)end) = write_data0;

        if (is_stop_memery_selfchk() == TRUE) {
            return SELFCHK_CANCELLED;
        }
        
        memery_selfchk_watchdog(end, mem_watchdog);

        offset -= LONG_WIDTH;
    }
    
    /* 从地址高位操作，读0，写1，再写0，减低地址直到全空间都写为0 */
    offset = mem_size - LONG_WIDTH;
    while (offset >= 0) {
        end = mem_start + offset;
        
        read_buf = *((volatile u_int32_t *)end);
        if (read_buf != write_data0) {
            ret = SELFCHK_FAILED;
            memery_selfchk_print(end, write_data0, read_buf);
        }
        
        *((volatile u_int32_t *)end) = write_data1;
                
        *((volatile u_int32_t *)end) = write_data0;

        if (is_stop_memery_selfchk() == TRUE) {
            return SELFCHK_CANCELLED;
        }

        memery_selfchk_watchdog(end, mem_watchdog);

        offset -= LONG_WIDTH;
    }
    
    /* 从地址高位操作，读0，比较，增加地址直到比较完毕 */
    offset = mem_size - LONG_WIDTH;
    while (offset >= 0) {
        end = mem_start + offset;
        
        read_buf = *((volatile u_int32_t *)end);
        if (read_buf != write_data0) {
            ret = SELFCHK_FAILED;
            memery_selfchk_print(end, write_data0, read_buf);
        }

        if (is_stop_memery_selfchk() == TRUE) {
            return SELFCHK_CANCELLED;
        }

        memery_selfchk_watchdog(end, mem_watchdog);

        offset -= LONG_WIDTH;
    }

    return ret;
}


/**
 * Galloping算法 - Galloping算法
 * 
 * @mem_start: 内存自检起始地址
 * @mem_size: 内存自检大小
 * @write_data0: 进行内存自检的数据0
 * @write_data1: 进行内存自检的数据1
 * @mem_watchdog: 内存检测时的喂狗函数
 *
 * 用Galloping算法进行内存检测
 *
 * CTRL+C退出返回SELFCHK_CANCELLED，自检成功返回SELFCHK_SUCCESS，
 * 否则返回SELFCHK_FAILED
 */
static int memery_selfchk_galloping(u_int32_t mem_start, 
                                    u_int32_t mem_size, 
                                    u_int32_t write_data0,
                                    u_int32_t write_data1,
                                    SELFCHK_WATCHDOG_FUNC mem_watchdog)
{
    int ret;
    int offset;
    u_int32_t start, end, tmp_addr;
    u_int32_t read_buf;

    if ((mem_start & ADDRESS_MARK) != 0) {
        ROMLOG_PUTHEX(mem_start);
        ROMLOG_PRINT(" : the address is not alignment doubleword.\r\n");
        return SELFCHK_FAILED;
    }

    ret = SELFCHK_SUCCESS;
    offset = 0;

    /* 被测试单元全空间写 0 */
    while (offset < mem_size) {
        start = mem_start + offset;
        *((volatile u_int32_t *)start) = write_data0;

        if (is_stop_memery_selfchk() == TRUE) {
            return SELFCHK_CANCELLED;
        }

        memery_selfchk_watchdog(start, mem_watchdog);
        
        offset += LONG_WIDTH;
    }
    
    /* 第一个单元写1（基本单元），读取第二个单元, 然后返回来读取第一个单元。再对第一个
     * 单元写0。（第一个单元作为基本单元的操作完成之后，再把第二个单元作为基本单元） 
     */
    offset = 0;
    while (offset < mem_size - LONG_WIDTH) {
        start = mem_start + offset;
        
        *((volatile u_int32_t *)start) = write_data1;

        tmp_addr = start + LONG_WIDTH;

        read_buf = *((volatile u_int32_t *)tmp_addr);
        if (read_buf != write_data0) {
            ret = SELFCHK_FAILED;
            memery_selfchk_print(tmp_addr, write_data0, read_buf);
        }

        read_buf = *((volatile u_int32_t *)start);
        if (read_buf != write_data1) {
            ret = SELFCHK_FAILED;
            memery_selfchk_print(start, write_data1, read_buf);
        }

       *((volatile u_int32_t *)start) = write_data0;

        if (is_stop_memery_selfchk() == TRUE) {
            return SELFCHK_CANCELLED;
        }

        memery_selfchk_watchdog(start, mem_watchdog);
        
        offset += LONG_WIDTH;
    }

    /* 所有单元写0，进行一次完整的比较 */
    offset = 0;
    while (offset < mem_size) {
        start = mem_start + offset;

        read_buf = *((volatile u_int32_t *)start);
        if (read_buf != write_data0) {
            ret = SELFCHK_FAILED;
            memery_selfchk_print(start, write_data0, read_buf);
        }

        if (is_stop_memery_selfchk() == TRUE) {
            return SELFCHK_CANCELLED;
        }

        memery_selfchk_watchdog(start, mem_watchdog);
        
        offset += LONG_WIDTH;
    }
    
    /* 从高位地址操作，把最高位单元B写1，读取次高位地址B-1，然后返回写高地址B为
     * 0，与之前的反向操作，直到所有的地址都写0，再进行一次完整的比较 
     */
    offset = mem_size - LONG_WIDTH;
    while (offset > 0) {
        end = mem_start + offset;
        
        *((volatile u_int32_t *)end) = write_data1;

        tmp_addr = end - LONG_WIDTH;

        read_buf = *((volatile u_int32_t *)tmp_addr);
        if (read_buf != write_data0) {
            ret = SELFCHK_FAILED;
            memery_selfchk_print(tmp_addr, write_data0, read_buf);
        }

        read_buf = *((volatile u_int32_t *)end);
        if (read_buf != write_data1) {
            ret = SELFCHK_FAILED;
            memery_selfchk_print(end, write_data1, read_buf);
        }

       *((volatile u_int32_t *)end) = write_data0;

        if (is_stop_memery_selfchk() == TRUE) {
            return SELFCHK_CANCELLED;
        }

        memery_selfchk_watchdog(end, mem_watchdog);

        offset -= LONG_WIDTH;
    }

    /* 所有单元写0，进行一次完整的比较 */
    offset = 0;
    while (offset < mem_size) {
        start = mem_start + offset;
        
        read_buf = *((volatile u_int32_t *)start);
        if (read_buf != write_data0) {
            ret = SELFCHK_FAILED;
            memery_selfchk_print(start, write_data0, read_buf);
        }

        if (is_stop_memery_selfchk() == TRUE) {
            return SELFCHK_CANCELLED;
        }
        
        memery_selfchk_watchdog(start, mem_watchdog);
        
        offset += LONG_WIDTH;
    }
    
    return ret;
}


static int memery_selfchk_block_switch(u_int32_t block1_start,
                                              u_int32_t block2_start,
                                              u_int32_t sw_block_start,
                                              u_int32_t block_size,
                                              SELFCHK_WATCHDOG_FUNC mem_watchdog)
{
    int i;
    u_int32_t block1_addr, block2_addr, sw_addr;

    block1_addr = block1_start;
    block2_addr = block2_start;
    sw_addr = sw_block_start;

    for (i = 0; i < block_size / LONG_WIDTH; i++) {
        sw_addr = sw_block_start + i * LONG_WIDTH;
        block1_addr = block1_start + i * LONG_WIDTH;
        block2_addr = block2_start + i * LONG_WIDTH;
        
        *((volatile u_int32_t *)sw_addr) = *((volatile u_int32_t *)block1_addr);
        *((volatile u_int32_t *)block1_addr) = *((volatile u_int32_t *)block2_addr);
        *((volatile u_int32_t *)block2_addr) = *((volatile u_int32_t *)sw_addr);

        memery_selfchk_watchdog(sw_addr, mem_watchdog);
        
        if (is_stop_memery_selfchk() == TRUE) {
            return SELFCHK_CANCELLED;
        }
    }

    return SELFCHK_SUCCESS;    
}


/**
 * memery_selfchk_move_block - move block算法
 * 
 * @start: 内存自检起始地址
 * @size: 内存自检大小
 * @mem_watchdog: 内存检测时的喂狗函数
 *
 * 用move block算法进行内存检测
 *
 * CTRL+C退出返回SELFCHK_CANCELLED，自检成功返回SELFCHK_SUCCESS，
 * 否则返回SELFCHK_FAILED
 */
static int memery_selfchk_move_block(u_int32_t mem_start,
                                     u_int32_t mem_size,
                                     u_int32_t block_size,
                                     SELFCHK_WATCHDOG_FUNC mem_watchdog)
{
    int i, j, ret;
    int block_num, n;
    int offset;
    u_int32_t block1_addr, block2_addr, last_block;
    u_int32_t start;
    u_int32_t data_buf, read_buf;

    if ((mem_start & ADDRESS_MARK) != 0) {
        ROMLOG_PUTHEX(mem_start);
        ROMLOG_PRINT(" : the address is not alignment doubleword.\r\n");
        return SELFCHK_FAILED;
    }

    if ((block_size & BLOCK_LEVEL_SIZE_MARK) != 0) {
        ROMLOG_PRINT("The block size ");
        ROMLOG_PUTHEX(block_size);
        ROMLOG_PRINT(" is not alignment ");
        ROMLOG_PUTHEX(BLOCK_LEVEL_SIZE);
        ROMLOG_PRINT(".\r\n");
        return SELFCHK_FAILED;
    }

    ret = SELFCHK_SUCCESS;
    block_num = mem_size / block_size;
    n = block_num - 1;
    last_block = mem_start + block_size * (block_num - 1);

    /* 对应内存地址填充它的地址值 */
	offset = 0;
    while (offset < mem_size) {
        start = mem_start + offset;
        
        data_buf = start;
        *((volatile u_int32_t *)start) = data_buf;
        
        if (is_stop_memery_selfchk() == TRUE) {
            return SELFCHK_CANCELLED;
        }
        
        memery_selfchk_watchdog(start, mem_watchdog);
        
        offset += LONG_WIDTH;
    }
    
    /* 除程序运行空间之外确定一个测试空间A~B，A为低位地址，B为高位地址。
     * 从A~B分为n+1个空间，n空间为数据空间，第n+1空间为交换空间。"<－>"表示对应的数据块通
     * 过交换空间互换，如下:
	 * 1<－>2，1<－>3，1<－>4…1<－>n
	 * 2<－>3，2<－>4，2<－>5…2<－>n
	 * .
	 * .
	 * n-1<－>n
	 */
    for (i = 0; i < n - 1; i++) {
        block1_addr = mem_start + block_size * i;
        
        for (j = i + 1; j < n; j++) {
            memery_selfchk_watchdog(0, mem_watchdog);
            block2_addr = mem_start + block_size * j;
            ret = memery_selfchk_block_switch(block1_addr, block2_addr, last_block, block_size, mem_watchdog);
            if (ret == SELFCHK_CANCELLED) {
                return ret;
            }
        }
    }
    
    /* n-1<－>n，正向交换结束。
	 * n<－>n-1，n<－>n-2，n<－>n-3，n<－>n-4…n<－>1。
	 * n-1<－>n-2，n-1<－>n-3，n-1<－>n-4，n-1<－>n-5，…n-1<－>1。
	 * n-2<－>n-3，n-2<－>n-4，n-2<－>n-5，n-2<－>n-6，…n-2<－>1。
	 * .
	 * .
	 * 2<－>1。反向交换结束。目前数据和交换前的数据理论计算一样。
	 */
    for (i = n - 1; i > 0; i--) {
        block1_addr = mem_start + block_size * i;
        
        for (j = i - 1; j >= 0; j--) {
            block2_addr = mem_start + block_size * j;
            ret = memery_selfchk_block_switch(block1_addr, block2_addr, last_block, block_size, mem_watchdog);
            if (ret == SELFCHK_CANCELLED) {
                return ret;
            }
        }
        
        memery_selfchk_watchdog(block1_addr, mem_watchdog);
    }

    
    /* 与原来的数据进行比较 */
    offset = 0;
    while (offset < mem_size - block_size) {
        start = mem_start + offset;
        
        data_buf = start;
        read_buf = *((volatile u_int32_t *)start);
        
        if (read_buf != data_buf) {
            ret = SELFCHK_FAILED;
            memery_selfchk_print(start, data_buf, read_buf);
        }
        
        if (is_stop_memery_selfchk() == TRUE) {
            return SELFCHK_CANCELLED;
        }
        
        memery_selfchk_watchdog(start, mem_watchdog);

        offset += LONG_WIDTH;
    }
    
    return ret;
}

static void memery_selfchk_test(u_int32_t start, u_int32_t size, SELFCHK_WATCHDOG_FUNC mem_watchdog)
{
    int i;
    int key;
    int ret;
    u_int32_t mem_data;

begin:
    ROMLOG_PRINT("\r\n******************************************* \r\n");
    ROMLOG_PRINT("0. Moving Inversions, Ones&Zeros\r\n");

    ROMLOG_PRINT("1. Moving Inversions, 8 Bit Pat\r\n");

    ROMLOG_PRINT("2. Moving Inversions, 32 Bit Pat\r\n");

    ROMLOG_PRINT("3. Modulo 20, Ones&Zeros\r\n");

    ROMLOG_PRINT("4. Modulo 20, 8 Bit Pat\r\n");

    ROMLOG_PRINT("5. Mats\r\n");

    ROMLOG_PRINT("6. March C\r\n");

    ROMLOG_PRINT("7. March G\r\n");

    ROMLOG_PRINT("8. Galloping\r\n");

    ROMLOG_PRINT("9. Move Block\r\n");
   
    ROMLOG_PRINT("******************************************* \r\n");
    ROMLOG_PRINT("Press a key to choose check method: ");
    key = '0';
    while (1) {
//        key = wait_key_cmd();
        ROMLOG_PRINT("\r\nBegin memery check\r\n");
        if (key == '0' ) {
            ROMLOG_PRINT("Moving Inversions, Ones&Zeros, Please Wait...\r\n");
            mem_data = 0xFFFFFFFF;
            ret = memery_selfchk_moving_inversions(start, 
                                                   size, 
                                                   mem_data, 
                                                   LONG_WIDTH,
                                                   mem_watchdog);

//            break;
        } else if (key == '1') {
            /* 数据格式为0x80808080，0x40404040，0x20202020，0x10101010，0x08080808，
             * 0x04040404，0x02020202，0x01010101 
             */
            ROMLOG_PRINT("Moving Inversions, 8 Bit Pat, Please Wait...\r\n");
            for (i = 0; i < 8; i++) {
                 mem_data = 0x80808080 >> i;
                 ret = memery_selfchk_moving_inversions(start, 
                                                        size, 
                                                        mem_data, 
                                                        LONG_WIDTH,
                                                        mem_watchdog);
                
                if (ret == SELFCHK_CANCELLED) {
                    goto stop;
                }
            }

            /* 反码 */
            for (i = 0; i < 8; i++) {
                 mem_data = 0x80808080 >> i;
                 ret = memery_selfchk_moving_inversions(start, 
                                                        size, 
                                                        ~mem_data, 
                                                        LONG_WIDTH,
                                                        mem_watchdog);
                
                if (ret ==SELFCHK_CANCELLED) {
                    goto stop;
                }
            }
//            break;
        } else if (key == '2') {
            ROMLOG_PRINT("Moving Inversions, 32 Bit Pat, Please Wait...\r\n");
            for (i = 0; i < 32; i++) {
                 mem_data = 0x00000001 << i;
                 ret = memery_selfchk_moving_inversions(start, 
                                                        size, 
                                                        mem_data, 
                                                        LONG_WIDTH,
                                                        mem_watchdog);
                
                if (ret == SELFCHK_CANCELLED) {
                    goto stop;
                }
            }
//            break;
        } else if (key == '3'){
            ROMLOG_PRINT("Modulo 20, Ones&Zeros, Please Wait...\r\n");
            mem_data = 0xFFFFFFFF;
			/* Modulo-X算法 */
            ret = memery_selfchk_modulo_x(start, size, mem_data, mem_watchdog);

//            break;;
        } else if (key == '4'){
            ROMLOG_PRINT("Modulo 20, 8 Bit Pat, Please Wait...\r\n");

            for (i = 0; i < 8; i++) {
                mem_data = 0x80808080 >> i;
                ret = memery_selfchk_modulo_x(start, size, mem_data, mem_watchdog);
                
                if (ret == SELFCHK_CANCELLED) {
                    goto stop;
                }
            }

            for (i = 0; i < 8; i++) {
                mem_data = 0x80808080 >> i;
                ret = memery_selfchk_modulo_x(start, size, ~mem_data, mem_watchdog);
                
                if (ret == SELFCHK_CANCELLED) {
                    goto stop;
                }
            }
//            break;;
        } else if (key == '5'){
            ROMLOG_PRINT("Mats, Please Wait...\r\n");
            
            mem_data = 0xF0F0F0F0;
            ret = memery_selfchk_mats(start, size, mem_data, ~mem_data, mem_watchdog);
            
//            break;;
        }  else if (key == '6'){
            ROMLOG_PRINT("March C, Please Wait...\r\n");
            
            mem_data = 0x0F0F0F0F;
            ret = memery_selfchk_march_c(start, size, mem_data, ~mem_data, mem_watchdog);
            
//            break;;
        } else if (key == '7'){
            ROMLOG_PRINT("March G, Please Wait...\r\n");
            
            mem_data = 0xF0F0F0F0;
            ret = memery_selfchk_march_g(start, size, mem_data, ~mem_data, mem_watchdog);
            
//            break;;
        } else if (key == '8' ) {
            ROMLOG_PRINT("Galloping, Please Wait...\r\n");
            
            mem_data = 0x0F0F0F0F;
            ret = memery_selfchk_galloping(start, size, mem_data, ~mem_data, mem_watchdog);

//            break;;
        } else if (key == '9' ) {
            ROMLOG_PRINT("Move Block, Please Wait...\r\n");
            ret = memery_selfchk_move_block(start, size, 16 * BLOCK_LEVEL_SIZE, mem_watchdog);
            
//            break;;
        }
        switch (ret) {
    case SELFCHK_FAILED:
        ROMLOG_PRINT("Failed to check memery.\r\n");
        break;
    case SELFCHK_SUCCESS:
        ROMLOG_PRINT("Success to check memery.\r\n");
        break;
    case SELFCHK_CANCELLED:
        ROMLOG_PRINT("Stop to check memery.\r\n");
        break;
    default:
        break;
    }
        if (key == '7'){
			key = '0';
		}
		key++;
    }
    
stop:
    switch (ret) {
    case SELFCHK_FAILED:
        ROMLOG_PRINT("Failed to check memery.\r\n");
        break;
    case SELFCHK_SUCCESS:
        ROMLOG_PRINT("Success to check memery.\r\n");
        break;
    case SELFCHK_CANCELLED:
        ROMLOG_PRINT("Stop to check memery.\r\n");
        break;
    default:
        break;
    }

    goto begin;
}

int memory_selfchk(int ac, char **av)
{
	u_int32_t start, eaddr, size;
	
//	start = heaptop;
	start = 0xa0100000;
#if defined(__mips__)
//	eaddr = (memorysize & 0x3fffffff) + ((int)heaptop & 0xc0000000);
	eaddr = 0xa2000000;
#else
	eaddr = memorysize;
#endif
	
	/* 起始地址和结束地址都设置为4字节对齐 */
	start = start & ~0x3;
	eaddr &= ~0x3;
	size = size & (~ADDRESS_MARK);
	
//	/* 地址和大小都设置为4字节对齐 */
//	start = (start + ADDRESS_MARK) & (~ADDRESS_MARK);
//	size = size & (~ADDRESS_MARK);

	ppc_cache_enable();

	memery_selfchk_test(start, size, memery_selfcheck_watchdog);

	return 0;
}
