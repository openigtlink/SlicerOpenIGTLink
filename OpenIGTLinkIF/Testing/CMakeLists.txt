PROJECT(Test)

#-----------------------------------------------------------------------------
set(KIT_TEST_SRCS
  vtkMRMLConnectorCommandSendAndReceiveTest.cxx
  )
  
set(${KIT}_TARGET_LIBRARIES
  vtkSlicerOpenIGTLinkIFModuleMRML
  SlicerBaseLogic
  )

#-----------------------------------------------------------------------------
add_executable(vtkMRMLConnectorCommandSendAndReceiveTest ${KIT_TEST_SRCS})
target_link_libraries(vtkMRMLConnectorCommandSendAndReceiveTest ${${KIT}_TARGET_LIBRARIES})