/*
 * Copyright:
 * ----------------------------------------------------------------
 * This confidential and proprietary software may be used only as
 * authorised by a licensing agreement from ARM Limited
 *   (C) COPYRIGHT 2003,2004 ARM Limited
 *       ALL RIGHTS RESERVED
 * The entire notice above must be reproduced on all authorised
 * copies and copies may only be made to the extent permitted
 * by a licensing agreement from ARM Limited.
 * ----------------------------------------------------------------
 * File:     common.c
 * Release:  Version 2.0
 * ----------------------------------------------------------------
 *
 *  ----------------------------------------
 *  Version and Release Control Information:
 *
 *  File Name              : $RCSfile: common.c,v $
 *  File Revision          : $Revision: 1.1 $
 *
 *  ----------------------------------------
 */

/*
 * Code implementation file for the common functions.
 */

#include <stdio.h>
#include "common.h"

#ifndef udelay
#define udelay delay
#endif

#ifndef MAX_UDELAY_MS
#define MAX_UDELAY_MS   500
#endif

#ifndef mdelay
#define mdelay(n) (\
        (__builtin_constant_p(n) && (n)<=MAX_UDELAY_MS) ? udelay((n)*1000) : \
        ({unsigned long __ms=(n); while (__ms--) udelay(1000);}))
#endif


// Declaration of the global used to indicate AutoRun mode
unsigned int RunAllTests;
unsigned int CMType;

#if 0

// Return whether we're on a CT7TDMI
int ct7tdmi(void)
{
    volatile unsigned int *proc_id;

    proc_id = (volatile unsigned int *)0x10000084;
    if((*proc_id >> 24) == 0)
        return 1;
    else
        return 0;
}

void turn_icache_on(void)
{
    unsigned int cache_bits;

    if(ct7tdmi())
        return;

    // Turning the I cache on
    __asm {
        MRC p15, 0, cache_bits, c1, c0, 0
    }

    cache_bits |=  (1 << 12);       // [12] for I cache

    __asm {
        MCR p15, 0, cache_bits, c1, c0, 0
    }
}

void turn_icache_off(void)
{
    unsigned int cache_bits = 0;

    if(ct7tdmi())
        return;

    // Turning the I cache back to off
    __asm {
        MRC p15, 0, cache_bits, c1, c0, 0
    }

    cache_bits &= ~(1 << 12);       // [12] for I cache

    __asm {
        MCR p15, 0, cache_bits, c1, c0, 0
    }

}
void timer_start(const unsigned int duration, const enum Time_Units t_unit)
{
    int total_usec = 0;
    const unsigned int units[3] = { 1,1000,1000000 };   // Just a LUT

    // Calculate the total duration in usecs
    total_usec = units[t_unit] * duration;

    *TIMER1_CTRL = 0x00;            // Stop timer
    *TIMER1_LOAD = total_usec;      // Assign timer duration
    *TIMER1_CTRL = 0x82;            // Start timer in 32-bit mode
    return;
}

// Tell whether timer has expired
int timeout(const unsigned int duration, const enum Time_Units t_unit)
{
    int total_usec = 0;
    const unsigned int units[3] = { 1,1000,1000000 };

    //debug("%-25s%u\n","Total duration in usec:",duration);
    //ebug("%-25s%s\n","Duration unit:",
    //((t_unit == 2) ? "Seconds" : ((t_unit == 1 ) ?"Milliseconds" : "Microseconds")));
    //debug("%-25s%u\n","Current timer value:",*TIMER1_VALUE);

    total_usec = units[t_unit] * duration;

    if(*TIMER1_VALUE <= total_usec) // Hasn't rolled over yet
        return 0;
    else
        return 1;
}
#endif

/* Sleep function to delay n*mS
 * Uses Timer1 which is a 16bit down counter that
 * is clocked at 1MHz/PRESCALE. Prescale is 1 so
 * etimer is reached every mS. The msec value must
 * be in the range: 2^32 > msec > 0.
 * To ensure the timer is re-loaded before the first
 * comparison it is necessary to read and compare the
 * timer value before starting.
 */
