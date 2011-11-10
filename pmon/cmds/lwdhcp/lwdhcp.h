#ifndef			__LWDHCP__
#define			__LWDHCP__

/*
#include <sys/param.h>
#include <sys/file.h>
#include <sys/syslog.h>
#include <sys/endian.h>

#ifdef		KERNEL
#undef		KERNEL
#include <sys/socket.h>
#else
#include <sys/socket.h>
#endif
*/

//#include <sys/types.h>
#include <stdio.h>
//#include <linux/types.h>


#ifndef		PMON
	#ifndef __LINUX_TYPES_H_
	#define __LINUX_TYPES_H_

	typedef unsigned char       u8;
	typedef unsigned short      u16;
	typedef unsigned int        u32;

	typedef signed char                s8;
	typedef signed short               s16;
	typedef signed long                s32;

	typedef signed int                 sint;
	
	typedef signed long slong;
	#endif /* __LINUX_TYPES_H_ */
#else
//	typedef		uint8_t			u8;
//	typedef		uint16_t		u16;
//	typedef 	uint32_t		u32;

#endif



#define		LWDHCP_DEBUG	1

#ifdef		LWDHCP_DEBUG
	#define		DbgPrint(str, args...)		printf(str, ##args)
	#define		PERROR(str)					perror(str)
#else
	#define		DbgPrint(str, args...)
	#define		PERROR(str)
#endif


#define		LWDHCP_MESSAGE		1

#ifdef		LWDHCP_MESSAGE	
	#define		Message(str, args...)		printf(str, ##args)
#else
	#define		Message(str, args...)
#endif


struct client_config_t
{
#define		INTERFACE_MAXLEN		20
	char		interface[INTERFACE_MAXLEN];
	char		arp[16];							//store the MAC	address

	u_int32_t	addr;
	int			ifindex;
};

extern struct client_config_t 	client_config;

#endif		//	__LWDHCP__
