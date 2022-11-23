cmake_minimum_required( VERSION 3.20.0 ) # Latest version of CMake when this file was created.

#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /DEBUG:FULL")
#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /DEBUG:FULL")

set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /MTd")

set( HEADER_FILES
    inc/Sponza.h
 "SponzaExe.h")

set( SRC_FILES   
    "main.cpp"
    src/Sponza.cpp
 "SponzaExe.cpp")

add_library ( Sponza SHARED
    ${HEADER_FILES} 
    ${SRC_FILES}
)

target_include_directories( Sponza 
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/inc
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries( Sponza 
    Engine
    D3DCompiler.lib
    Shlwapi.lib
)


add_executable ( SponzaBin WIN32
    ${HEADER_FILES} 
    ${SRC_FILES}
)

target_include_directories( SponzaBin 
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/inc
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries( SponzaBin 
    Engine
    D3DCompiler.lib
    Shlwapi.lib
)

# Set debug database to enable the "Edit and Continue" feature.
target_compile_options( SponzaBin 
    PRIVATE $<$<CONFIG:Debug>:/ZI>
)

target_link_options(EngineBin PUBLIC $<$<CXX_COMPILER_ID:MSVC>:/DEBUG:FULL>)

# Set Local Debugger Settings (Command Arguments and Environment Variables)
set( COMMAND_ARGUMENTS "-wd \"${CMAKE_SOURCE_DIR}\"" )
configure_file( Sponza.vcxproj.user.in ${CMAKE_CURRENT_BINARY_DIR}/Sponza.vcxproj.user @ONLY )
