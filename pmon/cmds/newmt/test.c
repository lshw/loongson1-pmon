/* test.c - MemTest-86  Version 3.2
 *
 * Released under version 2 of the Gnu Public License.
 * By Chris Brady
 */

#include "test.h"
#include "fb.h"

static void update_err_counts(void);
static void print_err_counts(void);

int segs;

static inline ulong roundup(ulong value, ulong mask)
{
	return (value + mask) & ~mask;
}



void poll_errors(void)
{
}



/*
 * Memory address test, walking ones
 */
void addr_tst1()
{
	int i, j, k;
	volatile ulong *pt;
	volatile ulong *end;
	ulong bad, mask, bank;

	/* Test the global address bits */
	for (p1=0, j=0; j<2; j++) {
        	hprint(LINE_PAT, COL_PAT, p1);

		/* Set pattern in our lowest multiple of 0x20000 */
		p = (ulong *)roundup((ulong)v->map[0].start, 0x1ffff);
		*p = p1;
	
		/* Now write pattern compliment */
		p1 = ~p1;
		end = v->map[segs-1].end;
		for (i=0; i<1000; i++) {
			mask = 4;
			do {
				pt = (ulong *)((ulong)p | mask);
				if (pt == p) {
					mask = mask << 1;
					continue;
				}
				if (pt >= end) {
					break;
				}
				*pt = p1;
				if ((bad = *p) != ~p1) {
					ad_err1((ulong *)p, (ulong *)mask,
						bad, ~p1);
					i = 1000;
				}
				mask = mask << 1;
			} while(mask);
		}
		do_tick();
		BAILR
	}

	/* Now check the address bits in each bank */
	/* If we have more than 8mb of memory then the bank size must be */
	/* bigger than 256k.  If so use 1mb for the bank size. */
	if (v->pmap[v->msegs - 1].end > (0x800000 >> 12)) {
		bank = 0x100000;
	} else {
		bank = 0x40000;
	}
	for (p1=0, k=0; k<2; k++) {
        	hprint(LINE_PAT, COL_PAT, p1);

		for (j=0; j<segs; j++) {
			p = v->map[j].start;
			/* Force start address to be a multiple of 256k */
			p = (ulong *)roundup((ulong)p, bank - 1);
			end = v->map[j].end;
			while (p < end) {
				*p = p1;

				p1 = ~p1;
				for (i=0; i<200; i++) {
					mask = 4;
					do {
						pt = (ulong *)
						    ((ulong)p | mask);
						if (pt == p) {
							mask = mask << 1;
							continue;
						}
						if (pt >= end) {
							break;
						}
						*pt = p1;
						if ((bad = *p) != ~p1) {
							ad_err1((ulong *)p,
							    (ulong *)mask,
							    bad,~p1);
							i = 200;
						}
						mask = mask << 1;
					} while(mask);
				}
				if (p + bank > p) {
					p += bank;
				} else {
					p = end;
				}
				p1 = ~p1;
			}
		}
		do_tick();
		BAILR
		p1 = ~p1;
	}
}

/*
 * Memory address test, own address
 */
void addr_tst2()
{
	int j, done;
	volatile ulong *pe;
	volatile ulong *end, *start;

        cprint(LINE_PAT, COL_PAT, "        ");

	/* Write each address with it's own address */
	for (j=0; j<segs; j++) {
		start = v->map[j].start;
		end = v->map[j].end;
		pe = (ulong *)start;
		p = start;
		done = 0;
		do {
			/* Check for overflow */
			if (pe + SPINSZ > pe) {
				pe += SPINSZ;
			} else {
				pe = end;
			}
			if (pe >= end) {
				pe = end;
				done++;
			}
			if (p == pe ) {
				break;
			}

 			for (; p < pe; p++) {
 				*p = (ulong)p;
 			}
 
			do_tick();
			BAILR
		} while (!done);
	}

	/* Each address should have its own address */
	for (j=0; j<segs; j++) {
		start = v->map[j].start;
		end = v->map[j].end;
		pe = (ulong *)start;
		p = start;
		done = 0;
		do {
			/* Check for overflow */
			if (pe + SPINSZ > pe) {
				pe += SPINSZ;
			} else {
				pe = end;
			}
			if (pe >= end) {
				pe = end;
				done++;
			}
			if (p == pe ) {
				break;
			}
{
ulong bad;
			for (; p < pe; p++) {
 				if((bad = *p) != (ulong)p) {
 					ad_err2((ulong)p, bad);
 				}
 			}
 }
			do_tick();
			BAILR
		} while (!done);
	}
}

