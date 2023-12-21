#!/bin/bash

cd $( dirname $0 )
opt=/opt/mips-gcc-8.3

if ! [ -x binutils-gdb-2.28 ] || ! [ -x gcc-8.3/src ] ; then
wget -c http://ftp.loongnix.cn/toolchain/gcc/release/mips/gcc8/mips-loongson-gcc8-linux-gnu-2021-02-08-src.tar
tar xvf mips-loongson-gcc8-linux-gnu-2021-02-08-src.tar &
wget -c http://ftp.loongnix.cn/toolchain/gcc/release/mips/gcc7/mips-loongson-gcc7-linux-gnu-2021-02-08-src.tar
tar xvf mips-loongson-gcc7-linux-gnu-2021-02-08-src.tar
tar Jxvf mips-loongson-gcc7-linux-gnu-2021-02-08-src/binutils-gdb-2.28.tar.xz
wait
tar Jxvf mips-loongson-gcc8-linux-gnu-2021-02-08-src/gcc-8.3.tar.xz
fi

export CFLAGS=" -Wno-error"

rm -rf $opt binutils-build gcc-build
mkdir -p $opt/{bin,lib,share} binutils-build
PATH=ccache:$PATH

cd binutils-build

../binutils-gdb-2.28/configure --prefix=$opt --target=mipsel-linux-gnu --with-sysroot=$opt --enable-32-bit-bfd --disable-nls --enable-shared

make configure-host
make -j10 
make install

cd ..
mkdir -p gcc-build8 && cd gcc-build8
../gcc-8.3/src/configure --prefix=$opt --target=mipsel-linux-gnu --with-sysroot=$opt --disable-multilib --with-mpfr --with-newlib --disable-nls --disable-shared --disable-threads --enable-languages=c --with-float=soft  --enable-static --with-abi=32
make all-gcc  -j10
make all-target-libgcc -j10
make install-gcc
make install-target-libgcc

cd /opt
tar cvfJ $(dirname $(realpath $0))/mips_gcc8.3_binutils2.28.tar.xz $(basename $opt)
cd $(dirname $0)
ls -l mips_gcc8.3_binutils2.28.tar.xz
