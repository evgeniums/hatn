INSTALL(DIRECTORY "${HATN_SOURCE_DIR}/cmake" DESTINATION lib)
INSTALL(DIRECTORY "${HATN_SOURCE_DIR}/scripts" DESTINATION lib/hatn
    PATTERN "__pycache__" EXCLUDE
    PATTERN "*.pyc" EXCLUDE
)
INSTALL(FILES ${HATN_SOURCE_DIR}/VERSION
    DESTINATION lib/cmake/hatn
)

MACRO(HATN_INSTALL_LIB LibName)

    IF (BUILD_STATIC)
        INSTALL(TARGETS ${LibName}
                ARCHIVE DESTINATION lib
        )
    ELSE (BUILD_STATIC)
        IF (INSTALL_DEV)
            INSTALL(TARGETS ${LibName}
                RUNTIME DESTINATION bin
                LIBRARY DESTINATION lib
                ARCHIVE DESTINATION lib
            )
        ELSE(INSTALL_DEV)
            INSTALL(TARGETS ${LibName}
                RUNTIME DESTINATION bin
                LIBRARY DESTINATION lib
            )
        ENDIF(INSTALL_DEV)
    ENDIF(BUILD_STATIC)

    IF (INSTALL_DEV)

        SET(Args ${ARGN})
        LIST(LENGTH Args NumArgs)
        IF(NumArgs GREATER 0)
            FOREACH(ARG ${Args})
                MESSAGE(STATUS "Install include directory ${ARG}")
                INSTALL(DIRECTORY ${ARG} DESTINATION include)
            ENDFOREACH()
        ENDIF()

    ENDIF(INSTALL_DEV)

ENDMACRO(HATN_INSTALL_LIB)
