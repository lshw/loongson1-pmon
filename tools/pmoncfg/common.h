#ifndef __COMMON_H__
#define __COMMON_H__

#ifndef major
#define major(x)        ((int32_t)(((u_int32_t)(x) >> 8) & 0xff))
#endif
#ifndef minor
#define minor(x)        ((int32_t)((x) & 0xff))
#endif
#ifndef makedev
#define makedev(x,y)    ((dev_t)(((x) << 8) | (y)))
#endif

#ifndef NODEV
#define NODEV -1
#endif

#endif /* __COMMON_H__ */
