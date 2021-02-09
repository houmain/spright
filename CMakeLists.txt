cmake_minimum_required(VERSION 3.12)
project(spright)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_EXTENSIONS OFF)

set(SOURCES
    src/common.cpp
    src/settings.cpp
    src/image.cpp
    src/input.cpp
    src/InputParser.cpp
    src/packing.cpp
    src/output.cpp
    src/main.cpp
    src/test.cpp
)

set(SOURCES ${SOURCES}
    libs/stb/stb_impl.c
    libs/miniz/miniz.c
    libs/texpack/packer.cpp
    libs/texpack/rbp/MaxRects.cpp
)

set(INCLUDES ${INCLUDES}
    libs
)

if(NOT MSVC)
    set_source_files_properties(${SOURCES}
       PROPERTIES COMPILE_FLAGS  "-Wall -Wextra -Wsign-conversion -Wconversion")
    link_libraries(stdc++fs)
else()
    add_compile_options(/GR- /W1)
    set(MSVC_SHARED_RUNTIME OFF)
endif()

file(GLOB_RECURSE HEADERS include *.h *.hpp *.inc)

if (WIN32)
    add_compile_definitions(UNICODE _WIN32_WINNT=0x0501)
endif()

include_directories(.)
include_directories(SYSTEM ${INCLUDES})
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${SOURCES} ${HEADERS})
add_executable(${PROJECT_NAME} ${SOURCES} ${HEADERS})

if(WIN32 AND NOT MSVC)
  target_link_options(${PROJECT_NAME} BEFORE PUBLIC -municode)
endif()

if(NOT WIN32)
  install(TARGETS ${PROJECT_NAME} DESTINATION "bin")
else(NOT WIN32)
  install(TARGETS ${PROJECT_NAME} DESTINATION .)
endif(NOT WIN32)

