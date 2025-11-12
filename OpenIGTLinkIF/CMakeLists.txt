cmake_minimum_required(VERSION 3.20.6...3.22.6 FATAL_ERROR)

set(MODULE_NAME "OpenIGTLinkIF")
set(MODULE_TITLE "OpenIGTLinkIF")

string(TOUPPER ${MODULE_NAME} MODULE_NAME_UPPER)

#-----------------------------------------------------------------------------
# OpenIGTlinkIF version number.
set(OpenIGTLinkIF_VERSION_MAJOR "2")
set(OpenIGTLinkIF_VERSION_MINOR "0")
set(OpenIGTLinkIF_VERSION_PATCH "0")

# --------------------------------------------------------------------------
# Check if version 2 protocol is available
# Features that depnd on OpenIGTLink Protocol 2 are only available
# if the OpenIGTLink library is compiled with OpenIGTLink_PROTOCOL_VERSION_2
# option.

#add_definitions(-DOpenIGTLinkIF_USE_VERSION_2)
#add_definitions(-DOpenIGTLinkIF_LIBRARY_VERSION="${OpenIGTLink_VERSION_MAJOR}-${OpenIGTLink_VERSION_MINOR}-${OpenIGTLink_VERSION_PATCH} for protocol ver. 2")
set(OpenIGTLinkIO_DIR "" CACHE PATH "OpenIGTLinkIO directory")
find_package(OpenIGTLinkIO REQUIRED PATHS ${OpenIGTLinkIO_DIR})

#-----------------------------------------------------------------------------
add_subdirectory(MRML)
add_subdirectory(Logic)
add_subdirectory(Widgets)

#-----------------------------------------------------------------------------
set(MODULE_EXPORT_DIRECTIVE "Q_SLICER_QTMODULES_OPENIGTLINKIF_EXPORT")

# Additional includes - Current_{source,binary} and Slicer_{Libs,Base} already included
set(MODULE_INCLUDE_DIRECTORIES
  ${vtkSlicer${MODULE_NAME}ModuleLogic_INCLUDE_DIRS}
  ${vtkSlicer${MODULE_NAME}ModuleMRML_INCLUDE_DIRS}
  ${qSlicer${MODULE_NAME}ModuleWidgets_INCLUDE_DIRS}
  )

set(MODULE_SRCS
  qSlicerOpenIGTLinkIFModule.cxx
  qSlicerOpenIGTLinkIFModule.h
  )

set(MODULE_MOC_SRCS
  qSlicerOpenIGTLinkIFModule.h
  )

set(MODULE_UI_SRCS
)

set(MODULE_TARGET_LIBRARIES
  vtkSlicerOpenIGTLinkIFModuleLogic
  vtkSlicerOpenIGTLinkIFModuleMRML
  qSlicerOpenIGTLinkIFModuleWidgets
  ${MRML_LIBRARIES}
  )

set(MODULE_RESOURCES
  )

#-----------------------------------------------------------------------------
slicerMacroBuildLoadableModule(
  NAME ${MODULE_NAME}
  TITLE ${MODULE_TITLE}
  EXPORT_DIRECTIVE ${MODULE_EXPORT_DIRECTIVE}
  INCLUDE_DIRECTORIES ${MODULE_INCLUDE_DIRECTORIES}
  SRCS ${MODULE_SRCS}
  MOC_SRCS ${MODULE_MOC_SRCS}
  UI_SRCS ${MODULE_UI_SRCS}
  TARGET_LIBRARIES ${MODULE_TARGET_LIBRARIES}
  RESOURCES ${MODULE_RESOURCES}
  )


if(BUILD_TESTING)
  add_subdirectory(Testing)
endif()
