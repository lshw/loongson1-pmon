#!/bin/bash
#本脚本用于编译龙芯1C的pmon
#本脚本在debian5-debian9,ubuntu16.04 测试没有问题,i386和amd64都可以
#在win10的ubuntu子系统编译，也是可以的

arch=`dpkg --print-architecture`

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
make cfg all tgt=rom CROSS_COMPILE=mipsel-linux-gnu- LANG=C
cp gzrom.bin ../pmon_ls1c_openloongson.bin
make cfg all tgt=ram CROSS_COMPILE=mipsel-linux-gnu- LANG=C
cp gzram ../install.1c
cd ..

commitDate=`git log -1 |grep ^Date |awk '{printf $2 " "$3" "$4" "$5" "$6}'`
commitDate=`date +%Y%m%d --date="$commitDate"`
tar cvfz pmon_ls1c_openloongson_${commitDate}.tar.gz flashrom.sh readme.md install.1c pmon_ls1c_openloongson*.bin Targets/LS1X/conf/ls1c
ls -l install.1c pmon_ls1c_openloongson*.bin pmon_ls1c_openloongson_${commitDate}.tar.gz
