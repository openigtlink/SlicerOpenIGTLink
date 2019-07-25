set(KIT ${PROJECT_NAME})

#-----------------------------------------------------------------------------
set(${KIT}_TEST_SRCS
  vtkMRMLConnectorCommandSendAndReceiveTest.cxx
  vtkMRMLConnectorImageSendAndReceiveTest.cxx
  )
if(SlicerOpenIGTLink_USE_VP9)
  LIST(APPEND ${KIT}_TEST_SRCS
    vtkMRMLConnectorVideoSendAndReceiveTest.cxx
  )
endif()
#-----------------------------------------------------------------------------
#set(CMAKE_TESTDRIVER_BEFORE_TESTMAIN "DEBUG_LEAKS_ENABLE_EXIT_ERROR();" )
create_test_sourcelist(Tests ${KIT}CxxTests.cxx
  ${${KIT}_TEST_SRCS}
  # Add source of your tests after this line.
  #EXTRA_INCLUDE vtkMRMLDebugLeaksMacro.h
  )
list(APPEND Tests ${${KIT}_TEST_SRCS})

set(${KIT}_TARGET_LIBRARIES
  vtkSlicerOpenIGTLinkIFModuleMRML
  SlicerBaseLogic
  )

# Add '--launcher-additional-settings' to launch command,
# but only if additional launcher settings file is available
# (the file is not created if the extension is bundled with the main application).
if(NOT "${Slicer_ADDITIONAL_LAUNCHER_SETTINGS}" STREQUAL "")
  list(FIND Slicer_LAUNCH_COMMAND "--launch" launch_index)
  list(INSERT Slicer_LAUNCH_COMMAND ${launch_index} ${Slicer_ADDITIONAL_LAUNCHER_SETTINGS})
endif()

#-----------------------------------------------------------------------------
add_executable(${KIT}CxxTests ${Tests})
set_target_properties(${KIT}CxxTests PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/${Slicer_BIN_DIR})
target_link_libraries(${KIT}CxxTests ${${KIT}_TARGET_LIBRARIES})

#-----------------------------------------------------------------------------
simple_test(vtkMRMLConnectorCommandSendAndReceiveTest)
simple_test(vtkMRMLConnectorImageSendAndReceiveTest)
if(SlicerOpenIGTLink_USE_VP9)
  simple_test(vtkMRMLConnectorVideoSendAndReceiveTest)
endif()

