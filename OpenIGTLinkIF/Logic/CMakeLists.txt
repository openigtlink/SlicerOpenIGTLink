project(vtkSlicer${MODULE_NAME}ModuleLogic)

set(KIT "${PROJECT_NAME}")

set(${KIT}_EXPORT_DIRECTIVE "VTK_SLICER_OPENIGTLINKIF_MODULE_LOGIC_EXPORT")

set(${KIT}_INCLUDE_DIRECTORIES
  ${vtkSlicerOpenIGTLinkIFModuleMRML_SOURCE_DIR}
  ${vtkSlicerOpenIGTLinkIFModuleMRML_BINARY_DIR}
  ${OpenIGTLinkIO_INCLUDE_DIRS}
  )

set(${KIT}_SRCS
  vtkSlicerOpenIGTLinkIFLogic.cxx
  vtkSlicerOpenIGTLinkIFLogic.h
  )

set(${KIT}_TARGET_LIBRARIES
  vtkSlicerOpenIGTLinkIFModuleMRML
  OpenIGTLink
  ${OpenIGTLinkIO_LIBRARIES}
  ${VTK_LIBRARIES}
  ${ITK_LIBRARIES}
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


foreach(p IN LISTS ${KIT}_INCLUDE_DIRECTORIES)
  target_include_directories(${KIT} PUBLIC $<BUILD_INTERFACE:${p}>)
endforeach()
foreach(p IN LISTS ${KIT}_INCLUDE_DIRS)
  target_include_directories(${KIT} PUBLIC $<BUILD_INTERFACE:${p}>)
endforeach()
target_include_directories(${KIT} PUBLIC $<INSTALL_INTERFACE:${${KIT}_INSTALL_INCLUDE_DIR}>)
