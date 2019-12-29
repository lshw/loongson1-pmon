本项目，来自于龙芯广州分公司为龙芯俱乐部提供的龙芯1C智龙主板提供的pmon源码,适用于龙芯1B  
build_ls1c_openloongson.sh  一键编译龙芯1C智龙v2/v3 pmon  
build_ls1b_dev.sh 一键编译龙芯1B开发板的pmon  
build_gcc.sh 一键编译龙芯的交叉开发环境gcc-4.9  

编译：  
 git clone https://github.com/lshw/loongson1-pmon  
 cd loongson1-pmon  
 ./build_ls1c_openloongson.sh  
 执行 build_ls1c_openloongson.sh 生成2个文件:gzram.* pmon_*.bin  
 
配置文件  
 pmon-ls1x-openloongson/Targets/LS1X/conf/ls1c  
 该配置文件内容与ls1c_300a_openloongson相同，ls1c_300a_openloongson是该开发板的备份配置文件  
  
nand flash分区:
  若分区设为：  
  kernel 起始于2MByte，大小20M  用于内核    
  rootfs 起始于22MByte,大小106MByte  用于根文件系统  
 1.新的方式:  
  在pmon命令行下用set 命令设置，兼容 linux的mtdparts方式。
  set mtdparts ls1x-nand:20M@2M(kernel),106M@22M(rootfs)  
  新的方式，分区表会传递给linux kernel ，需要linux kernel支持mtdparts  
 2.老的方式:  
  老的方式，注意分区大小需要与linux kernel中的一致  
  修改 Targets/LS1X/dev/ls1x_nand.c  
#ifndef MTDPARTS
	add_mtd_device(ls1x_mtd, 2*1024*1024, 20*1024*1024, "kernel");
	add_mtd_device(ls1x_mtd, 22*1024*1024, 106*1024*1024, "rootfs");
  

刷写内核和文件系统命令如下:  
  devcp tftp://192.168.1.3/vmlinux /dev/mtd0  
  mtd_erase /dev/mtd1 
  devcp tftp://192.168.1.3/rootfs-yaffs2.img /dev/mtd1 yaf nw  

  如果用spiflash启动，不用nand启动的话可以根据自己使用的情况修改分区，注意linux内核中也要修改。 

龙芯1C的u盘刷机文件名是autoexec.bat ,里面可以放置pmon命令，会在开机时自动执行， 第一行为版本号，用于防止重复刷机  



另外， 此源码也可用于龙芯1B开发板，执行build_ls1b_dev.sh ,生成龙芯1B的pmon，  
龙芯1B的u盘刷机文件名是autoexec.1b ,里面可以放置pmon命令，会在开机时自动执行， 第一行为版本号，用于防止重复刷机  

为了防止误刷，并强制测试新版本的rom， 屏蔽了原来的刷机指令 load -rf bfc00000, 只在ram版本的install.1b,install.1c里可以使用pmon升级指令， 需要先boot /dev/fs/fat@usb0/install.1c 先运行ram版本的新版本，运行正常后， 在新版本的ram版pmon下，可以使用load -rf bfc00000 /dev/fs/fat@usb0/pmon_ls1c_openloongson.bin 进行升级。      

有问题， 请到github给我留言 https://github.com/lshw  ，或发邮件到liushiwei@gmail.com  
