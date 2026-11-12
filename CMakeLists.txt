#-------------------- CMAKE Configuration
cmake_minimum_required(VERSION 3.24)

# -Wno-dev by default (developer and deprecation warnings, mostly from the submodules): no need to
# pass it, nor to alias cmake with it, which breaks 'cmake --build' and 'cmake --install'.
# 'cmake -Wdev ...' shows them again.
if(NOT DEFINED CACHE{CMAKE_SUPPRESS_DEVELOPER_WARNINGS})
    set(CMAKE_SUPPRESS_DEVELOPER_WARNINGS ON CACHE INTERNAL "Suppress developer warnings (-Wno-dev)")
endif()

# Everything is generated in a build directory, never in the sources: from the project directory,
# 'cmake --preset default' (or 'cmake -B build'); from a build directory, 'cmake <path to project>'.
get_filename_component(_src "${CMAKE_SOURCE_DIR}" REALPATH)
get_filename_component(_bin "${CMAKE_BINARY_DIR}" REALPATH)
if(_src STREQUAL _bin)
    message(FATAL_ERROR
        "In-source builds are not allowed. Remove what this run (and any former in-source build) created:\n"
        "    cmake -P cmake/CleanInSourceBuild.cmake\n"
        "then, from the project directory:\n"
        "    cmake --preset default        (or: cmake -B build)\n"
        "    cmake --build build -j\n"
        "    cmake --build build --target install\n"
        "Other presets: debug (build-debug/), asan (build-asan/).\n"
        "Or from a build directory: mkdir build && cd build && cmake .. && make")
endif()

project(DeepSkyDisplay LANGUAGES CXX)

if(NOT WIN32)
    string(ASCII 27 Esc)
    set(ColourReset "${Esc}[m")
    set(ColourBold  "${Esc}[1m")
    set(Red         "${Esc}[31m")
    set(Green       "${Esc}[32m")
    set(Yellow      "${Esc}[33m")
    set(Blue        "${Esc}[34m")
    set(Magenta     "${Esc}[35m")
    set(Cyan        "${Esc}[36m")
    set(White       "${Esc}[37m")
    set(BoldRed     "${Esc}[1;31m")
    set(BoldGreen   "${Esc}[1;32m")
    set(BoldYellow  "${Esc}[1;33m")
    set(BoldBlue    "${Esc}[1;34m")
    set(BoldMagenta "${Esc}[1;35m")
    set(BoldCyan    "${Esc}[1;36m")
    set(BoldWhite   "${Esc}[1;37m")
endif()

message("${BoldCyan}----------- DeepSkyDisplay (DSTdisplay) -----------${ColourReset}")
message("${Green}-- Project source directory :${ColourReset} ${CMAKE_SOURCE_DIR}")
message("${Green}-- Project build directory  :${ColourReset} ${CMAKE_BINARY_DIR}")

if(DEFINED ENV{DeepSkyROOT})
    set(DeepSkyROOT $ENV{DeepSkyROOT})
else()
    set(DeepSkyROOT ${CMAKE_SOURCE_DIR}/..)
endif()
get_filename_component(DeepSkyROOT "${DeepSkyROOT}" ABSOLUTE)

if(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
    set(CMAKE_INSTALL_PREFIX ${DeepSkyROOT} CACHE PATH "default install path" FORCE)
endif()

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/modules")

#-------------------- Options
option(WITH_ASAN   "Build DSTdisplay and its tests with AddressSanitizer" OFF)
option(BUILD_TESTS "Build the GoogleTest suite" ON)
option(WITH_DEBUG  "Deprecated: same as CMAKE_BUILD_TYPE=Debug and WITH_ASAN=ON (use the asan preset)" OFF)

if(WITH_DEBUG)
    message(WARNING "WITH_DEBUG is deprecated: use 'cmake --preset asan' (Debug + AddressSanitizer, in build-asan/)")
    set(CMAKE_BUILD_TYPE Debug)
    set(WITH_ASAN ON)
endif()

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

# Products of this build: executables in bin/, libraries in lib/ (objects stay in CMakeFiles/)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

# The highest standard the compiler supports, from C++23 down
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED OFF)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
# compile_commands.json in the build directory, for clangd / IntelliSense
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

#-------------------- COMPILER
# These flags apply to every C++ target of this directory, the googletest submodule included.
set(_release "$<AND:$<COMPILE_LANGUAGE:CXX>,$<CONFIG:Release>>")
set(_debug   "$<AND:$<COMPILE_LANGUAGE:CXX>,$<CONFIG:Debug>>")

