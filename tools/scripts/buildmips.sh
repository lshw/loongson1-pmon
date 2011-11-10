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
# Build OCELOT Target
#
BuildTarget Ocelot OCELOTEB
BuildTarget Ocelot OCELOTEL

#
# Build OCELOT_C Target
#
BuildTarget Ocelot_C OCELOT-CEB
BuildTarget Ocelot_C OCELOT-CEL

#
# Build OCELOT_G Target
#
BuildTarget Ocelot_G OCELOT-GEB
BuildTarget Ocelot_G OCELOT-GEL
