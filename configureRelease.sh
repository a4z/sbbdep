
BUILDDIR=buildRelease

A4SQLT3DIR=$(dirname $(pwd))/a4sqlt3

if [ -d $BUILDDIR ] ; then
 rm -r $BUILDDIR/*
else
 mkdir $BUILDDIR
fi


cd $BUILDDIR && \
 cmake ../ -DCMAKE_BUILD_TYPE=Release #-DALL_A4_TESTING_DISABLED=OFF