if(CMAKE_SYSTEM_NAME MATCHES "Linux")
    set(OperatingSystem "Linux")
    add_compile_options("$<$<COMPILE_LANGUAGE:CXX>:-Wall;-pipe;-pthread;-fexceptions;-Wno-unknown-pragmas;-Wmisleading-indentation;-mtune=native;-ffp-contract=on>")
    if(CMAKE_SYSTEM_PROCESSOR MATCHES "x86_64|amd64")
        add_compile_options("$<$<COMPILE_LANGUAGE:CXX>:-m64>")
        add_compile_definitions(Linuxx86_64)
    elseif(CMAKE_SYSTEM_PROCESSOR MATCHES "aarch64|arm64")
        add_compile_definitions(Linuxaarch64)
    endif()
    add_compile_options("$<${_release}:-flto>")
    add_link_options(-pthread "$<$<CONFIG:Release>:-flto>")
    # Interprocedural optimization (LTO) of the release build, when supported
    include(CheckIPOSupported)
    check_ipo_supported(RESULT ipo_supported OUTPUT ipo_error)
    if(ipo_supported)
        set(CMAKE_INTERPROCEDURAL_OPTIMIZATION_RELEASE ON)
    endif()
elseif(CMAKE_SYSTEM_NAME MATCHES "Darwin")
    set(OperatingSystem "Mac OS X")
    add_compile_options("$<$<COMPILE_LANGUAGE:CXX>:-Wall;-mcpu=native;-mtune=native;-ffp-contract=on>")
    add_compile_definitions(Darwin)
    if(CMAKE_SYSTEM_PROCESSOR MATCHES "arm64|aarch64")
        add_compile_definitions(Darwinarm64)
        set(CMAKE_OSX_ARCHITECTURES "arm64")
    elseif(CMAKE_SYSTEM_PROCESSOR MATCHES "x86_64|amd64")
        add_compile_options("$<$<COMPILE_LANGUAGE:CXX>:-m64>")
        add_compile_definitions(Darwinx86_64)
        set(CMAKE_OSX_ARCHITECTURES "x86_64")
    endif()
    add_compile_options("$<${_release}:-flto=thin>")
    add_link_options("$<$<CONFIG:Release>:-flto=thin>")
endif()

add_compile_options("$<${_release}:-O3;-fomit-frame-pointer>")
# No LTO in debug: on macOS it replaces the .o files with LLVM bitcode, leaving the debug map with
# empty OSO entries that LLDB cannot follow.
add_compile_options("$<${_debug}:-O0;-g>")
add_compile_definitions("$<$<CONFIG:Debug>:__DEBUG__>")

if(WITH_ASAN)
    add_compile_options("$<$<COMPILE_LANGUAGE:CXX>:-fsanitize=address;-fno-omit-frame-pointer;-g>")
    add_link_options(-fsanitize=address)
endif()

message("${BoldYellow}-- CMAKE INSTALL PREFIX : ${ColourReset}${CMAKE_INSTALL_PREFIX}")
message("${Green}-- DeepSkyROOT :${ColourReset} ${DeepSkyROOT}")
message("${Green}-- BUILD TYPE :${ColourReset} ${CMAKE_BUILD_TYPE}")
message("${Green}-- CXX COMPILER :${ColourReset} ${CMAKE_CXX_COMPILER}")
message("${Green}-- USE CXX STANDARD :${ColourReset} ${CMAKE_CXX_STANDARD}")
if(WITH_ASAN)
    message("${Blue}-- AddressSanitizer ACTIVATED${ColourReset}")
endif()

#-------------------- VERSION FROM THE GIT TAG
find_package(Git)
include(GetGitRevisionDescription)
git_describe(VERSION --always --tags --long)

string(REGEX REPLACE "^v([0-9]+)\\..*" "\\1" VERSION_MAJOR "${VERSION}")
string(REGEX REPLACE "^v[0-9]+\\.([0-9]+).*" "\\1" VERSION_MINOR "${VERSION}")
string(REGEX REPLACE "^v[0-9]+\\.[0-9]+\\.([0-9]+).*" "\\1" VERSION_PATCH "${VERSION}")
string(REGEX REPLACE "^v[0-9]+\\.[0-9]+\\.[0-9]+.(-*)" "\\1" VERSION_SHA1 "${VERSION}")
set(VERSION_SHORT "v${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_PATCH}")
message("-- PKG VERSION : ${VERSION} (${VERSION_SHORT}, sha1 ${VERSION_SHA1})")

