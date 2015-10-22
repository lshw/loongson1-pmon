本项目，来自与龙芯广州分公司为龙芯俱乐部提供的龙芯1C开龙主板提供的pmon源码 
 
安装软件： 
 aptitude install bison flex xutils-dev 
编译安装pmoncfg文件: 
 cd tools/pmoncfg 
make pmoncfg 
make install 
 
编译：  
 pmon-ls1x-openloongson源码目录下  
 cd zloader.ls1c  
 make cfg all tgt=rom CROSS_COMPILE=mipsel-linux-  
  
配置文件  
 pmon-ls1x-openloongson/Targets/LS1X/conf/ls1c  
 该配置文件内容与ls1c_300a_openloongson相同，ls1c_300a_openloongson是该开发板的备份配置文件  
  
nand flash分区：  
  在pmon-ls1x-openloongson/Targets/LS1X/dev/ls1x_nand.c文件中设置分区  
  
  #if defined(LS1CSOC)  
	add_mtd_device(ls1x_mtd, 0, 1024*1024, "bootloader");  
	add_mtd_device(ls1x_mtd, 1024*1024, 13*1024*1024, "kernel");  
	add_mtd_device(ls1x_mtd, 14*1024*1024, 50*1024*1024, "rootfs");  
	add_mtd_device(ls1x_mtd, (50+14)*1024*1024, 64*1024*1024, "data");  
  分区为：  
  bootloader 1MByte  保留给nand启动用  
  kernel 13MByte  用于烧录内核    
  rootfs 50MByte  用于烧录根文件系统  
  data 64MByte 可以用作其他  
  
  注意分区大小要与linux kernel中的一致  
  bootloader分区保留给nand启动用，所以pmon烧录内核和根文件系统的命令变为：  
  devcp tftp://192.168.1.3/vmlinux /dev/mtd1  
  mtd_erase /dev/mtd2  
  devcp tftp://192.168.1.3/rootfs-yaffs2.img /dev/mtd2 yaf nw  
  就是/dev/mtd0不用，注意不要烧录错  
  
  如果用spiflash启动，不用nand启动的话可以根据自己使用的情况修改分区，注意linux内核中也要修改。  
