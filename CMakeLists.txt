# Require at least cmake 2.8.4
cmake_minimum_required(VERSION 2.8.4 FATAL_ERROR)

# Name of this project
project(draw)

# Install in GNU-style directory layout  (copied from japan/CMakeLists.txt)
include(GNUInstallDirs)


message(STATUS "System name ${CMAKE_SYSTEM_NAME}")
#MAC specific variable
if(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
    set(CMAKE_MACOSX_RPATH ON)
    set(CMAKE_SKIP_BUILD_RPATH FALSE)
    set(CMAKE_BUILD_WITH_INSTALL_RPATH FALSE)
    set(CMAKE_INSTALL_RPATH "${CMAKE_INSTALL_PREFIX}/lib")
    set(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)
    list(FIND CMAKE_PLATFORM_IMPLICIT_LINK_DIRECTORIES "${CMAKE_INSTALL_PREFIX}/lib" isSystemDir)
    if("${isSystemDir}" STREQUAL "-1")
        set(CMAKE_INSTALL_RPATH "${CMAKE_INSTALL_PREFIX}/lib")
    endif()
endif()

# Local path for cmake modules, before ${CMAKE_ROOT}/Modules/ is checked
set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake/modules")

#  Install directory
if (CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
  message(STATUS "    Install-prefix was at default -> forcing it to the source-dir" )
  message(STATUS "    Use -DCMAKE_INSTALL_PREFIX=/usr/local to set to something else" )
  set (CMAKE_INSTALL_PREFIX "${CMAKE_SOURCE_DIR}"
        CACHE PATH "default install path" FORCE )
else()
  set(CMAKE_INSTALL_PREFIX ${CMAKE_SOURCE_DIR})
  set(CMAKE_INSTALL_RPATH "${CMAKE_INSTALL_PREFIX}/lib")
endif()

# Add the project include directory
include_directories(${CMAKE_CURRENT_BINARY_DIR}/include)

# Load ROOT and setup include directory
find_package(ROOT 6 REQUIRED Gui Minuit2)
include_directories(${ROOT_INCLUDE_DIR})

add_definitions(-std=c++11)


build_root_dictionary(
    draw                         # path to dictionary to generate
    ${PROJECT_SOURCE_DIR}/include/drawConfig.hh
    ${PROJECT_SOURCE_DIR}/include/draw.hh
    ${PROJECT_SOURCE_DIR}/include/libstrparse.hh
    TARGETS draw
    LINKDEF "${PROJECT_SOURCE_DIR}/include/drawLinkDef.h"  # ROOT linkDef file
    OPTIONS -p
)


include_directories(${PROJECT_SOURCE_DIR}/include)


#----------------------------------------------------------------------------
# Locate sources and headers for this project
# NB: headers are included so they will show up in IDEs
#
file(GLOB sources ${PROJECT_SOURCE_DIR}/src/*.cc)
file(GLOB headers ${PROJECT_SOURCE_DIR}/include/*.hh)

#----------------------------------------------------------------------------
# Add the executable, and link it to the Geant4 libraries
#----------------------------------------------------------------------------
# Generate dictionaries as well (so we can write objects to ROOT files
#
add_library(draw-lib SHARED ${sources} ${headers} drawDict.cxx)
set_target_properties(draw-lib PROPERTIES OUTPUT_NAME draw)
target_link_libraries(draw-lib PUBLIC  ROOT::Libraries)

add_executable(draw-bin draw.cc)
set_target_properties(draw-bin PROPERTIES OUTPUT_NAME draw)
target_link_libraries(draw-bin draw-lib)

#----------------------------------------------------------------------------
#
add_custom_target(draw DEPENDS draw-bin)

#----------------------------------------------------------------------------
# Install the executable to 'bin' directory under CMAKE_INSTALL_PREFIX
#
#install(TARGETS draw-bin DESTINATION ${CMAKE_INSTALL_BINDIR})
#install(TARGETS draw-lib DESTINATION ${CMAKE_INSTALL_LIBDIR})