set(CMAKE_PROJECT_VERSION       "${VERSION}")
set(CMAKE_PROJECT_VERSION_MAJOR "${VERSION_MAJOR}")
set(CMAKE_PROJECT_VERSION_MINOR "${VERSION_MINOR}")
set(CMAKE_PROJECT_VERSION_PATCH "${VERSION_PATCH}")

# A clone without a reachable tag (e.g. a shallow CI clone) has no numeric version: the library
# stays unversioned and no CMake package is installed.
if(VERSION MATCHES "^v[0-9]+\\.[0-9]+\\.[0-9]+")
    set(DSTdisplay_NUMERIC_VERSION "${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_PATCH}")
    set(PROJECT_VERSION "${DSTdisplay_NUMERIC_VERSION}")
else()
    message(WARNING "No version tag found (git describe gave '${VERSION}'): DSTdisplay will not be versioned")
endif()

# Generated sources stay in the build directory
set(GENERATED_DIR ${CMAKE_BINARY_DIR}/generated)
configure_file(${CMAKE_SOURCE_DIR}/modules/DSD_version.cxx.in ${GENERATED_DIR}/src/DSD_version.cxx)
configure_file(${CMAKE_SOURCE_DIR}/modules/gTest_version.h.in ${GENERATED_DIR}/test/gTest_version.h)

#-------------------- DeepSky libraries (installed in DeepSkyROOT)
message("${Red}----------- SEARCH DeepSky libraries -----------${ColourReset}")
set(DSTfits_MIN_VERSION 1.5)
# Only look in DeepSkyROOT: the RPATH points there, and CMake would otherwise fall back on another
# install (e.g. found through PATH) when the one in DeepSkyROOT is too old.
find_package(DSTfits ${DSTfits_MIN_VERSION} REQUIRED PATHS ${DeepSkyROOT} NO_DEFAULT_PATH)
# Imported targets are system targets by default, which turns ${DeepSkyROOT}/include into -isystem:
# it then loses its -I priority over other installed copies and hides warnings from our own headers.
# Needs CMake >= 3.25; older versions ignore the property and keep -isystem.
set_target_properties(DSTfits::DSTfits PROPERTIES SYSTEM OFF)
message("-- ${Green}Found${ColourReset} DSTfits ${DSTfits_VERSION}: ${DSTfits_DIR}")

# The DSTfits headers include cfitsio and Minuit2 headers, installed beside them
foreach(_dir cfitsio Minuit2)
    if(NOT EXISTS ${DeepSkyROOT}/include/${_dir})
        message(FATAL_ERROR "${DeepSkyROOT}/include/${_dir} not found: install deepskyfits first")
    endif()
endforeach()
add_library(deepsky_externals INTERFACE)
target_include_directories(deepsky_externals INTERFACE ${DeepSkyROOT}/include/cfitsio ${DeepSkyROOT}/include/Minuit2)
target_link_directories(deepsky_externals INTERFACE ${DeepSkyROOT}/lib)

#-------------------- Qt6 (optional: without it, the GUI is disabled)
find_package(Qt6 COMPONENTS Widgets Core Gui PrintSupport Concurrent)
if(Qt6_FOUND)
    message("${Blue}-- Qt6 FOUND : ${ColourReset} ${Qt6_VERSION}")
    set(QT_LIBS Qt6::Widgets Qt6::Core Qt6::Gui Qt6::PrintSupport Qt6::Concurrent)
    add_compile_definitions(__WITH_QT__)
    # Qt's code generators
    set(CMAKE_AUTOMOC ON)
    set(CMAKE_AUTOUIC ON)
    set(CMAKE_AUTORCC ON)
else()
    message("${Red}-- Qt6 NOT FOUND. GUI will be disabled${ColourReset}")
    set(QT_LIBS "")
endif()

#-------------------- RPATH
# This build comes first, then DeepSkyROOT: an installed copy must not replace what the tests are testing.
set(CMAKE_BUILD_RPATH "${CMAKE_LIBRARY_OUTPUT_DIRECTORY};${DeepSkyROOT}/lib")
set(CMAKE_INSTALL_RPATH "${DeepSkyROOT}/lib")
# Also the directories of the external libraries (Qt from Homebrew or conda, ...)
set(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)
if(CMAKE_SYSTEM_NAME MATCHES "Linux")
    # RPATH rather than RUNPATH: also used to find the dependencies of the DeepSky libraries
    add_link_options(-Wl,--disable-new-dtags)
