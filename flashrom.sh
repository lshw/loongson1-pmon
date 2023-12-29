#!/bin/bash
#在rom文件后面补0xff ，调整大小到512k,以便flashrom 使用
#./flashrom.sh gzrom.bin [n] [chipname]
if ! [ "$2" ] ;then
n=1 #//16:8M  2:1M
else
n=$2
fi
size=$(( 524288*$n ))
echo $size
cp $1 /tmp/$1.tmp
dd if=/dev/zero count=524288 bs=$n |sed 's/\x00/\xff/g' >>/tmp/$1.tmp 2>/dev/null
ls -l /tmp/$1.tmp
dd if=/tmp/$1.tmp of=/tmp/$1.resize count=524288 bs=$n >/dev/null 2>/dev/null
rm /tmp/$1.tmp
if [ "$3" ] ; then
chipname="-c $3"
fi
flashrom -p ch341a_spi $chipname -w /tmp/$1.resize
