cmake_minimum_required(VERSION 2.8.0)

project(sort)

add_subdirectory(SP)

include_directories( SP )
link_directories( ${CMAKE_CURRENT_SOURCE_DIR}/out/ )
file(GLOB sort_src "*.cpp")
ADD_EXECUTABLE( # Sets the name of the library.
			  sort_str
             # Provides a relative path to your source file(s).
             # Associated headers in the same location as their source
             # file are automatically included.
             ${sort_src} )
set_target_properties( sort_str PROPERTIES RUNTIME_OUTPUT_DIRECTORY   "${CMAKE_CURRENT_SOURCE_DIR}/out" )
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=gnu++11 -g -O2 -pthread -march=corei7-avx -Wall -Wno-switch-enum -Wno-switch -Wno-error=non-virtual-dtor")

target_link_libraries( # Specifies the target library.
					   sort_str
                       # Denpendences
                       sort-sp )