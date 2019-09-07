project(vtkSlicer${MODULE_NAME}ModuleMRML)

set(KIT "${PROJECT_NAME}")

set(${KIT}_EXPORT_DIRECTIVE "VTK_SLICER_OPENIGTLINKIF_MODULE_MRML_EXPORT")
set(${KIT}_INCLUDE_DIRECTORIES
  ${Slicer_Base_INCLUDE_DIRS}
  ${CMAKE_CURRENT_SOURCE_DIR}
  ${CMAKE_CURRENT_BINARY_DIR}
  ${CMAKE_CURRENT_SOURCE_DIR}/../Logic
  ${CMAKE_CURRENT_BINARY_DIR}/../Logic
  ${OpenIGTLinkIO_INCLUDE_DIRS}
  )

set(${KIT}_SRCS
  vtkMRMLIGTLConnectorNode.cxx
  vtkMRMLIGTLStatusNode.cxx
  )

if(OpenIGTLink_PROTOCOL_VERSION GREATER 1)
  list(APPEND ${KIT}_SRCS
    vtkMRMLIGTLQueryNode.cxx
    vtkMRMLIGTLSensorNode.cxx
    vtkMRMLIGTLTrackingDataBundleNode.cxx
    vtkMRMLIGTLTrackingDataQueryNode.cxx
    vtkMRMLImageMetaListNode.cxx
    vtkMRMLLabelMetaListNode.cxx
    vtkSlicerOpenIGTLinkCommand.cxx
    )
endif()

if(SlicerOpenIGTLink_USE_VP9)
  list(APPEND ${KIT}_SRCS
    vtkIGTLVP9VolumeCodec.cxx
  )
endif()

set(${KIT}_TARGET_LIBRARIES
  OpenIGTLink
  ${OpenIGTLinkIO_LIBRARIES}
  ${MRML_LIBRARIES}
  SlicerBaseLogic
  vtkSlicerAnnotationsModuleMRML
  vtkSlicerMarkupsModuleMRML
  qSlicerBaseQTGUI
  )

#-----------------------------------------------------------------------------
SlicerMacroBuildModuleMRML(
  NAME ${KIT}
  EXPORT_DIRECTIVE ${${KIT}_EXPORT_DIRECTIVE}
  INCLUDE_DIRECTORIES ${${KIT}_INCLUDE_DIRECTORIES}
  SRCS ${${KIT}_SRCS}
  TARGET_LIBRARIES ${${KIT}_TARGET_LIBRARIES}
  )
