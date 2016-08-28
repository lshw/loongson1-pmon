#!/bin/bash
#本脚本在debian5-debian9测试没有问题
if ! [ -x /opt/gcc-3.4.6-2f ]  ; then
wget http://mirrors.ustc.edu.cn/loongson/loongson1b/Tools/toolchain/gcc-3.4.6-2f.tar.gz -c
tar zxvf gcc-3.4.6-2f.tar.gz
mkdir -p /opt
mv gcc-3.4.6-2f /opt
fi

PATH=/opt/gcc-3.4.6-2f/bin:`pwd`/tools/pmoncfg:$PATH

if ! [ "`which bison`" ] ;then
 apt-get install bison
fi

if ! [ "`which flex`" ] ;then
 apt-get install flex
fi

make all tgt=rom

cd zloader.ls1c.openloongson
make cfg all tgt=rom CROSS_COMPILE=mipsel-linux- LANG=C
cp gzrom.bin ../pmon_openloongson.bin
make cfg all tgt=ram CROSS_COMPILE=mipsel-linux- LANG=C
cp gzram ../gzram.openloongson
