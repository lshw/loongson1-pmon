#!/bin/bash
#本脚本用于编译龙芯1C的pmon
#本脚本在debian5-debian9,ubuntu16.04 测试没有问题,i386和amd64都可以
#目前不可以在win10的ubuntu子系统编译，因为它不支持i386架构

arch=`dpkg --print-architecture`
if [ "_$arch" == "_amd64" ] ; then
#amd64
if ! [ -x /opt/gcc-4.9-ls232 ]  ; then
wget https://mirrors.tuna.tsinghua.edu.cn/loongson/loongson1c_bsp/gcc-4.9/gcc-4.9-ls232.tar.gz -c
if [ $? != 0 ] ; then
wget https://www.anheng.com.cn/loongson/loongson1c_bsp/gcc-4.9/gcc-4.9-ls232.tar.gz -c
fi
tar zxvf gcc-4.3-ls232.tar.gz -C /opt
fi
PATH=/opt/gcc-4.9-ls232/bin:$PATH
else
#i386
if ! [ -x /opt/gcc-4.3-ls232 ]  ; then
wget https://mirrors.ustc.edu.cn/loongson/loongson1c_bsp/gcc-4.3/gcc-4.3-ls232.tar.gz -c
if [ $? != 0 ] ; then
wget https://mirrors.tuna.tsinghua.edu.cn/loongson/loongson1c_bsp/gcc-4.3/gcc-4.3-ls232.tar.gz -c
fi
tar zxvf gcc-4.3-ls232.tar.gz -C /opt
fi
PATH=/opt/gcc-4.3-ls232/bin:$PATH
fi

if ! [ -e /tmp/pmon_install.txt ] ; then
apt-get update
apt-get -y install zlib1g  make bison flex xutils-dev ccache
touch /tmp/pmon_install.txt
fi
PATH=`pwd`/ccache:`pwd`/tools/pmoncfg:$PATH

if ! [ "`which pmoncfg`" ] ; then
cd tools/pmoncfg
make
cd ../..
fi

cd zloader.ls1c.openloongson
make cfg all tgt=rom CROSS_COMPILE=mipsel-linux- LANG=C
cp gzrom.bin ../pmon_openloongson.bin
make cfg all tgt=ram CROSS_COMPILE=mipsel-linux- LANG=C
cp gzram ../install_ram.bin
ls -l ../install_ram.bin ../pmon_openloongson.bin
