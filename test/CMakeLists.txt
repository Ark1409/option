cmake_minimum_required(VERSION 3.11)

list(APPEND CMAKE_MODULE_PATH "${option_SOURCE_DIR}/cmake")
include(utils)

find_package(GTest)
if(NOT GTest_FOUND)
    include(FetchContent)
    FetchContent_Declare(
        googletest
        GIT_REPOSITORY https://github.com/google/googletest.git
        GIT_TAG        58d77fa8070e8cec2dc1ed015d66b454c8d78850 # v1.12.1 - last C++11
    )
    FetchContent_MakeAvailable(googletest)
endif()

add_executable(option_test EXCLUDE_FROM_ALL optional.cpp)
target_link_libraries(option_test GTest::gtest GTest::gtest_main option::option)
set_target_properties(option_test PROPERTIES CXX_STANDARD 11)

add_warnings(option_test)
add_werror(option_test)

add_test(option_tests ${CMAKE_CTEST_COMMAND}
    --build-and-test "${CMAKE_CURRENT_SOURCE_DIR}" "${CMAKE_CURRENT_BINARY_DIR}"
    --build-generator ${CMAKE_GENERATOR}
    --build-makeprogram ${CMAKE_MAKE_PROGRAM}
    --build-nocmake
    --build-target option_test
    --test-command option_test
)
