IF (NOT STATIC_BUILD)
    IF (NOT WIN32)
        SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fPIC")
    ENDIF (NOT WIN32)
ENDIF (NOT STATIC_BUILD)

IF (THIRDPARTY_ENABLE_FMT)
  IF (NOT SYSTEM_FMTLIB)
      IF (INSTALL_DEV)
          MESSAGE(STATUS "Install fmt")
          SET(FMT_INSTALL ON CACHE BOOL "Install fmt")
      ELSE()
          SET(FMT_INSTALL OFF CACHE BOOL "Do not install fmt")
      ENDIF()

      IF (NOT WIN32)
        SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fPIC")
      ENDIF()

      # 1. Save the current state of BUILD_SHARED_LIBS
      set(OLD_BUILD_SHARED_LIBS ${BUILD_SHARED_LIBS})

      # 2. Force {fmt} to build as a static library
      set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)

      # 3. Add the subproject
      ADD_SUBDIRECTORY(fmt)

      # 4. Restore the previous state for other targets
      set(BUILD_SHARED_LIBS ${OLD_BUILD_SHARED_LIBS} CACHE BOOL "" FORCE)

  ENDIF(NOT SYSTEM_FMTLIB)
ENDIF()

SET(ARGS_BUILD_EXAMPLE OFF CACHE BOOL "Do not build args examples")
SET(ARGS_BUILD_UNITTESTS OFF CACHE BOOL "Do not build args tests")

ADD_SUBDIRECTORY(args)
ADD_SUBDIRECTORY(function2)
ADD_SUBDIRECTORY(cppcodec)
