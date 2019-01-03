cmake_minimum_required(VERSION 2.8.0)

### libsp

project(sort-sp)

include_directories(
)

link_directories(
)

file(GLOB sp_src "*.cpp")

add_library( # Sets the name of the library.
             sort-sp
             # Sets the library as a shared library.
             STATIC
             # Provides a relative path to your source file(s).
             # Associated headers in the same location as their source
             # file are automatically included.
             ${sp_src})

set_target_properties( sort-sp PROPERTIES ARCHIVE_OUTPUT_DIRECTORY  "${CMAKE_CURRENT_SOURCE_DIR}/../out" )
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O2 -std=gnu++11 -Wno-switch-enum -Wno-switch -Wno-error=non-virtual-dtor -g -march=corei7-avx")

target_link_libraries( # Specifies the target library.
                       sort-sp
                       # Denpendences
                        )

##########################################################################################