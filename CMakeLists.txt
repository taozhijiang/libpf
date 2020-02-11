cmake_minimum_required (VERSION 2.8.12)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -O2")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Woverloaded-virtual -Wpointer-arith -Wwrite-strings -Werror=format -march=native " )
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-sign-compare " )
set(CMAKE_CXX_STANDARD 11)

project(libpf-demo)

# 编译结果中的库和可执行文件的路径
set(EXECUTABLE_OUTPUT_PATH ${CMAKE_BINARY_DIR}/bin)
set(LIBRARY_OUTPUT_PATH ${CMAKE_BINARY_DIR}/lib)

set(DEPENDENCY_PREFIX "../../opensource/")
set(INCLUDE_DIRS
        ${PROJECT_SOURCE_DIR}/
        ${PROJECT_SOURCE_DIR}/include
        )
include_directories( ${INCLUDE_DIRS} )

link_directories(
        ${PROJECT_SOURCE_DIR}/lib/
)

# 库的主程序代码路径
add_subdirectory( source/)

add_executable(libpf-demo main.cpp)
set (EXTRA_LIBS ${EXTRA_LIBS} pf pthread)
target_link_libraries(
        libpf-demo
        ${EXTRA_LIBS}
)