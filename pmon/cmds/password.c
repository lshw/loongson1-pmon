
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/param.h>
#include <sys/types.h>
#include <sys/errno.h>
#include <sys/endian.h>

#include <pmon.h>
#include <exec.h>
#include <pmon/loaders/loadfn.h>

#include <termio.h>
#include <endian.h>

#include <signal.h>
#include <setjmp.h>
#include <ctype.h>

#ifdef _KERNEL
#undef _KERNEL
#include <sys/ioctl.h>
#define _KERNEL
#else
#include <sys/ioctl.h>
#endif

#ifdef __mips__
#include <machine/cpu.h>
#endif

#include <md5.h>
//#include <window.h>

#include "pflash.h"
#include "dev/pflash_tgt.h"

#define FLASH_OFFS (tgt_flashmap()->fl_map_size - 0x2000)
#define PASSRAM_SIZE 105
#define PASSRAM_OFFS 0

#define USE_USER_PWD 1 << 0
#define SET_USER_PWD 1 << 1
#define USE_ADMI_PWD 1 << 2
#define SET_ADMI_PWD 1 << 3
#define USE_SYS_PWD 1 << 4
#define SET_SYS_PWD 1 <<5

#define LENGTH 34
	
extern struct fl_map *tgt_flashmap();

const Optdesc cmd_password_opts[]=
{
	{"-s <newpwd>", "set net new password \"pwd\"!"},
	{"-u <user|admin>", "set password type, user or admin."},
	{"-p <0|1>", "set useable."},
	{"-c", "clear current password."},
	{0}
};

#define CLEAR_PWD 1 << 0
#define SET_PWD 1 << 1
#define NEW_PWD 1 << 2
#define USE_PWD 1 << 3

/*
 *  Calculate checksum. If 'set' checksum is calculated and set.
 */
static int
cksum(void *p, size_t s, int set)
{
	u_int16_t sum = 0;
	u_int8_t *sp = p + 3;
	int sz = s / 2;

	if(set) {
		*(u_int8_t *)p = 0;
		*((u_int8_t *)p+1) = 0;
	}
	while(sz--) {
		sum += (*sp++) << 8;
		sum += *sp++;
	}
	if(set) {
		sum = -sum;
		*(u_int8_t *)p = sum >> 8;
		*((u_int8_t *)p+1) = sum;
		//printf("%d, %d\n", sum>>8, sum);
	}
	return(sum);
}

int pwd_clear(char *user)
{
    char buf[PASSRAM_SIZE + 1];
    char* passram;
    u_int8_t flag;
    passram = tgt_flashmap()->fl_map_base + FLASH_OFFS;
    memcpy(buf, passram , PASSRAM_SIZE);
    if (strcmp(user, "all") == 0)
    {
        bzero(buf, PASSRAM_SIZE);
    }
    else if (strcmp(user, "user") == 0)
    {
        memset(buf+3, 0, 34);
	cksum(buf, PASSRAM_SIZE - 3, 1);
        flag = buf[2];
        flag &= ~USE_USER_PWD;
        flag &= ~SET_USER_PWD;
        buf[2] = flag;
    }
    else if (strcmp(user, "admin") == 0)
    {
        memset(buf+37, 0, 34);
	cksum(buf, PASSRAM_SIZE - 3, 1);
        flag = buf[2];
        flag &= ~USE_ADMI_PWD;
        flag &= ~SET_ADMI_PWD;
        buf[2] = flag;
    }
    else if (!strcmp(user,"sys"))
    {
        memset(buf+71, 0, 34);
	cksum(buf, PASSRAM_SIZE - 3, 1);
        flag = buf[2];
        flag &= ~USE_SYS_PWD;
        flag &= ~SET_SYS_PWD;
        buf[2] = flag;
    	
    }
    fl_program((void *)passram, (void *)buf, PASSRAM_SIZE, 0);
    return TRUE;
}

