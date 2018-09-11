#!/bin/bash
#本脚本用于编译龙芯1B的pmon
#本脚本在debian5-debian9,ubuntu16.04测试没有问题,i386和amd64都可以
#目前不可以在win10的ubuntu子系统编译，因为它不支持i386架构
if ! [ -x /opt/gcc-4.3-ls232 ]  ; then
wget https://mirrors.ustc.edu.cn/loongson/loongson1c_bsp/gcc-4.3/gcc-4.3-ls232.tar.gz -c
tar zxvf gcc-4.3-ls232.tar.gz -C /opt
dpkg --add-architecture i386 #增加i386架构的libz.so.1 龙芯提供的交叉编译工具缺这个运行库
fi
apt-get update
apt-get -y install zlib1g:i386

PATH=/opt/gcc-4.3-ls232/bin:`pwd`/tools/pmoncfg:$PATH
#git pull

if ! [ "`which bison`" ] ;then
  install=y
fi

if ! [ "`which make`" ] ;then
  install=y
fi

if ! [ "`which flex`" ] ;then
  install=y
fi

if ! [ "`which makedepend`" ] ;then
 install=y
fi

if [ "$install" == "y" ] ; then
  apt-get update
  apt-get install -y make bison flex xutils-dev
fi

make pmontools
cd zloader.ls1b.dev
make cfg all tgt=rom CROSS_COMPILE=mipsel-linux- LANG=C
cp gzrom.bin ../pmon_ls1b_dev.bin
make cfg all tgt=ram CROSS_COMPILE=mipsel-linux- LANG=C
cp gzram ../gzram.ls1b_dev.bin
