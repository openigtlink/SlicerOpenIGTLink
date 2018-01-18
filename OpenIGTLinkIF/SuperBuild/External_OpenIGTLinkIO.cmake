IF(OpenIGTLinkIO_DIR)
  # OpenIGTLinkIO has been built already
  FIND_PACKAGE(OpenIGTLinkIO REQUIRED PATHS ${OpenIGTLinkIO_DIR} NO_DEFAULT_PATH)
  MESSAGE(STATUS "Using OpenIGTLinkIO available at: ${OpenIGTLinkIO_DIR}")
  SET(Slicer_OpenIGTLinkIO_DIR "${OpenIGTLinkIO_DIR}")
ELSE()
  # OpenIGTLinkIO has not been built yet, so download and build it as an external project
  
  SET (OpenIGTLinkIO_SRC_DIR "${CMAKE_BINARY_DIR}/Deps/OpenIGTLinkIO")
  SET (Slicer_OpenIGTLinkIO_DIR "${CMAKE_BINARY_DIR}/Deps/OpenIGTLinkIO-bin" CACHE INTERNAL "Path to store OpenIGTLinkIO binaries")
  ExternalProject_Add( OpenIGTLinkIO
    PREFIX "${CMAKE_BINARY_DIR}/Deps/OpenIGTLinkIO-prefix"
    SOURCE_DIR "${OpenIGTLinkIO_SRC_DIR}"
    BINARY_DIR "${Slicer_OpenIGTLinkIO_DIR}"
    #--Download step--------------
    GIT_REPOSITORY https://github.com/IGSIO/OpenIGTLinkIO.git
    GIT_TAG master
    #--Configure step-------------
    CMAKE_ARGS 
      ${ep_common_args}
      -DCMAKE_RUNTIME_OUTPUT_DIRECTORY:PATH=${CMAKE_RUNTIME_OUTPUT_DIRECTORY}
      -DCMAKE_LIBRARY_OUTPUT_DIRECTORY:PATH=${CMAKE_LIBRARY_OUTPUT_DIRECTORY}
      -DCMAKE_ARCHIVE_OUTPUT_DIRECTORY:PATH=${CMAKE_ARCHIVE_OUTPUT_DIRECTORY}
      -DBUILD_SHARED_LIBS:BOOL=OFF
      -DBUILD_EXAMPLES:BOOL=OFF
      -DBUILD_TESTING:BOOL=OFF
      -DVTK_DIR:PATH=${VTK_DIR}
      -DOpenIGTLink_DIR:PATH=${Slicer_OpenIGTLink_DIR}
      -DIGTLIO_USE_GUI:BOOL=OFF
      -DCMAKE_CXX_FLAGS:STRING=${CMAKE_CXX_FLAGS}
      -DCMAKE_C_FLAGS:STRING=${CMAKE_C_FLAGS}
    #--Build step-----------------
    #--Install step-----------------
    INSTALL_COMMAND ""
    DEPENDS ${OpenIGTLinkIO_DEPENDENCIES}
    )  
ENDIF()