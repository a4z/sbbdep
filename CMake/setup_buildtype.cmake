# creates BUILD_TYPE_DEFINES
# which is either RELEASE or DEBUG
# and add this to the defines, 
# requires macros to be incldued before this file

if (NOT have_myBuildTypeSetup)

  if(NOT CMAKE_BUILD_TYPE)
    SET(CMAKE_BUILD_TYPE Release CACHE STRING
        "Choose the type of build, options are: None Debug Release RelWithDebInfo MinSizeRel."
        FORCE )
  endif(NOT CMAKE_BUILD_TYPE)

  if (CMAKE_BUILD_TYPE STREQUAL "Debug") 
      set (BUILD_TYPE_DEFINES "DEBUG")
  else (CMAKE_BUILD_TYPE STREQUAL "Debug")
      set (BUILD_TYPE_DEFINES "RELEASE")
  endif (CMAKE_BUILD_TYPE STREQUAL "Debug")

  # needs macros to be included first!
  PREFIX_COMPILER_DEFINES(BUILD_TYPE_DEFINES)
  
  add_definitions(${BUILD_TYPE_DEFINES})

  unset(BUILD_TYPE_DEFINES)

  set( have_myBuildTypeSetup ON BOOL)

endif (NOT have_myBuildTypeSetup)