endif()

#-------------------- libDSTdisplay
message("${Red}----------- libDSTdisplay -----------${ColourReset}")
set(DSTDISPLAY_SOURCES
    src/QCustomContours.cxx
    src/QtDysplay.cxx
    src/QtFITSviewer.cxx
    src/QtFitsPloting.cxx
    ${GENERATED_DIR}/src/DSD_version.cxx
)
file(GLOB DSTDISPLAY_PUBLIC_HEADERS ${CMAKE_SOURCE_DIR}/include/DSTdisplay/*.h)

add_library(DSTdisplay SHARED ${DSTDISPLAY_SOURCES} ${DSTDISPLAY_PUBLIC_HEADERS})
if(DSTdisplay_NUMERIC_VERSION)
    set_target_properties(DSTdisplay PROPERTIES
        VERSION   "${DSTdisplay_NUMERIC_VERSION}"
        SOVERSION "${VERSION_MAJOR}"
    )
endif()
target_include_directories(DSTdisplay PUBLIC
    ${CMAKE_SOURCE_DIR}/include ${CMAKE_SOURCE_DIR}/include/DSTdisplay ${CMAKE_SOURCE_DIR}/src)
target_link_libraries(DSTdisplay PUBLIC DSTfits::DSTfits deepsky_externals ${QT_LIBS})
# Installed with the public headers: src/QtFITSviewer.h and the header uic generates from its .ui
set(DSTDISPLAY_INSTALLED_HEADERS ${DSTDISPLAY_PUBLIC_HEADERS} ${CMAKE_SOURCE_DIR}/src/QtFITSviewer.h)
if(Qt6_FOUND)
    list(APPEND DSTDISPLAY_INSTALLED_HEADERS ${CMAKE_BINARY_DIR}/DSTdisplay_autogen/include/ui_QtFITSviewer.h)
endif()

#-------------------- Demonstration programs (util/)
file(GLOB UTIL_SOURCES CONFIGURE_DEPENDS ${CMAKE_SOURCE_DIR}/util/*.cxx)
foreach(util_source IN LISTS UTIL_SOURCES)
    get_filename_component(uname ${util_source} NAME_WE)
    add_executable(${uname} ${util_source})
    target_link_libraries(${uname} PRIVATE DSTdisplay)
endforeach()

#-------------------- Tests
if(BUILD_TESTS)
    message("${Red}----------- GoogleTest -----------${ColourReset}")
    if(NOT EXISTS ${CMAKE_SOURCE_DIR}/googletest/CMakeLists.txt)
        message(FATAL_ERROR "${BoldRed}googletest is missing: run 'git submodule update --init' (or configure with -DBUILD_TESTS=OFF)${ColourReset}")
    endif()
    set(DSTDISPLAY_TESTS ON)
    enable_testing()
    set(INSTALL_GTEST OFF CACHE BOOL "" FORCE)
    set(BUILD_GMOCK   OFF CACHE BOOL "" FORCE)
    set(BUILD_SHARED_LIBS OFF)
    add_subdirectory(googletest EXCLUDE_FROM_ALL)
    include(GoogleTest)

    # The tests run from the build directory: whatever they write (PNG plots, ...) stays there
    file(GLOB TEST_SOURCES CONFIGURE_DEPENDS ${CMAKE_SOURCE_DIR}/test/gTest_*.cxx)
    set(TEST_NAMES "")
    foreach(test_source IN LISTS TEST_SOURCES)
        get_filename_component(tname ${test_source} NAME_WE)
        list(APPEND TEST_NAMES ${tname})
        add_executable(${tname} ${test_source})
        target_link_libraries(${tname} PRIVATE DSTdisplay gtest gtest_main)
        target_include_directories(${tname} PRIVATE ${GENERATED_DIR}/test)
        # Tests that include QtFITSviewer.h need the generated ui_QtFITSviewer.h
        if(Qt6_FOUND)
            target_include_directories(${tname} PRIVATE ${CMAKE_BINARY_DIR}/DSTdisplay_autogen/include)
        endif()
        gtest_discover_tests(${tname} WORKING_DIRECTORY ${CMAKE_BINARY_DIR} PROPERTIES TIMEOUT 300)
    endforeach()
    message("${Blue}-- Test executables ${ColourReset} ${TEST_NAMES} (run with ctest)")
else()
    set(DSTDISPLAY_TESTS OFF)
endif()

#-------------------- Doxygen documentation (target: doc, written in build/doc)
find_package(Doxygen)
if(DOXYGEN_FOUND)
    # The Doxyfile paths are relative to the sources: run there, but write in the build directory
    file(READ ${CMAKE_SOURCE_DIR}/DSL.Doxyfile _doxyfile)
    string(APPEND _doxyfile "\nOUTPUT_DIRECTORY = ${CMAKE_BINARY_DIR}/doc\n")
    file(WRITE ${GENERATED_DIR}/DSL.Doxyfile "${_doxyfile}")
    set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS ${CMAKE_SOURCE_DIR}/DSL.Doxyfile)
    add_custom_target(doc
        COMMAND ${DOXYGEN_EXECUTABLE} ${GENERATED_DIR}/DSL.Doxyfile
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        COMMENT "Generating API documentation with Doxygen in ${CMAKE_BINARY_DIR}/doc"
        VERBATIM
    )
endif()

#-------------------- Installation
install(FILES ${DSTDISPLAY_INSTALLED_HEADERS} DESTINATION include/DSTdisplay)
install(TARGETS DSTdisplay
    RUNTIME DESTINATION exe
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
)

if(DSTdisplay_NUMERIC_VERSION)
    # CMake package, so that dependent projects can call find_package(DSTdisplay <version>)
    include(CMakePackageConfigHelpers)
    set(DSTdisplay_CMAKE_DIR lib/cmake/DSTdisplay)
    configure_package_config_file(
        ${CMAKE_SOURCE_DIR}/modules/DSTdisplayConfig.cmake.in
        ${GENERATED_DIR}/DSTdisplayConfig.cmake
        INSTALL_DESTINATION ${DSTdisplay_CMAKE_DIR}
    )
    write_basic_package_version_file(
        ${GENERATED_DIR}/DSTdisplayConfigVersion.cmake
        VERSION ${DSTdisplay_NUMERIC_VERSION}
        COMPATIBILITY AnyNewerVersion
    )
    install(FILES
        ${GENERATED_DIR}/DSTdisplayConfig.cmake
        ${GENERATED_DIR}/DSTdisplayConfigVersion.cmake
        DESTINATION ${DSTdisplay_CMAKE_DIR}
    )

    # CMake never removes files from the install directory: drop the versioned libraries
    # (libDSTdisplay.1.2.4.dylib, libDSTdisplay.so.1.2.4, ...) left behind by previous installs.
    install(CODE "set(DSTdisplay_KEEP_VERSIONS \"${DSTdisplay_NUMERIC_VERSION};${VERSION_MAJOR}\")")
    install(CODE [[
        file(GLOB _dsd_libs "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/libDSTdisplay.*")
        foreach(_dsd_lib ${_dsd_libs})
            get_filename_component(_dsd_name "${_dsd_lib}" NAME)
            if(_dsd_name MATCHES "^libDSTdisplay\\.(so\\.)?([0-9.]*[0-9])(\\.dylib)?$")
                list(FIND DSTdisplay_KEEP_VERSIONS "${CMAKE_MATCH_2}" _dsd_keep)
                if(_dsd_keep EQUAL -1)
                    message(STATUS "Removing old library: ${_dsd_lib}")
                    file(REMOVE "${_dsd_lib}")
                endif()
            endif()
        endforeach()
    ]])
endif()

#-------------------- Summary
message("${BoldGreen}----------- DeepSkyDisplay ${VERSION} configured -----------${ColourReset}")
message("${Green}-- Build directory  :${ColourReset} ${CMAKE_BINARY_DIR}")
message("${Green}-- Build type       :${ColourReset} ${CMAKE_BUILD_TYPE}")
message("${Green}-- AddressSanitizer :${ColourReset} ${WITH_ASAN}")
message("${Green}-- Tests            :${ColourReset} ${DSTDISPLAY_TESTS}")
message("${Green}-- DSTdisplay       :${ColourReset} ${DSTdisplay_NUMERIC_VERSION}")
message("${Green}-- DSTfits          :${ColourReset} ${DSTfits_VERSION}")
message("${Green}-- Qt6              :${ColourReset} ${Qt6_FOUND} ${Qt6_VERSION}")
message("${Green}-- Install prefix   :${ColourReset} ${CMAKE_INSTALL_PREFIX}")
if(WITH_ASAN)
    message("${Yellow}--                    AddressSanitizer build: installing it replaces the release libDSTdisplay${ColourReset}")
endif()
message("${BoldGreen}-------------------------------------------------${ColourReset}")
