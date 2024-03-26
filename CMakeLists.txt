cmake_minimum_required(VERSION 3.12)

set(CMAKE_CXX_STANDARD 20)

project(Cantor LANGUAGES CXX)

include_directories(src/core)
include_directories(src/sys)
include_directories(src/hash)
include_directories(src/render)
include_directories(src/font)
include_directories(src/draw)
include_directories(src/ui)
include_directories(src/third_party)

add_executable(${PROJECT_NAME} src/app/main.cpp)

set_target_properties(${PROJECT_NAME} PROPERTIES WIN32_EXECUTABLE TRUE)


if ( CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
    target_compile_options( ${PROJECT_NAME} PUBLIC
        -diagnostics:column
        -Wall
        -WL
        -nologo
        -GR-
        -EHa-
        -EHc-
        -FC
        -Zi
        -Zc:strictStrings-
        -DVC_EXTRALEAN
        -DINC_EMBED
        -wd5045 # spectre
        -wd4505 # unreferenced function removed
        -wd4456 # variable shadowing
        -wd4100 # unreferenced formal parameter
        -wd4189 # variable initialised but not referenced
        -wd5246 # initialisation of anon struct/union should be brace wrapped
        -wd4201 # nameless struct/union
        -wd4061 # no explicit case label in switch
        -wd5262 # implicit fall through
        -wd4127 # some constexpr blabber
        -wd4365 # '=' conversion
        -wd4820 # padding
        -wd4062 # switch unhandled cases
        )
endif()
