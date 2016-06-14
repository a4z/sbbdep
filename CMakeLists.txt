cmake_minimum_required(VERSION 2.8)

project(sbbdep C CXX) 

include( a4cmk/macros.cmake )

set(sbbdep_MAJOR_VERSION 0)
set(sbbdep_MINOR_VERSION 2)
set(sbbdep_PATCH_VERSION 6)

set(sbbdep_VERSION ${sbbdep_MAJOR_VERSION}.${sbbdep_MINOR_VERSION}.${sbbdep_PATCH_VERSION})

set(sbbdep_CONFIG_HEADER "${CMAKE_CURRENT_SOURCE_DIR}/include/sbbdep/config.hpp")
configure_file("${CMAKE_CURRENT_SOURCE_DIR}/src/config.in" "${sbbdep_CONFIG_HEADER}")


if(NOT CMAKE_BUILD_TYPE)

  SET(CMAKE_BUILD_TYPE Release CACHE STRING
      "Choose the type of build, options are: None Debug Release RelWithDebInfo MinSizeRel."
      FORCE )
endif(NOT CMAKE_BUILD_TYPE)


include( a4cmk/setup_compiler.cmake )


if (CMAKE_BUILD_TYPE STREQUAL "Debug") 
    set (_DEFAULT_FLAGS "DEBUG")
else (CMAKE_BUILD_TYPE STREQUAL "Debug")
    set (_DEFAULT_FLAGS "RELEASE")
endif (CMAKE_BUILD_TYPE STREQUAL "Debug")

PREFIX_COMPILER_DEFINES(_DEFAULT_FLAGS)
add_definitions(${_DEFAULT_FLAGS})

unset(_DEFAULT_FLAGS)


if( ${CMAKE_PROJECT_NAME} STREQUAL ${PROJECT_NAME})
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin )
    # for internal build dir use no LIB_SUFFIX, but might change
    set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib )
    set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib )
endif( ${CMAKE_PROJECT_NAME} STREQUAL ${PROJECT_NAME})


include( a4cmk/setup_boost.cmake )

if (CMAKE_BUILD_TYPE STREQUAL "Release")
  #in Release only add testing if explicite asked to do so
  if (NOT DEFINED ALL_A4_TESTING_DISABLED)
    set (ALL_A4_TESTING_DISABLED TRUE)
  endif (NOT DEFINED ALL_A4_TESTING_DISABLED)
endif (CMAKE_BUILD_TYPE STREQUAL "Release")  
  
include( a4cmk/setup_testing.cmake )  

include_directories( BEFORE
include
)


##########################################################################################

SET ( libsbbdep_HDR

include/sbbdep/cache.hpp

include/sbbdep/dircontent.hpp
include/sbbdep/elffile.hpp
include/sbbdep/error.hpp
include/sbbdep/ldconf.hpp
include/sbbdep/log.hpp
include/sbbdep/path.hpp
include/sbbdep/pathname.hpp
include/sbbdep/pkg.hpp
include/sbbdep/pkgname.hpp

include/sbbdep/utils/backgroundjob.hpp
include/sbbdep/utils/concurrentpeek.hpp


${sbbdep_CONFIG_HEADER}
)
#-----------------------------------------------------------------------------------------
SET ( libsbbdep_SRCHDR
src/sbbdep/cachesql.hpp
src/sbbdep/conststr.hpp
)
#-----------------------------------------------------------------------------------------
SET ( libsbbdep_SRC

src/sbbdep/cache.cpp

src/sbbdep/cachesql.cpp
src/sbbdep/dircontent.cpp
src/sbbdep/error.cpp
src/sbbdep/elffile.cpp
src/sbbdep/ldconf.cpp
src/sbbdep/log.cpp
src/sbbdep/path.cpp
src/sbbdep/pathname.cpp
src/sbbdep/pkg.cpp
src/sbbdep/pkgname.cpp


)
##########################################################################################

