cmake_minimum_required(VERSION 3.5)

project(OpenIGTLinkIF)

#-----------------------------------------------------------------------------
# Extension meta-information
set(EXTENSION_HOMEPAGE "https://github.com/openigtlink/OpenIGTLinkIF")
set(EXTENSION_CATEGORY "IGT")
set(EXTENSION_CONTRIBUTORS "Junichi Tokuda (SPL), Longquan Chen (SPL)")
set(EXTENSION_DESCRIPTION "This extension provides support for procedures with ZFrame Registration")
set(EXTENSION_ICONURL "")
set(EXTENSION_SCREENSHOTURLS "")
set(EXTENSION_DEPENDS "")
set(EXTENSION_BUILD_SUBDIRECTORY inner-build)
#-----------------------------------------------------------------------------

set(SUPERBUILD_TOPLEVEL_PROJECT inner)

# Extension dependencies
find_package(Slicer REQUIRED)
include(${Slicer_USE_FILE})

INCLUDE(${CMAKE_ROOT}/Modules/ExternalProject.cmake)

INCLUDE(SuperBuild/External_OpenIGTLink.cmake)
SET(OpenIGTLinkIO_DEPENDENCIES OpenIGTLink)
INCLUDE(SuperBuild/External_OpenIGTLinkIO.cmake)
LIST(APPEND OpenIGTLinkIF_DEPENDENCIES OpenIGTLink)
LIST(APPEND OpenIGTLinkIF_DEPENDENCIES OpenIGTLinkIO)

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
include(${Slicer_EXTENSION_CPACK})
