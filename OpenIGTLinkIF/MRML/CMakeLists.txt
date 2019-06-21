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

  if (Slicer_VERSION VERSION_LESS "4.11.0")
    #TODO: When the stable version of Slicer includes the Texts module/widgets, they can be removed from SlicerOpenIGTLink
    list(APPEND ${KIT}_SRCS
      Text/vtkMRMLTextNode.cxx
      Text/vtkMRMLTextStorageNode.cxx
      )
    list(APPEND ${KIT}_INCLUDE_DIRECTORIES
      ${CMAKE_CURRENT_SOURCE_DIR}/Text
      ${CMAKE_CURRENT_BINARY_DIR}/Text
      )
    configure_file(Text/vtkMRMLTextNode.h ${CMAKE_CURRENT_BINARY_DIR}/Text/vtkMRMLTextNode.h COPYONLY)
	configure_file(Text/vtkMRMLTextStorageNode.h ${CMAKE_CURRENT_BINARY_DIR}/Text/vtkMRMLTextStorageNode.h COPYONLY)
  endif()
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
