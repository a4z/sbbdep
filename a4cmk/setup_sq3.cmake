
if (NOT setup_sq3)


    set( sq3_INCLUDE_DIR ${sq3_INCLUDE_DIR} CACHE PATH "Path to where sq3 header directory location")

    set( sq3_LIBRARY_DIR ${sq3_LIBRARY_DIR} CACHE PATH "Path to sq3 library")

	set( sq3_LINK_STATIC ON CACHE BOOL "link against static (ON) ore shared (OFF) build of sq3 library")

    if(sq3_LINK_STATIC)
        set( ORIGINAL_CMAKE_FIND_LIBRARY_SUFFIXES ${CMAKE_FIND_LIBRARY_SUFFIXES})
        if(WIN32)
          set(CMAKE_FIND_LIBRARY_SUFFIXES .lib .a ${CMAKE_FIND_LIBRARY_SUFFIXES})
        else(WIN32)
          set(CMAKE_FIND_LIBRARY_SUFFIXES .a )
        endif(WIN32)
    endif(sq3_LINK_STATIC)


    # for dev mode, bevor others...
    include_directories( BEFORE
        ${sq3_INCLUDE_DIR}
    )


    if(sq3_LINK_STATIC)
        set(CMAKE_FIND_LIBRARY_SUFFIXES ${ORIGINAL_CMAKE_FIND_LIBRARY_SUFFIXES})
    endif(sq3_LINK_STATIC)


    
endif(NOT setup_sq3)

if(NOT sq3_library OR sq3_library-NOTFOUND)
    find_library(sq3_library  sq3
        PATHS ${sq3_LIBRARY_DIR}
        DOC "sq3 library name"
    )
endif(NOT sq3_library OR sq3_library-NOTFOUND)


