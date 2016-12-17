cmake_minimum_required(VERSION 2.8)

project(sbbdep C CXX) 


set(sbbdep_MAJOR_VERSION 0)
set(sbbdep_MINOR_VERSION 2)
set(sbbdep_PATCH_VERSION 7)

set(sbbdep_VERSION ${sbbdep_MAJOR_VERSION}.${sbbdep_MINOR_VERSION}.${sbbdep_PATCH_VERSION})

set(sbbdep_CONFIG_HEADER "${CMAKE_CURRENT_SOURCE_DIR}/include/sbbdep/config.hpp")
configure_file("${CMAKE_CURRENT_SOURCE_DIR}/src/config.in" "${sbbdep_CONFIG_HEADER}")


# Include necessary submodules
set(CMAKE_MODULE_PATH
  "${sbbdep_SOURCE_DIR}/CMake"
)



#optional enable personal test/trial dir
include(CMakeLocalOpts.cmake OPTIONAL)

include( macros )
include( setup_compiler )
include( setup_buildtype )



# redundant in my files 2
if( ${CMAKE_PROJECT_NAME} STREQUAL ${PROJECT_NAME})
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin )
    # for internal build dir use no LIB_SUFFIX, but might change
    set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib )
    set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib )
endif( ${CMAKE_PROJECT_NAME} STREQUAL ${PROJECT_NAME})

# redundant in my files 2

include( setup_boost )

if (CMAKE_BUILD_TYPE STREQUAL "Release")
  #in Release only add testing if explicite asked to do so
  if (NOT DEFINED ALL_A4_TESTING_DISABLED)
    set (ALL_A4_TESTING_DISABLED TRUE)
  endif (NOT DEFINED ALL_A4_TESTING_DISABLED)
endif (CMAKE_BUILD_TYPE STREQUAL "Release")  
  

# needs to be done before testing is included  
#TODO , sqlite & dl only needed  if sl3 was build with that requirement ...
# TODO check if there is a installed libsl3, the internal one should be taken
if(EXISTS ${PROJECT_SOURCE_DIR}/libsl3)
  
  MESSAGE ("setup internal libsl3")
  
  set(libsl3_BUILD_TESTS OFF)
  add_subdirectory(${PROJECT_SOURCE_DIR}/libsl3)
  
  include_directories(SYSTEM 
    ${PROJECT_SOURCE_DIR}/libsl3/include
  )
 
  #SET(libsl3_LIBRARY ${CMAKE_ARCHIVE_OUTPUT_DIRECTORY}/libsl3.a)  
  SET(libsl3_LIBRARY sl3)  
else(EXISTS ${PROJECT_SOURCE_DIR}/libsl3)
#  find_library (libsl3_LIBRARY NAMES sl3 libsl3 libsl3.a)
#  TOOD write a ticket to create a find sl3.cmake file
   SET(libsl3_LIBRARY sl3)  
endif(EXISTS ${PROJECT_SOURCE_DIR}/libsl3)
  

include( setup_testing )    


include_directories( 
${CMAKE_CURRENT_SOURCE_DIR}/include
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

ADD_EXECUTABLE( sbbdepcli ${sbbdepcli_SRC} )

set(sbbdep_REQURIED_LIBS sbbdep ${libsl3_LIBRARY}  )

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
  SET(CMAKE_MAN_DIR "man"  CACHE STRING
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




if(EXISTS trials)
    add_subdirectory(trials)  
endif(EXISTS trials)





