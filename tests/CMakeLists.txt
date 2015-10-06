#TODO, if running from within _slk, ... 

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${sbbdep_TEST_BINARY_DIR} )
SET (sbbdep_TEST_DEFINES ${a4_TEST_DEFINES})
PREFIX_COMPILER_DEFINES(sbbdep_TEST_DEFINES)
add_definitions(${sbbdep_TEST_DEFINES})


#if (USE_BOOSTTESTING_DYNLINK)
    FIND_PACKAGE( Boost COMPONENTS unit_test_framework  )
    set( myBoostLibs  ${Boost_UNIT_TEST_FRAMEWORK_LIBRARY} )   
#endif(USE_BOOSTTESTING_DYNLINK)

#no idea why the include and link fieles do not wiork since there is a sub dir..

include_directories(
    ${CMAKE_SOURCE_DIR}/include
)

SET( sbbtest_SRC
a4testing.cpp
a4testing.hpp
findbinpath.cpp
pathtest.cpp
pknametest.cpp
replorig.cpp

#cacheup1_2.cppnot supported anymore
)


ADD_EXECUTABLE( sbbtest ${sbbtest_SRC} )
# if sl3 build is mt, pthread requried ... 
TARGET_LINK_LIBRARIES( sbbtest pthread  sbbdep sl3 ${myBoostLibs} )




add_subdirectory(quicktest)