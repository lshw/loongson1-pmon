#!/bin/bash
PATH=/opt/gcc-3.4.6-2f/bin:`pwd`/tools/pmoncfg:$PATH
#git pull

if [ "`which bison`" ] ;then
apt-get install bison
fi

if [ "`which flex`" ] ;then
apt-get install flex
fi

if [ "`which makedepend`" ] ;then
apt-get install xutils-dev
fi

make pmontools
cd zloader.ls1c.openloongson
make cfg all tgt=rom CROSS_COMPILE=mipsel-linux- LANG=C
cp gzrom.bin ..
make cfg all tgt=ram CROSS_COMPILE=mipsel-linux- LANG=C
cp gzram ..