/*
 * Test all of memory using a "moving inversions" algorithm using the
 * pattern in p1 and it's complement in p2.
 */
void movinv1(int iter, ulong p1, ulong p2)
{
	int i, j, done;
	volatile ulong *pe;
	volatile ulong len;
	volatile ulong *start,*end;

	/* Display the current pattern */
        hprint(LINE_PAT, COL_PAT, p1);

	/* Initialize memory with the initial pattern.  */
	for (j=0; j<segs; j++) {
		start = v->map[j].start;
		end = v->map[j].end;
		pe = start;
		p = start;
		done = 0;
		do {
			/* Check for overflow */
			if (pe + SPINSZ > pe) {
				pe += SPINSZ;
			} else {
				pe = end;
			}
			if (pe >= end) {
				pe = end;
				done++;
			}
			len = pe - p;
			if (p == pe ) {
				break;
			}
 			for (; p < pe; p++) {
 				*p = p1;
 			}
 
			do_tick();
			BAILR
		} while (!done);
	}

	/* Do moving inversions test. Check for initial pattern and then
	 * write the complement for each memory location. Test from bottom
	 * up and then from the top down.  */
	for (i=0; i<iter; i++) {
		for (j=0; j<segs; j++) {
			start = v->map[j].start;
			end = v->map[j].end;
			pe = start;
			p = start;
			done = 0;
			do {
				/* Check for overflow */
				if (pe + SPINSZ > pe) {
					pe += SPINSZ;
				} else {
					pe = end;
				}
				if (pe >= end) {
					pe = end;
					done++;
				}
				if (p == pe ) {
					break;
				}
{
ulong bad;
 				for (; p < pe; p++) {
 					if ((bad=*p) != p1) {
 						error((ulong*)p, p1, bad);
 					}
 					*p = p2;
				}
 
}
				do_tick();
				BAILR
			} while (!done);
		}
		for (j=segs-1; j>=0; j--) {
			start = v->map[j].start;
			end = v->map[j].end;
			pe = end -1;
			p = end -1;
			done = 0;
			do {
				/* Check for underflow */
				if (pe - SPINSZ < pe) {
					pe -= SPINSZ;
				} else {
					pe = start;
				}
				if (pe <= start) {
					pe = start;
					done++;
				}
				if (p == pe ) {
					break;
				}
{
ulong bad;
 				do {
 					if ((bad=*p) != p2) {
 						error((ulong*)p, p2, bad);
 					}
 					*p = p1;
 				} while (p-- > pe);
 
}
	 	do_tick();
				BAILR
			} while (!done);
		}
	}
}

/*
 * Test all of memory using a "half moving inversions" algorithm using random
 * numbers and their complment as the data pattern. Since we are not able to
 * produce random numbers in reverse order testing is only done in the forward
 * direction.
 */
