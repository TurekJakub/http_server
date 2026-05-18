include(ExternalProject)
include(FetchContent)

cmake_minimum_required(VERSION 4.0)

project(cpp_programming_project LANGUAGES CXX)

if(CMAKE_CXX_COMPILER_ID MATCHES "GNU" OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    add_compile_options(-Wall -Wextra -pedantic -Werror -Wreorder)
else(CMAKE_CXX_COMPILER_ID MATCHES "MSVC")
    add_compile_options(/W4 /WX)
endif()

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib")
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(ASIO_VERSION asio-1-36-0)
SET(OPEN_SSL_VERSION openssl-3.6.0)

if(UNIX)
    set(OPEN_SSL_BUILD_TOOL make)
    set(EXTRA_BUILD_FLAGS)
    set(EXTRA_LINKING)
    set(LIB_FILE_EXTENSION .a)
else(WIN32)
    set(OPEN_SSL_BUILD_TOOL nmake)
    set(LIB_FILE_EXTENSION .lib)
    set(EXTRA_LINKING Crypt32.lib)


    find_program(NASM_EXECUTABLE nasm)
    if(NASM_EXECUTABLE)
        set(EXTRA_BUILD_FLAGS)
    else()
        set(EXTRA_BUILD_FLAGS no-asm)
    endif()
endif()

find_package(Perl)

if(NOT ${Perl_FOUND} AND WIN32)
    if(CMAKE_SIZEOF_VOID_P EQUAL 8)
        set(DOWNLOAD_URL https://github.com/StrawberryPerl/Perl-Dist-Strawberry/releases/download/SP_54201_64bit/strawberry-perl-5.42.0.1-64bit-portable.zip)
        set(DOWNLOAD_HASH a1cde185656cf307b51670eed69f648b9eff15b5c518cb136e027c628e650b71)
    else()
        set(DOWNLOAD_URL https://strawberryperl.com/download/5.32.1.1/strawberry-perl-no64-5.32.1.1-32bit-portable.zip)
        set(DOWNLOAD_HASH 1d82f0b6e64ca937b41073d87fb3309e901e62c8c1edf9b0cc4c8469c6064965)
    endif()

    set(INSTALL_DIR "${CMAKE_CURRENT_BINARY_DIR}/strawberry_perl-prefix/src/strawberry_perl")
    set(PERL_DEPENDENCY strawberry_perl)


    ExternalProject_Add(strawberry_perl
        URL ${DOWNLOAD_URL}
        URL_HASH SHA256=${DOWNLOAD_HASH}
        DOWNLOAD_NAME "strawberry_perl.zip"

        CONFIGURE_COMMAND ""
        BUILD_COMMAND ""
        INSTALL_COMMAND ""

        LOG_DOWNLOAD 1
    )

    set(PERL_EXECUTABLE ${INSTALL_DIR}/perl/bin/perl.exe)
else()
    set(PERL_DEPENDENCY)
endif()

ExternalProject_Add(asio
    GIT_REPOSITORY "https://github.com/chriskohlhoff/asio.git"
    GIT_TAG        ${ASIO_VERSION}

    CONFIGURE_COMMAND ""
    BUILD_COMMAND ""
    INSTALL_COMMAND ""

    LOG_DOWNLOAD 1
)

ExternalProject_Get_Property(asio SOURCE_DIR)
set(ASIO_SOURCE_DIR ${SOURCE_DIR})

set(OPENSSL_DIR ${CMAKE_BINARY_DIR}/openssl-install)
get_filename_component(OPENSSL_DIR_ABS ${OPENSSL_DIR} ABSOLUTE)

ExternalProject_Add(openssl
    GIT_REPOSITORY "https://github.com/openssl/openssl.git"
    GIT_TAG        ${OPEN_SSL_VERSION}
    GIT_SUBMODULES ""  

    BUILD_IN_SOURCE 1

    DEPENDS ${PERL_DEPENDENCY}
    UPDATE_COMMAND ""
    CONFIGURE_COMMAND ${PERL_EXECUTABLE} <SOURCE_DIR>/Configure no-shared no-tests ${EXTRA_BUILD_FLAGS} --openssldir=${OPENSSL_DIR_ABS} --prefix=${OPENSSL_DIR_ABS}
    BUILD_COMMAND "${OPEN_SSL_BUILD_TOOL}"
    INSTALL_COMMAND ""

    LOG_CONFIGURE 1
    LOG_BUILD 1
    LOG_DOWNLOAD 1

)

ExternalProject_Get_Property(openssl SOURCE_DIR)
set(OPEN_SSL_SOURCE_DIR ${SOURCE_DIR})

add_library(OpenSSL_SSL STATIC IMPORTED)
set_property(TARGET OpenSSL_SSL PROPERTY IMPORTED_LOCATION ${OPEN_SSL_SOURCE_DIR}/libssl${LIB_FILE_EXTENSION})
add_dependencies(OpenSSL_SSL openssl)

add_library(OpenSSL_Crypto STATIC IMPORTED)
set_property(TARGET OpenSSL_Crypto PROPERTY IMPORTED_LOCATION ${OPEN_SSL_SOURCE_DIR}/libcrypto${LIB_FILE_EXTENSION})
add_dependencies(OpenSSL_Crypto openssl)

if(CMAKE_CXX_COMPILER_ID MATCHES "MSVC")
    target_compile_definitions(http_server PRIVATE  _WIN32_WINNT=0x0A00)
endif()

add_library(http_lib STATIC
    src/libs/http/http.cpp
    src/libs/utils/string_utils.cpp
    src/libs/http/server.cpp
    src/libs/utils/fs_utils.cpp
    src/libs/http/mime_type.cpp
)

add_dependencies(http_lib openssl asio)

target_include_directories(http_lib PUBLIC
    include
)

target_include_directories(http_lib PRIVATE
    ${ASIO_SOURCE_DIR}/asio/include
    ${OPEN_SSL_SOURCE_DIR}/include
    src/libs
)

target_link_libraries(http_lib PRIVATE
    OpenSSL_SSL
    OpenSSL_Crypto
    ${EXTRA_LINKING}
)

if(CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME)

FetchContent_Declare(
    tomlplusplus
    GIT_REPOSITORY https://github.com/marzer/tomlplusplus.git
    GIT_TAG        v3.4.0
)
FetchContent_MakeAvailable(tomlplusplus)

add_executable(http_server 
    src/http_server/main.cpp
    src/http_server/config_parser.cpp
    src/http_server/handlers/static_files_handler.cpp
    src/http_server/handlers/default_handler.cpp
    src/http_server/static_file_server.cpp   
)

target_link_libraries(http_server
    PRIVATE
    http_lib
    tomlplusplus::tomlplusplus
)
endif()

