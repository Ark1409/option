cmake_minimum_required(VERSION 3.13..3.28)

project(option VERSION 1.0.0 LANGUAGES CXX)

list(APPEND CMAKE_MODULE_PATH "${option_SOURCE_DIR}/cmake")

include(utils)

option(OPTION_TESTS "Build the tests directory")

add_library(option STATIC include/option/optional.hpp src/optional.cpp)
add_library(option::option ALIAS option)

target_include_directories(option PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>)
target_compile_features(option PUBLIC cxx_std_11)
set_target_properties(option PROPERTIES CXX_EXTENSIONS OFF)

if("${CMAKE_SOURCE_DIR}" STREQUAL "${CMAKE_CURRENT_SOURCE_DIR}")
    add_warnings(option)
    target_compile_definitions(option PRIVATE OPTION_OPTIONAL_BOOL=1)
endif()

include(CheckIPOSupported)
check_ipo_supported(RESULT ipo_sup)
if(ipo_sup)
    set_target_properties(option PROPERTIES INTERPROCEDURAL_OPTIMIZATION TRUE)
endif()

if(OPTION_TESTS)
    include(CTest)
    add_subdirectory(test)
endif()

include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

write_basic_package_version_file(
    "${CMAKE_CURRENT_BINARY_DIR}/optionConfigVersion.cmake"
    VERSION "${PROJECT_VERSION}"
    COMPATIBILITY SameMajorVersion
)
configure_package_config_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake/optionConfig.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/optionConfig.cmake"
    INSTALL_DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/option"
)

install(TARGETS option EXPORT optionTargets
    RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}"
    LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}"
    ARCHIVE DESTINATION "${CMAKE_INSTALL_LIBDIR}"
    INCLUDES DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}"
)
install(DIRECTORY include/ DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}")
install(EXPORT optionTargets
    DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/option"
    NAMESPACE option::
)
install(FILES "${CMAKE_CURRENT_BINARY_DIR}/optionConfig.cmake" "${CMAKE_CURRENT_BINARY_DIR}/optionConfigVersion.cmake"
    DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/option"
)