void movinvr()
{
	int i, j, done, seed1, seed2;
	volatile ulong *pe;
	volatile ulong *start,*end;
	ulong num;

	/* Initialize memory with initial sequence of random numbers.  */
	if (v->rdtsc) {
		//asm __volatile__ ("rdtsc":"=a" (seed1),"=d" (seed2));
	    seed1 = 521288629 +CPU_GetCOUNT();
		seed2 = 362436069 -CPU_GetCOUNT();
	} else {
		seed1 = 521288629 + v->pass;
		seed2 = 362436069 - v->pass;
	}

	/* Display the current seed */
        hprint(LINE_PAT, COL_PAT, seed1);
	Rand_seed(seed1,seed2);
	for (j=0; j<segs; j++) {
		start = v->map[j].start;
		end = v->map[j].end;
		pe = start;
		p = start;
		done = 0;
		do {
			/* Check for overflow */
			if (pe + SPINSZ > pe) {
				pe += SPINSZ;
			} else {
				pe = end;
			}
			if (pe >= end) {
				pe = end;
				done++;
			}
			if (p == pe ) {
				break;
			}

			for (; p < pe; p++) {
				*p = Rand();
			}


			do_tick();
			BAILR
		} while (!done);
	}

	/* Do moving inversions test. Check for initial pattern and then
	 * write the complement for each memory location. Test from bottom
	 * up and then from the top down.  */
	for (i=0; i<2; i++) {
		Rand_seed(seed1,seed2);
		for (j=0; j<segs; j++) {
			start = v->map[j].start;
			end = v->map[j].end;
			pe = start;
			p = start;
			done = 0;
			do {
				/* Check for overflow */
				if (pe + SPINSZ > pe) {
					pe += SPINSZ;
				} else {
					pe = end;
				}
				if (pe >= end) {
					pe = end;
					done++;
				}
				if (p == pe ) {
					break;
				}
{
ulong bad;
				for (; p < pe; p++) {
					num = Rand();
					if (i) {
						num = ~num;
					}
					if ((bad=*p) != num) {
						error((ulong*)p, num, bad);
					}
					*p = ~num;
				}
}
				do_tick();
				BAILR
			} while (!done);
		}
	}
}

void movinv32(int iter, ulong p1, ulong lb, ulong hb, int sval, int off)
{
	int i, j, k=0, n = 0, done;
	volatile ulong *pe;
	volatile ulong *start, *end;
	ulong pat, p3;

	p3 = sval << 31;

	/* Display the current pattern */
        hprint(LINE_PAT, COL_PAT, p1);

	/* Initialize memory with the initial pattern.  */
	for (j=0; j<segs; j++) {
		start = v->map[j].start;
		end = v->map[j].end;
		pe = start;
		p = start;
		done = 0;
		k = off;
		pat = p1;
		do {
			/* Check for overflow */
			if (pe + SPINSZ > pe) {
				pe += SPINSZ;
			} else {
				pe = end;
			}
			if (pe >= end) {
				pe = end;
				done++;
			}
			if (p == pe ) {
				break;
			}
			/* Do a SPINSZ section of memory */
 			while (p < pe) {
 				*p = pat;
 				if (++k >= 32) {
 					pat = lb;
 					k = 0;
 				} else {
 					pat = pat << 1;
 					pat |= sval;
 				}
 				p++;
 			}
 
			do_tick();
			BAILR
		} while (!done);
	}

	/* Do moving inversions test. Check for initial pattern and then
	 * write the complement for each memory location. Test from bottom
	 * up and then from the top down.  */
	for (i=0; i<iter; i++) {
		for (j=0; j<segs; j++) {
			start = v->map[j].start;
			end = v->map[j].end;
			pe = start;
			p = start;
			done = 0;
			k = off;
			pat = p1;
			do {
				/* Check for overflow */
				if (pe + SPINSZ > pe) {
					pe += SPINSZ;
				} else {
					pe = end;
				}
				if (pe >= end) {
					pe = end;
					done++;
				}
				if (p == pe ) {
					break;
				}
{int bad;
				while (p < pe) {
 					if ((bad=*p) != pat) {
 						error((ulong*)p, pat, bad);
 					}
 					*p = ~pat;
 					if (++k >= 32) {
 						pat = lb;
 						k = 0;
 					} else {
 						pat = pat << 1;
 						pat |= sval;
 					}
 					p++;
 				}
}
				do_tick();
				BAILR
			} while (!done);
		}

		/* Since we already adjusted k and the pattern this
		 * code backs both up one step
		 */
		if (--k < 0) {
			k = 31;
		}
		for (pat = lb, n = 0; n < k; n++) {
			pat = pat << 1;
			pat |= sval;
		}
		k++;
		for (j=segs-1; j>=0; j--) {
			start = v->map[j].start;
			end = v->map[j].end;
			p = end -1;
			pe = end -1;
			done = 0;
			do {
				/* Check for underflow */
				if (pe - SPINSZ < pe) {
					pe -= SPINSZ;
				} else {
					pe = start;
				}
				if (pe <= start) {
					pe = start;
					done++;
				}
				if (p == pe ) {
					break;
				}
{int bad;
 				do {
 					if ((bad=*p) != ~pat) {
 						error((ulong*)p, ~pat, bad);
 					}
 					*p = pat;
 					if (--k <= 0) {
 						pat = hb;
 						k = 32;
 					} else {
 						pat = pat >> 1;
 						pat |= p3;
 					}
 				} while (p-- > pe);
}
				do_tick();
				BAILR
			} while (!done);
		}
	}
}

