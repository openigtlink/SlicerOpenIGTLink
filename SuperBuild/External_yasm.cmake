set(proj YASM)

# Set dependency list
if(${CMAKE_PROJECT_NAME}_USE_SYSTEM_${proj})
  unset(${proj}_DIR CACHE)
  include(${CMAKE_SOURCE_DIR}/SuperBuild/FindYASM.cmake)
  if(${proj}_FOUND)
    set(${proj}_DIR ${${proj}_BINARY_DIR})
  endif()
endif()

# Sanity checks
if(DEFINED ${proj}_DIR AND NOT EXISTS ${${proj}_DIR})
  message(FATAL_ERROR "${proj}_DIR variable is defined but corresponds to nonexistent directory")
endif()

if(NOT DEFINED OpenIGTLink_DIR AND NOT ${CMAKE_PROJECT_NAME}_USE_SYSTEM_${proj})
  set(CMAKE_PROJECT_INCLUDE_EXTERNAL_PROJECT_ARG)
  if(CTEST_USE_LAUNCHERS)
    set(CMAKE_PROJECT_INCLUDE_EXTERNAL_PROJECT_ARG
      "-DCMAKE_PROJECT_OpenIGTLink_INCLUDE:FILEPATH=${CMAKE_ROOT}/Modules/CTestUseLaunchers.cmake")
  endif()

  SET(YASM_PYTHON_EXECUTABLE "" CACHE STRING "Python Interpreter")
  if("${YASM_PYTHON_EXECUTABLE}" STREQUAL "")
    find_package(PythonInterp "2.7" REQUIRED)
    SET(YASM_PYTHON_EXECUTABLE ${PYTHON_EXECUTABLE})
  endif()

  ExternalProject_SetIfNotDefined(
    ${CMAKE_PROJECT_NAME}_${proj}_GIT_REPOSITORY
    "https://github.com/yasm/yasm.git"
    QUIET
    )

  ExternalProject_SetIfNotDefined(
    ${CMAKE_PROJECT_NAME}_${proj}_GIT_TAG
    master
    QUIET
    )
  set(${proj}_SOURCE_DIR ${CMAKE_BINARY_DIR}/Deps/${proj})
  set(${proj}_BINARY_DIR ${CMAKE_BINARY_DIR}/Deps/${proj}-bin)
  ExternalProject_Add(${proj}
    ${${proj}_EP_ARGS}
    GIT_REPOSITORY "${${CMAKE_PROJECT_NAME}_${proj}_GIT_REPOSITORY}"
    GIT_TAG "${${CMAKE_PROJECT_NAME}_${proj}_GIT_TAG}"
    SOURCE_DIR ${${proj}_SOURCE_DIR}
    BINARY_DIR ${${proj}_BINARY_DIR}
    CMAKE_CACHE_ARGS
      ${CMAKE_PROJECT_INCLUDE_EXTERNAL_PROJECT_ARG}
      ${PLATFORM_SPECIFIC_ARGS}
      -DCMAKE_CXX_COMPILER:FILEPATH=${CMAKE_CXX_COMPILER}
      -DCMAKE_CXX_FLAGS:STRING=${ep_common_cxx_flags}
      -DCMAKE_C_COMPILER:FILEPATH=${CMAKE_C_COMPILER}
      -DCMAKE_C_FLAGS:STRING=${ep_common_c_flags}
      -DCMAKE_LIBRARY_OUTPUT_DIRECTORY:STRING=${YASM_BINARY_DIR}
      -DCMAKE_RUNTIME_OUTPUT_DIRECTORY:STRING=${YASM_BINARY_DIR}
      -DYASM_INSTALL_BIN_DIR:STRING="bin"
      -DPYTHON_EXECUTABLE:STRING=${YASM_PYTHON_EXECUTABLE}
      -DCMAKE_ARCHIVE_OUTPUT_DIRECTORY:PATH=${YASM_BINARY_DIR}
      -DBUILD_TESTING:BOOL=OFF 
      -DBUILD_EXAMPLES:BOOL=OFF
      -DBUILD_SHARED_LIBS:BOOL=OFF
    #--Build step-----------------
    BUILD_ALWAYS 1
    INSTALL_COMMAND ""  
    )

  ExternalProject_GenerateProjectDescription_Step(${proj})

  set(${proj}_DIR ${${proj}_BINARY_DIR})
  
else()
  ExternalProject_Add_Empty(${proj} DEPENDS ${${proj}_DEPENDENCIES})
endif()

ExternalProject_Message(${proj} "${proj}_DIR:${${proj}_DIR}")
mark_as_superbuild(${proj}_DIR:PATH)
