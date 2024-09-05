cmake_minimum_required( VERSION 3.28 )
project( json5cpp )

set( CMAKE_CXX_STANDARD 23 )
set( CMAKE_CXX_STANDARD_REQUIRED ON )
set( CMAKE_C_STANDARD 23 )
set( CMAKE_C_STANDARD_REQUIRED ON )

if( UNIX )
    add_definitions( -fpermissive )
elseif( WIN64 )
    add_definitions( /clatest /cpplatest)
endif()

include_directories( include/json5 )

add_subdirectory( src )

