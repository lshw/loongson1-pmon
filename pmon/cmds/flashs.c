#include <machine/pio.h>
#include <stdio.h>
#include <pmon.h>
#include <cpu.h>

#include <pflash.h>
#include <flash.h>

static unsigned long flashs_rombase;

//extern union commondata;
//extern int (*syscall1)(int type,long long addr,union commondata *mydata);
//extern int (*syscall2)(int type,long long addr,union commondata *mydata);

#if NMOD_FLASH_ST
#include <include/pflash.h>
static int erase(int argc, char **argv)
{
	struct fl_map *map;
	int offset;

	if (argc != 2)
		return -1;
	map = tgt_flashmap();
	map++;
	printf("%x,%d,%d,", map, map->fl_map_width, map->fl_map_bus);
	offset = strtoul(argv[1], 0, 0);
	printf(",%x\n", offset);
	fl_erase_sector_st(map, 0, offset);
	return 0;
}

static int program(int argc, char **argv)
{
	struct fl_map *map;
	int offset;
	int i;

	if (argc != 3)
		return -1;
	map = tgt_flashmap();
	map++;
	printf("%x,%d,%d,", map, map->fl_map_width, map->fl_map_bus);
	offset = strtoul(argv[1],0,0);
	for (i=0; i<strlen(argv[2]); i=i+2)
		fl_program_st(map, 0, offset+i, &argv[2][i]);

	return 0;
}
#endif

#if defined(NMOD_FLASH_ST)&&defined(FLASH_ST_DEBUG)
#include "c2311.c"
int cmd_testst(void)
{
	ParameterType fp; /* Contains all Flash Parameters */
	ReturnType rRetVal; /* Return Type Enum */
	Flash(ReadManufacturerCode, &fp);
	printf("Manufacturer Code: %08Xh\r\n",
	fp.ReadManufacturerCode.ucManufacturerCode);
	Flash(ReadDeviceId, &fp);
	printf("Device Code: %08Xh\r\n",
	fp.ReadDeviceId.ucDeviceId);
	fp.BlockErase.ublBlockNr = 10; /* block number 10 will be erased */
	rRetVal = Flash(BlockErase, &fp); /* function execution */
	return rRetVal;
} /* EndFunction Main*/
#endif

static int rom_read(int type, long long addr, union commondata *mydata)
{
	memcpy(&mydata->data1, (long)(flashs_rombase+addr), type);
	return 0;
}

static int rom_write(int type, long long addr, union commondata *mydata)
{
	char *nvrambuf;
	char *nvramsecbuf;
	char *nvram;
	int offs;
	struct fl_device *dev = fl_devident(flashs_rombase, 0);
	int nvram_size = dev->fl_secsize;

	nvram = flashs_rombase + addr;
	if(fl_program_device(nvram, &mydata->data1, type, FALSE)) {
		return -1;
	}

	if (bcmp(nvram, &mydata->data1, type)) {
		offs = (int)nvram &(nvram_size - 1);
		nvram = (int)nvram & ~(nvram_size - 1);

		/* Deal with an entire sector even if we only use part of it */

		/* If NVRAM is found to be uninitialized, reinit it. */

		/* Find end of evironment strings */
		nvramsecbuf = (char *)malloc(nvram_size);

		if (nvramsecbuf == 0) {
			printf("Warning! Unable to malloc nvrambuffer!\n");
			return(-1);
		}

		memcpy(nvramsecbuf, nvram, nvram_size);
		if (fl_erase_device(nvram, nvram_size, FALSE)) {
			printf("Error! Nvram erase failed!\n");
			free(nvramsecbuf);
			return(0);
		}

		nvrambuf = nvramsecbuf + offs;

		memcpy(nvrambuf, &mydata->data1, type);

		if (fl_program_device(nvram, nvramsecbuf, nvram_size, FALSE)) {
			printf("Error! Nvram program failed!\n");
			free(nvramsecbuf);
			return(0);
		}
		free(nvramsecbuf);
	}
	return 0;
}

static int flashs(int ac, char **av)
{
	struct fl_device *dev;

	if (ac != 2)
		return -1;
	flashs_rombase = strtoul(av[1], 0, 0);
	dev = fl_devident(flashs_rombase, 0);
	if (!dev) {
		printf("can not find flash\n");
		return -1;
	}
	else {
		syscall1 = rom_read;
		syscall2 = rom_write;
		syscall_addrtype = 0;
	}
	return 0;
}


static const Cmd flashs_cmds[] = {
	{"MyCmds"},
	{"flashs",	"rom", 0, "select flash for read/write", flashs, 0, 99, CMD_REPEAT},
#if NMOD_FLASH_ST
	{"erase","[0 1]",0,"cache [0 1]",erase,0,99,CMD_REPEAT},
	{"program","[0 1]",0,"cache [0 1]",program,0,99,CMD_REPEAT},
#ifdef FLASH_ST_DEBUG
	{"testst","n",0,"",cmd_testst,0,99,CMD_REPEAT},
#endif
#endif
	{0, 0}
};

static void flashs_init_cmd __P((void)) __attribute__ ((constructor));

static void flashs_init_cmd(void)
{
	cmdlist_expand(flashs_cmds, 1);
}
