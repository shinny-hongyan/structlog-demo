cmake_minimum_required(VERSION 3.13)
project(demo)

set(CMAKE_CXX_STANDARD 17)
include_directories(.)

# compile lib
add_library(structlog STATIC
        structlog/number.cpp
        structlog/string.cpp
        structlog/structlog.cpp
        )

# compile demo
add_executable(demo
        main.cpp
        )
target_link_libraries(demo structlog -lpthread)

