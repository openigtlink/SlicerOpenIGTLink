project(vtkSlicer${MODULE_NAME}ModuleMRML)

set(KIT ${PROJECT_NAME})

set(${KIT}_EXPORT_DIRECTIVE "VTK_SLICER_${MODULE_NAME_UPPER}_MODULE_MRML_EXPORT")

set(${KIT}_INCLUDE_DIRECTORIES
  ${vtkSlicerOpenIGTLinkIFModuleMRML_INCLUDE_DIRS}
  ${Slicer_Base_INCLUDE_DIRS}
  )

set(${KIT}_SRCS
  vtkMRMLPlusRemoteNode.cxx
  vtkMRMLPlusRemoteNode.h
  vtkMRMLPlusServerNode.cxx
  vtkMRMLPlusServerNode.h
  vtkMRMLPlusServerLauncherNode.cxx
  vtkMRMLPlusServerLauncherNode.h
  vtkSlicerPlusOpenIGTLinkCommand.cxx
  vtkSlicerPlusOpenIGTLinkCommand.h
  )

set(${KIT}_TARGET_LIBRARIES
  vtkSlicerOpenIGTLinkIFModuleMRML
  )

# --------------------------------------------------------------------------
SlicerMacroBuildModuleMRML(
  NAME ${KIT}
  EXPORT_DIRECTIVE ${${KIT}_EXPORT_DIRECTIVE}
  INCLUDE_DIRECTORIES ${${KIT}_INCLUDE_DIRECTORIES}
  SRCS ${${KIT}_SRCS}
  TARGET_LIBRARIES ${${KIT}_TARGET_LIBRARIES}
  )