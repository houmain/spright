cmake_minimum_required(VERSION 3.12)
project(spright)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE "Release" CACHE STRING "Build type" FORCE)
endif()

find_package(Git)
if(NOT VERSION AND GIT_FOUND)
    execute_process(COMMAND ${GIT_EXECUTABLE} describe --tags
                  WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
                  OUTPUT_VARIABLE VERSION
                  OUTPUT_STRIP_TRAILING_WHITESPACE)
endif()
configure_file("src/version.h.in" "${CMAKE_SOURCE_DIR}/src/_version.h")

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_EXTENSIONS OFF)

set(SOURCES
    src/common.cpp
    src/Rect.cpp
    src/settings.cpp
    src/image.cpp
    src/input.cpp
    src/InputParser.cpp
    src/Definition.cpp
    src/trimming.cpp
    src/packing.cpp
    src/pack_binpack.cpp
    src/pack_compact.cpp
    src/pack_single.cpp
    src/pack_origin.cpp
    src/pack_keep.cpp
    src/pack_lines.cpp
    src/output_texture.cpp
    src/output_description.cpp
    src/globbing.cpp
    src/debug.cpp
    src/main.cpp
    libs/rect_pack/rect_pack.cpp
)

if(NOT MSVC)
    set_source_files_properties(${SOURCES}
       PROPERTIES COMPILE_FLAGS  "-Wall -Wextra -Wsign-conversion -Wconversion -Wno-missing-field-initializers")
endif()

file(GLOB CHIPMUNK_SOURCES libs/chipmunk/src/*.c)

set(SOURCES ${SOURCES}
    libs/rect_pack/stb_rect_pack.cpp
    libs/rect_pack/MaxRectsBinPack.cpp
    libs/stb/stb_impl.cpp
    libs/miniz/miniz.c
    libs/gifenc/gifenc.c
    ${CHIPMUNK_SOURCES}
)

set(INCLUDES ${INCLUDES}
    libs
    libs/chipmunk/include
)
file(GLOB_RECURSE HEADERS include src/**.h libs/*.hpp libs/**.h )

if(UNIX AND NOT APPLE)
    link_libraries(stdc++fs pthread)

elseif(WIN32)
    add_compile_definitions(UNICODE _WIN32_WINNT=0x0501)
    set(SOURCES ${SOURCES}
        libs/utf8_console/utf8_console.cpp
    )

    add_compile_options(/GR- /W1 /permissive- /Zc:hiddenFriend-)
    set(MSVC_SHARED_RUNTIME OFF)
endif()

include_directories(.)
include_directories(SYSTEM ${INCLUDES})
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${SOURCES} ${HEADERS})
add_executable(${PROJECT_NAME} ${SOURCES} ${HEADERS})

option(ENABLE_TEST "Enable tests")
if(ENABLE_TEST)
    set(TEST_SOURCES ${SOURCES}
        test/catch.cpp
        test/test-main.cpp
        test/test-common.cpp
        test/test-scope.cpp
        test/test-autocompletion.cpp
        test/test-packing.cpp
        test/test-performance.cpp
        test/test-globbing.cpp
        test/test-templates.cpp
        test/test-pivot.cpp
    )
    list(REMOVE_ITEM TEST_SOURCES src/main.cpp)
    set(CMAKE_CXX_STANDARD 20)
    add_executable(spright-tests ${TEST_SOURCES})
    add_compile_definitions(EMBED_TEST_FILES)
endif()

# install
set(DOC_FILES LICENSE README.md CHANGELOG.md THIRD-PARTY.md)
install(TARGETS ${PROJECT_NAME} DESTINATION . COMPONENT Application)
install(FILES ${DOC_FILES} DESTINATION . COMPONENT Application)

# package
set(CPACK_PACKAGE_NAME "spright")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "spright")
set(CPACK_PACKAGE_VENDOR "Albert Kalchmair")
set(CPACK_PACKAGE_INSTALL_DIRECTORY "spright")
set(CPACK_PACKAGE_EXECUTABLES "spright;spright")
set(CPACK_PACKAGE_HOMEPAGE_URL "https://github.com/houmain/spright")
set(CPACK_RESOURCE_FILE_README "${CMAKE_SOURCE_DIR}/README.md")
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_SOURCE_DIR}/LICENSE")
set(CPACK_STRIP_FILES TRUE)
if(VERSION MATCHES "^[0-9]+\.[0-9]+\.[0-9]+")
    string(REGEX MATCHALL "[0-9]+" VERSION_LIST "${VERSION}")
    list(GET VERSION_LIST 0 CPACK_PACKAGE_VERSION_MAJOR)
    list(GET VERSION_LIST 1 CPACK_PACKAGE_VERSION_MINOR)
    list(GET VERSION_LIST 2 CPACK_PACKAGE_VERSION_PATCH)
endif()
include(CPack)

