/* $Id: exec_srec.c,v 1.1.1.1 2006/02/23 23:08:56 root Exp $ */

/*
 * Copyright (c) 2000 Opsycon AB  (www.opsycon.se)
 * Copyright (c) 2002 Patrik Lindergren (www.lindergren.com)
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
 *	This product includes software developed by Patrik Lindergren.
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

#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <exec.h>

#include <pmon.h>
#include <pmon/loaders/loadfn.h>

#include "mod_debugger.h"
#include "mod_load.h"
#include "mod_symbols.h"
//#include "gzip.h"
#define NGZIP 0
#if NGZIP > 0
#include <gzipfs.h>
#endif /* NGZIP */

static long   load_wince (int fd, char *buf, int *n, int flags);


/*************************************************************
 * Read a NK.bin(WinCE kernel)
 *
 *
 **************************************************************/
#define HEAD_LEN           15

unsigned char cehead[HEAD_LEN];

struct ce_head{
    unsigned int StartAddress;
    unsigned int Length;
};

struct record_head{    
    unsigned int LoadAddress;
    unsigned int Length;
    unsigned int CheckSum;
};

int check_bin_head (void)
{
    /****************************************
     * 7 bytes are b, 0, 0, 0, f, f ,\n
     * 0x4230303046460A
     ****************************************/


    if (cehead[0] == 0x42 &&
	cehead[1] == 0x30 &&
	cehead[2] == 0x30 &&
	cehead[3] == 0x30 &&
	cehead[4] == 0x46 &&
	cehead[5] == 0x46 &&
	cehead[6] == 0xA)
	return 0;

    return 1;

}

int
check_sum(
    unsigned char * buf,
    int len,
    unsigned int checksum)
{
    unsigned int count,i;

    for (i = 0,count = 0 ; i < len ; i++)
	count += buf[i];

    if (count == checksum)
	return 0;

    return 1;
}


#if 0

UINT32
load_flash(UINT32 address,
	UINT32 *start_address)
{
    int i;
    struct ce_head ce_hdr;
    struct record_head rec_hdr;
    
       flash_read(cehead,(char *)address,HEAD_LEN);

    if(check_bin_head( ))
	return -1;
    
    address += HEAD_LEN;
    memcpy((char*)(&ce_hdr.StartAddress),(char*)(cehead+7),8);
    
 
    printf("CE start address:=0x%x,length=0x%x\n",ce_hdr.StartAddress,ce_hdr.Length);

    *start_address = ce_hdr.StartAddress;

    //get first record option
    flash_read((unsigned char *)&rec_hdr.LoadAddress,(unsigned char *)address,sizeof(struct record_head));
    address += sizeof(struct record_head);

    i = rec_hdr.LoadAddress - *start_address;

    
    while ( i < ce_hdr.Length){
	i = rec_hdr.LoadAddress + rec_hdr.Length - *start_address;
	printf("Load ..address = 0x%08x,length=0x%08x,",rec_hdr.LoadAddress,rec_hdr.Length);	
	printf("had read Length = 0x%08x\r",i);
	
	//get record content
	flash_read((unsigned char*)(rec_hdr.LoadAddress),(unsigned char *)address,rec_hdr.Length);
	address += rec_hdr.Length;
	  
	if (check_sum((unsigned char*)(rec_hdr.LoadAddress),rec_hdr.Length,rec_hdr.CheckSum)){
	    printf("CheckSum Error \n");
	    return -1;
	}
	
	//get a record option
	flash_read((unsigned char *)&rec_hdr.LoadAddress,(unsigned char *)address,sizeof(struct record_head));
	address += sizeof(struct record_head);

	//NK.bin end
	if ( rec_hdr.LoadAddress == 0)
	    break;
	
    }
    printf("\n");
    return 0;
}

#endif

/* have *n byte(s) be read to buf
 *
 *
 */


static long
   load_wince (int fd, char *buf, int *n, int flags)
{

	int count = 0;
	int l, stat;
	struct ce_head ce_hdr;
	struct record_head rec_hdr;
	int r_count;


	/* wince have 7 bytes head */
	lseek(fd,*n,0);	
	read(fd,cehead,7);

	if (cehead[0] != 0x42)
	  {
	    return -1;
	  }

       

#if NGZIP > 0
	gz_open(fd);
	*n = 0;
	if (gz_lseek (fd, 0, SEEK_SET) != 0 ) {
	  	
		gz_close(fd);
		return -1;
	}
#else
#endif /* NGZIP */



	if(check_bin_head()) {
#if NGZIP > 0
		gz_close(fd);
#endif /* NGZIP */
		return -1;
	}
	
	fprintf (stderr, "(wince NK.bin)  \n");
	

	l = read(fd,&ce_hdr,8);
	if (l != 8)
	  return -1;
	
	fprintf(stderr,"CE start address:=0x%x,length=0x%x\n",ce_hdr.StartAddress,ce_hdr.Length);
	

	dl_entry = ce_hdr.StartAddress;

    //get first record option
	read(fd,(unsigned char *)&rec_hdr.LoadAddress,sizeof(struct record_head));
  

	count = rec_hdr.LoadAddress - dl_entry;

    
    while ( count < ce_hdr.Length){
	count = rec_hdr.LoadAddress + rec_hdr.Length - dl_entry;
	fprintf(stderr,"Load ..address = 0x%08x,length=0x%08x,",rec_hdr.LoadAddress,rec_hdr.Length);	
	fprintf(stderr,"had read Length = 0x%08x\r",count);
	
	//get record content
	read(fd,(unsigned char*)(rec_hdr.LoadAddress),rec_hdr.Length);
	  
	if (check_sum((unsigned char*)(rec_hdr.LoadAddress),rec_hdr.Length,rec_hdr.CheckSum)){
	  fprintf(stderr,"CheckSum Error \n");
	    return -1;
	}
	
	//get a record option
	read(fd,(unsigned char *)&rec_hdr.LoadAddress,sizeof(struct record_head));

	//NK.bin end
	if ( rec_hdr.LoadAddress == 0)
	    break;
	
    }



#if NGZIP > 0
	gz_close(fd);
#endif /* NGZIP */

       
	printf("\b\b, loaded %d bytes", count);
	return(dl_entry);

}



static ExecType wince_exec =
{
	"wince",
	load_wince,
	EXECFLAGS_NONE,
};


static void init_exec __P((void)) __attribute__ ((constructor));

static void
   init_exec()
{
	/*
	 * Install ram based file system.
	 */
 
	exec_init(&wince_exec);
}