int pwd_exist(void)
{
    char* passram;
    char buff[PASSRAM_SIZE + 1];
    char buff1[PASSRAM_SIZE + 1];
    int i = 0;

    memset(buff, 0, sizeof(buff));
    memset(buff1, 0, sizeof(buff1));

    passram = tgt_flashmap()->fl_map_base + FLASH_OFFS;

    memcpy(buff, passram, PASSRAM_SIZE);
    memcpy(buff1, buff, PASSRAM_SIZE);
    cksum(buff, PASSRAM_SIZE - 3, 1);

    if (strncmp(buff, buff1, PASSRAM_SIZE) != 0)
    {
    	buff[2] = 0;
	fl_program((void *)passram, (void *)buff, PASSRAM_SIZE, 0);
        return 0;
    }

    return 1;
}


static void print_password()
{
	char* passram;
	char buff[PASSRAM_SIZE + 1];
	char buff1[PASSRAM_SIZE + 1];
	int i = 0;
	
	memset(buff, 0, sizeof(buff));
	memset(buff1, 0, sizeof(buff1));
	
	passram = tgt_flashmap()->fl_map_base + FLASH_OFFS;

	memcpy(buff, passram, PASSRAM_SIZE);
	memcpy(buff1, buff, PASSRAM_SIZE);

	printf("buff1[%x,%x,%x,", buff1[0], buff1[1], buff1[2]);
	for (i = 3; i < PASSRAM_SIZE; i++)
	{
		printf("%c", buff1[i]);
	}
	printf("]\n");

	cksum(buff, PASSRAM_SIZE - 3, 1);
	printf("buff[%x,%x,%x,", buff[0], buff[1], buff[2]);
	for (i = 3; i < PASSRAM_SIZE; i++)
	{
		printf("%c", buff[i]);
	}
	printf("]\n");
}

int user_index(char* user)
{
    if(!strcmp(user,"user"))
    	return 0;
    if(!strcmp(user,"admin"))
    	return 1;
    if(!strcmp(user,"sys"))
    	return 2;
}

int pwd_set(char *user,char* npwd)
{
    char* passram;
    char buf[PASSRAM_SIZE + 1];
    u_int8_t flag;
    char crypted[50];
    int index = user_index(user);

    passram = tgt_flashmap()->fl_map_base + FLASH_OFFS;

    strcpy(crypted, "$1$abcdefgh$");
    if (make_md5_password(npwd, crypted) != 0)
    {
        printf("make password error\n");
        return -1;
    }

    memset(buf, 0, sizeof(buf));
    memcpy(buf, passram, PASSRAM_SIZE);
    memcpy(buf + 3+LENGTH*index, crypted, strlen(crypted));
    cksum(buf, PASSRAM_SIZE - 3, 1);
    printf("%s\n", buf +3);

    flag = buf[2];
    flag |= 1<<(index*2+1);
    buf[2] = flag;
    fl_program((void *)passram, (void *)buf, PASSRAM_SIZE, 0);
}

int pwd_cmp(char *user,char* npwd)
{
    char* passram;
    char buf[PASSRAM_SIZE + 1];
    u_int8_t flag;
    char crypted[50];
    int index = user_index(user);

    memset(buf, 0, sizeof(buf));

    passram = tgt_flashmap()->fl_map_base + FLASH_OFFS + 3+LENGTH*index;

    memcpy(buf, passram, 34);

    if (check_md5_password(npwd, buf) == 0)
    {
        return TRUE;
    }
    else
        return FALSE;
}


void pwd_set_used(char *user,int used)
{
    char* passram;
    char buf[PASSRAM_SIZE + 1];
    u_int8_t flag;
    int index = user_index(user);

    passram = tgt_flashmap()->fl_map_base + FLASH_OFFS;
    memcpy(buf, passram, PASSRAM_SIZE);

    flag = buf[2];
    if (used)
        flag |= 1<<(index*2);
    else
        flag &= ~(1<<(index*2));
    buf[2] = flag;
    fl_program((void *)passram, (void *)buf, PASSRAM_SIZE, 0);
}


int pwd_is_used(char *user)
{
    char* passram;
    char buf[PASSRAM_SIZE + 1];
    u_int8_t flag;
    int index = user_index(user);

    passram = tgt_flashmap()->fl_map_base + FLASH_OFFS;
    flag = *(passram + 2);

    if (flag & (1<<(index*2)))
        return TRUE;
    return FALSE;
}

