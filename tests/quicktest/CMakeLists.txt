#TODO, if running from within _slk, ... 


ADD_EXECUTABLE( quicktest quicktest.cpp )
# if sl3 build is mt, pthread requried ... 
TARGET_LINK_LIBRARIES( quicktest pthread ${SBBDEP_REQURIED_LIBS} )

