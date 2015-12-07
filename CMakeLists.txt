CMAKE_MINIMUM_REQUIRED(VERSION 2.8)
project(arrayfire-demo)

# Compile with C++11 support.
include(CheckCXXCompilerFlag)
CHECK_CXX_COMPILER_FLAG("-std=c++11" COMPILER_SUPPORTS_CXX11)
CHECK_CXX_COMPILER_FLAG("-std=c++0x" COMPILER_SUPPORTS_CXX0X)
if(COMPILER_SUPPORTS_CXX11)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
elseif(COMPILER_SUPPORTS_CXX0X)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++0x")
else()
        message(STATUS "The compiler ${CMAKE_CXX_COMPILER} has no C++11 support. Please use a different C++ compiler.")
endif()

# Always use the optimized build
SET(CMAKE_BUILD_TYPE "Release")
# Enable if something needs to be debugged
#SET(CMAKE_BUILD_TYPE "Debug")

SET(EXECUTABLE_OUTPUT_PATH ${CMAKE_SOURCE_DIR}/bin)
SET(LIBRARY_OUTPUT_PATH ${CMAKE_SOURCE_DIR}/lib)
LIST(APPEND CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/CMakeModules)

add_subdirectory(src)

# CPack package generation
# General settings
SET(CPACK_GENERATOR "DEB;TGZ")
SET(CPACK_PACKAGE_NAME "xilinx_demos")
SET(CPACK_PACKAGE_VERSION 1.0.0)
SET(CPACK_PACKAGE_CONTACT "Peter Andreas Entschev <peter@arrayfire.com>")
SET(CPACK_PACKAGE_DESCRIPTION "A demonstration program for Xilinx FPGAs with ArrayFire algorithms")
SET(CPACK_PACKAGE_DESCRIPTION_SUMMARY "A demonstration program for Xilinx FPGAs with ArrayFire (http://www.arrayfire.com) algorithms")

# Debian-specific settings
SET(CPACK_DEBIAN_PACKAGE_ARCHITECTURE ${OS_ARCHITECTURE})
SET(CPACK_DEBIAN_PACKAGE_SECTION "devel")

# Windows (NSIS) specific settings:


# Run CPack
INCLUDE(CPack)
