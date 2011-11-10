#include "autoconf.h"
#include <sys/device.h>
#include <sys/param.h>
struct loopdev_softc {
    /* General disk infos */
    struct device sc_dev;
    char dev[64];
    int fd,bs,seek,count,access;
#if NGZIP > 0
    int unzip;
#endif
};
