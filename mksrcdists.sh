#!/bin/sh

thisdir=$(dirname $(readlink -f $0))


#define tmp dir and archive name
libsl3=${libsl3:-$(readlink -f $thisdir/../libsl3)}
echo $libsl3

#TODO test that libsl3 is a dir
if [ ! -d $libsl3 ] ; then
    echo "libsl3 directory missing"
    exit 1
fi
   

#get the version number from generated config file
version=$(grep -i SBBDEP_MAJOR_VERSION $thisdir/include/sbbdep/config.hpp | tr -d -c '[0-9]').\
$(grep -i SBBDEP_MINOR_VERSION $thisdir/include/sbbdep/config.hpp | tr -d -c '[0-9]').\
$(grep -i SBBDEP_PATCH_VERSION $thisdir/include/sbbdep/config.hpp | tr -d -c '[0-9]')

tmpdir=/tmp
outname=$tmpdir/sbbdep-${version}.tar.gz



#remove possible existings 
rm -rf ${outname} ${tmpdir}/sbbdep-${version}

#copy wanted stuff into versioned dir in tmp 
rsync -a \
--exclude=*~ \
--exclude=.?* \
--exclude=build* \
 $thisdir/*  ${tmpdir}/sbbdep-${version} \

#add libsl3 as a subdir

rsync -a \
--exclude=*~ \
--exclude=.?* \
--exclude=build* \
 $libsl3/*  ${tmpdir}/sbbdep-${version}/libsl3 \

 
 #pack the stuff 
tar -C ${tmpdir} -cvzf ${outname} sbbdep-${version}


