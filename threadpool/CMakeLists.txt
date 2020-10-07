cmake_minimum_required(VERSION 3.5)
project(ModThreadContral)

set(CMAKE_CXX_STANDARD 11)

set(CMAKE_MODULE_PATH
            ${CMAKE_MODULE_PATH}
            ${CMAKE_CURRENT_SOURCE_DIR}/cmake/modules
       )

include_directories(./)
include(AddCompilerFlags)

add_c_compiler_flag(-Wnested-externs -Wstrict-prototypes)

add_compiler_flag(
            -Wall
            -Wextra
            -Wformat
            -Wvla
            -Wformat-security
            -Wcast-align
            -Wunused-parameter
            )


option(EXTRA_WARNINGS "Enable extra warnings" OFF)
if(EXTRA_WARNINGS)
    add_compiler_flag(-Wshadow)
    add_cxx_compiler_flag(-Wsuggest-override)
else()
    add_compiler_flag(-Wno-unused-parameter)
endif()


add_library(threadcontral
   scheduler.cpp
   sync.cpp
   threadinterrupt.cpp
)

target_link_libraries(threadcontral boost_chrono boost_filesystem boost_program_options boost_thread pthread)

#add_executable(tasktest main.cpp)

#target_link_libraries(tasktest threadcontral)

