
#include <stdio.h>
#include "elf.h"

#define REOTEXT_OFFSET 0x10

static struct ehdr 	exec;
static struct phdr	prgm;
static struct shdr	sect;
static struct sym	symb;


#ifdef DO_SWAP
#define SWAP16(x) \
		  (((x & 0x00ff) << 8) | \
		  ((x & 0xff00) >> 8))

#define SWAP32(x) \
		  (((x & 0x000000ff) << 24) | \
		  ((x & 0xff000000) >> 24) | \
		  ((x & 0x00ff0000) >> 8) | \
		  ((x & 0x0000ff00) << 8))
#else
#define SWAP16(x) (x)
#define SWAP32(x) (x)
#endif /* DO_SWAP */

/*---------------------------------------------------------------------------
 * bootelf -- strip and reorganize the elf executable into format
 * 	      suitable for blasting into PROM.
 *
 * args:	infile	- the elf executable file
 *		outfile - name of file to write striped data to
 *
 * There is also a problem in that the constant initialized data follows the code
 * but the 'etext' symbol only reflects the end of the code.  Hence it is impossible
 * for the ROM code to know where the end of the constant initialized data ends so
 * it can copy the normal (non-constant) initialized data out to RAM.
 *
 *	[CODE SEGMENT][CONST INIT DATA]  [INIT DATA][UNINIT DATA]
 *	\_____________________________/  \______________________/
 *		READ ONLY		      WRITEABLE DATA
 *	   Program Header #1		     Program Header #0
 *
 * Since the bootelf converter knows the size of Program Headers, we insert this
 * value into a known variable (reotext) where the rom code can read it and make
 * use of the value. (Yes this is an aweful hack!)
 *---------------------------------------------------------------------------*/
