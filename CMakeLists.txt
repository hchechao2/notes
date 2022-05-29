cmake_minimum_required(VERSION 3.10)
project(coy VERSION 0.1)
set(CMAKE_CXX_STANDARD 17)
add_definitions("-Wall -Werror -g")

file(GLOB_RECURSE coy_sources ${PROJECT_SOURCE_DIR}/*/*.cpp)
add_library(coy STATIC ${coy_sources})
target_link_libraries(coy pthread)
target_include_directories(coy PUBLIC ./include)


