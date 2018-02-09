cmake_minimum_required(VERSION 3.5)

project(SlicerOpenIGTLink)

#-----------------------------------------------------------------------------
# Extension meta-information
set(EXTENSION_HOMEPAGE "https://github.com/openigtlink/SlicerOpenIGTLink")
set(EXTENSION_CATEGORY "IGT")
set(EXTENSION_CONTRIBUTORS "Junichi Tokuda (SPL), Longquan Chen (SPL)")
set(EXTENSION_DESCRIPTION "This extension adds OpenIGTLink communication interface to 3D Slicer to exchange real-time imaging, tracking, and other data.")
set(EXTENSION_ICONURL "https://raw.githubusercontent.com/openigtlink/SlicerOpenIGTLink/master/OpenIGTLinkIF.png")
set(EXTENSION_SCREENSHOTURLS "https://raw.githubusercontent.com/openigtlink/SlicerOpenIGTLink/master/Screenshots/Overview.png")
set(EXTENSION_DEPENDS "")
set(EXTENSION_BUILD_SUBDIRECTORY inner-build)
#-----------------------------------------------------------------------------

set(SUPERBUILD_TOPLEVEL_PROJECT inner)

# Extension dependencies
find_package(Slicer REQUIRED)
include(${Slicer_USE_FILE})
mark_as_superbuild(Slicer_DIR)

option(Slicer_USE_VP9 "Build OpenIGTLinkIF with VP9 support" ON)
INCLUDE(${CMAKE_ROOT}/Modules/ExternalProject.cmake)

#-----------------------------------------------------------------------------
# SuperBuild setup
option(SlicerOpenIGTLink_SUPERBUILD "Build OpenIGTLink related modules and the projects it depends on." ON)
mark_as_advanced(SlicerOpenIGTLink_SUPERBUILD)
if(SlicerOpenIGTLink_SUPERBUILD)
  include("${CMAKE_CURRENT_SOURCE_DIR}/SuperBuild.cmake")
  return()
endif()

#-----------------------------------------------------------------------------
# Extension modules
add_subdirectory(OpenIGTLinkIF)
## NEXT_MODULE

#-----------------------------------------------------------------------------
include(${Slicer_EXTENSION_GENERATE_CONFIG}) 
set(CPACK_INSTALL_CMAKE_PROJECTS "${CPACK_INSTALL_CMAKE_PROJECTS};${CMAKE_BINARY_DIR};${EXTENSION_NAME};ALL;/")
set(CPACK_INSTALL_CMAKE_PROJECTS "${CPACK_INSTALL_CMAKE_PROJECTS};${OpenIGTLink_DIR};OpenIGTLink;RuntimeLibraries;/")
set(CPACK_INSTALL_CMAKE_PROJECTS "${CPACK_INSTALL_CMAKE_PROJECTS};${OpenIGTLinkIO_DIR};OpenIGTLinkIO;RuntimeLibraries;/")
include(${CMAKE_CURRENT_SOURCE_DIR}/CMake/SlicerBlockInstallVP9.cmake)
MESSAGE(STATUS "CPACK_INSTALL_CMAKE_PROJECTS = ${CPACK_INSTALL_CMAKE_PROJECTS}")
include(${Slicer_EXTENSION_CPACK})
#-----------------------------------------------------------------------------
if(NOT Slicer_SOURCE_DIR)
  # Configure 'OpenIGTLinkIFConfig.cmake.in'
  include(CMakePackageConfigHelpers)
  # For a build tree
  set(SHARE_DIR_CONFIG ${CMAKE_BINARY_DIR}/${Slicer_SHARE_DIR})
  set(OpenIGTLinkIO_DIR_CONFIG ${OpenIGTLinkIO_DIR})
  set(SlicerOpenIGTLink_CONFIG_CODE "####### Expanded from \@SlicerOpenIGTLink_CONFIG_CODE\@ #######\n")
  set(SlicerOpenIGTLink_CONFIG_CODE "${SlicerOpenIGTLink_CONFIG_CODE}##################################################")
  set(_config ${CMAKE_BINARY_DIR}/SlicerOpenIGTLinkConfig.cmake)
  configure_package_config_file(
      CMake/SlicerOpenIGTLinkConfig.cmake.in
      ${_config}
      INSTALL_DESTINATION ${CMAKE_BINARY_DIR}
      PATH_VARS SHARE_DIR_CONFIG OpenIGTLinkIO_DIR_CONFIG
      NO_CHECK_REQUIRED_COMPONENTS_MACRO
  )
  configure_package_config_file(
      CMake/OpenIGTLinkConfig.cmake.in
      ${CMAKE_BINARY_DIR}/OpenIGTLinkConfig.cmake
      INSTALL_DESTINATION ${CMAKE_BINARY_DIR}
      NO_CHECK_REQUIRED_COMPONENTS_MACRO
  )
  configure_package_config_file(
      CMake/OpenIGTLinkIOConfig.cmake.in
      ${CMAKE_BINARY_DIR}/OpenIGTLinkIOConfig.cmake
      INSTALL_DESTINATION ${CMAKE_BINARY_DIR}
  )
endif()