int pwd_is_set(char *user)
{
    char* passram;
    char buf[PASSRAM_SIZE + 1];
    u_int8_t flag;
    int index = user_index(user);

    passram = tgt_flashmap()->fl_map_base + FLASH_OFFS;
    //flag = passram + 2;
    memcpy(buf, passram, PASSRAM_SIZE);

    flag = buf[2];
    if (flag & (1<<(index*2+1)))
        return TRUE;
    return FALSE;
}



int cmd_password (int ac, char *av[])
{
    int flags,c,err;
    extern int optind;
    extern char *optarg;
    char npwd[25] = {0};
    char rnpwd[25] = {0};
    char pwd[25] = {0};
    char user[5] = "user";
    char pval[2] = {0};

    flags = 0;
    optind = err = 0;
    while ((c = getopt (ac, av, "scu:p:z")) != EOF) 
    {
        switch (c) 
        {
            case 'c':
                flags |= CLEAR_PWD; 
                break;
            case 's':
                flags |= NEW_PWD; 
                break;
            case 'u':
                strcpy(user, optarg);
                break;
            case 'p':
                flags |= USE_PWD;
                pval[0] = *optarg;
                break;
	    case 'z':
	    	pwd_clear("all");	
		break;
            default:
                err++;
                break;
        }
    }

    if (err) {
        return EXIT_FAILURE;
    }

    /*
    if (optind < ac )
    {
        strcpy(pwd, av[optind++]);
        flags |= SET_PWD;
    } 

    if (flags & CLEAR_PWD)
    {
        pwd_clear(user);
        return 0;
    }
    */

    /* 判断是否已经初始化密码 
    if (pwd_exist() == 0)
    {
        if (!sflag)
        {
            //TODO
            pwd_clear("all");
            printf("no password, please set!\n");
            return EXIT_FAILURE;
        }
    }
    */

    {
        if (pwd_exist() && pwd_is_set(user))
        {
            if (!(flags & SET_PWD))
            {
                printf("(current) BIOS %s Password: ",user);
                scanf("%s", pwd);
            }
            putchar('\n');
            if (pwd_cmp(user,pwd))
            {
                printf("Current BIOS Admin Password is correct!\n");
                if (flags & CLEAR_PWD)
                {
                    pwd_clear(user);
                    printf("Empty the current BIOS %s Password!\n",user);
                }
                else if (flags & USE_PWD)
                {
                    pwd_set_used(user,atoi(pval));
                }
                else
                {
                        printf("New BIOS %s Password: ",user);
                        scanf("%s", npwd);
                        putchar('\n');
                        printf("Retype new BIOS %s Password: ",user);
                        scanf("%s", rnpwd);
                        putchar('\n');
                        if((strcmp(npwd, rnpwd) == 0) && (strlen(npwd) != 0))
                        {
                            pwd_set(user,npwd);
                            printf("New BIOS %s Password is set!\n",user);
                        }
                        else
                        {
                            printf("Sorry, passwords do not match or invalid.\n");
                        }
                }

            }
            else
                printf("You enter the password is incorrect!\n");

        }
        else
        {
            if (flags & NEW_PWD)
            {
                pwd_set(user,pwd);
                printf("New BIOS %s Password is set!\n",user);
            }
            else
                printf("BIOS %s Password is not set!\n",user);
        }
    }

    print_password();	
    return EXIT_SUCCESS;

}



/*
 *  Command table registration
 *  ==========================
 */

static const Cmd PasswordCmd[] =
{
    {"RAYS Commands for PMON 2000"},
    {"password",	"[-c|-s <newpwd>] [currentpwd]",
        cmd_password_opts,
        "Set pmon password.",
        cmd_password, 0, 99, CMD_ALIAS},
    {0, 0}
};

static void init_cmd __P((void)) __attribute__ ((constructor));

    static void
init_cmd()
{
    cmdlist_expand(PasswordCmd, 1);
}

