cmake_minimum_required(VERSION 3.16)
project(XuperTV LANGUAGES CXX)

# Desktop (Windows / Linux) build. The Nintendo Switch build keeps using the
# devkitPro Makefile; see WINDOWS.md for the Windows instructions.
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  set(CMAKE_BUILD_TYPE Release)
endif()

option(XUPER_FETCH_MBEDTLS "Download and build mbedTLS 3.6 instead of using the system copy" OFF)
option(XUPER_BUILD_TESTS "Build the unit tests (ctest)" ON)

find_package(PkgConfig REQUIRED)
find_package(Threads REQUIRED)
find_package(JPEG REQUIRED)
pkg_check_modules(SDL2 REQUIRED IMPORTED_TARGET sdl2)
pkg_check_modules(SDL2_TTF REQUIRED IMPORTED_TARGET SDL2_ttf)
pkg_check_modules(MPV REQUIRED IMPORTED_TARGET mpv)
pkg_check_modules(CURL REQUIRED IMPORTED_TARGET libcurl)
pkg_check_modules(PNG REQUIRED IMPORTED_TARGET libpng)
pkg_check_modules(WEBP REQUIRED IMPORTED_TARGET libwebp)

# mbedTLS only supplies MD5, AES, 3DES and base64 here (mbedcrypto).
if(XUPER_FETCH_MBEDTLS)
  include(FetchContent)
  set(ENABLE_TESTING OFF CACHE BOOL "" FORCE)
  set(ENABLE_PROGRAMS OFF CACHE BOOL "" FORCE)
  set(MBEDTLS_FATAL_WARNINGS OFF CACHE BOOL "" FORCE)
  set(USE_SHARED_MBEDTLS_LIBRARY OFF CACHE BOOL "" FORCE)
  set(USE_STATIC_MBEDTLS_LIBRARY ON CACHE BOOL "" FORCE)
  FetchContent_Declare(mbedtls
    GIT_REPOSITORY https://github.com/Mbed-TLS/mbedtls.git
    GIT_TAG mbedtls-3.6.2
    GIT_SHALLOW ON)
  FetchContent_MakeAvailable(mbedtls)
  set(XUPER_MBEDCRYPTO mbedcrypto)
else()
  find_library(XUPER_MBEDCRYPTO NAMES mbedcrypto REQUIRED)
  find_path(XUPER_MBEDTLS_INCLUDE NAMES mbedtls/md5.h REQUIRED)
  if(XUPER_MBEDTLS_INCLUDE)
    include_directories(${XUPER_MBEDTLS_INCLUDE})
  endif()
endif()

include_directories(include include/vendor)
if(MSVC)
  add_compile_options(/utf-8 /W3)
else()
  add_compile_options(-Wall -Wextra)
endif()

set(XUPER_CORE source/catalog.cpp source/codec.cpp source/ranger_codec.cpp)

add_executable(XuperTV
  source/main.cpp source/player.cpp source/view.cpp source/artwork.cpp source/trace.cpp
  source/service.cpp source/local_bridge.cpp ${XUPER_CORE})
target_link_libraries(XuperTV PRIVATE
  PkgConfig::SDL2 PkgConfig::SDL2_TTF PkgConfig::MPV PkgConfig::CURL
  PkgConfig::PNG PkgConfig::WEBP JPEG::JPEG ${XUPER_MBEDCRYPTO} Threads::Threads)

if(WIN32)
  # WebView2 frontend is hosted with the MSYS2 MinGW-compatible webview headers.
  find_path(WEBVIEW_INCLUDE_DIR NAMES webview/webview.h REQUIRED)
  find_path(WEBVIEW2_INCLUDE_DIR NAMES WebView2.h REQUIRED)
  target_include_directories(XuperTV PRIVATE ${WEBVIEW_INCLUDE_DIR} ${WEBVIEW2_INCLUDE_DIR})
  target_sources(XuperTV PRIVATE source/web_ui.cpp)
  target_link_libraries(XuperTV PRIVATE advapi32 ole32 shell32 shlwapi user32 version winhttp ws2_32)
  add_custom_command(TARGET XuperTV POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
            ${CMAKE_SOURCE_DIR}/web-ui ${CMAKE_CURRENT_BINARY_DIR}/web-ui)
endif()

if(XUPER_BUILD_TESTS)
  enable_testing()
  function(xuper_test name)
    add_executable(${name} ${ARGN})
    target_link_libraries(${name} PRIVATE ${XUPER_MBEDCRYPTO})
    # The tests rely on assert() (some with side effects), so keep it in Release
    # builds, and run from the repo root where tests/fixtures lives.
    if(MSVC)
      target_compile_options(${name} PRIVATE /UNDEBUG)
    else()
      target_compile_options(${name} PRIVATE -UNDEBUG)
    endif()
    add_test(NAME ${name} COMMAND ${name} WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
  endfunction()
  xuper_test(catalog_test tests/catalog_test.cpp source/catalog.cpp)
  xuper_test(codec_test tests/codec_test.cpp source/codec.cpp)
  xuper_test(ranger_codec_test tests/ranger_codec_test.cpp source/ranger_codec.cpp)
  xuper_test(saved_catalog_test tests/saved_catalog_test.cpp source/catalog.cpp)
  xuper_test(playback_state_test tests/playback_state_test.cpp)
  xuper_test(playback_error_test tests/playback_error_test.cpp)
  xuper_test(installation_test tests/installation_test.cpp)
  xuper_test(platform_test tests/platform_test.cpp)
endif()
