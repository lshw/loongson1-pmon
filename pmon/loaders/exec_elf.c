/* $Id: exec_elf.c,v 1.1.1.1 2006/09/14 01:59:08 root Exp $ */
/*
 * Copyright (c) 2002 Opsycon AB  (www.opsycon.se)
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
 *	This product includes software developed by Opsycon AB.
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
#include <sys/param.h>
#include <sys/types.h>
#include <sys/file.h>
#include <sys/errno.h>
#include <sys/endian.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <exec.h>
#include "elf.h"

#include <pmon.h>
#include <pmon/loaders/loadfn.h>

#ifdef __mips__
#include <machine/cpu.h>
#endif

#include "gzip.h"
#include "elf32only.h"
#if NGZIP > 0
#include <gzipfs.h>
#endif /* NGZIP */

static int	bootseg;
static unsigned long tablebase;
static unsigned int lastaddr=0;;
extern  char *dl_Oloadbuffer;
extern unsigned long long dl_loffset;

int highmemcpy(long long dst,long long src,long long count);
int highmemset(long long addr,unsigned char c,unsigned long long count);

static int myflags;
static int bootread(int fd, void *addr, int size)
{
	int i;

	if (bootseg++ > 0)
		fprintf (stderr, "\b + ");
	fprintf (stderr, "0x%x/%d ", addr + dl_offset, size);

	if (!dl_checksetloadaddr (addr + dl_offset, size, 1))
		return (-1);

#if NGZIP > 0
	if (myflags&ZFLAG) {
		if (myflags&OFLAG) {
			if (lastaddr)dl_loffset += (unsigned long long)(addr-lastaddr);
			lastaddr = addr;
			i = 0;
			while (i<size) {
				int i1;
				int len = min(0x1000,size-i);
				i1 = gz_read (fd,dl_Oloadbuffer,len);
				if (i1<0)
					break;
				highmemcpy(dl_loffset+i,dl_Oloadbuffer,i1);
				i += i1;
				if (i1<len)
					break;
			}
		}
		else i = gz_read (fd, addr + dl_offset, size);
	} else
#endif /* NGZIP */
{
	if(myflags&OFLAG){
		if(lastaddr)dl_loffset +=(unsigned long long)(addr-lastaddr);
		lastaddr=addr; 
		i=0;
		while(i<size)
		{
		int i1;
		int len=min(0x1000,size-i);
		i1=read (fd,dl_Oloadbuffer,len);
		if(i1<0)break;
		highmemcpy(dl_loffset+i,dl_Oloadbuffer,i1);
		i+=i1;
		if(i1<len)break;
		}
	}
	else i = read (fd, addr + dl_offset, size);
	}

	if (i < size) {
		if (i >= 0)
			fprintf (stderr, "\nread failed (corrupt object file?)");
		else
			perror ("\nsegment read");
		return (-1);
	}
	return size;
}


static int bootclear(int fd, void *addr, int size)
{

	if (bootseg++ > 0)
		fprintf (stderr, "\b + ");
	fprintf (stderr, "0x%x/%d(z) ", addr + dl_offset, size);

	if (!dl_checkloadaddr (addr + dl_offset, size, 1))
		return (-1);

	if (size > 0)
		if(myflags&OFLAG)
		highmemset((unsigned long long)(dl_loffset-lastaddr+(unsigned long)addr),0,size);
		else bzero (addr + dl_offset, size);
	    
	return size;
}

static Elf32_Shdr *elfgetshdr(int fd, Elf32_Ehdr *ep)
{
	Elf32_Shdr *shtab;
	unsigned size = ep->e_shnum * sizeof(Elf32_Shdr);

	shtab = (Elf32_Shdr *) malloc (size);
	if (!shtab) {
		fprintf (stderr,"\nnot enough memory to read section headers");
		return (0);
	}

#if NGZIP > 0
if(myflags&ZFLAG){
	if (gz_lseek (fd, ep->e_shoff, SEEK_SET) != ep->e_shoff ||
	    gz_read (fd, shtab, size) != size) {
		perror ("\nsection headers");
		free (shtab);
		return (0);
	}
	}else
#endif /* NGZIP */
	if (lseek (fd, ep->e_shoff, SEEK_SET) != ep->e_shoff ||
	    read (fd, shtab, size) != size) {
		perror ("\nsection headers");
		free (shtab);
		return (0);
	}

	return (shtab);
}

