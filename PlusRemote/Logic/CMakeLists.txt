project(vtkSlicer${MODULE_NAME}ModuleLogic)

set(KIT ${PROJECT_NAME})

set(${KIT}_EXPORT_DIRECTIVE "VTK_SLICER_${MODULE_NAME_UPPER}_MODULE_LOGIC_EXPORT")

set(${KIT}_INCLUDE_DIRECTORIES
  ${vtkSlicer${MODULE_NAME}ModuleMRML_INCLUDE_DIRS}
  ${vtkSlicerVolumeRenderingModuleLogic_INCLUDE_DIRS}
  ${vtkSlicerVolumeRenderingModuleMRML_INCLUDE_DIRS}
  ${vtkSlicerVolumeRenderingModuleMRMLDisplayableManager_SOURCE_DIR}
  ${vtkSlicerVolumeRenderingModuleMRMLDisplayableManager_BINARY_DIR}
  )

set(${KIT}_SRCS
  vtkSlicer${MODULE_NAME}Logic.cxx
  vtkSlicer${MODULE_NAME}Logic.h
  )

set(${KIT}_TARGET_LIBRARIES
  vtkSlicer${MODULE_NAME}ModuleMRML
  vtkSlicerVolumeRenderingModuleLogic
  vtkSlicerVolumeRenderingModuleMRML
  vtkSlicerVolumeRenderingModuleMRMLDisplayableManager
  qSlicerVolumeRenderingModuleWidgets
  )

if (Slicer_VERSION VERSION_LESS "4.11.0")
  #TODO: When the stable version of Slicer includes the Texts module/widgets, they can be removed from SlicerOpenIGTLink
  list(APPEND ${KIT}_INCLUDE_DIRECTORIES
    ${vtkSlicerOpenIGTLinkIFModuleMRML_SOURCE_DIR}/Text
    )
endif()


#-----------------------------------------------------------------------------
SlicerMacroBuildModuleLogic(
  NAME ${KIT}
  EXPORT_DIRECTIVE ${${KIT}_EXPORT_DIRECTIVE}
  INCLUDE_DIRECTORIES ${${KIT}_INCLUDE_DIRECTORIES}
  SRCS ${${KIT}_SRCS}
  TARGET_LIBRARIES ${${KIT}_TARGET_LIBRARIES}
  )
