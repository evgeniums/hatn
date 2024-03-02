OPTION(SYSTEM_FMTLIB "Use system fmtlib library instead of bundled in thirdparty" OFF)
IF (SYSTEM_FMTLIB)
    IF (NOT FMT_HEADER_ONLY)
        ADD_LIBRARY(fmt INTERFACE IMPORTED)
        IF (FMT_LIB_DIR)
            LINK_DIRECTORIES(${LINK_DIRECTORIES} ${FMT_LIB_DIR})
        ENDIF(FMT_LIB_DIR)
        SET_PROPERTY(TARGET fmt PROPERTY INTERFACE_LINK_LIBRARIES fmt)
    ENDIF(NOT FMT_HEADER_ONLY)
    IF (FMT_INCLUDE_DIR)
        INCLUDE_DIRECTORIES(${INCLUDE_DIRECTORIES} ${FMT_INCLUDE_DIR})
    ENDIF(FMT_INCLUDE_DIR)

ELSE()

IF (INSTALL_DEV)

INSTALL(DIRECTORY "${HATN_SOURCE_DIR}/thirdparty/fmt/include/fmt" DESTINATION include)
IF (MSVC)
    INSTALL(FILES ${HATN_BINARY_DIR}/thirdparty/fmt/$<CONFIGURATION>/fmtd.lib
            DESTINATION lib
            CONFIGURATIONS Debug
           )
    INSTALL(FILES ${HATN_BINARY_DIR}/thirdparty/fmt/$<CONFIGURATION>/fmt.lib
           DESTINATION lib
           CONFIGURATIONS Release
          )
ELSE (MSVC)
    INSTALL(FILES ${HATN_BINARY_DIR}/thirdparty/fmt/libfmtd.a
            DESTINATION lib
            CONFIGURATIONS Debug
            )
    INSTALL(FILES ${HATN_BINARY_DIR}/thirdparty/fmt/libfmt.a
            DESTINATION lib
            CONFIGURATIONS Release
            )
ENDIF (MSVC)

ENDIF()
    
ENDIF (SYSTEM_FMTLIB)
