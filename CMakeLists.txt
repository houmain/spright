cmake_minimum_required(VERSION 3.12)
project(spright)

set(GIT_COMMIT_LAST_TAG "unknown")
find_package(Git)
if(GIT_FOUND)
    execute_process(COMMAND ${GIT_EXECUTABLE} describe --tags
                  WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
                  OUTPUT_VARIABLE GIT_COMMIT_LAST_TAG
                  OUTPUT_STRIP_TRAILING_WHITESPACE)
endif()
configure_file("src/version.h.in" "${CMAKE_SOURCE_DIR}/src/_version.h")

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_EXTENSIONS OFF)

set(SOURCES
    src/common.cpp
    src/settings.cpp
    src/image.cpp
    src/input.cpp
    src/InputParser.cpp
    src/trimming.cpp
    src/packing.cpp
    src/pack_binpack.cpp
    src/pack_compact.cpp
    src/pack_single.cpp
    src/pack_keep.cpp
    src/output.cpp
    src/globbing.cpp
    src/main.cpp
    libs/rect_pack/rect_pack.cpp
)

if(NOT MSVC)
    set_source_files_properties(${SOURCES}
       PROPERTIES COMPILE_FLAGS  "-Wall -Wextra -Wsign-conversion -Wconversion -Wno-missing-field-initializers")
    link_libraries(stdc++fs pthread)
else()
    add_compile_options(/GR- /W1)
    set(MSVC_SHARED_RUNTIME OFF)
endif()

file(GLOB CHIPMUNK_SOURCES libs/chipmunk/src/*.c)

set(SOURCES ${SOURCES}
    libs/rect_pack/stb_rect_pack.cpp
    libs/rect_pack/MaxRectsBinPack.cpp
    libs/stb/stb_impl.cpp
    libs/miniz/miniz.c
    libs/texpack/bleeding.cpp
    libs/humus/humus.cpp
    ${CHIPMUNK_SOURCES}
)

set(INCLUDES ${INCLUDES}
    libs
    libs/chipmunk/include
)

file(GLOB_RECURSE HEADERS include src/**.h libs/*.hpp libs/**.h )

option(ENABLE_PORTABLE "Enable portable build")
if(ENABLE_PORTABLE)
    if(NOT MSVC)
        add_compile_options(-flto -ffunction-sections -fdata-sections)
        set(CMAKE_EXE_LINKER_FLAGS " -static -s -Wl,--gc-sections")
        link_libraries(-static-libgcc -static-libstdc++)
    endif()
endif()

if (WIN32)
    add_compile_definitions(UNICODE _WIN32_WINNT=0x0501)

    set(SOURCES ${SOURCES}
        libs/utf8_console/utf8_console.cpp
    )
endif()

include_directories(.)
include_directories(SYSTEM ${INCLUDES})
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${SOURCES} ${HEADERS})
add_executable(${PROJECT_NAME} ${SOURCES} ${HEADERS})

if(WIN32 AND NOT MSVC)
  target_link_options(${PROJECT_NAME} BEFORE PUBLIC -municode)
endif()

option(ENABLE_TEST "Enable tests")
if(ENABLE_TEST)
    set(TEST_SOURCES ${SOURCES}
        test/test-main.cpp
        test/test-scope.cpp
        test/test-autocompletion.cpp
        test/test-packing.cpp
        test/test-performance.cpp
        test/test-globbing.cpp
        test/test-templates.cpp
    )
    list(REMOVE_ITEM TEST_SOURCES src/main.cpp)
    add_executable(spright-tests ${TEST_SOURCES})
    add_compile_definitions(EMBED_TEST_FILES)
endif()

if(NOT WIN32)
    install(TARGETS ${PROJECT_NAME} DESTINATION "bin")
else()
    install(TARGETS ${PROJECT_NAME} DESTINATION .)
endif()
