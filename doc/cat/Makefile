# File: Makefile
# Purpose: Principle Makefile for PMON and IMON
# Author: Phil Bunce (pjb@carmel.com)
# Revision History:
#	970304	Removed msim-related stuff
#	970304	Removed stuff to build separate imon tape
#	970317	Added stuff for global version file
#	970317	Added *-release targets
#	970318	Added *-only targets
#	980410	Changed bsps to bsps.old, and bsps2 to bsps.
#	980901	Added ALL_TARG so as to not build imon w sdp1000.

.SUFFIXES: .lst .tar .exe .zip
include defines.mk

# This is the top level Makefile for the PMON source package. 

# To make everything, type 'make all' in this directory. Make builds the
# directories in the following order:
#
#	1. The programs in the tools directory using your host's native 
#	   C compiler. 
#
#	2. The modules in the lib directory using your MIPS-targetted 
#	   compiler.  After they have been compiled and assembled the 
#	   modules are archived to create a library. As many as four
#	   separate libraries may be created.
#
#		lib/bg/libc.a	-- big endian, no gp refs
#		lib/bo/libc.a	-- big endian, use gp refs
#		lib/lg/libc.a	-- little endian, no gp refs
#		lib/lo/libc.a	-- little endian, use gp refs
#
#	3. The modules in the mon directory using your MIPS-targetted 
#	   compiler.  After they have been compiled and assembled the 
#	   modules are archived to create a library. As many as two
#	   separate libraries may be created.
#
#		mon/bg/libmon.a	-- big endian, no gp refs
#		mon/lg/libmon.a	-- little endian, no gp refs
#
#	4. The modules in the imon directory using your MIPS-targetted 
#	   compiler.  After they have been compiled and assembled the 
#	   modules are linked to create a IMON executable in either
#	   imon/bg/imon.rec (big endian) or imon/lg/imon.rec 
#	   (little endian). These files can be used to make an IMON
#	   PROM.
#
#	5. The modules in the pmon directory using your MIPS-targetted 
#	   compiler.  After they have been compiled and assembled the 
#	   modules are linked to create a PMON executable in either
#	   pmon/bg/pmon.rec (big endian) or pmon/lg/pmon.rec 
#	   (little endian). These files can be used to make a PMON
#	   PROM.
#
# You can now compile your own program for execution under PMON/IMON,
# or build one of the many example programs provided in the
# 'examples' subdirectory. To build all the example programs, cd
# the examples directory, and then type 'make'.

# Default is to build just tools/ and lib/. This will allow you to
# compile and download your apps.
lib-only: $(LIB_ONLY)
	@echo "lib-only is now up to date"

# Specifiy this target if you need to build an IMON
imon-only: lib-only MONd IMONd
	@echo "imon-only is now up to date"

# Specifiy this target if you need to build a PMON
pmon-only: lib-only MONd PMONd
	@echo "pmon-only is now up to date"

# Specifiy this target if you need to build the BSPs
bsps-only: lib-only
	@echo "### bsps"
	@cd bsps;$(MAKE) all
	@echo "bsps-only is now up to date"

# Specifiy this target if you want to build everything
all:  $(ALL_TARG)
	@echo "lsipkg is now up to date"

clean: 
	@$(MAKE) clean_b "CMD=clean"

#=====================================================================
#		LSI Targets - used for maintenance

lsi-all :
	@$(MAKE) all 
	@$(MAKE) EXAMPLESd "CMD=all"

lsi-win :
	@$(MAKE) DLLd IMON95d

depend:
	@$(MAKE) depend_b "CMD=depend"

# 'make reduce' removes unneeded .o files, but leaves the package
# usable by the pmcc command.
reduce: 
	@$(MAKE) clean_b "CMD=reduce"

clean_b: $(CLEAN_DIRS)
depend_b: TOOLSd LIBd MONd PMONd IMONd BSPSd

PMONd:
	@echo "### pmon"
	@cd pmon;$(MAKE) $(CMD)

IMONd:
	@echo "### imon"
	@cd imon;$(MAKE) $(CMD)

MONd:
	@echo "### mon"
	@cd mon;$(MAKE) $(CMD)

LIBd:
	@echo "### lib"
	@cd lib;$(MAKE) $(CMD)

LIBSAd:
	@echo "### libsa"
	@cd libsa;$(MAKE) $(CMD)

TOOLSd:	
	@echo "### tools"
	@cd tools;$(MAKE) $(CMD)

INCLUDEd:
	@echo "### include"
	@cd include;$(MAKE) $(CMD)

EXAMPLESd:
	@echo "### examples"
	@cd examples;$(MAKE) $(CMD)

FPEMd:
	@echo "### fpem"
	@cd fpem;$(MAKE) $(CMD)

OPTFPEMd:
	@echo "### optfpem"
	@cd optfpem;$(MAKE) $(CMD)

BSPSd:
	@echo "### bsps"
	@cd bsps;$(MAKE) $(CMD)

HTMLd:
	@echo "### html"
	@cd html;$(MAKE) $(CMD)

DLLd:
	@echo "### dll"
	@cd dll;$(MAKE) $(CMD)

IMON95d:
	@echo "### imon95"
	@cd imon95;$(MAKE) $(CMD)

HERE:
	@ $(MAKE) $(CMD)

###################################################################
# The remainder of this file is for use by LSI when building releases

OTHERS = Makefile README install.c

PMON_TAPE_DIRS = LIBd LIBSAd MONd TOOLSd INCLUDEd EXAMPLESd HTMLd PMONd \
	IMONd BSPSd HERE
FPEM_TAPE_DIRS = FPEMd
OPTFPEM_TAPE_DIRS = OPTFPEMd

# This gets executed by the HERE rule
tape: revhistory index.htm
	@mklst . $(OTHERS) index.htm version  tape.lst

pmon.tar :
	@rm -f tape.lst
	@$(MAKE) pmon_tape "CMD=tape"
	@mktgz.bat

# make a self extracting archive of PMON for MSDOS
pmon.exe: pmon.tgz install.exe
	@echo "making pmon.exe.."
	@rm -rf pmon.dos
	@mkdir pmon.dos
	@rm -f pmon.zip 
	@mkexe.bat

pmon_tape: $(PMON_TAPE_DIRS)
fpem_tape: $(FPEM_TAPE_DIRS)
optfpem_tape: $(OPTFPEM_TAPE_DIRS)

COMPONENTS = mon/version lib/version tools/version html/version \
	     include/version imon/version pmon/version bsps/version 
revhistory : $(OTHERS) $(COMPONENTS)
	@rm -f pmon.dos/install.exe
	@newversion $(NVSW) -cl $(COMPONENTS)

index.htm : index.src version
	@mkreadme index.src version  index.htm
readme : README.src version
	@mkreadme README.src version  README

# These two targets are used to generate the stuff on the PMON web pages

# make a binary distribution (for DOS)
# cd into appropriate directory first
# then type: make -f $LSIPKG/Makefile pmon-dos-bin.exe
pmon-dos-bin.zip:
	rm -f pmon-dos-bin.zip
	zip -qr pmon-dos-bin.zip tools/*.exe lib/?o/*.a lib/*/crt?.o

# make prom image files for PMON
proms:
	mkpromfiles

