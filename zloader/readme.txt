zloader实现pmon的gzip映象的加载.
zloader特点是完全不需修改原来的pmon代码，解压后直接跳到initmips处执行.
  make cfg     配置pmon
  make tgt=rom 生成flash的烧写的bios:gzrom
  make tgt=ram 生成可以网络加载的bios:gzram
  make tgt=zlib_rom 生成flash的烧写的bios:zlib_gzrom
  make tgt=zlib_ram 生成可以网络加载的bios:zlib_gzram
  make tgt=sim 压缩算法linux下仿真
  qiaochong
  2006.2.8
