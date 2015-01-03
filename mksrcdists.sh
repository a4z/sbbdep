#!/bin/sh

#get the version number from generated config file
version=$(grep -i SBBDEP_MAJOR_VERSION include/sbbdep/config.hpp | tr -d -c '[0-9]').\
$(grep -i SBBDEP_MINOR_VERSION include/sbbdep/config.hpp | tr -d -c '[0-9]').\
$(grep -i SBBDEP_PATCH_VERSION include/sbbdep/config.hpp | tr -d -c '[0-9]')


outname=sbbdep-${version}.tar.gz

#remove possible existings 
rm ${outname} 
rm /tmp/${outname} 
 
# TODO check this shoudl handle metadata, not_ .?* 
tar -C ../ \
--exclude=*~ \
--exclude=.?* \
--exclude=build* \
-cvzf /tmp/${outname} sbbdep   
 
mv /tmp/${outname} ./

