#!/bin/bash
#本脚本在debian5-debian9测试没有问题,i386和amd64都可以
if ! [ -x /opt/gcc-4.3-ls232 ]  ; then
wget https://mirrors.ustc.edu.cn/loongson/loongson1c_bsp/gcc-4.3/gcc-4.3-ls232.tar.gz -c
tar zxvf gcc-4.3-ls232.tar.gz -C /opt
dpkg --add-architecture i386 #增加i386架构的libz.so.1 龙芯提供的交叉编译工具缺这个运行库
apt-get update
apt-get -y install zlib1g:i386
fi

PATH=/opt/gcc-4.3-ls232/bin:`pwd`/tools/pmoncfg:$PATH

if ! [ "`which pmoncfg`" ] ; then
cd tools/pmoncfg
make
cd ../..
fi

if ! [ "`which bison`" ] ;then
 apt-get install bison
fi

if ! [ "`which make`" ] ;then
 apt-get install make
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
