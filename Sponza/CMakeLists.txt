cmake_minimum_required( VERSION 3.12.1 ) # Latest version of CMake when this file was created.

set( HEADER_FILES
    inc/Sponza.h
)

set( SRC_FILES
    src/Sponza.cpp
    src/main.cpp
)

link_directories (${CMAKE_BINARY_DIR}/lib)

add_executable( Sponza WIN32
    ${HEADER_FILES} 
    ${SRC_FILES}
)

target_include_directories( Sponza
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/inc
    PUBLIC ${CMAKE_SOURCE_DIR}/Engine/inc
    PUBLIC ${CMAKE_SOURCE_DIR}/Engine/external
)

target_link_libraries( Sponza
    Engine.lib
    D3DCompiler.lib
    Shlwapi.lib
)

# Set debug database to enable the "Edit and Continue" feature.
target_compile_options( Sponza
    PRIVATE $<$<CONFIG:Debug>:/ZI>
)

# Set Local Debugger Settings (Command Arguments and Environment Variables)
set( COMMAND_ARGUMENTS "-wd \"${CMAKE_SOURCE_DIR}\"" )
configure_file( Sponza.vcxproj.user.in ${CMAKE_CURRENT_BINARY_DIR}/Sponza.vcxproj.user @ONLY )
