#TODO, if running from within _slk, ... 

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${sbbdep_TEST_BINARY_DIR} )
SET (sbbdep_TEST_DEFINES ${a4_TEST_DEFINES})
PREFIX_COMPILER_DEFINES(sbbdep_TEST_DEFINES)
add_definitions(${sbbdep_TEST_DEFINES})


ADD_EXECUTABLE( quicktest quicktest.cpp )
# if sq3 build is mt, pthread requried ... 
TARGET_LINK_LIBRARIES( quicktest pthread ${SBBDEP_REQURIED_LIBS} )