//extern unsigned int my_cycle_count;
unsigned int my_cycle_count=0;
void apSleep(unsigned int msec)
{
#if 0
    unsigned int utimer, etimer, ttimer;

    utimer       = 0;
    etimer       = 0xFFFF - 1000;       // 1MHz clock ticks 1000 times/msec.
                                        // 32KHz clock ticks 32 times/msec.
    ttimer       = SLEEP_TIMEOUT;

    *TIMER0_CTRL = 0x00;
    *TIMER0_LOAD = 0xFFFF;

    // Wait for timer to be loaded
    while((*TIMER0_LOAD != 0xFFFF) && ttimer)
        ttimer--;
    // Wait for timer to count down
    *TIMER0_CTRL = 0x80;                // Enable the timer
    do {
        if ((*TIMER0_VALUE & 0xFFFF) < etimer) {
            *TIMER0_LOAD = 0xFFFF;
            utimer++;
        }
    } while(utimer < msec);
#else
//zgj    mdelay(msec);
    unsigned int i=0;
    unsigned int j=0;
    unsigned int t=0;

        if(my_cycle_count != 0)
            t=my_cycle_count;
        else
            t=20;
    while(i < msec )
    {
        j=t;
        while(j--);
        i++;
    }
#endif
}

#if 0

/* Sleep like function to delay n*uS
 * Unlike apSleep the delay value is has a fairly limited
 * range and must be in the range: 65536 (65.5mS) > usec > 0
 */
void apSleepus(unsigned int usec)
{
    unsigned int etimer, ttimer;

    if (usec > 0xFFFF)
        usec = 0xFFFF;
    etimer       = 0xFFFF - usec;
    ttimer       = SLEEP_TIMEOUT;
    *TIMER0_CTRL = 0x00;
    *TIMER0_LOAD = 0xFFFF;

    // Wait for timer to be loaded
    while((*TIMER0_LOAD != 0xFFFF) && ttimer)
        ttimer--;

    // Wait for timer to count down
    *TIMER0_CTRL = 0x80;
    while((*TIMER0_VALUE & 0xFFFF) > etimer)
        continue;
}

/* Register bit(s) test */
unsigned int register_test(volatile unsigned int *addr,int firstbit, int lastbit)
{
    int n;
    unsigned int origval;

    origval = *addr;

    for (n=0;n<32;n++) {
        if (n >= firstbit && n <= lastbit) {
            *addr=1 << n;
            apSleepus(10);
            if (!(*addr & (1 << n)))
                return ((1 << 5) | n);
        }
    }

    for (n=0;n<32;n++) {
        if (n >= firstbit && n <= lastbit) {
            *addr = 0xffffffff ^ (1 <<n);
            apSleepus(10);
            if (*addr & (1 <<n))
                return ((2<<5)|n);
        }
    }
    *addr = origval;
    return 0;
}


boardtype_t detect_board(void)
{
    volatile unsigned int * sysid;
    sysid = (volatile unsigned int *)SYS_BASE;

    // Detect if fitted to AB/PB926EJ-S or EB
    if((*sysid & 0x0FFF0000) == ARM_CMID_EB) {

        return board_eb;
    }
    else if((*sysid & 0xF000) == ARM_CMID_PB) {

        return board_pb926;
    }
    else if((*sysid & 0xF000) == ARM_CMID_AB) {

        return board_ab926;
    }
    else {

        return board_unknown;
    }
}



void Wait_For_Enter(int always)
{
    int c=0;

    if (!RunAllTests || always) {
        printf ("Press Enter to continue...");

        do
        {
            fflush(stdin);
            c = getchar ();
        }
        while ((c != '\n') && (c != EOF));
    }
    printf ("Running...\n");
}

// Ask user for the result of the test
int Get_OK(void)
{
	char c;

    c = getchar();

    getchar();

    if ( (c == 'y') || (c == 'Y') )
        return TRUE;
    else
        return FALSE;
}
#endif
