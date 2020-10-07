cmake_minimum_required(VERSION 2.8)

project(utxodb)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
set(CMAKE_BUILD_TYPE "Debug")

add_subdirectory(threadpool)


include_directories(./include)
include_directories(./threadpool)

aux_source_directory(./src DIR_SRCS)

set(base_src src/main.cpp
			 src/common.cpp
			 src/db_mysql.cpp
			 src/rpc.cpp
			 src/syncer.cpp)
set(job_src src/job/base.cpp
			src/job/job.cpp)

set(link_lib curl mysqlclient event boost_system boost_filesystem boost_program_options glog gmp gmpxx threadcontral)

add_executable(dbutxo ${base_src} ${job_src})

target_link_libraries(dbutxo ${link_lib})
