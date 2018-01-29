project(qSlicer${MODULE_NAME}ModuleWidgets)

set(KIT ${PROJECT_NAME})

set(${KIT}_EXPORT_DIRECTIVE "Q_SLICER_MODULE_${MODULE_NAME_UPPER}_WIDGETS_EXPORT")

set(${KIT}_INCLUDE_DIRECTORIES
  ${CMAKE_BUILD_DIR}
  ${vtkSlicerOpenIGTLinkIFModuleMRML_SOURCE_DIR}
  ${vtkSlicerOpenIGTLinkIFModuleMRML_BINARY_DIR}
  )

set(${KIT}_SRCS
  qSlicerOpenIGTLinkIFModuleWidget.cxx
  qSlicerOpenIGTLinkIFModuleWidget.h
  qMRMLSceneIGTLConnectorModel.cxx
  qMRMLSceneIGTLConnectorModel.h
  qMRMLIGTLConnectorTreeView.cxx
  qMRMLIGTLConnectorTreeView.h
  qMRMLIGTLIOModel.cxx
  qMRMLIGTLIOModel.h
  qMRMLIGTLIOModel_p.h
  qMRMLIGTLIOTreeView.cxx
  qMRMLIGTLIOTreeView.h
  qSlicerIGTLConnectorPropertyWidget.cxx
  qSlicerIGTLConnectorPropertyWidget.h
  qSlicerIGTLIONodeSelectorWidget.cxx
  qSlicerIGTLIONodeSelectorWidget.h
  )

set(${KIT}_MOC_SRCS
  qSlicerOpenIGTLinkIFModuleWidget.h
  qMRMLSceneIGTLConnectorModel.h
  qMRMLIGTLConnectorTreeView.h
  qMRMLIGTLIOModel.h
#  qMRMLIGTLIOModel_p.h
  qMRMLIGTLIOTreeView.h
  qSlicerIGTLConnectorPropertyWidget.h
  qSlicerIGTLIONodeSelectorWidget.h
  )

set(${KIT}_UI_SRCS
  Resources/UI/qSlicerIGTLConnectorPropertyWidget.ui
  Resources/UI/qSlicerIGTLIONodeSelectorWidget.ui
  Resources/UI/qSlicerOpenIGTLinkIFModule.ui
  )

set(${KIT}_RESOURCES
  Resources/qSlicerOpenIGTLinkIFModule.qrc
  )

set(${KIT}_TARGET_LIBRARIES
  ${MRML_LIBRARIES}
  vtkSlicerOpenIGTLinkIFModuleLogic
  vtkSlicerOpenIGTLinkIFModuleMRML
  )

#-----------------------------------------------------------------------------
SlicerMacroBuildModuleWidgets(
  NAME ${KIT}
  EXPORT_DIRECTIVE ${${KIT}_EXPORT_DIRECTIVE}
  INCLUDE_DIRECTORIES ${${KIT}_INCLUDE_DIRECTORIES}
  SRCS ${${KIT}_SRCS}
  MOC_SRCS ${${KIT}_MOC_SRCS}
  UI_SRCS ${${KIT}_UI_SRCS}
  TARGET_LIBRARIES ${${KIT}_TARGET_LIBRARIES}
  RESOURCES ${${KIT}_RESOURCES}
  WRAP_PYTHONQT
  )

# Plugin
#add_subdirectory(Plugins)
