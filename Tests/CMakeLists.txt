cmake_minimum_required( VERSION 3.20.0 ) # Latest version of CMake when this file was created.

#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /DEBUG:FULL")
#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /DEBUG:FULL")

set( HEADER_FILES 
    "Tests.h"
    )

set( SRC_FILES   
    "main.cpp"
    "Tests.cpp")

add_executable ( Tests WIN32
    ${HEADER_FILES} 
    ${SRC_FILES}
)

target_include_directories( Tests 
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries( Tests 
    Engine
    D3DCompiler.lib
    Shlwapi.lib
)

# Set debug database to enable the "Edit and Continue" feature.
target_compile_options( Tests 
    PRIVATE $<$<CONFIG:Debug>:/ZI>
)

target_link_options(EngineBin PUBLIC $<$<CXX_COMPILER_ID:MSVC>:/DEBUG:FULL>)

# Set Local Debugger Settings (Command Arguments and Environment Variables)
set( COMMAND_ARGUMENTS "-wd \"${CMAKE_SOURCE_DIR}\"" )
configure_file( Tests.vcxproj.user.in ${CMAKE_CURRENT_BINARY_DIR}/Tests.vcxproj.user @ONLY )
