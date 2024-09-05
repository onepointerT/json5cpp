
file( GLOB SRCFILES *.cpp )
add_library( json5 STATIC ${SRCFILES} )
target_link_libraries( json5 stdc++ )
