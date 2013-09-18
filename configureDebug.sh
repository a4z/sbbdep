
BUILDDIR=buildDebug

A4ZDIR=$(dirname $(pwd))/a4z
A4SQLT3DIR=$(dirname $(pwd))/a4sqlt3

if [ -d $BUILDDIR ] ; then
 rm -r $BUILDDIR/*
else
 mkdir $BUILDDIR
fi


cd $BUILDDIR && cmake ../ -DCMAKE_BUILD_TYPE=Debug \
-DUSE_BOOSTTESTING_DYNLINK=ON \
-Da4sqlt3_INCLUDE_DIR=$A4SQLT3DIR/include \
-Da4sqlt3_LIBRARY_DIR=$A4SQLT3DIR/$BUILDDIR/lib 