/*
 * Test all of memory using modulo X access pattern.
 */
void modtst(int offset, int iter, ulong p1, ulong p2)
{
	int j, k, l, done;
	volatile ulong *pe;
	volatile ulong *start, *end;

	/* Display the current pattern */
        hprint(LINE_PAT, COL_PAT-2, p1);
	cprint(LINE_PAT, COL_PAT+6, "-");
        dprint(LINE_PAT, COL_PAT+7, offset, 2, 1);

	/* Write every nth location with pattern */
	for (j=0; j<segs; j++) {
		start = v->map[j].start;
		end = v->map[j].end;
		pe = (ulong *)start;
		p = start+offset;
		done = 0;
		do {
			/* Check for overflow */
			if (pe + SPINSZ > pe) {
				pe += SPINSZ;
			} else {
				pe = end;
			}
			if (pe >= end) {
				pe = end;
				done++;
			}
			if (p == pe ) {
				break;
			}
 			for (; p < pe; p += MOD_SZ) {
				*p = p1;
 			}
 
			do_tick();
			BAILR
		} while (!done);
	}

	/* Write the rest of memory "iter" times with the pattern complement */
	for (l=0; l<iter; l++) {
		for (j=0; j<segs; j++) {
			start = v->map[j].start;
			end = v->map[j].end;
			pe = (ulong *)start;
			p = start;
			done = 0;
			k = 0;
			do {
				/* Check for overflow */
				if (pe + SPINSZ > pe) {
					pe += SPINSZ;
				} else {
					pe = end;
				}
				if (pe >= end) {
					pe = end;
					done++;
				}
				if (p == pe ) {
					break;
				}
 				for (; p < pe; p++) {
 					if (k != offset) {
						*p = p2;
 					}
 					if (++k > MOD_SZ-1) {
 						k = 0;
 					}
 				}

				do_tick();
				BAILR
			} while (!done);
		}
	}

	/* Now check every nth location */
	for (j=0; j<segs; j++) {
		start = v->map[j].start;
		end = v->map[j].end;
		pe = (ulong *)start;
		p = start+offset;
		done = 0;
		do {
			/* Check for overflow */
			if (pe + SPINSZ > pe) {
				pe += SPINSZ;
			} else {
				pe = end;
			}
			if (pe >= end) {
				pe = end;
				done++;
			}
			if (p == pe ) {
				break;
			}
{int bad;
 			for (; p < pe; p += MOD_SZ) {
 				if ((bad=*p) != p1) {
 					error((ulong*)p, p1, bad);
 				}
 			}
}
			do_tick();
			BAILR
		} while (!done);
	}
        cprint(LINE_PAT, COL_PAT, "          ");
}

/*
 * Test memory using block moves 
 * Adapted from Robert Redelmeier's burnBX test
 */
