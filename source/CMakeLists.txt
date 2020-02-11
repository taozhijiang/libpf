cmake_minimum_required (VERSION 2.8.12)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -O2")
set(CMAKE_CXX_STANDARD 11)

aux_source_directory(. LIBPF_SOURCE)
add_library(pf STATIC ${LIBPF_SOURCE})

install(TARGETS pf
        RUNTIME DESTINATION BIN
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib/static )

