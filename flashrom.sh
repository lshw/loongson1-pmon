#!/bin/bash
#在rom文件后面补0xff ，调整大小到512k,以便flashrom 使用
cp $1 /tmp/$1.tmp
printf "%0.s\xff" {1..524288} >>/tmp/$1.tmp
dd if=/tmp/$1.tmp of=/tmp/$1.512k count=1 bs=524288
rm /tmp/$1.tmp
flashrom -p ch341a_spi -w /tmp/$1.512k
rm /tmp/$1.512k
