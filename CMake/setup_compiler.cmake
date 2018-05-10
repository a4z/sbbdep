

if (NOT have_myCompilerSetup)

 
    include(CheckCXXCompilerFlag)
    CHECK_CXX_COMPILER_FLAG("-std=c++14" COMPILER_SUPPORTS_CXX14)
    if(COMPILER_SUPPORTS_CXX14)
    
    else()
            message(STATUS "The compiler ${CMAKE_CXX_COMPILER} has no C++11 support. Please use a different C++ compiler.")
    endif()


    if ( CMAKE_COMPILER_IS_GNUCC OR CMAKE_COMPILER_IS_GNUCXX )
    
        message(STATUS "set up defaults for gcc")
    
        if (NOT CMAKE_C_FLAGS)
            SET (CMAKE_C_FLAGS "-Wall -Wextra -pedantic -std=c11 -pipe")
        endif(NOT CMAKE_C_FLAGS)
        
       SET (CMAKE_C_FLAGS ${CMAKE_C_FLAGS} CACHE STRING 
        "Flags used by the compiler during all build types."
        FORCE )
    
        if (NOT CMAKE_CXX_FLAGS)
            SET (CMAKE_CXX_FLAGS "-Wall -Wextra -pedantic -std=c++14 -pipe -pthread")
        endif(NOT CMAKE_CXX_FLAGS)
    
       SET (CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS} CACHE STRING 
        "Flags used by the compiler during all build types." 
        FORCE )  
        
        if (CODECOVERAGE) 
            add_definitions(--coverage -fprofile-arcs -ftest-coverage)
            set (OPTION_GCOVLIB gcov CACHE STRING INTERNAL)
        endif(CODECOVERAGE)

        if (CMAKE_BUILD_TYPE STREQUAL "Debug")
            find_program(CCACHE_BIN ccache)
            if(CCACHE_BIN)
                set_property(GLOBAL PROPERTY RULE_LAUNCH_COMPILE ${CCACHE_BIN})
                set_property(GLOBAL PROPERTY RULE_LAUNCH_LINK ${CCACHE_BIN})

                # ccache uses -I when compiling without preprocessor, which makes clang complain.
                if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
                    set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Qunused-arguments -fcolor-diagnostics")
                    set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Qunused-arguments -fcolor-diagnostics")
                endif()
            endif()
        endif (CMAKE_BUILD_TYPE STREQUAL "Debug")

    elseif( MSVC )
    # TODO default flags ? 
    
    else ( CMAKE_COMPILER_IS_GNUCC OR CMAKE_COMPILER_IS_GNUCXX )
        # 
        # cmake ../  -DCMAKE_C_COMPILER=clang -DCMAKE_CXX_COMPILER=clang++ -DCMAKE_BUILD_TYPE=Debug
        if(${CMAKE_C_COMPILER} MATCHES "clang")
            message(STATUS "set up defaults for clang")
            #this is somehow to much , TODO find out what is usefull
            #set(MYCLANGWARNINGS "-Weverything")  
            set(MYCLANGWARNINGS "-Wall -Wextra -pedantic")
            
            if (NOT CMAKE_C_FLAGS)
                SET (CMAKE_C_FLAGS "-std=c99 ${MYCLANGWARNINGS}")
            endif(NOT CMAKE_C_FLAGS)
            
           SET (CMAKE_C_FLAGS ${CMAKE_C_FLAGS} CACHE STRING 
            "Flags used by the compiler during all build types." 
            FORCE )
        
            if (NOT CMAKE_CXX_FLAGS)
                SET (CMAKE_CXX_FLAGS "-std=c++14 -pipe -pthread ${MYCLANGWARNINGS}")
            endif(NOT CMAKE_CXX_FLAGS)
        
           SET (CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS} CACHE STRING 
            "Flags used by the compiler during all build types." 
            FORCE )       
    
        else (${CMAKE_C_COMPILER} MATCHES "clang")
            message(WARNING "unknown compiler")                      
        endif(${CMAKE_C_COMPILER} MATCHES "clang")    
        
    endif ( CMAKE_COMPILER_IS_GNUCC OR CMAKE_COMPILER_IS_GNUCXX )

    set(have_myCompilerSetup ON BOOL)    

endif (NOT have_myCompilerSetup)