static void *gettable(int size, char *name, int flags)
{
	unsigned long base;

	if( !(flags & KFLAG)) {
		/* temporarily use top of memory to hold a table */
		base = (tablebase - size) & ~7;
		if (base < dl_maxaddr) {
			fprintf (stderr, "\nnot enough memory for %s table", base);
			return 0;
		}
		tablebase = base;
	}
	else {
		/* Put table after loaded code to support kernel DDB */
		tablebase = roundup(tablebase, sizeof(long));
		base = tablebase;
		tablebase += size;
	}
	return (void *) base;
}

static void *readtable(int fd, int offs, void *base, int size, char *name, int flags)
{
#if NGZIP > 0
if(myflags&ZFLAG){
	if (gz_lseek (fd, offs, SEEK_SET) != offs ||
	    gz_read (fd, base, size) != size) {
		fprintf (stderr, "\ncannot read %s table", name);
		return 0;
	}
	}else
#endif /* NGZIP */
	if (lseek (fd, offs, SEEK_SET) != offs ||
	    read (fd, base, size) != size) {
		fprintf (stderr, "\ncannot read %s table", name);
		return 0;
	}
	return (void *) base;
}

static int elfreadsyms(int fd, Elf32_Ehdr *eh, Elf32_Shdr *shtab, int flags)
{
	Elf32_Shdr *sh, *strh, *shstrh, *ksh;
	Elf32_Sym *symtab;
	Elf32_Ehdr *keh;
	char *shstrtab, *strtab, *symend;
	int nsym, offs, size, i;
	int *symptr;

	/* Fix up twirl */
	if (bootseg++ > 0) {
		fprintf (stderr, "\b + ");
	}

	/*
	 *  If we are loading symbols to support kernel DDB symbol handling
	 *  make room for an ELF header at _end and after that a section
	 *  header. DDB then finds the symbols using the data put here.
	 */
	if(flags & KFLAG) {
		tablebase = roundup(tablebase, sizeof(long));
		symptr = (int *)tablebase;
		tablebase += sizeof(int *) * 2;
		keh = (Elf32_Ehdr *)tablebase;
		tablebase += sizeof(Elf32_Ehdr); 
		tablebase = roundup(tablebase, sizeof(long));
		ksh = (Elf32_Shdr *)tablebase;
		tablebase += roundup((sizeof(Elf32_Shdr) * eh->e_shnum), sizeof(long)); 
		memcpy(ksh, shtab, roundup((sizeof(Elf32_Shdr) * eh->e_shnum), sizeof(long)));
		sh = ksh;
	}
	else {
		sh = shtab;
	}
	shstrh = &sh[eh->e_shstrndx];

	for (i = 0; i < eh->e_shnum; sh++, i++) {
		if (sh->sh_type == SHT_SYMTAB) {
			break;
		}
	}
	if (i >= eh->e_shnum) {
		return (0);
	}

	if(flags & KFLAG) {
		strh = &ksh[sh->sh_link];
		nsym = sh->sh_size / sh->sh_entsize;
		offs = sh->sh_offset;
		size = sh->sh_size;
		fprintf (stderr, "%d syms ", nsym);
	} else {
		strh = &shtab[sh->sh_link];
		nsym = (sh->sh_size / sh->sh_entsize) - sh->sh_info;
		offs = sh->sh_offset + (sh->sh_info * sh->sh_entsize);
		size = nsym * sh->sh_entsize;
		fprintf (stderr, "%d syms ", nsym);
	}



	/*
	 *  Allocate tables in correct order so the kernel grooks it.
	 *  Then we read them in the order they are in the ELF file.
	 */
	shstrtab = gettable(shstrh->sh_size, "shstrtab", flags);
	strtab = gettable(strh->sh_size, "strtab", flags);
	symtab = gettable(size, "symtab", flags);
	symend = (char *)symtab + size;


	do {
		if(shstrh->sh_offset < offs && shstrh->sh_offset < strh->sh_offset) {
#if 0
			/*
			 *  We would like to read the shstrtab from the file but since this
			 *  table is located in front of the shtab it is already gone. We can't
			 *  position backwards outside the current segment when using tftp.
			 *  Instead we create the names we need in the string table because
			 *  it can be reconstructed from the info we now have access to.
			 */
			if (!readtable (shstrh->sh_offset, (void *)shstrtab,
					shstrh->sh_size, "shstring", flags)) {
				return(0);
			}
#else
			memset(shstrtab, 0, shstrh->sh_size);
			strcpy(shstrtab + shstrh->sh_name, ".shstrtab");
			strcpy(shstrtab + strh->sh_name, ".strtab");
			strcpy(shstrtab + sh->sh_name, ".symtab");
#endif
			shstrh->sh_offset = 0x7fffffff;
		}

		if (offs < strh->sh_offset && offs < shstrh->sh_offset) {
			if (!(readtable(fd, offs, (void *)symtab, size, "sym", flags))) {
				return (0);
			}
			offs = 0x7fffffff;
		}

		if (strh->sh_offset < offs && strh->sh_offset < shstrh->sh_offset) {
			if (!(readtable (fd, strh->sh_offset, (void *)strtab,
					 strh->sh_size, "string", flags))) {
				return (0);
			}
			strh->sh_offset = 0x7fffffff;
		}
		if (offs == 0x7fffffff && strh->sh_offset == 0x7fffffff &&
		    shstrh->sh_offset == 0x7fffffff) {
			break;
		}
	} while(1);


	if(flags & KFLAG) {
		/*
		 *  Update the kernel headers with the current info.
		 */
		shstrh->sh_offset = (Elf32_Off)shstrtab - (Elf32_Off)keh;
		strh->sh_offset = (Elf32_Off)strtab - (Elf32_Off)keh;
		sh->sh_offset = (Elf32_Off)symtab - (Elf32_Off)keh;
		memcpy(keh, eh, sizeof(Elf32_Ehdr));
		keh->e_phoff = 0;
		keh->e_shoff = sizeof(Elf32_Ehdr);
		keh->e_phentsize = 0;
		keh->e_phnum = 0;

		printf("\nKernel debugger symbols ELF hdr @ %p", keh);

		symptr[0] = (int)keh;
		symptr[1] = roundup((int)symend, sizeof(int));

	} else {

		/*
		 *  Add all global sybols to PMONs internal symbol table.
		 */
		for (i = 0; i < nsym; i++, symtab++) {
			int type;

			dotik (4000, 0);
			if (symtab->st_shndx == SHN_UNDEF ||
			    symtab->st_shndx == SHN_COMMON) {
				continue;
			}

			type = ELF_ST_TYPE (symtab->st_info);
			if (type == STT_SECTION || type == STT_FILE) {
				continue;
			}

			/* only use globals and functions */
			if (ELF_ST_BIND(symtab->st_info) == STB_GLOBAL ||
			    type == STT_FUNC){
				if (symtab->st_name >= strh->sh_size) {
					fprintf (stderr, "\ncorrupt string pointer");
					return (0);
				}
			}
			if (!newsym (strtab + symtab->st_name, symtab->st_value)) {
				fprintf (stderr, "\nonly room for %d symbols", i);
				return (0);
			}
		}
	}
	return (1);
}

