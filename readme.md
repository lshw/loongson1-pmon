本项目，来自于龙芯广州分公司为龙芯俱乐部提供的龙芯1C开龙主板提供的pmon源码  
  
编译：  
 git clone https://github.com/lshw/loongson1-pmon
 cd loongson1-pmon
 ./build_openloongson.sh
 执行 build_openloongson.sh 生成2个文件:gzram.* pmon_*.bin 
  
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

龙芯1C的u盘刷机文件名是autoexec.bat ,里面可以放置pmon命令，会在开机时自动执行， 第一行为版本号，用于防止重复刷机



另外， 此源码也可用于龙芯1B开发板，执行build_ls1b_dev.sh ,生成龙芯1B的pmon，
龙芯1B的u盘刷机文件名是autoexec.1b ,里面可以放置pmon命令，会在开机时自动执行， 第一行为版本号，用于防止重复刷机

