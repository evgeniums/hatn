IF(ENABLE_TRANSLATIONS)

	FIND_PROGRAM(MSGFMT_EXE msgfmt PATHS "${GETTEXT_PATH}")
	IF (NOT MSGFMT_EXE)
		MESSAGE(FATAL_ERROR "\n***************\nCould not find msgfmt.\nPlease, install gettext and/or set valid GETTEXT_PATH.\n***************\n")
	ENDIF()
	IF(NOT GETTEXT_PATH)
	    GET_FILENAME_COMPONENT(GETTEXT_PATH "${MSGFMT_EXE}" PATH)
	ENDIF(NOT GETTEXT_PATH)
	SET(GETTEXT_PATH "${GETTEXT_PATH}" CACHE PATH "Path to Gettext-tools")
	SET(ENV{GETTEXT_PATH} "${GETTEXT_PATH}")

	MESSAGE(STATUS "Set GETTEXT_PATH to $ENV{GETTEXT_PATH}")

ENDIF(ENABLE_TRANSLATIONS)

FUNCTION(INSTALL_TRANSLATIONS)
    IF (EXISTS ${CMAKE_CURRENT_BINARY_DIR}/translations)
        INSTALL(DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/translations/"
                DESTINATION share/locale
                OPTIONAL
                )
    ENDIF()
ENDFUNCTION(INSTALL_TRANSLATIONS)

FUNCTION(COMPILE_TRANSLATIONS)

    IF (NOT "${HATNCOMMON_PYTHON_SCRIPTS_DIR}" STREQUAL "")
        MESSAGE(STATUS "Path to translation python scripts ${HATNCOMMON_PYTHON_SCRIPTS_DIR}")
        SET (TRANSLATION_SCRIPTS "${HATNCOMMON_PYTHON_SCRIPTS_DIR}/translation" CACHE PATH "Path to translation python scripts")

        IF (NOT MSVC)
            ADD_CUSTOM_TARGET(update-po
                COMMAND ${CMAKE_COMMAND} -E env GETTEXT_PATH=${GETTEXT_PATH} ${PYTHON_EXE} ${TRANSLATION_SCRIPTS}/update-po-files.py --top_project_folder ${HATN_SOURCE_DIR} --project_folder ${CMAKE_CURRENT_SOURCE_DIR} --basename ${PROJECT_NAME} --pot_folder ${CMAKE_CURRENT_BINARY_DIR}
                USES_TERMINAL
            )
        ENDIF()

        IF (EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/translations)
            ADD_CUSTOM_TARGET(generate-mo-${PROJECT_NAME} ALL
                COMMAND ${CMAKE_COMMAND} -E env GETTEXT_PATH=${GETTEXT_PATH} ${PYTHON_EXE} ${TRANSLATION_SCRIPTS}/compile-mo-files.py --top_project_folder ${HATN_SOURCE_DIR} --project_folder ${CMAKE_CURRENT_SOURCE_DIR} --basename ${PROJECT_NAME} --mo_folder "${CMAKE_CURRENT_BINARY_DIR}/translations/"
            )
            ADD_DEPENDENCIES(${PROJECT_NAME} generate-mo-${PROJECT_NAME})
        ENDIF()
    ENDIF()

ENDFUNCTION(COMPILE_TRANSLATIONS)
