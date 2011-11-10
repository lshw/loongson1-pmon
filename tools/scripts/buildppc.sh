MAKE=gmake
PMONCFG=pmoncfg
PMONBASE=`cd ../..; pwd`
PMONDEST=${PMONBASE}/bld
mkdir -p ${PMONDEST}

function BuildTarget
{
    TARGET=$1
    SPEC=$2
    cd ${PMONBASE}/Targets/${TARGET}/conf
    ${PMONCFG} ${SPEC}
    cd ../compile/${SPEC}
    ${MAKE} depend
    ${MAKE}
    cp pmon ${PMONDEST}/pmon.${TARGET}
}

#
# Build CK3 Target
#
BuildTarget CK3 CK3

#
# Build PowerPMC230 Target
#
BuildTarget PowerPMC230 GENERIC

#
# Build PowerPMC250 Target
#
BuildTarget PowerPMC250 GENERIC

#
# Build PowerCore-cPCI680 Target
#
BuildTarget cPCI680 GENERIC

#
# Build Power7e Target
#
BuildTarget Power7e POWER7E

#
# Build VG4 Target
#
BuildTarget VG4 VG4

#
# Build EV64260 Target
#
BuildTarget EV64260 EV64260

#
# Build DB64360 Target
#
BuildTarget DB64360 DB64360

#
# Build EV96132 Target
#
BuildTarget EV96132 EV96132
