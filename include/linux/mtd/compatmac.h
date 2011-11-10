
/*
 * mtd/include/compatmac.h
 *
 * $Id: compatmac.h,v 1.1.1.1 2006/05/08 03:32:49 cpu Exp $
 *
 * Extensions and omissions from the normal 'linux/compatmac.h'
 * files. hopefully this will end up empty as the 'real' one 
 * becomes fully-featured.
 */


/* First, include the parts which the kernel is good enough to provide 
 * to us 
 */
   
#ifndef __LINUX_MTD_COMPATMAC_H__
#define __LINUX_MTD_COMPATMAC_H__

//#include <linux/compatmac.h>
#include <linux/types.h> /* used later in this header */
#include <linux/module.h>
#ifndef LINUX_VERSION_CODE
//#include <linux/version.h>
#endif


/* Modularization issues */
#  define REGISTER_SYMTAB(tab) /* nothing */

#  define __MODULE_STRING(s)         /* nothing */
#  define MODULE_PARM(v,t)           /* nothing */
#  define MODULE_PARM_DESC(v,t)      /* nothing */
#  define MODULE_AUTHOR(n)           /* nothing */
#  define MODULE_DESCRIPTION(d)      /* nothing */
#  define MODULE_SUPPORTED_DEVICE(n) /* nothing */


/* Other change in the fops are solved using pseudo-types */
#  define lseek_t      int
#  define lseek_off_t  off_t

/* changed the prototype of read/write */

# define count_t int
# define read_write_t int


#  define release_t int
#  define release_return(x) return (x)
#define __exit
#define __init

#define init_MUTEX(x) do {*(x) = MUTEX;} while (0)
#define RQFUNC_ARG void
#define blkdev_dequeue_request(req) do {CURRENT = req->next;} while (0)

#define __MOD_INC_USE_COUNT(mod)                                        
#define __MOD_DEC_USE_COUNT(mod)                                        




#define DECLARE_WAIT_QUEUE_HEAD(x) struct wait_queue *x = NULL
#define init_waitqueue_head init_waitqueue

static inline int try_inc_mod_count(struct module *mod)
{
	return 1;
}


/* Yes, I'm aware that it's a fairly ugly hack.
   Until the __constant_* macros appear in Linus' own kernels, this is
   the way it has to be done.
 DW 19/1/00
 */

//#include <asm/byteorder.h>
#define __LITTLE_ENDIAN

#ifndef __constant_cpu_to_le16

#ifdef __BIG_ENDIAN
#define __constant_cpu_to_le64(x) ___swab64((x))
#define __constant_le64_to_cpu(x) ___swab64((x))
#define __constant_cpu_to_le32(x) ___swab32((x))
#define __constant_le32_to_cpu(x) ___swab32((x))
#define __constant_cpu_to_le16(x) ___swab16((x))
#define __constant_le16_to_cpu(x) ___swab16((x))
#define __constant_cpu_to_be64(x) ((__u64)(x))
#define __constant_be64_to_cpu(x) ((__u64)(x))
#define __constant_cpu_to_be32(x) ((__u32)(x))
#define __constant_be32_to_cpu(x) ((__u32)(x))
#define __constant_cpu_to_be16(x) ((__u16)(x))
#define __constant_be16_to_cpu(x) ((__u16)(x))
#else
#ifdef __LITTLE_ENDIAN
#define __constant_cpu_to_le64(x) ((__u64)(x))
#define __constant_le64_to_cpu(x) ((__u64)(x))
#define __constant_cpu_to_le32(x) ((__u32)(x))
#define __constant_le32_to_cpu(x) ((__u32)(x))
#define __constant_cpu_to_le16(x) ((__u16)(x))
#define __constant_le16_to_cpu(x) ((__u16)(x))
#define __constant_cpu_to_be64(x) ___swab64((x))
#define __constant_be64_to_cpu(x) ___swab64((x))
#define __constant_cpu_to_be32(x) ___swab32((x))
#define __constant_be32_to_cpu(x) ___swab32((x))
#define __constant_cpu_to_be16(x) ___swab16((x))
#define __constant_be16_to_cpu(x) ___swab16((x))
#else
#error No (recognised) endianness defined (unless it,s PDP)
#endif /* __LITTLE_ENDIAN */
#endif /* __BIG_ENDIAN */

#endif /* ifndef __constant_cpu_to_le16 */

  #define mod_init_t int  __init
  #define mod_exit_t void  

#ifndef THIS_MODULE
#ifdef MODULE
#define THIS_MODULE (&__this_module)
#else
#define THIS_MODULE (NULL)
#endif
#endif

//#include <linux/interrupt.h>
#define spin_lock_bh(lock) do {start_bh_atomic();spin_lock(lock);}while(0);
#define spin_unlock_bh(lock) do {spin_unlock(lock);end_bh_atomic();}while(0);

#endif /* __LINUX_MTD_COMPATMAC_H__ */


