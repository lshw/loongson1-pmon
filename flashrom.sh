#!/bin/bash
#./flashrom.sh gzrom.bin [chipname]

if ! [ "$1" ] ; then
  echo $0 gzrom.bin [W25X40]
  exit
fi

if ! [ -e $1 ] ; then
  echo not find $1
  exit
fi
size=$( ls -l $1|awk '{print $5}' )
if [ "$2" ] ; then
 chipname="-c $2"
fi
rm -f /tmp/$1.tmp
flashrom -p ch341a_spi $chipname -r /tmp/$1.tmp
cp -f $1 /tmp/$1.resize
dd if=/tmp/$1.tmp of=/tmp/$1.resize skip=$size seek=$size bs=1
 /tmp/$1.tmp
flashrom -p ch341a_spi $chipname -w /tmp/$1.resize
