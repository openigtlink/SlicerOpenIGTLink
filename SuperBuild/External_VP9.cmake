set(proj VP9)

# Set dependency list
set(${proj}_DEPENDS "")
if(UNIX)
  list(APPEND ${proj}_DEPENDS YASM)
endif()

# Include dependent projects if any
ExternalProject_Include_Dependencies(${proj} PROJECT_VAR proj DEPENDS_VAR ${proj}_DEPENDS)

if(${CMAKE_PROJECT_NAME}_USE_SYSTEM_${proj})
  unset(VP9_INCLUDE_DIR CACHE)
  unset(VP9_LIBRARY_DIR CACHE)
  unset(VP9_ROOT CACHE)
  include(${CMAKE_SOURCE_DIR}/SuperBuild/FindVP9.cmake)
endif()

# Sanity checks
if(DEFINED ${proj}_DIR AND NOT EXISTS ${${proj}_DIR})
  message(FATAL_ERROR "${proj}_DIR variable is defined but corresponds to nonexistent directory")
endif()

if(NOT DEFINED ${proj}_DIR AND NOT ${CMAKE_PROJECT_NAME}_USE_SYSTEM_${proj})

  if(UNIX)

    set(EP_SOURCE_DIR "${CMAKE_BINARY_DIR}/${proj}")

    include(ExternalProjectForNonCMakeProject)

    # environment
    set(_env_script ${CMAKE_BINARY_DIR}/${proj}_Env.cmake)
    ExternalProject_Write_SetBuildEnv_Commands(${_env_script})

    # configure step
    set(_configure_script ${CMAKE_BINARY_DIR}/${proj}_configure_step.cmake)
    file(WRITE ${_configure_script}
    "include(\"${_env_script}\")
set(${proj}_WORKING_DIR \"${EP_SOURCE_DIR}\")
ExternalProject_Execute(${proj} \"configure\" sh ${EP_SOURCE_DIR}/configure
  --disable-examples --as=yasm --enable-pic --disable-tools --disable-docs --disable-vp8 --disable-libyuv --disable-unit_tests --disable-postproc
  )
")

    # build step
    set(_build_script ${CMAKE_BINARY_DIR}/${proj}_build_step.cmake)
    file(WRITE ${_build_script}
    "include(\"${_env_script}\")
set(${proj}_WORKING_DIR \"${EP_SOURCE_DIR}\")
set(ENV{PATH} \"${YASM_DIR}:${YASM_DIR}/Debug:${YASM_DIR}/Release:$ENV{PATH}\")
ExternalProject_Execute(${proj} \"build\" make)
")

    ExternalProject_SetIfNotDefined(
      ${CMAKE_PROJECT_NAME}_${proj}_GIT_REPOSITORY
      "${EP_GIT_PROTOCOL}://github.com/webmproject/libvpx.git"
      QUIET
      )

    ExternalProject_SetIfNotDefined(
      ${CMAKE_PROJECT_NAME}_${proj}_GIT_TAG
      "v1.8.2"
      QUIET
      )

    ExternalProject_Add(${proj}
      GIT_REPOSITORY ${${CMAKE_PROJECT_NAME}_${proj}_GIT_REPOSITORY}
      GIT_TAG ${${CMAKE_PROJECT_NAME}_${proj}_GIT_TAG}
      SOURCE_DIR "${EP_SOURCE_DIR}"
      BUILD_IN_SOURCE 1
      CONFIGURE_COMMAND ${CMAKE_COMMAND} -P ${_configure_script}
      BUILD_COMMAND ${CMAKE_COMMAND} -P ${_build_script}
      INSTALL_COMMAND "" #${CMAKE_COMMAND} -P ${_install_script}
      DEPENDS
        ${${proj}_DEPENDS}
      )

    set(VP9_LIBRARY_DIR "${EP_SOURCE_DIR}")

  elseif(WIN32)

    set(BASE_VP9_URL "https://github.com/ShiftMediaProject/libvpx/releases/download/v1.8.2/libvpx_v1.8.2_")
    set(EP_SOURCE_DIR "${CMAKE_BINARY_DIR}/VP9")

    set(VP9_MSVC120_URL "${BASE_VP9_URL}msvc12.zip")
    set(VP9_MSVC140_URL "${BASE_VP9_URL}msvc14.zip")
    set(VP9_MSVC141_URL "${BASE_VP9_URL}msvc15.zip")
    set(VP9_MSVC142_URL "${BASE_VP9_URL}msvc16.zip")
    set(VP9_MSVC143_URL "${BASE_VP9_URL}msvc16.zip")

    if(NOT DEFINED VP9_MSVC${MSVC_TOOLSET_VERSION}_URL)
      message(FATAL_ERROR "There are no binaries available for Microsoft C++ compiler ${MSVC_VERSION}")
    endif()

    set(_download_url "${VP9_MSVC${MSVC_TOOLSET_VERSION}_URL}")

    set(_library_subdir "${VP9_binary_${MSVC_VERSION}_library_subdir}")

    set(VP9_LIBRARY_DIR "${EP_SOURCE_DIR}/${_library_subdir}")

    ExternalProject_Add(VP9
      URL ${_download_url}
      DOWNLOAD_DIR ${CMAKE_BINARY_DIR}
      SOURCE_DIR ${EP_SOURCE_DIR}
      CONFIGURE_COMMAND ""
      BUILD_COMMAND ""
      INSTALL_COMMAND ""
      DEPENDS
        ${${proj}_DEPENDENCIES}
      )

  endif()

  #-----------------------------------------------------------------------------
  # Launcher setting specific to build tree

  # library paths
  set(${proj}_LIBRARY_PATHS_LAUNCHER_BUILD)
  if(UNIX)
    list(APPEND ${proj}_LIBRARY_PATHS_LAUNCHER_BUILD
      ${VP9_LIBRARY_DIR}
      )
  endif()
  mark_as_superbuild(
    VARS ${proj}_LIBRARY_PATHS_LAUNCHER_BUILD
    LABELS "LIBRARY_PATHS_LAUNCHER_BUILD"
    )

  # paths
  set(${proj}_PATHS_LAUNCHER_BUILD
    ${VP9_LIBRARY_DIR}
    )
  mark_as_superbuild(
    VARS ${proj}_PATHS_LAUNCHER_BUILD
    LABELS "PATHS_LAUNCHER_BUILD"
    )

  # environment variables
  set(${proj}_ENVVARS_LAUNCHER_BUILD
    )
  mark_as_superbuild(
    VARS ${proj}_ENVVARS_LAUNCHER_BUILD
    LABELS "ENVVARS_LAUNCHER_BUILD"
    )

  #-----------------------------------------------------------------------------
  # Launcher setting specific to install tree

  set(vp9lib_subdir lib)
  if(WIN32)
    set(vp9lib_subdir bin)
  endif()

  # library paths
  set(${proj}_LIBRARY_PATHS_LAUNCHER_INSTALLED <APPLAUNCHER_DIR>/lib/VP9/${vp9lib_subdir})
  mark_as_superbuild(
    VARS ${proj}_LIBRARY_PATHS_LAUNCHER_INSTALLED
    LABELS "LIBRARY_PATHS_LAUNCHER_INSTALLED"
    )

  # paths
  set(${proj}_PATHS_LAUNCHER_INSTALLED
    )
  mark_as_superbuild(
    VARS ${proj}_PATHS_LAUNCHER_INSTALLED
    LABELS "PATHS_LAUNCHER_INSTALLED"
    )

  # environment variables
  set(${proj}_ENVVARS_LAUNCHER_INSTALLED
    )
  mark_as_superbuild(
    VARS ${proj}_ENVVARS_LAUNCHER_INSTALLED
    LABELS "ENVVARS_LAUNCHER_INSTALLED"
    )
else()
  ExternalProject_Add_Empty(${proj} DEPENDS ${${proj}_DEPENDS})
endif()

set(${proj}_DIR ${VP9_LIBRARY_DIR})
mark_as_superbuild(${proj}_DIR:PATH)