file(GLOB elfio_HDR RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}"
    "${CMAKE_CURRENT_SOURCE_DIR}/include/elfio/*.hpp")


set(libsbbdep_ALLSRC ${libsbbdep_SRC} ${libsbbdep_SRCHDR} ${libsbbdep_HDR} ${elfio_HDR})

add_library( sbbdep STATIC ${libsbbdep_ALLSRC}  )

SET ( sbbdepcli_SRC
    src/cli/appargs.cpp
    src/cli/appargs.hpp
    src/cli/appcli.cpp
    src/cli/appcli.hpp

    src/cli/main.cpp
    
    src/cli/report.hpp
    src/cli/report.cpp
    src/cli/report_require.cpp
    src/cli/report_whoneed.cpp
    src/cli/report_utils.cpp    
    src/cli/report_lookup.cpp
    src/cli/report_bdtree.cpp
    
    src/cli/featurex.hpp
    src/cli/featurex.cpp
)

  
#ADD_EXECUTABLE( sbbdepcli src/dummy.cpp )
ADD_EXECUTABLE( sbbdepcli ${sbbdepcli_SRC} )

#TODO, sqlite & dl only needed  if sl3 was build with that requirement ...
set(sbbdep_REQURIED_LIBS sbbdep sl3  )

# disable until I have a real split
# set(sbbdep_install_targets ${sbbdep_install_targets} sbbdep)

TARGET_LINK_LIBRARIES( sbbdepcli ${sbbdep_REQURIED_LIBS} )
SET_TARGET_PROPERTIES(sbbdepcli PROPERTIES OUTPUT_NAME sbbdep)
set(sbbdep_install_targets ${sbbdep_install_targets} sbbdepcli)


# normaly  I should use CMAKE_INSTALL_DOCDIR/MANDIR
# but here I know the locations on Slackware   

if(NOT CMAKE_DOC_DIR)
  SET(CMAKE_DOC_DIR "doc/sbbdep-${sbbdep_VERSION}"  CACHE STRING
      "Choose the target directory where documentation files will be installed."
      FORCE )
endif(NOT CMAKE_DOC_DIR)

if(NOT CMAKE_MAN_DIR)
  SET(CMAKE_MAN_DIR "usr/man"  CACHE STRING
      "Choose the target directory where man files will be installed."
      FORCE )
endif(NOT CMAKE_MAN_DIR)



set(sbbdep_DOC_FILES 
  README.txt README_db.txt AUTHORS COPYING
)

set(sbbdep_MAN_PAGE "${CMAKE_BINARY_DIR}/share/man/man1/sbbdep.1")
configure_file("${CMAKE_CURRENT_SOURCE_DIR}/sbbdep.man" "${sbbdep_MAN_PAGE}")


#only add install target if this is the top level project
if( ${CMAKE_PROJECT_NAME} STREQUAL ${PROJECT_NAME})
  #    INSTALL(FILES ${sbbdep_HDR} DESTINATION include/sbbdep)
    INSTALL(TARGETS ${sbbdep_install_targets} 
      RUNTIME DESTINATION bin
  #    LIBRARY DESTINATION lib${LIB_SUFFIX}
  #    ARCHIVE DESTINATION lib${LIB_SUFFIX}
    )
  INSTALL(FILES ${sbbdep_DOC_FILES} DESTINATION  ${CMAKE_DOC_DIR})
  INSTALL(FILES ${sbbdep_MAN_PAGE} DESTINATION  ${CMAKE_MAN_DIR}/man1)

endif( ${CMAKE_PROJECT_NAME} STREQUAL ${PROJECT_NAME})






#optional enable personal test/trial dir
include(CMakeLocalOpts.cmake OPTIONAL)
if(sbbdep_LOCAL_TRIALS)
    add_subdirectory(trials)  
endif(sbbdep_LOCAL_TRIALS)





