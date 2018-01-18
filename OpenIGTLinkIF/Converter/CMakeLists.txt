project(OpenIGTLinkIFConverter)

set(KIT "${PROJECT_NAME}")

set(${KIT}_EXPORT_DIRECTIVE "Q_SLICER_QTMODULES_OPENIGTLINKIF_CONVERTER_EXPORT")

if(OpenIGTLink_PROTOCOL_VERSION EQUAL 2)
  ADD_DEFINITIONS(-DOpenIGTLinkIF_USE_VERSION_2)
  ADD_DEFINITIONS(-DOpenIGTLinkIF_LIBRARY_VERSION="${OpenIGTLink_VERSION_MAJOR}-${OpenIGTLink_VERSION_MINOR}-${OpenIGTLink_VERSION_PATCH} for protocol ver. 2")
else()
  ADD_DEFINITIONS(-DOpenIGTLinkIF_LIBRARY_VERSION="${OpenIGTLink_VERSION_MAJOR}-${OpenIGTLink_VERSION_MINOR}-${OpenIGTLink_VERSION_PATCH} for protocol ver. 1")
endif()


set(${KIT}_INCLUDE_DIRECTORIES
  ${OpenIGTLinkIFMRML_SOURCE_DIR}
  ${OpenIGTLinkIFMRML_BINARY_DIR}
  ${qt_module_include_directories}
  ${OpenIGTLink_INCLUDE_DIRS}
  )

set(${KIT}_SRCS
  vtkIGTLToMRMLImage.cxx
  vtkIGTLToMRMLImage.h
  vtkIGTLToMRMLImageMetaList.cxx
  vtkIGTLToMRMLImageMetaList.h
  vtkIGTLToMRMLLinearTransform.cxx
  vtkIGTLToMRMLLinearTransform.h
  vtkIGTLToMRMLPosition.cxx
  vtkIGTLToMRMLPosition.h
  vtkIGTLToMRMLTrackingData.cxx
  vtkIGTLToMRMLTrackingData.h
  )

set(${KIT}_TARGET_LIBRARIES
  ${ITK_LIBRARIES}
  ${OpenIGTLink_LIBRARIES}
  OpenIGTLinkIFMRML
  )

#-----------------------------------------------------------------------------
SlicerMacroBuildModuleLogic(
  NAME ${KIT}
  EXPORT_DIRECTIVE ${${KIT}_EXPORT_DIRECTIVE}
  INCLUDE_DIRECTORIES ${${KIT}_INCLUDE_DIRECTORIES}
  SRCS ${${KIT}_SRCS}
  TARGET_LIBRARIES ${${KIT}_TARGET_LIBRARIES}
  )