void block_move(int iter)
{
	int i, j, done;
	ulong len;
	volatile ulong p, pe, pp;
	volatile ulong start, end;

        cprint(LINE_PAT, COL_PAT-2, "          ");

	/* Initialize memory with the initial pattern.  */
	for (j=0; j<segs; j++) {
		start = (ulong)v->map[j].start;
#ifdef USB_WAR
		/* We can't do the block move test on low memory beacuase
		 * BIOS USB support clobbers location 0x410 and 0x4e0
		 */
		if (start < 0x4f0) {
			start = 0x4f0;
		}
#endif
		end = (ulong)v->map[j].end;
		pe = start;
		p = start;
		done = 0;
		do {
			/* Check for overflow */
			if (pe + SPINSZ*4 > pe) {
				pe += SPINSZ*4;
			} else {
				pe = end;
			}
			if (pe >= end) {
				pe = end;
				done++;
			}
			if (p == pe ) {
				break;
			}
			len  = ((ulong)pe - (ulong)p) / 64;
	
			__asm__ volatile (
				"move $2,%0\n\t"
				"move $3,%2\n\t"
				"move $4,%3\n\t"
				"1:\n\t"
				"move $5,$4\n\t"
				"not $5\n\t"
				"sw $4,0($2)\n\t"
				"sw $4,4($2)\n\t"
				"sw $4,8($2)\n\t"
				"sw $4,12($2)\n\t"
				"sw $5,16($2)\n\t"
				"sw $5,20($2)\n\t"
				"sw $4,24($2)\n\t"
				"sw $4,28($2)\n\t"
				"sw $4,32($2)\n\t"
				"sw $4,36($2)\n\t"
				"sw $5,40($2)\n\t"
				"sw $5,44($2)\n\t"
				"sw $4,48($2)\n\t"
				"sw $4,52($2)\n\t"
				"sw $5,56($2)\n\t"
				"sw $5,60($2)\n\t"
				"rol $4,1\n\t"
				"addiu $2,64\n\t"
				"addiu $3,-1\n\t"
				"bnez $3,1b\n\t"
				"nop;\n\t"
				"move %0,$2\n\t"
				: "=r" (p)
				: "0" (p), "r" (len), "r" (1)
				: "$2","$3","$4","$5"
			);

			do_tick();
			BAILR
		} while (!done);
	}

	/* Now move the data around 
	 * First move the data up half of the segment size we are testing
	 * Then move the data to the original location + 32 bytes
	 */
	for (j=0; j<segs; j++) {
		start = (ulong)v->map[j].start;
#ifdef USB_WAR
		/* We can't do the block move test on low memory beacuase
		 * BIOS USB support clobbers location 0x410 and 0x4e0
		 */
		if (start < 0x4f0) {
			start = 0x4f0;
		}
#endif
		end = (ulong)v->map[j].end;
		pe = start;
		p = start;
		done = 0;
		do {
			/* Check for overflow */
			if (pe + SPINSZ*4 > pe) {
				pe += SPINSZ*4;
			} else {
				pe = end;
			}
			if (pe >= end) {
				pe = end;
				done++;
			}
			if (p == pe ) {
				break;
			}
			pp = p + ((pe - p) / 2);
			len  = ((ulong)pe - (ulong)p) / 8;
			for(i=0; i<iter; i++) {

			{int *src,*dst,j;
				src=p;
				dst=pp;
				for(j=0;j<len;j++)
				{
					*dst++=*src++;
				}
				dst=p;
				src=pp;
				for(j=0;j<i;j++)
				{
					*dst++=*src++;
				}
			}
				do_tick();
				BAILR
			}
			p = pe;
		} while (!done);
	}

	/* Now check the data 
	 * The error checking is rather crude.  We just check that the
	 * adjacent words are the same.
	 */
	for (j=0; j<segs; j++) {
		start = (ulong)v->map[j].start;
#ifdef USB_WAR
		/* We can't do the block move test on low memory beacuase
		 * BIOS USB support clobbers location 0x4e0 and 0x410
		 */
		if (start < 0x4f0) {
			start = 0x4f0;
		}
#endif
		end = (ulong)v->map[j].end;
		pe = start;
		p = start;
		done = 0;
		do {
			/* Check for overflow */
			if (pe + SPINSZ*4 > pe) {
				pe += SPINSZ*4;
			} else {
				pe = end;
			}
			if (pe >= end) {
				pe = end;
				done++;
			}
			if (p == pe ) {
				break;
			}


			{
			int *x=p;
			while(x<pe)
			{
			if(x[0]!=x[1])
			{
			 mv_error(x+1,x[0],x[1]);
			}
			x=x+2;
			}
			}
			
			do_tick();
			BAILR
		} while (!done);
	}
}

