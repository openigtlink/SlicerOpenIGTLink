if(Slicer_USE_VP9)
  set(VP9_INSTALL_LIB_DIR ${Slicer_INSTALL_ROOT}${Slicer_BUNDLE_EXTENSIONS_LOCATION}lib/VP9)
  SET(VP9_LIBRARY)
  if(VP9_DIR)
    set(extra_exclude_pattern)
    if(UNIX)
      list(APPEND extra_exclude_pattern
        REGEX "/bin" EXCLUDE
        )
    endif()
    if(APPLE)
      list(APPEND extra_exclude_pattern
        REGEX "libvpx_g.a" EXCLUDE
        )
    endif()
    message("vp9: ${VP9_DIR} ${VP9_INSTALL_LIB_DIR} ${VP9_LIBRARY}")
    install(FILES
      ${CMAKE_BINARY_DIR}/../license-VP9.txt
      DESTINATION ${VP9_INSTALL_LIB_DIR}
      )   
    if(WIN32)
      install(DIRECTORY
        ${VP9_DIR}/../vpx/
        DESTINATION ${VP9_INSTALL_LIB_DIR}/include
        USE_SOURCE_PERMISSIONS
        PATTERN "*.h"
      )
      install(DIRECTORY
        ${VP9_DIR}
        DESTINATION ${VP9_INSTALL_LIB_DIR}/include
        USE_SOURCE_PERMISSIONS
        PATTERN "*.h"
      )
      if("${CMAKE_GENERATOR}" MATCHES "(Win64|IA64)")
        install(FILES
          ${VP9_DIR}/x64/Release/vpxmd.lib
          DESTINATION ${VP9_INSTALL_LIB_DIR}/Release/
        )
        install(FILES
          ${VP9_DIR}/x64/Debug/vpxmdd.lib
          DESTINATION ${VP9_INSTALL_LIB_DIR}/Debug/
        )
      else()
        install(FILES
          ${VP9_DIR}/Win32/Release/vpxmd.lib
          DESTINATION ${VP9_INSTALL_LIB_DIR}/Release/
        )
        install(FILES
          ${VP9_DIR}/Win32/Debug/vpxmdd.lib
          DESTINATION ${VP9_INSTALL_LIB_DIR}/Debug/
        )
      endif()
    else()
      install(DIRECTORY
        ${VP9_DIR}/vpx/
        DESTINATION ${VP9_INSTALL_LIB_DIR}/include
        USE_SOURCE_PERMISSIONS
        PATTERN "*.h"
      ) 
      install(FILES
      ${VP9_DIR}/libvpx.a
      DESTINATION ${VP9_INSTALL_LIB_DIR}
      )  
    endif()       
  endif()
endif()