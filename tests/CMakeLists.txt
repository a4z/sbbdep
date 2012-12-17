#TODO, if running from within _slk, ... 

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${sbbdep_TEST_BINARY_DIR} )
SET (sbbdep_TEST_DEFINES ${a4_TEST_DEFINES})
PREFIX_COMPILER_DEFINES(sbbdep_TEST_DEFINES)
add_definitions(${sbbdep_TEST_DEFINES})


#if (USE_BOOSTTESTING_DYNLINK)
    FIND_PACKAGE( Boost COMPONENTS unit_test_framework  )
    set( myBoostLibs  ${Boost_UNIT_TEST_FRAMEWORK_LIBRARY} )   
#endif(USE_BOOSTTESTING_DYNLINK)

SET( sbbtest_SRC
main.cpp
findbinpath.cpp
pathtest.cpp
pknametest.cpp
replorig.cpp
varadmdir.cpp
cacheup1_2.cpp
)


ADD_EXECUTABLE( sbbtest ${sbbtest_SRC} )
# if a4sqlt3 build is mt, pthread requried ... 
TARGET_LINK_LIBRARIES( sbbtest pthread ${SBBDEP_REQURIED_LIBS} ${myBoostLibs} )