/*
 * Test memory for bit fade.
 */
#define STIME 5400
void bit_fade()
{
	int j;
	volatile ulong *pe;
	volatile ulong bad;
	volatile ulong *start,*end;

	test_ticks += (STIME * 2);
	v->pass_ticks += (STIME * 2);

	/* Do -1 and 0 patterns */
	p1 = 0;
	while (1) {

		/* Display the current pattern */
		hprint(LINE_PAT, COL_PAT, p1);

		/* Initialize memory with the initial pattern.  */
		for (j=0; j<segs; j++) {
			start = v->map[j].start;
			end = v->map[j].end;
			pe = start;
			p = start;
			for (p=end; p<end; p++) {
				*p = p1;
			}
			do_tick();
			BAILR
		}
		/* Snooze for 90 minutes */
		Sleep (STIME);

		/* Make sure that nothing changed while sleeping */
		for (j=0; j<segs; j++) {
			start = v->map[j].start;
			end = v->map[j].end;
			pe = start;
			p = start;
			for (p=end; p<end; p++) {
 				if ((bad=*p) != p1) {
					error((ulong*)p, p1, bad);
				}
			}
			do_tick();
			BAILR
		}
		if (p1 == 0) {
			p1=-1;
		} else {
			break;
		}
	}
}

/*
 * Display data error message. Don't display duplicate errors.
 */
void error(ulong *adr, ulong good, ulong bad)
{
	ulong xor;
	int patnchg;

	xor = good ^ bad;
#ifdef USB_WAR
	/* Skip any errrors that appear to be due to the BIOS using location
	 * 0x4e0 for USB keyboard support.  This often happens with Intel
         * 810, 815 and 820 chipsets.  It is possible that we will skip
	 * a real error but the odds are very low.
	 */
	if ((ulong)adr == 0x4e0 || (ulong)adr == 0x410) {
		return;
	}
#endif

	/* Process the address in the pattern administration */
	patnchg=insertaddress ((ulong) adr);

	update_err_counts();
	if (v->printmode == PRINTMODE_ADDRESSES) {

		/* Don't display duplicate errors */
		if ((ulong)adr == (ulong)v->eadr && xor == v->exor) {
			print_err_counts();
			dprint(v->msg_line, 62, ++ecount, 5, 0);
			return;
		}
		print_err(adr, good, bad, xor);

	} else if (v->printmode == PRINTMODE_PATTERNS) {
		print_err_counts();

		if (patnchg) { 
			printpatn();
		}
	}
}

/*
 * Display data error message from the block move test.  The actual failing
 * address is unknown so don't use this failure information to create
 * BadRAM patterns.
 */
void mv_error(ulong *adr, ulong good, ulong bad)
{
	ulong xor;

	update_err_counts();
	if (v->printmode == PRINTMODE_NONE) {
		return;
	}
	xor = good ^ bad;
	print_err(adr, good, bad, xor);
}

/*
 * Display address error message.
 * Since this is strictly an address test, trying to create BadRAM
 * patterns does not make sense.  Just report the error.
 */
void ad_err1(ulong *adr1, ulong *adr2, ulong good, ulong bad)
{
	ulong xor;
	update_err_counts();
	if (v->printmode == PRINTMODE_NONE) {
		return;
	}
	xor = ((ulong)adr1) ^ ((ulong)adr2);
	print_err(adr1, good, bad, xor);
}

/*
 * Display address error message.
 * Since this type of address error can also report data errors go
 * ahead and generate BadRAM patterns.
 */