int
main(ac, av)
int	ac;
char	*av[];
{
	FILE	*in, *out;
	int	c;
	int 	i;
	long	len;
	long	reotext;	/* Real end of text address */

	if( ac != 3 ) {
		fprintf(stderr, "use: %s infile outfile\n", av[0]);
		exit(1);
	}
	if( (in = fopen(av[1], "r")) == NULL ) {
		perror(av[1]);
		exit(1);
	}

	if( (out = fopen(av[2], "w")) == NULL ) {
		perror(av[2]);
		exit(1);
	}

	if( fread(&exec, sizeof(exec), 1, in) == 0 ) {
		fprintf(stderr, "fread fails\n");
		exit(1);
	}
	fprintf(stderr, "char elf magic number ");
	fprintf(stderr, "%02X %02X %02X %02X\n",
      			 exec.elf_magic[0],exec.elf_magic[1],exec.elf_magic[2],exec.elf_magic[3]);
	fprintf(stderr, "unsigned long magic number ");
	fprintf(stderr, "%08X  \n", SWAP32(exec.magic[0]));
	fprintf(stderr, "unsigned short object file type");
	fprintf(stderr, " %d \n", SWAP16(exec.type));
	fprintf(stderr, "unsigned short machine id");
	fprintf(stderr, " %d\n", SWAP16(exec.machine));
	fprintf(stderr, "unsigned long version file format");
	fprintf(stderr, " %ld\n", SWAP32(exec.version));
	fprintf(stderr, "unsigned long entry point");
	fprintf(stderr, " H(%08X) D(%ld)\n", SWAP32(exec.entry) , SWAP32(exec.entry));
	fprintf(stderr, "unsigned long phoff program header table offset ");
	fprintf(stderr, " H(%08X) D(%ld)\n", SWAP32(exec.phoff), SWAP32(exec.phoff));
	fprintf(stderr, "unsigned long shoff section header table offset ");
	fprintf(stderr, " H(%08X) D(%ld)\n", SWAP32(exec.shoff), SWAP32(exec.shoff));
	fprintf(stderr, "unsigned long flags processor specific flags");
	fprintf(stderr, " %08X\n", SWAP32(exec.flags));
	fprintf(stderr, "unsigned short ehsize elf header size in bytes ");
	fprintf(stderr, " H(%02X) D(%d)\n", SWAP16(exec.ehsize), SWAP16(exec.ehsize));
	fprintf(stderr, "unsigned short phsize program header size ");
	fprintf(stderr, " H(%02X) D(%d)\n", SWAP16(exec.phsize), SWAP16(exec.phsize));
	fprintf(stderr, "unsigned short phcount program header count ");
	fprintf(stderr, " %d\n", SWAP16(exec.phcount));
	fprintf(stderr, "unsigned short shsize section header size ");
	fprintf(stderr, " H(%02X) D(%d)\n", SWAP16(exec.shsize), SWAP16(exec.shsize));
	fprintf(stderr, "unsigned short shcount section header count ");
	fprintf(stderr, " %d\n", SWAP16(exec.shcount));
	fprintf(stderr, "unsigned short shstrndx section header string table index  ");
	fprintf(stderr, " %d\n\n\n", SWAP16(exec.shstrndx));

	/*---------------------------------------------------------------------------
	 * For the BOOTROM, the code is compiled to 0xbfc00000 while the data is usually
	 * compiled to address 0xa0000200 which is uncached DRAM or 0x00000200 which is
	 * the cached DRAM.  This causes a problem as the elf headers appear to be ordering
	 * in accending order (hence the data will appear before the code).  Thus we process
	 * the headers in reverse order.
	 *---------------------------------------------------------------------------*/
	for ( i =1 ; i >= 0; i--) {
		fseek(in,(SWAP32(exec.phoff) + (i* SWAP16(exec.phsize))),SEEK_SET);
	
		if( fread(&prgm, sizeof(prgm), 1, in) == 0 ) {
			fprintf(stderr, "fread fails\n");
			exit(1);
		}
		fprintf(stderr, "PROGRAM HEADER NUMBER %d\n", i);
		fprintf(stderr, "unsigned long type Segment type ");
		fprintf(stderr, " H(%08X) D(%ld)\n", SWAP32(prgm.type), SWAP32(prgm.type));
		fprintf(stderr, "unsigned long offset file offset ");
		fprintf(stderr, " H(%08X) D(%ld)\n", SWAP32(prgm.offset), SWAP32(prgm.offset));
		fprintf(stderr, "unsigned long vaddr virtual address ");
		fprintf(stderr, " H(%08X) D(%ld)\n", SWAP32(prgm.vaddr), SWAP32(prgm.vaddr));
		fprintf(stderr, "unsigned long paddr physical address ");
		fprintf(stderr, " H(%08X) D(%ld)\n", SWAP32(prgm.paddr), SWAP32(prgm.paddr));
		fprintf(stderr, "unsigned long filesz size of segment in file ");
		fprintf(stderr, " H(%08X) D(%ld)\n", SWAP32(prgm.filesz), SWAP32(prgm.filesz));
		fprintf(stderr, "unsigned long memsz size of segment in memory ");
		fprintf(stderr, " H(%08X) D(%ld)\n", SWAP32(prgm.memsz), SWAP32(prgm.memsz));
		fprintf(stderr, "unsigned long flags Segment flags ");
		fprintf(stderr, " H(%08X) D(%ld)\n", SWAP32(prgm.flags), SWAP32(prgm.flags));
		fprintf(stderr, "unsigned long align alignment file and memory");
		fprintf(stderr, " %ld\n", SWAP32(prgm.align));
	
		len = SWAP32(prgm.filesz);
		if (i == 1) {
			fseek(in, SWAP32(prgm.offset) , SEEK_SET);
			reotext = len | SWAP32(prgm.vaddr);		/* end of text addr */
		}
		else {
			int z;
#if 0
			fseek(in,prgm.offset + 0x200, SEEK_SET);
#else
			fseek(in, SWAP32(prgm.offset), SEEK_SET);
#endif
			/* NOP padding */
			z = SWAP32(prgm.paddr) & 0x1ff;
			while(z > 0) {
				putc(0x60, out);
				putc(0x00, out);
				putc(0x00, out);
				putc(0x00, out);
				z -= 4;
			}
		}

		if( len > 0 ) do {
			if( (c = getc(in)) != EOF )
			    putc(c, out);
			else
			    break;
		} while( --len > 0 );
		if( len > 0 )
			fprintf(stderr, "unexpected EOF on input, %d left\n", len);
		if((len = SWAP32(prgm.filesz) & 15) != 0) {
			while(len++ < 16) /* Align to 16 */
				putc( 0, out);
		}
	}

#if LD_DOES_IT_CORRECTLY
	fprintf(stderr, "Installing reotext = 0x%08x into file...\n", reotext);
	fseek(out, REOTEXT_OFFSET, SEEK_SET);
	fwrite(&reotext, sizeof(long), 1, out);
#endif
	fclose(in);
	fclose(out);
}

