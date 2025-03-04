cmake_minimum_required( VERSION 3.20.0 ) # Latest version of CMake when this file was created.

# Use solution folders to organize projects
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

#add_compile_definitions(DEBUG_EDITOR)

# Make sure DLL and EXE targets go to the same directory.
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib) # Output directory for static lib (.LIB)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin) # Output directory for shared lib (.DLL)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin) # Output directory for executables (.EXE)

set (CMAKE_CXX_STANDARD 17)

project( Engine3D_DX12 LANGUAGES CXX )

# Enable multithreaded builds
if( MSVC )
    add_compile_options(/MP)
endif()

add_subdirectory( DirectXTex )
add_subdirectory( Engine )
add_subdirectory( Sponza )
add_subdirectory( Tests )