#!/bin/bash
#本脚本用于编译龙芯1C的pmon
#本脚本在debian5-debian9,ubuntu16.04 测试没有问题,i386和amd64都可以
#在win10的ubuntu子系统编译，也是可以的

arch=`dpkg --print-architecture`
if [ "_$arch" == "_amd64" ] ; then
#amd64
if ! [ -x /opt/gcc-4.9-ls232 ]  ; then
wget https://mirrors.tuna.tsinghua.edu.cn/loongson/loongson1c_bsp/gcc-4.9/gcc-4.9-ls232.tar.xz -c
if [ $? != 0 ] ; then
wget https://www.anheng.com.cn/loongson/loongson1c_bsp/gcc-4.9/gcc-4.9-ls232.tar.xz -c
fi
tar Jxvf gcc-4.9-ls232.tar.xz -C /opt
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

cd zloader.ls1b.dev
make cfg all tgt=rom CROSS_COMPILE=mipsel-linux- LANG=C
cp gzrom.bin ../pmon_ls1b_dev.bin
make cfg all tgt=ram CROSS_COMPILE=mipsel-linux- LANG=C
cp gzram ../install.1b
cd ..

commitDate=`git log -1 |grep ^Date |awk '{printf $2 " "$3" "$4" "$5" "$6}'`
commitDate=`date +%Y%m%d --date="$commitDate"`
tar cvfz pmon_ls1b_dev_${commitDate}.tar.gz readme.md install.1b pmon_ls1b_dev.bin Targets/LS1X/conf/ls1b
ls -l install.1b pmon_ls1b_dev.bin pmon_ls1b_dev_${commitDate}.tar.gz
