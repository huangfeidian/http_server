cmake_minimum_required(VERSION 3.1)
project(http-server)

find_package(spdlog CONFIG REQUIRED)
INCLUDE_DIRECTORIES(${spdlog_include_dirs})

# set(ASIO_DIR "D:\\Github\\asio-1.10.6\\include")
# include_directories(${ASIO_DIR})

add_definitions(-DASIO_STANDALONE)
add_definitions(-DASIO_MSVC)


set(CMAKE_CXX_STANDARD 17)
if(MSVC)
add_definitions(-DBOOST_ASIO_HAS_MOVE)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /std:c++latest")
endif(MSVC)

include_directories(${PROJECT_SOURCE_DIR}/include)

file(GLOB BASE_SRC  "${PROJECT_SOURCE_DIR}/src/*.cpp")

add_executable(echo_server ${BASE_SRC} ${PROJECT_SOURCE_DIR}/test/echo_test.cpp)