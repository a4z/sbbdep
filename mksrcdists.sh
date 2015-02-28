#!/bin/sh

#get the version number from generated config file
version=$(grep -i SBBDEP_MAJOR_VERSION include/sbbdep/config.hpp | tr -d -c '[0-9]').\
$(grep -i SBBDEP_MINOR_VERSION include/sbbdep/config.hpp | tr -d -c '[0-9]').\
$(grep -i SBBDEP_PATCH_VERSION include/sbbdep/config.hpp | tr -d -c '[0-9]')

#define tmp dir and archive name
tmpdir=/tmp
outname=../sbbdep-${version}.tar.gz

#remove possible existings 
rm -rf ${outname} ${tmpdir}/sbbdep-${version}

#copy wanted stuff into versioned dir in tmp 
rsync -a \
--exclude=*~ \
--exclude=.?* \
--exclude=build* \
 $(pwd)/*  ${tmpdir}/sbbdep-${version} \

 #pack the stuff 
tar -C ${tmpdir} -cvzf ${outname} sbbdep-${version}


