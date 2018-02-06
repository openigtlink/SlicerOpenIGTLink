if(Slicer_USE_VP9)
  set(VP9_INSTALL_LIB_DIR ${Slicer_INSTALL_ROOT}lib/VP9)

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
    message("vp9: ${VP9_DIR} ${VP9_INSTALL_LIB_DIR}")
    install(DIRECTORY
      ${VP9_DIR}/
      DESTINATION ${VP9_INSTALL_LIB_DIR}
      COMPONENT ALL
      USE_SOURCE_PERMISSIONS
      REGEX "/vpx" 
      REGEX "/include" EXCLUDE
      REGEX "/demos" EXCLUDE
      PATTERN "*.a" 
      PATTERN "*.sh" EXCLUDE
      PATTERN "*.c" EXCLUDE
      PATTERN ".svn" EXCLUDE
      ${extra_exclude_pattern}
      )
  endif()
endif()