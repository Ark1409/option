cmake_minimum_required(VERSION 3.11)

list(APPEND CMAKE_MODULE_PATH "${option_SOURCE_DIR}/cmake")

include(utils)
include(FetchContent)

find_package(GTest 1.12.1)
if(NOT GTest_FOUND)
    FetchContent_Declare(
        googletest
        GIT_REPOSITORY https://github.com/google/googletest.git
        GIT_TAG        58d77fa8070e8cec2dc1ed015d66b454c8d78850 # v1.12.1 - last C++11
    )
    FetchContent_MakeAvailable(googletest)
endif()

add_executable(option_test optional.cpp)
target_link_libraries(option_test GTest::gtest GTest::gtest_main option::option)
target_compile_definitions(option_test PRIVATE OPTION_OPTIONAL_BOOL=1)

add_warnings(option_test)
add_werror(option_test)

add_test(option_test option_test)
