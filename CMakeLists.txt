cmake_minimum_required(VERSION 3.11)

project(option)

list(APPEND CMAKE_MODULE_PATH "${option_SOURCE_DIR}/cmake")

include(utils)

option(OPTION_TESTS "Build the tests directory")

add_library(option STATIC src/optional.cpp)
add_library(option::option ALIAS option)

target_include_directories(option PUBLIC include)
target_compile_features(option PUBLIC cxx_std_11)
set_target_properties(option PROPERTIES CXX_EXTENSIONS OFF)

add_warnings(option)

include(CheckIPOSupported)
check_ipo_supported(RESULT ipo_sup)
if(ipo_sup)
    set_property(TARGET option PROPERTY INTERPROCEDURAL_OPTIMIZATION TRUE)
endif()

if(OPTION_TESTS)
    include(CTest)
    add_subdirectory(test)
endif()
