
FIND_PACKAGE( Boost COMPONENTS unit_test_framework  )
set( myBoostLibs  ${Boost_UNIT_TEST_FRAMEWORK_LIBRARY} )   

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
cachestuff.cpp
#cacheup1_2.cppnot supported anymore
)


ADD_EXECUTABLE( sbbtest ${sbbtest_SRC} )
# if sl3 build is mt, pthread requried ... 
TARGET_LINK_LIBRARIES( sbbtest pthread  sbbdep sl3 ${myBoostLibs} )




add_subdirectory(quicktest)