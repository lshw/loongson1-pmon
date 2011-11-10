
#include <stdio.h>
#include "elf.h"

static struct ehdr 	exec;
static struct phdr	prgm;
static struct shdr	sect;
static struct sym	symb;

main(ac, av)
int	ac;
char	*av[];
{
FILE	*in;
int	c;
int 	i;
long	len;

	if( ac != 2 ) {
		fprintf(stderr, "use: %s file\n", av[0]);
		exit(1);
	}
	if( (in = fopen(av[1], "r")) == NULL ) {
		perror(av[1]);
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
	fprintf(stderr, "%08X  \n",exec.magic);
	fprintf(stderr, "unsigned short object file type");
	fprintf(stderr, " %d \n",exec.type);
	fprintf(stderr, "unsigned short machine id");
	fprintf(stderr, " %d\n",exec.machine);
	fprintf(stderr, "unsigned long version file format");
	fprintf(stderr, " %ld\n",exec.version);
	fprintf(stderr, "unsigned long entry point");
	fprintf(stderr, " H(%08X) D(%ld)\n",exec.entry,exec.entry);
	fprintf(stderr, "unsigned long phoff program header table offset ");
	fprintf(stderr, " H(%08X) D(%ld)\n",exec.phoff,exec.phoff);
	fprintf(stderr, "unsigned long shoff section header table offset ");
	fprintf(stderr, " H(%08X) D(%ld)\n",exec.shoff,exec.shoff);
	fprintf(stderr, "unsigned long flags processor specific flags");
	fprintf(stderr, " %08X\n",exec.flags);
	fprintf(stderr, "unsigned short ehsize elf header size in bytes ");
	fprintf(stderr, " H(%02X) D(%d)\n",exec.ehsize,exec.ehsize);
	fprintf(stderr, "unsigned short phsize program header size ");
	fprintf(stderr, " H(%02X) D(%d)\n",exec.phsize,exec.phsize);
	fprintf(stderr, "unsigned short phcount program header count ");
	fprintf(stderr, " %d\n",exec.phcount);
	fprintf(stderr, "unsigned short shsize section header size ");
	fprintf(stderr, " H(%02X) D(%d)\n",exec.shsize,exec.shsize);
	fprintf(stderr, "unsigned short shcount section header count ");
	fprintf(stderr, " %d\n",exec.shcount);
	fprintf(stderr, "unsigned short shstrndx section header string table index  ");
	fprintf(stderr, " %d\n\n\n",exec.shstrndx);

	for ( i =0;i<exec.phcount;i++) {
		fseek(in,(exec.phoff+(i*exec.phsize)),SEEK_SET);
	
		if( fread(&prgm, sizeof(prgm), 1, in) == 0 ) {
			fprintf(stderr, "fread fails\n");
			exit(1);
		}
	
		fprintf(stderr, "PROGRAM HEADER NUMBER ");
		fprintf(stderr, "%d\n",i);
		fprintf(stderr, "unsigned long type Segment type ");
		fprintf(stderr, " H(%08X) D(%ld)\n",prgm.type,prgm.type);
		fprintf(stderr, "unsigned long offset file offset ");
		fprintf(stderr, " H(%08X) D(%ld)\n",prgm.offset,prgm.offset);
		fprintf(stderr, "unsigned long vaddr virtual address ");
		fprintf(stderr, " H(%08X) D(%ld)\n",prgm.vaddr,prgm.vaddr);
		fprintf(stderr, "unsigned long paddr physical address ");
		fprintf(stderr, " H(%08X) D(%ld)\n",prgm.paddr,prgm.paddr);
		fprintf(stderr, "unsigned long filesz size of segment in file ");
		fprintf(stderr, " H(%08X) D(%ld)\n",prgm.filesz,prgm.filesz);
		fprintf(stderr, "unsigned long memsz size of segment in memory ");
		fprintf(stderr, " H(%08X) D(%ld)\n",prgm.memsz,prgm.memsz);
		fprintf(stderr, "unsigned long flags Segment flags ");
		fprintf(stderr, " H(%08X) D(%ld)\n",prgm.flags,prgm.flags);
		fprintf(stderr, "unsigned long align alignment file and memory");
		fprintf(stderr, " %ld\n\n\n",prgm.align);
	

		fseek(in,prgm.offset,SEEK_SET);
	
		len = prgm.filesz;
	
		if( len > 0 ) do {
			if( (c = getc(in)) != EOF )
				putchar(c);
			else
				break;
		} while( --len > 0 );

		if( len > 0 )
			fprintf(stderr, "unexpected EOF on input, %d left\n", len);

	}
#if 0
	fseek(in,exec.shoff,SEEK_SET);

	for ( i = 0;i<exec.shcount;i++) {


		if( fread(&sect, sizeof(sect), 1, in) == 0 ) {
			fprintf(stderr, "fread fails\n");
			exit(1);
		}
		
		fprintf(stderr, "SECTION HEADER NUMBER ");
		fprintf(stderr, "%d\n",i);	

		fprintf(stderr, "unsigned long name Offset into string table of section name ");
		fprintf(stderr, " H(%08X) D(%ld)\n",sect.name,sect.name);
		fprintf(stderr, "unsigned long type of section ");
		fprintf(stderr, " H(%08X) D(%ld)\n",sect.type,sect.type);
		fprintf(stderr, "unsigned long flags section flags ");
		fprintf(stderr, " H(%08X) D(%ld)\n",sect.flags,sect.flags);
		fprintf(stderr, "unsigned long addr Section virtual address at execution ");
		fprintf(stderr, " H(%08X) D(%ld)\n",sect.addr,sect.addr);
		fprintf(stderr, "unsigned long offset Section file offset ");
		fprintf(stderr, " H(%08X) D(%ld)\n",sect.offset,sect.offset);
		fprintf(stderr, "unsigned long size Section size ");
		fprintf(stderr, " H(%08X) D(%ld)\n",sect.size,sect.size);
		fprintf(stderr, "unsigned long link to another section ");
		fprintf(stderr, " H(%08X) D(%ld)\n",sect.link,sect.link);
		fprintf(stderr, "unsigned long info Additional section info ");
		fprintf(stderr, " H(%08X) D(%ld)\n",sect.info,sect.info);
		fprintf(stderr, "unsigned long align Section alignment ");
		fprintf(stderr, " %ld\n",sect.align);
		fprintf(stderr, "unsigned long esize Entry size if section holds table ");
		fprintf(stderr, " %ld\n\n\n",sect.esize);

	}

	for ( i = 0;i<exec.shcount;i++) {

		if( fread(&symb, sizeof(symb), 1, in) == 0 ) {
			fprintf(stderr, "fread fails\n");
			exit(1);
		}


		fprintf(stderr, "SYMBOL HEADER NUMBER ");
		fprintf(stderr, "%d\n",i);

		fprintf(stderr, "unsigned long name index into strtab of symbol name ");
		fprintf(stderr, " H(%08X) D(%ld)\n",symb.name,symb.name);
		fprintf(stderr, "unsigned long value Section offset virt addr or common align ");
		fprintf(stderr, " H(%08X) D(%ld)\n",symb.value,symb.value);
		fprintf(stderr, "unsigned long size of object referenced ");
		fprintf(stderr, " H(%08X) D(%ld)\n",symb.size,symb.size);
		fprintf(stderr, "unsigned type :4 symbol type ");
		fprintf(stderr, " %d\n",symb.type);
		fprintf(stderr, "unsigned binding: 4 Symbol binding ");
		fprintf(stderr, " %d\n",symb.binding);
		fprintf(stderr, "unsigned short shndx Section containing symbol");
		fprintf(stderr, " %d\n\n\n",symb.shndx);

	}


#endif

	fclose(in);
}

