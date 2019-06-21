project(qSlicer${MODULE_NAME}ModuleWidgets)

set(KIT ${PROJECT_NAME})

set(${KIT}_EXPORT_DIRECTIVE "Q_SLICER_MODULE_${MODULE_NAME_UPPER}_WIDGETS_EXPORT")

set(${KIT}_INCLUDE_DIRECTORIES
  ${vtkSlicerOpenIGTLinkIFModuleMRML_INCLUDE_DIRS}
  ${vtkSlicer${MODULE_NAME}ModuleMRML_INCLUDE_DIRS}
  )

set(${KIT}_SRCS
  qMRMLPlusServerLauncherRemoteWidget.cxx
  qMRMLPlusServerLauncherRemoteWidget.h
  qMRMLPlusServerLauncherTableView.cxx
  qMRMLPlusServerLauncherTableView.h
  )

set(${KIT}_MOC_SRCS
  qMRMLPlusServerLauncherRemoteWidget.h
  qMRMLPlusServerLauncherTableView.h
  )

set(${KIT}_UI_SRCS
  Resources/UI/qMRMLPlusServerLauncherRemoteWidget.ui
  Resources/UI/qMRMLPlusServerLauncherTableView.ui
  )

set(${KIT}_RESOURCES
  Resources/qMRMLPlusServerLauncherRemoteWidget.qrc
  )

set(${KIT}_TARGET_LIBRARIES
  vtkSlicerOpenIGTLinkIFModuleMRML
  vtkSlicer${MODULE_NAME}ModuleMRML
  vtkSlicer${MODULE_NAME}ModuleLogic
  )

if (Slicer_VERSION VERSION_LESS "4.11.0")
  #TODO: When the stable version of Slicer includes the Texts module/widgets, they can be removed from SlicerOpenIGTLink
  list(APPEND ${KIT}_INCLUDE_DIRECTORIES
    ${vtkSlicerOpenIGTLinkIFModuleMRML_SOURCE_DIR}/Text
    )
endif()

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
add_subdirectory(DesignerPlugins)