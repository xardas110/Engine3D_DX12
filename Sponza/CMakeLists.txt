cmake_minimum_required( VERSION 3.12.1 ) # Latest version of CMake when this file was created.

set( HEADER_FILES
    inc/Sponza.h
)

set( SRC_FILES
    src/main.cpp
    src/Sponza.cpp
)

add_executable( Sponza WIN32
    ${HEADER_FILES} 
    ${SRC_FILES}
)

target_include_directories( Sponza 
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/inc
)

target_link_libraries( Sponza 
    Engine
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
