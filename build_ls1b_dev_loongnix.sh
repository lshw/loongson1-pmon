#!/bin/bash
#本脚本用于编译龙芯1B的pmon
#本脚本在debian5-debian12 ubuntu16-23 测试没有问题
#在win10的子系统编译，也是可以的
#gcc使用的是龙芯公司提供下载的gcc7.3

if ! [ -x /opt/mips-loongson-gcc7.3-linux-gnu/2019.06-29 ]  ; then
wget http://ftp.loongnix.cn/toolchain/gcc/release/mips/gcc7/mips-loongson-gcc7.3-2019.06-29-linux-gnu.tar.gz -c
cat mips-loongson-gcc7.3-2019.06-29-linux-gnu.tar.gz |bunzip2 |tar x  -C /opt
fi

PATH=`pwd`/ccache:`pwd`/tools/pmoncfg:/opt/mips-loongson-gcc7.3-linux-gnu/2019.06-29/bin:$PATH
if ! [ "`which pmoncfg`" ] ; then
apt-get -y install zlib1g make bison flex xutils-dev libc6-dev ccache
cd tools/pmoncfg
make
cd ../..
fi

cd zloader.ls1b.dev
make cfg all tgt=rom CROSS_COMPILE=mips-linux-gnu- LANG=C
cp gzrom.bin ../pmon_ls1b_dev.bin
make cfg all tgt=ram CROSS_COMPILE=mips-linux-gnu- LANG=C
cp gzram ../install.1b
cd ..

commitDate=`git log -1 |grep ^Date |awk '{printf $2 " "$3" "$4" "$5" "$6}'`
commitDate=`date +%Y%m%d --date="$commitDate"`
tar cvfz pmon_ls1b_dev_${commitDate}.tar.gz readme.md install.1b pmon_ls1b_dev.bin Targets/LS1X/conf/ls1b
ls -l install.1b pmon_ls1b_dev.bin pmon_ls1b_dev_${commitDate}.tar.gz
