cmake_minimum_required(VERSION 2.8)

if(POLICY CMP0048)
  cmake_policy(SET CMP0048 NEW) # CMake 3.0.0
endif()

set(MODULE_NAME "OpenIGTLinkIF")
set(MODULE_TITLE "OpenIGTLink I/F")

string(TOUPPER ${MODULE_NAME} MODULE_NAME_UPPER)

#-----------------------------------------------------------------------------
find_package(Slicer REQUIRED)
include(${Slicer_USE_FILE})

#-----------------------------------------------------------------------------
# OpenIGTlinkIF version number.
SET(OpenIGTLinkIF_VERSION_MAJOR "2")
SET(OpenIGTLinkIF_VERSION_MINOR "0")
SET(OpenIGTLinkIF_VERSION_PATCH "0")

# --------------------------------------------------------------------------
# Check if version 2 protocol is available
# Features that depnd on OpenIGTLink Protocol 2 are only available
# if the OpenIGTLink library is compiled with OpenIGTLink_PROTOCOL_VERSION_2
# option.

#add_definitions(-DOpenIGTLinkIF_USE_VERSION_2)
#add_definitions(-DOpenIGTLinkIF_LIBRARY_VERSION="${OpenIGTLink_VERSION_MAJOR}-${OpenIGTLink_VERSION_MINOR}-${OpenIGTLink_VERSION_PATCH} for protocol ver. 2")

INCLUDE(${CMAKE_ROOT}/Modules/ExternalProject.cmake)

INCLUDE(SuperBuild/External_OpenIGTLink.cmake)
SET(OpenIGTLinkIO_DEPENDENCIES OpenIGTLinkLib)
INCLUDE(SuperBuild/External_OpenIGTLinkIO.cmake)
if("${OpenIGTLink_VERSION_MAJOR}.${OpenIGTLink_VERSION_MINOR}.${OpenIGTLink_VERSION_PATCH}" LESS 3.1.0)
    message(FATAL_ERROR "Version of OpenIGTLink should be at less 3.1.0.")
endif()
LIST(APPEND OpenIGTLinkIF_DEPENDENCIES OpenIGTLinkLib)
LIST(APPEND OpenIGTLinkIF_DEPENDENCIES OpenIGTLinkIO)
#-----------------------------------------------------------------------------
add_subdirectory(MRML)
add_subdirectory(Logic)

#-----------------------------------------------------------------------------
set(MODULE_EXPORT_DIRECTIVE "Q_SLICER_QTMODULES_OPENIGTLINKIF_EXPORT")

# Additional includes - Current_{source,binary} and Slicer_{Libs,Base} already included
set(MODULE_INCLUDE_DIRECTORIES
  ${CMAKE_CURRENT_SOURCE_DIR}/GUI
  ${CMAKE_CURRENT_BINARY_DIR}/GUI
  ${CMAKE_CURRENT_SOURCE_DIR}/Logic
  ${CMAKE_CURRENT_BINARY_DIR}/Logic
  ${CMAKE_CURRENT_SOURCE_DIR}/MRML
  ${CMAKE_CURRENT_BINARY_DIR}/MRML
  )

set(MODULE_SRCS
  qSlicerOpenIGTLinkIFModule.cxx
  qSlicerOpenIGTLinkIFModule.h
  qSlicerOpenIGTLinkIFModuleWidget.cxx
  qSlicerOpenIGTLinkIFModuleWidget.h
  GUI/qMRMLSceneIGTLConnectorModel.cxx
  GUI/qMRMLSceneIGTLConnectorModel.h
  GUI/qMRMLIGTLConnectorTreeView.cxx
  GUI/qMRMLIGTLConnectorTreeView.h
  GUI/qMRMLIGTLIOModel.cxx
  GUI/qMRMLIGTLIOModel.h
  GUI/qMRMLIGTLIOModel_p.h
  GUI/qMRMLIGTLIOTreeView.cxx
  GUI/qMRMLIGTLIOTreeView.h
  GUI/qSlicerIGTLConnectorPropertyWidget.cxx
  GUI/qSlicerIGTLConnectorPropertyWidget.h
  GUI/qSlicerIGTLIONodeSelectorWidget.cxx
  GUI/qSlicerIGTLIONodeSelectorWidget.h
  )

set(MODULE_MOC_SRCS
  qSlicerOpenIGTLinkIFModule.h
  qSlicerOpenIGTLinkIFModuleWidget.h
  GUI/qMRMLSceneIGTLConnectorModel.h
  GUI/qMRMLIGTLConnectorTreeView.h
  GUI/qMRMLIGTLIOModel.h
#  GUI/qMRMLIGTLIOModel_p.h
  GUI/qMRMLIGTLIOTreeView.h
  GUI/qSlicerIGTLConnectorPropertyWidget.h
  GUI/qSlicerIGTLIONodeSelectorWidget.h
  )

set(MODULE_UI_SRCS
  Resources/UI/qSlicerIGTLConnectorPropertyWidget.ui
  Resources/UI/qSlicerIGTLIONodeSelectorWidget.ui
  Resources/UI/qSlicerOpenIGTLinkIFModule.ui
)

set(MODULE_TARGET_LIBRARIES
  vtkSlicerOpenIGTLinkIFModuleLogic
  vtkSlicerOpenIGTLinkIFModuleMRML
  )

set(MODULE_RESOURCES
  Resources/qSlicerOpenIGTLinkIFModule.qrc
  )

#-----------------------------------------------------------------------------
slicerMacroBuildQtModule(
  NAME ${MODULE_NAME}
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
