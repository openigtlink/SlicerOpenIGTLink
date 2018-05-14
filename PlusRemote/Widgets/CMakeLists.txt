project(qSlicer${MODULE_NAME}ModuleWidgets)

set(KIT ${PROJECT_NAME})

set(${KIT}_EXPORT_DIRECTIVE "Q_SLICER_MODULE_${MODULE_NAME_UPPER}_WIDGETS_EXPORT")

set(${KIT}_INCLUDE_DIRECTORIES
  ${vtkSlicerOpenIGTLinkIFModuleMRML_INCLUDE_DIRS}
  ${vtkSlicerOpenIGTLinkIFModuleCommand_INCLUDE_DIRS}
  ${vtkSlicer${MODULE_NAME}ModuleMRML_INCLUDE_DIRS}
  )

set(${KIT}_SRCS
  qMRMLPlusLauncherRemoteWidget.cxx
  qMRMLPlusLauncherRemoteWidget.h
  )

set(${KIT}_MOC_SRCS
  qMRMLPlusLauncherRemoteWidget.h
  )

set(${KIT}_UI_SRCS
  Resources/UI/qMRMLPlusLauncherRemoteWidget.ui
  )

set(${KIT}_RESOURCES
  Resources/qMRMLPlusLauncherRemoteWidget.qrc
  )

set(${KIT}_TARGET_LIBRARIES
  vtkSlicerOpenIGTLinkIFModuleMRML
  vtkSlicer${MODULE_NAME}ModuleMRML
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

#-----------------------------------------------------------------------------
#add_subdirectory(DesignerPlugins)