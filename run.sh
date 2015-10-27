#!/bin/bash
PATH=/opt/gcc-3.4.6-2f/bin:$PATH
#git pull
cd zloader.ls1c.openloongson
make cfg all tgt=ram CROSS_COMPILE=mipsel-linux- LANG=C
cp gzram ..

make cfg all tgt=rom CROSS_COMPILE=mipsel-linux- LANG=C
cp gzrom.bin ..