void ad_err2(ulong *adr, ulong bad)
{
	int patnchg;

	/* Process the address in the pattern administration */
	patnchg=insertaddress ((ulong) adr);

	update_err_counts();
	if (v->printmode == PRINTMODE_ADDRESSES) {
		print_err(adr, (ulong)adr, bad, ((ulong)adr) ^ bad);
	} else if (v->printmode == PRINTMODE_PATTERNS) {
		print_err_counts();
		if (patnchg) { 
			printpatn();
		}
	}
}
void print_hdr(void)
{
	if (v->ecount > 1) {
		return;
	}
	cprint(LINE_HEADER, 0,  "Tst  Pass   Failing Address          Good       Bad     Err-Bits  Count Chan");
	cprint(LINE_HEADER+1, 0,"---  ----  -----------------------  --------  --------  --------  ----- ----");
}

static void update_err_counts(void)
{
	++(v->ecount);
	tseq[v->test].errors++;
}

static void print_err_counts(void)
{
	int i;
	char *pp;

#if NMOD_FRAMEBUFFER >0
	video_set_bg(128, 0, 0);
#endif

	dprint(LINE_INFO, COL_ERR, v->ecount, 6, 0);
	dprint(LINE_INFO, COL_ECC_ERR, v->ecc_ecount, 6, 0);

	/* Paint the error messages on the screen red to provide a vivid */
	/* indicator that an error has occured */ 
#if (NMOD_X86EMU_INT10 > 0)||(NMOD_X86EMU > 0)
	if (v->msg_line < 24) {
		for(i=0, pp=(char *)((SCREEN_ADR+v->msg_line*160+1));
				 i<76; i++, pp+=2) {
			*pp = 0x47;
		}
	}
#elif NMOD_FRAMEBUFFER >0
	video_set_bg(0, 0, 128);
#endif

}

static void common_err(ulong page, ulong offset)
{
	ulong mb;

	/* Check for keyboard input */
	print_hdr();
	check_input();
	scroll();
	print_err_counts();

	mb = page >> 8;
	dprint(v->msg_line, 0, v->test, 3, 0);
	dprint(v->msg_line, 4, v->pass, 5, 0);
	hprint(v->msg_line, 11, page);
	hprint2(v->msg_line, 19, offset, 3);
	cprint(v->msg_line, 22, " -      . MB");
	dprint(v->msg_line, 25, mb, 5, 0);
	dprint(v->msg_line, 31, ((page & 0xF)*10)/16, 1, 0);
}
/*
 * Print an individual error
 */
void print_err( ulong *adr, ulong good, ulong bad, ulong xor) 
{
	ulong page, offset;

	page = page_of(adr);
	offset = ((unsigned long)adr) & 0xFFF;
	common_err(page, offset);

	ecount = 1;
	hprint(v->msg_line, 36, good);
	hprint(v->msg_line, 46, bad);
	hprint(v->msg_line, 56, xor);
	dprint(v->msg_line, 66, ecount, 5, 0);
	v->eadr = adr;
	v->exor = xor;
}

/*
 * Print an ecc error
 */
void print_ecc_err(unsigned long page, unsigned long offset, 
	int corrected, unsigned short syndrome, int channel)
{
	if (!corrected) {
		update_err_counts();
	}
	++(v->ecc_ecount);
	if (v->printmode == PRINTMODE_NONE) {
		return;
	}
	common_err(page, offset);

	cprint(v->msg_line, 36, 
		corrected?"corrected           ": "uncorrected         ");
	hprint2(v->msg_line, 60, syndrome, 4);
	cprint(v->msg_line, 68, "ECC"); 
	dprint(v->msg_line, 74, channel, 2, 0);
}

#ifdef PARITY_MEM
/*
 * Print a parity error message
 */
void parity_err( unsigned long edi, unsigned long esi) 
{
	unsigned long addr;

	if (v->test == 5) {
		addr = esi;
	} else {
		addr = edi;
	}
	update_err_counts();
	if (v->printmode == PRINTMODE_NONE) {
		return;
	}
	common_err(page_of((void *)addr), addr & 0xFFF);
	cprint(v->msg_line, 36, "Parity error detected                ");
}
#endif


/*
 * Print the pattern array as a LILO boot option addressing BadRAM support.
 */
