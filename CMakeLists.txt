cmake_minimum_required(VERSION 2.8)

project(sbbdep C CXX) 

include( a4cmk/myMacros.cmake )

set(sbbdep_MAJOR_VERSION 0)
set(sbbdep_MINOR_VERSION 0)
set(sbbdep_PATCH_VERSION 9)

set(sbbdep_VERSION ${sbbdep_MAJOR_VERSION}.${sbbdep_MINOR_VERSION}.${sbbdep_PATCH_VERSION})

set(sbbdep_CONFIG_HEADER "${CMAKE_CURRENT_SOURCE_DIR}/include/sbbdep/config.hpp")
configure_file("${CMAKE_CURRENT_SOURCE_DIR}/src/config.in" "${sbbdep_CONFIG_HEADER}")


if(NOT CMAKE_BUILD_TYPE)

  SET(CMAKE_BUILD_TYPE RelWithDebInfo CACHE STRING
      "Choose the type of build, options are: None Debug Release RelWithDebInfo MinSizeRel."
      FORCE )
endif(NOT CMAKE_BUILD_TYPE)


if (NOT CMAKE_C_FLAGS)
    SET (CMAKE_C_FLAGS "-Wall -pedantic -std=c99")
endif(NOT CMAKE_C_FLAGS)

SET (CMAKE_C_FLAGS ${CMAKE_C_FLAGS} CACHE STRING 
"Flags used by the compiler during all build types." 
FORCE )


if (NOT CMAKE_CXX_FLAGS)
    SET (CMAKE_CXX_FLAGS "-Wall -pedantic -std=c++0x")
endif(NOT CMAKE_CXX_FLAGS)

SET (CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS}  CACHE STRING 
"Flags used by the compiler during all build types." 
FORCE )


if (CMAKE_BUILD_TYPE STREQUAL "Debug") 
    set (_DEFAULT_FLAGS "DEBUG")
else()
    set (_DEFAULT_FLAGS "RELEASE")
endif()

PREFIX_COMPILER_DEFINES(_DEFAULT_FLAGS)
add_definitions(${_DEFAULT_FLAGS})

unset(_DEFAULT_FLAGS)



if( ${CMAKE_PROJECT_NAME} STREQUAL ${PROJECT_NAME})
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin )
    set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib )
    set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib )
endif( ${CMAKE_PROJECT_NAME} STREQUAL ${PROJECT_NAME})


include( a4cmk/myBoostSetup.cmake )
include( a4cmk/mya4zSetup.cmake )
include( a4cmk/mya4sqlt3Setup.cmake )


include_directories(
include
)




##########################################################################################

SET ( libsbbdep_HDR
include/sbbdep/cache.hpp
include/sbbdep/cachecmds.hpp
include/sbbdep/cachedb.hpp
include/sbbdep/cachesql.hpp
include/sbbdep/depfilewriter.hpp
include/sbbdep/depwriter.hpp
include/sbbdep/dircontent.hpp
include/sbbdep/dynlinked.hpp
include/sbbdep/dynlinkedinfo.hpp
include/sbbdep/dynlinkedinfolist.hpp
include/sbbdep/error.hpp
include/sbbdep/filemagic.hpp
include/sbbdep/log.hpp
include/sbbdep/lddirs.hpp
include/sbbdep/path.hpp
include/sbbdep/pathname.hpp
include/sbbdep/pathnamelist.hpp
include/sbbdep/pathnameset.hpp
include/sbbdep/pkfab.hpp
include/sbbdep/pkg.hpp
include/sbbdep/pkgadmdir.hpp
include/sbbdep/pkgarchiv.hpp
include/sbbdep/pkgdestdir.hpp
include/sbbdep/pkgfile.hpp
include/sbbdep/pkgname.hpp
include/sbbdep/pkgonebinlib.hpp
include/sbbdep/pkoffile.hpp
include/sbbdep/stringlist.hpp
include/sbbdep/stringset.hpp
)
#-----------------------------------------------------------------------------------------
SET ( libsbbdep_SRCHDR
)
#-----------------------------------------------------------------------------------------
SET ( libsbbdep_SRC
src/sbbdep/cache.cpp
src/sbbdep/cachecmds.cpp
src/sbbdep/cachedb.cpp
src/sbbdep/cachesql.cpp
src/sbbdep/depfilewriter.cpp
src/sbbdep/depwriter.cpp
src/sbbdep/dircontent.cpp
src/sbbdep/dynlinked.cpp
src/sbbdep/filemagic.cpp
src/sbbdep/lddirs.cpp
src/sbbdep/log.cpp
src/sbbdep/path.cpp
src/sbbdep/pathname.cpp
src/sbbdep/pkfab.cpp
src/sbbdep/pkg.cpp
src/sbbdep/pkgadmdir.cpp
src/sbbdep/pkgarchiv.cpp
src/sbbdep/pkgdestdir.cpp
src/sbbdep/pkgfile.cpp
src/sbbdep/pkgname.cpp
src/sbbdep/pkgonebinlib.cpp
src/sbbdep/pkoffile.cpp

)
##########################################################################################


set(libsbbdep_ALLSRC ${libsbbdep_SRC} ${libsbbdep_SRCHDR} ${libsbbdep_HDR})

add_library( sbbdep STATIC ${libsbbdep_ALLSRC}  )

SET ( sbbdepcli_SRC
    src/appargs.cpp
    src/appargs.hpp
    src/appcli.cpp
    src/appcli.hpp
    src/main.cpp
)


ADD_EXECUTABLE( sbbdepcli ${sbbdepcli_SRC} )

#TODO, sqlite & dl only needed  if a4sqlt3 was build with that requirement ...
#set(SBBDEP_REQURIED_LIBS sbbdep a4zsqlt3 a4z sqlite3  dl elf) #macig still used in pkfab..
set(SBBDEP_REQURIED_LIBS sbbdep ${a4sqlt3_library} ${a4z_library} magic elf)

set(sbbdep_install_targets ${sbbdep_install_targets} sbbdep)

TARGET_LINK_LIBRARIES( sbbdepcli ${SBBDEP_REQURIED_LIBS} )
SET_TARGET_PROPERTIES(sbbdepcli PROPERTIES OUTPUT_NAME sbbdep)
set(sbbdep_install_targets ${sbbdep_install_targets} sbbdepcli)

FIND_PACKAGE( OpenMP ) 

set_target_properties(sbbdep sbbdepcli PROPERTIES COMPILE_FLAGS ${OpenMP_CXX_FLAGS})
set_target_properties(sbbdepcli PROPERTIES LINK_FLAGS ${OpenMP_CXX_FLAGS})


include( a4cmk/myTestSetup.cmake )

#add_subdirectory(tests)

#optional enable personal test/trial dir
include(CMakeLocalOpts.cmake OPTIONAL)
if(sbbdep_LOCAL_TRIALS)
    add_subdirectory(trials)  
endif(sbbdep_LOCAL_TRIALS)


#only add install target if this is the top level project
if( ${CMAKE_PROJECT_NAME} STREQUAL ${PROJECT_NAME})
    INSTALL(FILES ${sbbdep_HDR} DESTINATION include/sbbdep)
    INSTALL(TARGETS ${sbbdep_install_targets} 
      RUNTIME DESTINATION bin
      LIBRARY DESTINATION lib${LIB_SUFFIX}
      ARCHIVE DESTINATION lib${LIB_SUFFIX}
    )
endif( ${CMAKE_PROJECT_NAME} STREQUAL ${PROJECT_NAME})


