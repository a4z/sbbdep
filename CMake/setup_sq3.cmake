
if (NOT setup_sl3)


    set( sl3_INCLUDE_DIR ${sl3_INCLUDE_DIR} CACHE PATH "Path to where sl3 header directory location")

    set( sl3_LIBRARY_DIR ${sl3_LIBRARY_DIR} CACHE PATH "Path to sl3 library")

	set( sl3_LINK_STATIC ON CACHE BOOL "link against static (ON) ore shared (OFF) build of sl3 library")

    if(sl3_LINK_STATIC)
        set( ORIGINAL_CMAKE_FIND_LIBRARY_SUFFIXES ${CMAKE_FIND_LIBRARY_SUFFIXES})
        if(WIN32)
          set(CMAKE_FIND_LIBRARY_SUFFIXES .lib .a ${CMAKE_FIND_LIBRARY_SUFFIXES})
        else(WIN32)
          set(CMAKE_FIND_LIBRARY_SUFFIXES .a )
        endif(WIN32)
    endif(sl3_LINK_STATIC)


    # for dev mode, bevor others...
    include_directories( BEFORE
        ${sl3_INCLUDE_DIR}
    )


    if(sl3_LINK_STATIC)
        set(CMAKE_FIND_LIBRARY_SUFFIXES ${ORIGINAL_CMAKE_FIND_LIBRARY_SUFFIXES})
    endif(sl3_LINK_STATIC)


    
endif(NOT setup_sl3)

if(NOT sl3_library OR sl3_library-NOTFOUND)
    find_library(sl3_library  sl3
        PATHS ${sl3_LIBRARY_DIR}
        DOC "sl3 library name"
    )
endif(NOT sl3_library OR sl3_library-NOTFOUND)


