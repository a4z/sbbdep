
if (NOT have_mya4sqlt3Setup)


    set( a4sqlt3_INCLUDE_DIR ${a4sqlt3_INCLUDE_DIR} CACHE PATH "Path to where a4sqlt3 header directory location")

    set( a4sqlt3_LIBRARY_DIR ${a4sqlt3_LIBRARY_DIR} CACHE PATH "Path to a4sqlt3 library")

	set( a4sqlt3_LINK_STATIC ON CACHE BOOL "link against static (ON) ore shared (OFF) build of a4sqlt3 library")

    if(a4sqlt3_LINK_STATIC)
        set( ORIGINAL_CMAKE_FIND_LIBRARY_SUFFIXES ${CMAKE_FIND_LIBRARY_SUFFIXES})
        if(WIN32)
          set(CMAKE_FIND_LIBRARY_SUFFIXES .lib .a ${CMAKE_FIND_LIBRARY_SUFFIXES})
        else(WIN32)
          set(CMAKE_FIND_LIBRARY_SUFFIXES .a )
        endif(WIN32)
    endif(a4sqlt3_LINK_STATIC)


    # for dev mode, bevor others...
    include_directories( BEFORE
        ${a4sqlt3_INCLUDE_DIR}
    )


    if(a4sqlt3_LINK_STATIC)
        set(CMAKE_FIND_LIBRARY_SUFFIXES ${ORIGINAL_CMAKE_FIND_LIBRARY_SUFFIXES})
    endif(a4sqlt3_LINK_STATIC)


    
endif(NOT have_mya4sqlt3Setup)

if(NOT a4sqlt3_library OR a4sqlt3_library-NOTFOUND)
    find_library(a4sqlt3_library  a4sqlt3
        PATHS ${a4sqlt3_LIBRARY_DIR}
        DOC "a4sqlt3 library name"
    )
endif(NOT a4sqlt3_library OR a4sqlt3_library-NOTFOUND)


