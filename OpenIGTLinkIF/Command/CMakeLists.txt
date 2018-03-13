project(vtkSlicer${MODULE_NAME}ModuleCommand)

set(KIT "${PROJECT_NAME}")

set(${KIT}_EXPORT_DIRECTIVE "VTK_SLICER_OPENIGTLINKIF_MODULE_COMMAND_EXPORT")
set(${KIT}_INCLUDE_DIRECTORIES
  ${Slicer_Base_INCLUDE_DIRS}
  )

set(${KIT}_SRCS
  vtkSlicerOpenIGTLinkIFCommand.cxx
  vtkSlicerOpenIGTLinkIFCommand.h
  )

set(${KIT}_TARGET_LIBRARIES
  ${VTK_LIBRARIES}
  )

#-----------------------------------------------------------------------------
SlicerMacroBuildModuleLogic(
  NAME ${KIT}
  EXPORT_DIRECTIVE ${${KIT}_EXPORT_DIRECTIVE}
  INCLUDE_DIRECTORIES ${${KIT}_INCLUDE_DIRECTORIES}
  SRCS ${${KIT}_SRCS}
  TARGET_LIBRARIES ${${KIT}_TARGET_LIBRARIES}
  )