long
   load_elf (int fd, char *buf, int *n, int flags)
{
	Elf32_Ehdr *ep;
	Elf32_Phdr *phtab = 0;
	Elf32_Shdr *shtab = 0;
	unsigned int nbytes;
	int i;
	Elf32_Off highest_load = 0;

	bootseg = 0;
	myflags=flags;

#ifdef __mips__
	tablebase = PHYS_TO_CACHED(memorysize);
#else
	tablebase = memorysize;
#endif

#if NGZIP > 0

	
	lseek(fd,*n,0);	
	read(fd,buf,2);
	if(((unsigned char)buf[0]==0x1f) && ((unsigned char)buf[1]==0x8b))flags |=ZFLAG;
	else flags &=~ZFLAG;
	myflags=flags;
	lseek(fd,*n,0);	
if(myflags&ZFLAG){
	gz_open(fd);
	*n = 0;
	gz_lseek (fd, 0, SEEK_SET);
	}
#endif /* NGZIP */

	ep = (Elf32_Ehdr *)buf;
	if (sizeof(*ep) > *n) {
#if NGZIP > 0
if(myflags&ZFLAG)
		*n += gz_read (fd, buf+*n, sizeof(*ep)-*n);
else 
#endif /* NGZIP */
{
	lseek(fd,*n,0);	
	*n += read (fd, buf+*n, sizeof(*ep)-*n);
}
		if (*n < sizeof(*ep)) {
#if NGZIP > 0
if(myflags&ZFLAG)			gz_close(fd);
#endif /* NGZIP */
			return -1;
		}
	}

	/* check header validity */
	if (ep->e_ident[EI_MAG0] != ELFMAG0 ||
	    ep->e_ident[EI_MAG1] != ELFMAG1 ||
	    ep->e_ident[EI_MAG2] != ELFMAG2 ||
	    ep->e_ident[EI_MAG3] != ELFMAG3) {

#if NGZIP > 0
if(myflags&ZFLAG)		gz_close(fd);
#endif /* NGZIP */

		return (-1);
	}

	fprintf (stderr, "(elf)\n");

	{
		char *nogood = (char *)0;
#if (NELF32ONLY==0)
		if (ep->e_ident[EI_CLASS] == ELFCLASS64)
			return load_elf64(fd, buf, n, flags);
#endif

		if (ep->e_ident[EI_CLASS] != ELFCLASS32)
			nogood = "not 32-bit";
		else if (
#if BYTE_ORDER == BIG_ENDIAN
			 ep->e_ident[EI_DATA] != ELFDATA2MSB
#endif
#if BYTE_ORDER == LITTLE_ENDIAN
			 ep->e_ident[EI_DATA] != ELFDATA2LSB
#endif
			  )
			nogood = "incorrect endianess";
		else if (ep->e_ident[EI_VERSION] != EV_CURRENT)
			nogood = "version not current";
		else if (
#ifdef powerpc
			 ep->e_machine != EM_PPC
#else /* default is MIPS */
#define GREENHILLS_HACK
#ifdef GREENHILLS_HACK
			 ep->e_machine != 10 && 
#endif
			 ep->e_machine != EM_MIPS
#endif
			  )
			nogood = "incorrect machine type";

		if (nogood) {
			fprintf (stderr, "Invalid ELF: %s\n", nogood);
#if NGZIP > 0
if(myflags&ZFLAG)			gz_close(fd);
#endif /* NGZIP */
			return -2;
		}
	}

	/* Is there a program header? */
	if (ep->e_phoff == 0 || ep->e_phnum == 0 ||
	    ep->e_phentsize != sizeof(Elf32_Phdr)) {
		fprintf (stderr, "missing program header (not executable)\n");
#if NGZIP > 0
if(myflags&ZFLAG)		gz_close(fd);
#endif /* NGZIP */
		return (-2);
	}

	/* Load program header */
#if _ORIG_CODE_
	nbytes = ep->e_phnum * sizeof(Elf32_Phdr);
#else
	/* XXX: We need to figure out why it works by adding 32!!!! */
	nbytes = ep->e_phnum * sizeof(Elf32_Phdr)+32;
#endif
	phtab = (Elf32_Phdr *) malloc (nbytes);
	if (!phtab) {
		fprintf (stderr,"\nnot enough memory to read program headers");
#if NGZIP > 0
if(myflags&ZFLAG)		gz_close(fd);
#endif /* NGZIP */
		return (-2);
	}

#if NGZIP > 0
if(myflags&ZFLAG){
	if (gz_lseek (fd, ep->e_phoff, SEEK_SET) != ep->e_phoff || 
	    gz_read (fd, (void *)phtab, nbytes) != nbytes) {
		perror ("program header");
		free (phtab);
		gz_close(fd);
		return (-2);
	}
	}else
#endif /* NGZIP */
	if (lseek (fd, ep->e_phoff, SEEK_SET) != ep->e_phoff || 
	    read (fd, (void *)phtab, nbytes) != nbytes) {
		perror ("program header");
		free (phtab);
		return (-2);
	}

	/*
	 * From now on we've got no guarantee about the file order, 
	 * even where the section header is.  Hopefully most linkers
	 * will put the section header after the program header, when
	 * they know that the executable is not demand paged.  We assume
	 * that the symbol and string tables always follow the program 
	 * segments.
	 */

	/* read section table (if before first program segment) */
	if (!(flags & NFLAG) && ep->e_shoff < phtab[0].p_offset)
		shtab = elfgetshdr (fd, ep);

	/* load program segments */
	if (!(flags & YFLAG)) {
		/* We cope with a badly sorted program header, as produced by 
		 * older versions of the GNU linker, by loading the segments
		 * in file offset order, not in program header order. */
		while (1) {
			Elf32_Off lowest_offset = ~0;
			Elf32_Phdr *ph = 0;

			/* find nearest loadable segment */
			for (i = 0; i < ep->e_phnum; i++)
				if (phtab[i].p_type == PT_LOAD && phtab[i].p_offset < lowest_offset) {
					ph = &phtab[i];
					lowest_offset = ph->p_offset;
				}
			if (!ph)
				break;		/* none found, finished */

			/* load the segment */
			if (ph->p_filesz) {
#if NGZIP > 0
if(myflags&ZFLAG){
				if (gz_lseek (fd, ph->p_offset, SEEK_SET) != ph->p_offset) {
					fprintf (stderr, "seek failed (corrupt object file?)\n");
					if (shtab)
						free (shtab);
					free (phtab);
					gz_close(fd);
					return (-2);
				}
				}else
#endif /* NGZIP */
				if (lseek (fd, ph->p_offset, SEEK_SET) != ph->p_offset) {
					fprintf (stderr, "seek failed (corrupt object file?)\n");
					if (shtab)
						free (shtab);
					free (phtab);
					return (-2);
				}
				if (bootread (fd, (void *)ph->p_vaddr, ph->p_filesz) != ph->p_filesz) {
					if (shtab) free (shtab);
					free (phtab);
#if NGZIP > 0
if(myflags&ZFLAG)					gz_close(fd);
#endif /* NGZIP */
					return (-2);
				}
			}
			if((ph->p_vaddr + ph->p_memsz) > highest_load) {
				highest_load = ph->p_vaddr + ph->p_memsz;
			}
			if (ph->p_filesz < ph->p_memsz)
				bootclear (fd, (void *)ph->p_vaddr + ph->p_filesz, ph->p_memsz - ph->p_filesz);
			ph->p_type = PT_NULL; /* remove from consideration */
		}
	}

	if (flags & KFLAG) {
		highest_load = roundup(highest_load, sizeof(long));
		tablebase = highest_load;
	}
	if (!(flags & NFLAG)) {
		/* read section table (if after last program segment) */
		if (!shtab)
			shtab = elfgetshdr (fd, ep);
		if (shtab) {
			elfreadsyms (fd, ep, shtab, flags);
			free (shtab);
		}
	}

	free (phtab);
#if NGZIP > 0
if(myflags&ZFLAG)	gz_close(fd);
#endif /* NGZIP */
	return (ep->e_entry + dl_offset);
}


static ExecType elf_exec = {
	"elf",
	load_elf,
	EXECFLAGS_NONE,
};


static void init_exec __P((void)) __attribute__ ((constructor));

static void init_exec(void)
{
	/*
	 * Install ram based file system.
	 */
	exec_init(&elf_exec);
}

