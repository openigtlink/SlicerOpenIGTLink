set(KIT ${PROJECT_NAME}Plugins)

set(${KIT}_EXPORT "Q_SLICER_MODULE_${MODULE_NAME_UPPER}_WIDGETS_PLUGINS_EXPORT")

set(${KIT}_SRCS
  qSlicerPlusRemoteModuleWidgetsAbstractPlugin.cxx
  qSlicerPlusRemoteModuleWidgetsAbstractPlugin.h
  qSlicerPlusRemoteModuleWidgetsPlugin.cxx
  qSlicerPlusRemoteModuleWidgetsPlugin.h
  qMRMLPlusServerLauncherRemoteWidgetPlugin.cxx
  qMRMLPlusServerLauncherRemoteWidgetPlugin.h
  qMRMLPlusServerLauncherTableViewPlugin.cxx
  qMRMLPlusServerLauncherTableViewPlugin.h
  )

set(${KIT}_MOC_SRCS
  qMRMLPlusServerLauncherRemoteWidgetPlugin.h
  qSlicerPlusRemoteModuleWidgetsPlugin.h
  qMRMLPlusServerLauncherTableViewPlugin.h
  )

set(${KIT}_TARGET_LIBRARIES
  qSlicerPlusRemoteModuleWidgets
  )

#-----------------------------------------------------------------------------
# Designer plugins have to be installed at the same location as other
# Slicer core designer plugins
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${Slicer_DIR}/bin)

ctkMacroBuildQtDesignerPlugin(
  NAME ${KIT}
  EXPORT_DIRECTIVE ${${KIT}_EXPORT}
  FOLDER "Module-${MODULE_NAME}"
  SRCS ${${KIT}_SRCS}
  MOC_SRCS ${${KIT}_MOC_SRCS}
  TARGET_LIBRARIES ${${KIT}_TARGET_LIBRARIES}
  )
