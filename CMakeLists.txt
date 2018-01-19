cmake_minimum_required(VERSION 3.5)

#-----------------------------------------------------------------------------
# Extension meta-information
set(EXTENSION_HOMEPAGE "https://github.com/openigtlink/SlicerOpenIGTLink")
set(EXTENSION_CATEGORY "IGT")
set(EXTENSION_CONTRIBUTORS "Junichi Tokuda (SPL), Longquan Chen (SPL)")
set(EXTENSION_DESCRIPTION "This extension adds OpenIGTLink communication interface to 3D Slicer to exchange real-time imaging, tracking, and other data.")
set(EXTENSION_ICONURL "https://github.com/openigtlink/SlicerOpenIGTLink/blob/master/OpenIGTLinkIF.png")
set(EXTENSION_SCREENSHOTURLS "https://github.com/openigtlink/SlicerOpenIGTLink/blob/master/Screenshots/Overview.png")
set(EXTENSION_DEPENDS "")
set(EXTENSION_BUILD_SUBDIRECTORY inner-build)
#-----------------------------------------------------------------------------

set(SUPERBUILD_TOPLEVEL_PROJECT inner)
project(OpenIGTLinkIF)
# Extension dependencies
find_package(Slicer REQUIRED)
include(${Slicer_USE_FILE})

INCLUDE(${CMAKE_ROOT}/Modules/ExternalProject.cmake)

#-----------------------------------------------------------------------------
# SuperBuild setup
option(OpenIGTLinkIF_SUPERBUILD "Build OpenIGTLinkIF and the projects it depends on." ON)
mark_as_advanced(OpenIGTLinkIF_SUPERBUILD)
if(OpenIGTLinkIF_SUPERBUILD)
  include("${CMAKE_CURRENT_SOURCE_DIR}/SuperBuild.cmake")
  return()
endif()

#-----------------------------------------------------------------------------
# Extension modules
add_subdirectory(OpenIGTLinkIF)
## NEXT_MODULE

#-----------------------------------------------------------------------------
include(${Slicer_EXTENSION_GENERATE_CONFIG})
set(CPACK_INSTALL_CMAKE_PROJECTS "${CPACK_INSTALL_CMAKE_PROJECTS};${CMAKE_BINARY_DIR};OpenIGTLinkIF;ALL;/")
include(${Slicer_EXTENSION_CPACK})
