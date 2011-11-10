/* $Id: ld.script,v 1.1.1.1 2006/09/14 01:59:08 root Exp $ */
OUTPUT_FORMAT(OUT_FORMAT, "elf32-tradbigmips",
              OUT_FORMAT)
OUTPUT_ARCH(OUT_ARCH)
ENTRY(_start)
SECTIONS
{
  /* Read-only sections, merged into text segment: */
  . = STARTADDR;
  .text      : 
  {
    _ftext = . ;
    *(.text)
    *(.rodata)
    *(.rodata1)
    *(.reginfo)
    *(.init)
    *(.stub)
    /* .gnu.warning sections are handled specially by elf32.em.  */
    *(.gnu.warning)
  } =0
  _etext = .;
  PROVIDE (etext = .);
  .fini      : { *(.fini)    } =0
  .data    :
  {
    _fdata = . ;
    *(.data)
   . = ALIGN(32);
   *(.data.align32)
   . = ALIGN(64);
   *(.data.align64)
   . = ALIGN(128);
   *(.data.align128)
   . = ALIGN(4096);
   *(.data.align4096)
    CONSTRUCTORS
  }
  .data1   : { *(.data1) }
  .ctors         :
  {
                __CTOR_LIST__ = .;
                LONG((__CTOR_END__ - __CTOR_LIST__) / 4 - 2)
               *(.ctors)
                LONG(0)
                __CTOR_END__ = .;
  }
  .dtors         :
  {
                __DTOR_LIST__ = .;
                LONG((__DTOR_END__ - __DTOR_LIST__) / 4 - 2)
               *(.dtors)
                LONG(0)
                __DTOR_END__ = .;
  }
  _gp = ALIGN(16) + 0x7ff0;
  .got           :
  {
    *(.got.plt) *(.got)
   }
  /* We want the small data sections together, so single-instruction offsets
     can access them all, and initialized data all before uninitialized, so
     we can shorten the on-disk segment size.  */
  .sdata     : { *(.sdata) }
  .lit8 : { *(.lit8) }
  .lit4 : { *(.lit4) }
  _edata  =  .;
  PROVIDE (edata = .);
  __bss_start = .;
  _fbss = .;
  .sbss      : { *(.sbss) *(.scommon) }
  .bss       :
  {
   *(.dynbss)
   *(.bss)
   . = ALIGN(32);
   *(.bss.align32)
   . = ALIGN(64);
   *(.bss.align64)
   . = ALIGN(128);
   *(.bss.align128)
   . = ALIGN(4096);
   *(.bss.align4096)
   *(COMMON)
  }
  _end = . ;
  PROVIDE (end = .);
  /* These are needed for ELF backends which have not yet been
     converted to the new style linker.  */
  .stab 0 : { *(.stab) }
  .stabstr 0 : { *(.stabstr) }
  /* DWARF debug sections.
     Symbols in the .debug DWARF section are relative to the beginning of the
     section so we begin .debug at 0.  It's not clear yet what needs to happen
     for the others.   */
  .debug          0 : { *(.debug) }
  .debug_srcinfo  0 : { *(.debug_srcinfo) }
  .debug_aranges  0 : { *(.debug_aranges) }
  .debug_pubnames 0 : { *(.debug_pubnames) }
  .debug_sfnames  0 : { *(.debug_sfnames) }
  .line           0 : { *(.line) }
  /* These must appear regardless of  .  */
  .gptab.sdata : { *(.gptab.data) *(.gptab.sdata) }
  .gptab.sbss : { *(.gptab.bss) *(.gptab.sbss) }
}
