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
set(EXTENSION_DEPENDS "NA")
set(EXTENSION_BUILD_SUBDIRECTORY inner-build)

set(SUPERBUILD_TOPLEVEL_PROJECT inner)

#-----------------------------------------------------------------------------
# Extension dependencies
find_package(Slicer REQUIRED)
include(${Slicer_USE_FILE})
mark_as_superbuild(Slicer_DIR)

#-----------------------------------------------------------------------------
# Options
option(SlicerOpenIGTLink_USE_VP9 "Build SlicerOpenIGTLink with VP9 support" OFF)
mark_as_superbuild(SlicerOpenIGTLink_USE_VP9)

#-----------------------------------------------------------------------------
# SuperBuild setup
option(${EXTENSION_NAME}_SUPERBUILD "Build ${EXTENSION_NAME} and the projects it depends on." ON)
mark_as_advanced(${EXTENSION_NAME}_SUPERBUILD)
if(${EXTENSION_NAME}_SUPERBUILD)
  include("${CMAKE_CURRENT_SOURCE_DIR}/SuperBuild.cmake")
  return()
endif()

#-----------------------------------------------------------------------------
# Extension modules
add_subdirectory(OpenIGTLinkIF)
add_subdirectory(OpenIGTLinkRemote)
add_subdirectory(PlusRemote)
add_subdirectory(UltrasoundRemoteControl)
add_subdirectory(GenericSerialDeviceRemoteControl)
## NEXT_MODULE

#-----------------------------------------------------------------------------
# Generate SlicerOpenIGTLinkConfig.cmake file
# Add OpenIGTLink and OpenIGTLinkIO targets to SlicerOpenIGTLinkConfig.cmake file
set(SlicerOpenIGTLink_CUSTOM_CONFIG "
include(\"${OpenIGTLink_DIR}/OpenIGTLinkConfig.cmake\")
include(\"${OpenIGTLinkIO_DIR}/OpenIGTLinkIOConfig.cmake\")
")
include(${Slicer_EXTENSION_GENERATE_CONFIG})

#-----------------------------------------------------------------------------
# Packaging
set(EXTENSION_CPACK_INSTALL_CMAKE_PROJECTS)
list(APPEND EXTENSION_CPACK_INSTALL_CMAKE_PROJECTS "${OpenIGTLink_DIR};OpenIGTLink;RuntimeLibraries;/")
list(APPEND EXTENSION_CPACK_INSTALL_CMAKE_PROJECTS "${OpenIGTLinkIO_DIR};OpenIGTLinkIO;RuntimeLibraries;/")
set(${EXTENSION_NAME}_CPACK_INSTALL_CMAKE_PROJECTS "${EXTENSION_CPACK_INSTALL_CMAKE_PROJECTS}" CACHE STRING "List of external projects to install" FORCE)
list(APPEND CPACK_INSTALL_CMAKE_PROJECTS "${CMAKE_BINARY_DIR};${EXTENSION_NAME};ALL;/")
list(APPEND CPACK_INSTALL_CMAKE_PROJECTS "${${EXTENSION_NAME}_CPACK_INSTALL_CMAKE_PROJECTS}")
include(${CMAKE_CURRENT_SOURCE_DIR}/CMake/SlicerBlockInstallVP9.cmake)
include(${Slicer_EXTENSION_CPACK})
