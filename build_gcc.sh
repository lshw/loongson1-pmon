#!/bin/bash

#生成龙芯ls232(loongson1)交叉编译工具

#下载gcc4.9.3 binutile2.24 源码从 http://cgit.loongnix.org/cgit
#git clone http://cgit.loongnix.org/cgit/gcc-4.9.3
#git clone http://cgit.loongnix.org/cgit/glibc-2.20 
#debian9-amd64 测试ok


apt install libmpc-dev libmpfr-dev libgmp-dev ccache

opt=/opt/gcc-4.9-ls232

export CFLAGS=" -Wno-error"


rm -rf $opt
mkdir -p $opt/{bin,lib,share} binutils-build
mkdir ccache
ln -s /usr/bin/ccache ccache/gcc
ln -s /usr/bin/ccache ccache/cc
ln -s /usr/bin/ccache ccache/g++
PATH=ccache:$PATH


cd binutils-build

../binutils-2.24/configure --prefix=$opt --target=mipsel-linux --with-sysroot=$opt --enable-32-bit-bfd --disable-nls --enable-shared

make configure-host
make -j 4
make install

cd ..
mkdir -p gcc-build && cd gcc-build
../gcc-4.9.3/configure --prefix=$opt --target=mipsel-linux --with-sysroot=$opt --disable-multilib --with-newlib --disable-nls --disable-shared --disable-threads --enable-languages=c --with-float=soft  --enable-static --with-abi=32
make all-gcc  -j4
make all-target-libgcc -j4
make install-gcc
make install-target-libgcc


