cmake_minimum_required(VERSION 3.12)
project(spright)

set(CMAKE_CONFIGURATION_TYPES "Debug" "Release")
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE "Release" CACHE STRING "Build type" FORCE)
endif()

find_package(Git)
if(NOT VERSION AND GIT_FOUND)
    execute_process(COMMAND ${GIT_EXECUTABLE} describe --tags
                  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
                  OUTPUT_VARIABLE VERSION
                  OUTPUT_STRIP_TRAILING_WHITESPACE)
endif()
configure_file("src/version.h.in" "${CMAKE_CURRENT_SOURCE_DIR}/src/_version.h")

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_EXTENSIONS OFF)

set(SOURCES
    src/common.cpp
    src/settings.cpp
    src/image.cpp
    src/image_draw.cpp
    src/image_io.cpp
    src/input.cpp
    src/InputParser.cpp
    src/Definition.cpp
    src/transforming.cpp
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
list(FILTER CHIPMUNK_SOURCES EXCLUDE REGEX ".*(Spring|Joint|Motor).c$")
add_compile_definitions(CP_SPACE_DISABLE_DEBUG_API)

set(SOURCES ${SOURCES}
    libs/rect_pack/stb_rect_pack.cpp
    libs/rect_pack/MaxRectsBinPack.cpp
    libs/stb/stb_impl.cpp
    libs/miniz/miniz.c
    libs/gifenc/gifenc.c
    libs/cpp-base64/base64.cpp
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

    if(MSVC)
        add_compile_options(/GR- /GL /Os /MP /W3 /wd4100)
        add_link_options($<$<CONFIG:RELEASE>:/LTCG>)
        add_compile_options($<$<CONFIG:RELEASE>:/MT>)
        add_compile_definitions(_CRT_SECURE_NO_WARNINGS _CRT_NONSTDC_NO_WARNINGS)
    endif()
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
option(PORTABLE "Install files for a portable package")
if(NOT PORTABLE AND NOT CMAKE_SYSTEM_NAME MATCHES "Windows")
    # normal installation
    install(TARGETS ${PROJECT_NAME} DESTINATION "bin" COMPONENT Application)
    install(DIRECTORY templates DESTINATION "share/spright" COMPONENT Application)
else()
    # portable package (.ZIP)
    set(DOC_FILES LICENSE README.md CHANGELOG.md THIRD-PARTY.md)
    install(TARGETS ${PROJECT_NAME} DESTINATION . COMPONENT Application)
    install(FILES ${DOC_FILES} DESTINATION . COMPONENT Application)
    install(DIRECTORY templates DESTINATION . COMPONENT Application)
endif()

# package
set(CPACK_PACKAGE_NAME "spright")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "An advanced sprite sheet packer and sprite annotator")
set(CPACK_PACKAGE_VENDOR "Albert Kalchmair")
set(CPACK_PACKAGE_INSTALL_DIRECTORY "spright")
set(CPACK_PACKAGE_EXECUTABLES "spright;spright")
set(CPACK_PACKAGE_HOMEPAGE_URL "https://github.com/houmain/spright")
set(CPACK_PACKAGE_CONTACT ${CPACK_PACKAGE_HOMEPAGE_URL})
set(CPACK_RESOURCE_FILE_README "${CMAKE_SOURCE_DIR}/README.md")
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_SOURCE_DIR}/LICENSE")
set(CPACK_STRIP_FILES TRUE)
set(CPACK_DEBIAN_PACKAGE_SECTION utils)
set(CPACK_RPM_PACKAGE_LICENSE "GPLv3")

if(VERSION MATCHES "^[0-9]+\.[0-9]+\.[0-9]+")
    string(REGEX MATCHALL "[0-9]+" VERSION_LIST "${VERSION}")
    list(GET VERSION_LIST 0 CPACK_PACKAGE_VERSION_MAJOR)
    list(GET VERSION_LIST 1 CPACK_PACKAGE_VERSION_MINOR)
    list(GET VERSION_LIST 2 CPACK_PACKAGE_VERSION_PATCH)
endif()

include(CPack)