void printpatn (void)
{
       int idx=0;
       int x;

	/* Check for keyboard input */
	check_input();

       if (v->numpatn == 0)
               return;

       scroll();

       cprint (v->msg_line, 0, "badram=");
       x=7;

       for (idx = 0; idx < v->numpatn; idx++) {

               if (x > 80-22) {
                       scroll();
                       x=7;
               }
               cprint (v->msg_line, x, "0x");
               hprint (v->msg_line, x+2,  v->patn[idx].adr );
               cprint (v->msg_line, x+10, ",0x");
               hprint (v->msg_line, x+13, v->patn[idx].mask);
               if (idx+1 < v->numpatn)
                       cprint (v->msg_line, x+21, ",");
               x+=22;
       }
}
	
#include <time.h>
/*
 * Show progress by displaying elapsed time and update bar graphs
 */
void do_tick(void)
{
	int i, pct;

	/* FIXME only print serial error messages from the tick handler */
	if (v->ecount) {
		print_err_counts();
	}
	
	nticks++;
	v->total_ticks++;

	pct = 100*nticks/test_ticks;
	dprint(1, COL_MID+4, pct, 3, 0);
	i = (BAR_SIZE * pct) / 100;
	while (i > v->tptr) {
		if (v->tptr >= BAR_SIZE) {
			break;
		}
		cprint(1, COL_MID+9+v->tptr, "#");
		v->tptr++;
	}
	
	pct = 100*v->total_ticks/v->pass_ticks;
	dprint(0, COL_MID+4, pct, 3, 0);
	i = (BAR_SIZE * pct) / 100;
	while (i > v->pptr) {
		if (v->pptr >= BAR_SIZE) {
			break;
		}
		cprint(0, COL_MID+9+v->pptr, "#");
		v->pptr++;
	}

	/* We can't do the elapsed time unless the rdtsc instruction
	 * is supported
	 */
	if (v->rdtsc) {
    struct tm tm;
    time_t t;
	char str[20];

#ifdef HAVE_TOD
    t = tgt_gettime ();
    tm = *localtime (&t);
    sprintf(str,"%02d:%02d:%02d",tm.tm_hour, tm.tm_min, tm.tm_sec);
	cprint(LINE_TIME, COL_TIME,str);
#endif
/*
		asm __volatile__(
			"rdtsc":"=a" (l),"=d" (h));
		asm __volatile__ (
			"subl %2,%0\n\t"
			"sbbl %3,%1"
			:"=a" (l), "=d" (h)
			:"g" (v->startl), "g" (v->starth),
			"0" (l), "1" (h));
		t = h * ((unsigned)0xffffffff / v->clks_msec) / 1000;
		t += (l / v->clks_msec) / 1000;
		i = t % 60;
		dprint(LINE_TIME, COL_TIME+9, i%10, 1, 0);
		dprint(LINE_TIME, COL_TIME+8, i/10, 1, 0);
		t /= 60;
		i = t % 60;
		dprint(LINE_TIME, COL_TIME+6, i % 10, 1, 0);
		dprint(LINE_TIME, COL_TIME+5, i / 10, 1, 0);
		t /= 60;
		dprint(LINE_TIME, COL_TIME, t, 4, 0);
*/
	}

	/* Check for keyboard input */
	check_input();

	/* Poll for ECC errors */
	poll_errors();
}
extern void delay(int microseconds);
void Sleep(int n)
{
	int i, ip=0;
	ulong  t;

	t=0;
	/* loop for n seconds */
	while (1) {
		delay(1000);
		t++;

		/* Is the time up? */
		if (t >= n) {
			break;
		}

		/* Display the elapsed time on the screen */
		i = t % 60;
		dprint(LINE_TIME, COL_TIME+9, i%10, 1, 0);
		dprint(LINE_TIME, COL_TIME+8, i/10, 1, 0);
		if (i != ip) {
			do_tick();
			BAILR
			ip = i;
		}
		t /= 60;
		i = t % 60;
		dprint(LINE_TIME, COL_TIME+6, i % 10, 1, 0);
		dprint(LINE_TIME, COL_TIME+5, i / 10, 1, 0);
		t /= 60;
		dprint(LINE_TIME, COL_TIME, t, 4, 0);
	}
}


