project(vtkSlicer${MODULE_NAME}ModuleLogic)

set(KIT ${PROJECT_NAME})

set(${KIT}_EXPORT_DIRECTIVE "VTK_SLICER_${MODULE_NAME_UPPER}_MODULE_LOGIC_EXPORT")

set(${KIT}_INCLUDE_DIRECTORIES ${vtkSlicerOpenIGTLinkIFModuleLogic_INCLUDE_DIRS})

set(${KIT}_SRCS
  vtkSlicerOpenIGTLinkRemoteLogic.cxx
  vtkSlicerOpenIGTLinkRemoteLogic.h
  )

LIST(APPEND ${KIT}_TARGET_LIBRARIES
  ${ITK_LIBRARIES}
  vtkSlicer${MODULE_NAME}ModuleMRML
  vtkSlicerOpenIGTLinkIFModuleLogic
  )

if (Slicer_VERSION VERSION_LESS "4.11.0")
  #TODO: When the stable version of Slicer includes the Texts module/widgets, they can be removed from SlicerOpenIGTLink
  list(APPEND ${KIT}_INCLUDE_DIRECTORIES
    ${vtkSlicerOpenIGTLinkIFModuleMRML_SOURCE_DIR}/Text
	${vtkSlicerOpenIGTLinkIFModuleMRML_BINARY_DIR}/Text
    )
endif()


#-----------------------------------------------------------------------------
SlicerMacroBuildModuleLogic(
  NAME ${KIT}
  EXPORT_DIRECTIVE ${${KIT}_EXPORT_DIRECTIVE}
  INCLUDE_DIRECTORIES ${${KIT}_INCLUDE_DIRECTORIES}
  SRCS ${${KIT}_SRCS}
  TARGET_LIBRARIES ${${KIT}_TARGET_LIBRARIES}
  ${DISABLE_WRAP_PYTHON}
  )
