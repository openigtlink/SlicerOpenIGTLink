IF(OpenIGTLink_DIR)
  # OpenIGTLink has been built already
  FIND_PACKAGE(OpenIGTLink REQUIRED NO_MODULE)
  if("${OpenIGTLink_VERSION_MAJOR}.${OpenIGTLink_VERSION_MINOR}.${OpenIGTLink_VERSION_PATCH}" LESS 3.1.0)
    message(FATAL_ERROR "Version of OpenIGTLink should be at less 3.1.0.")
  endif()
  MESSAGE(STATUS "Using OpenIGTLink available at: ${OpenIGTLink_DIR}")
  SET (Slicer_OpenIGTLink_DIR "${OpenIGTLink_DIR}")
ELSE()
  # OpenIGTLink has not been built yet, so download and build it as an external project
  SET (OpenIGTLink_SRC_DIR "${CMAKE_BINARY_DIR}/Deps/OpenIGTLink")
  SET (Slicer_OpenIGTLink_DIR "${CMAKE_BINARY_DIR}/Deps/OpenIGTLink-bin" CACHE INTERNAL "Path to store OpenIGTLink binaries")
  ExternalProject_Add( OpenIGTLink
    PREFIX "${CMAKE_BINARY_DIR}/Deps/OpenIGTLink-prefix"
    SOURCE_DIR "${OpenIGTLink_SRC_DIR}"
    BINARY_DIR "${Slicer_OpenIGTLink_DIR}"
    #--Download step--------------
    GIT_REPOSITORY https://github.com/openigtlink/OpenIGTLink.git
    GIT_TAG master
    #--Configure step-------------
    CMAKE_ARGS 
      -DCMAKE_CXX_COMPILER:FILEPATH=${CMAKE_CXX_COMPILER}
      -DCMAKE_CXX_FLAGS:STRING=${ep_common_cxx_flags}
      -DCMAKE_C_COMPILER:FILEPATH=${CMAKE_C_COMPILER}
      -DCMAKE_C_FLAGS:STRING=${ep_common_c_flags}
      -DCMAKE_CXX_STANDARD:STRING=${CMAKE_CXX_STANDARD}
      -DCMAKE_CXX_STANDARD_REQUIRED:BOOL=${CMAKE_CXX_STANDARD_REQUIRED}
      -DCMAKE_CXX_EXTENSIONS:BOOL=${CMAKE_CXX_EXTENSIONS}
      ${CMAKE_PROJECT_INCLUDE_EXTERNAL_PROJECT_ARG}
      -DBUILD_SHARED_LIBS:BOOL=ON
      -DBUILD_EXAMPLES:BOOL=OFF
      -DBUILD_TESTING:BOOL=OFF
      -DOpenIGTLink_SUPERBUILD:BOOL=OFF
      -DOpenIGTLink_PROTOCOL_VERSION_2:BOOL=OFF
      -DOpenIGTLink_PROTOCOL_VERSION_3:BOOL=ON
      -DOpenIGTLink_USE_H264:BOOL=ON
      -DCMAKE_CXX_FLAGS:STRING=${CMAKE_CXX_FLAGS}
      -DCMAKE_C_FLAGS:STRING=${CMAKE_C_FLAGS}
    #--Build step-----------------
    BUILD_ALWAYS 1
    #--Install step-----------------
    INSTALL_COMMAND ""
    DEPENDS ${OpenIGTLink_DEPENDENCIES}
    )  
ENDIF()