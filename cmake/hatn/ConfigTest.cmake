FUNCTION(TEST_HATN_MODULE_PLUGIN MODULE_NAME PLUGIN_NAME)
    MESSAGE(STATUS "Adding tests for plugin \"${PLUGIN_NAME}\" for module \"${MODULE_NAME}\"")
    ADD_DEPENDENCIES(${PROJECT_NAME} hatn${PLUGIN_NAME})
    SET (PLUGIN_SRC_DIR "${HATN_SOURCE_DIR}/${MODULE_NAME}/plugins/${PLUGIN_NAME}")
    SET (MODULE_DST_DIR "plugins/${MODULE_NAME}")    

    IF (BUILD_STATIC)
        SET(PLUGIN_SRC_DIRS ${PLUGIN_SRC_DIRS} ${PLUGIN_SRC_DIR}/include PARENT_SCOPE)
    ELSE(BUILD_STATIC)
        ADD_CUSTOM_TARGET(plugin-${MODULE_NAME}-hatn${PLUGIN_NAME} ALL DEPENDS hatn${PLUGIN_NAME})
        CONSTRUCT_LIBNAMES("hatn${PLUGIN_NAME}${LIB_POSTFIX}" "${MODULE_NAME}/plugins/${PLUGIN_NAME}" ${MODULE_DST_DIR})
        ADD_CUSTOM_COMMAND(TARGET plugin-${MODULE_NAME}-hatn${PLUGIN_NAME} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                    ${SRC_LIB}
                    ${DST_LIB}
                  )
    ENDIF(BUILD_STATIC)

    IF (EXISTS ${PLUGIN_SRC_DIR}/test/assets)
        MESSAGE(STATUS "Copying test assets for plugin \"${PLUGIN_NAME}\" for module \"${MODULE_NAME}\" from ${PLUGIN_SRC_DIR}/test/assets to ${MODULE_DST_DIR}/hatn${PLUGIN_NAME}/assets")
        COPY_PATH_BINDIR(${PLUGIN_SRC_DIR}/test/assets ${MODULE_DST_DIR}/hatn${PLUGIN_NAME}/assets)
    ENDIF()
ENDFUNCTION(TEST_HATN_MODULE_PLUGIN)

FUNCTION(TEST_HATN_MODULE_PLUGINS MODULE)
    STRING (TOLOWER ${MODULE} MODULE_NAME)
    SET (MODULE_PLUGINS_PATH "${HATN_SOURCE_DIR}/${MODULE_NAME}/plugins")
    FOREACH(plugin_name ${BUILD_PLUGINS})
        IF ("${plugin_name}" STREQUAL "all")
            IF (EXISTS ${MODULE_PLUGINS_PATH})
                SUBDIRLIST(PLUGIN_LIST ${MODULE_PLUGINS_PATH})
                FOREACH(plugin ${PLUGIN_LIST})
                    TEST_HATN_MODULE_PLUGIN(${MODULE_NAME} ${plugin})
                    SET(PLUGIN_SRC_DIRS ${PLUGIN_SRC_DIRS} PARENT_SCOPE)
                ENDFOREACH()
            ENDIF()
            BREAK()
        ELSE ()
            IF (EXISTS "${MODULE_PLUGINS_PATH}/${plugin_name}")
                TEST_HATN_MODULE_PLUGIN(${MODULE_NAME} ${plugin_name})
                SET(PLUGIN_SRC_DIRS ${PLUGIN_SRC_DIRS} PARENT_SCOPE)
            ENDIF()
        ENDIF()
    ENDFOREACH()
ENDFUNCTION(TEST_HATN_MODULE_PLUGINS)

FUNCTION (TEST_HATN_MODULE name)

    STRING (TOUPPER ${name} UPPER_MODULE_NAME)
    SET (${UPPER_MODULE_NAME}_TEST_SRC ${HATN_${UPPER_MODULE_NAME}_SRC}/test)

    IF(NOT EXISTS ${${UPPER_MODULE_NAME}_TEST_SRC})
        RETURN()
    ENDIF()

    IF(${DEV_MODULE})
        STRING (TOUPPER ${DEV_MODULE} UPPER_DEV_MODULE_NAME)
    ENDIF()
    IF("${DEV_MODULE}" STREQUAL "" OR "${DEV_MODULE}" STREQUAL "all" OR "${UPPER_DEV_MODULE_NAME}" STREQUAL ${UPPER_MODULE_NAME})

        MESSAGE(STATUS "Adding tests for module ${name}")

        TEST_HATN_MODULE_PLUGINS(${name})

        SET (MODULE_TEST_SRC ${${UPPER_MODULE_NAME}_TEST_SRC})
        INCLUDE(${MODULE_TEST_SRC}/test.cmake)

        STRING(SUBSTRING ${name} 0 1 FIRST_LETTER)
        STRING(TOUPPER ${FIRST_LETTER} FIRST_LETTER)
        STRING(REGEX REPLACE "^.(.*)" "${FIRST_LETTER}\\1" fname "${name}")

        FILE(WRITE "${CMAKE_CURRENT_BINARY_DIR}/tmp${name}.cmake" "Test${fname}()")
        INCLUDE(${CMAKE_CURRENT_BINARY_DIR}/tmp${name}.cmake)

        IF (EXISTS ${MODULE_TEST_SRC}/assets)
            MESSAGE(STATUS "Will copy assets for module ${name}")
            COPY_PATH_BINDIR(${MODULE_TEST_SRC}/assets ${name}/assets)
        ELSE()
            MESSAGE(STATUS "No need to copy assets for module ${name}")
        ENDIF()

    ENDIF ()

ENDFUNCTION(TEST_HATN_MODULE)

FUNCTION (TEST_HATN_MODULES)

    SET(Args ${ARGN})
    LIST(LENGTH Args NumArgs)
    IF(NumArgs GREATER 0)
        FOREACH(ARG ${Args})
            TEST_HATN_MODULE(${ARG})
        ENDFOREACH()
    ENDIF()

ENDFUNCTION (TEST_HATN_MODULES)

FUNCTION(CREATE_TEST_CONFIG_FILE)

    IF (NOT EXISTS ${TEST_BINARY_DIR}/hatn_test_config.h)

        IF (HATN_TEST_WRAP_C)
            SET(TEST_CONFIG_H_DEFS
    "${TEST_CONFIG_H_DEFS}
    #ifndef HATN_TEST_WRAP_C \n\
        #define HATN_TEST_WRAP_C \n\
    #endif
    ")
            TARGET_COMPILE_DEFINITIONS(${PROJECT_NAME} PRIVATE -DHATN_TEST_WRAP_C)
        ENDIF(HATN_TEST_WRAP_C)

        GET_PROPERTY(PLUGIN_LIST GLOBAL PROPERTY PLUGIN_LIST)
        MESSAGE(STATUS "Test PLUGIN_LIST=${PLUGIN_LIST}")

        IF (PLUGIN_LIST)
            LIST(REMOVE_DUPLICATES PLUGIN_LIST)
            LIST(JOIN PLUGIN_LIST "," plugins)
            SET(PLUGINS_LINE "static const std::set<std::string> HATN_TEST_PLUGINS={${plugins}};\n")
        ELSE()
            SET(PLUGINS_LINE "static const std::set<std::string> HATN_TEST_PLUGINS;\n")
        ENDIF ()
        SET(TEST_CONFIG_H_TEXT "${TEST_CONFIG_H_TEXT}\n${PLUGINS_LINE}")

        FILE(WRITE ${TEST_BINARY_DIR}/hatn_test_config.h
"/***************************************************/\n\
/*****This file is auto-generated. Do not edit.*****/\n\
/***************************************************/\n\
\n\
#ifndef HATN_TEST_CONFIG_H \n\
#define HATN_TEST_CONFIG_H \n\
${TEST_CONFIG_H_DEFS}
\n\
#include <string> \n\
#include <set> \n\
\n\
namespace hatn { namespace test {\n\
${TEST_CONFIG_H_TEXT}
}}\n\
#endif
"
            )


    ENDIF()

ENDFUNCTION(CREATE_TEST_CONFIG_FILE)

MACRO(ADD_HATN_CTEST Name Label RunTest)

    IF (BUILD_ANDROID)

        ADD_TEST(NAME ${Name}
                         WORKING_DIRECTORY ${WORKING_DIR}
                         COMMAND $ENV{ANDROID_PLATFORM_TOOLS}/adb shell "cd /data/local/tmp/test && ./${TEST_EXEC_CMD} --logger=HRF,test_suite --report_level=no --result_code=no --logger=XML,all,${RESULT_XML_DIR}/${TARGET_EXE}.xml ${RunTest}"
                     )

    ELSEIF (BUILD_IOS)

        ADD_TEST(NAME ${Name}
                         WORKING_DIRECTORY ${WORKING_DIR}
                         COMMAND xcrun simctl spawn ${HATN_TEST_IOS_DEVICE} ${TEST_EXEC_CMD} --logger=HRF,test_suite --report_level=no --result_code=no --logger=XML,all,${RESULT_XML_DIR}/${TARGET_EXE}.xml ${RunTest}
                     )

    ELSE()

        ADD_TEST(NAME ${Name}
                         WORKING_DIRECTORY ${WORKING_DIR}
                         COMMAND ${TEST_EXEC_CMD} --logger=HRF,test_suite --report_level=no --result_code=no --logger=XML,all,${RESULT_XML_DIR}/${TARGET_EXE}.xml ${MEMORY_LEAKS} ${RunTest}
                    )

    ENDIF()

    SET_TESTS_PROPERTIES(${Name} PROPERTIES LABELS ${Label})

ENDMACRO()

FUNCTION(ADD_HATN_CTESTS MODULE_NAME)

	GET_PROPERTY(TEST_SUITES GLOBAL PROPERTY HATN_TEST_SUITES)
	SET(TEST_CASES "")
	SET(WORKING_DIR ${BINDIR})
	
	FOREACH(SOURCE_FILE_NAME ${TEST_SOURCES})

                MESSAGE(STATUS "Reading test source file ${SOURCE_FILE_NAME}")

                FILE(READ "${SOURCE_FILE_NAME}" SOURCE_FILE_CONTENTS)

                STRING(REGEX MATCH "BOOST_AUTO_TEST_SUITE\\( *([A-Za-z_0-9]+) *[\\),]" FOUND_TEST_SUITE ${SOURCE_FILE_CONTENTS})

                IF (NOT FOUND_TEST_SUITE)

                    MESSAGE(STATUS "Not found test suite in ${SOURCE_FILE_NAME}")
                    STRING(REGEX MATCH "BOOST_FIXTURE_TEST_SUITE\\( *([A-Za-z_0-9]+) *[\\),]" FOUND_TEST_SUITE ${SOURCE_FILE_CONTENTS})

                ENDIF()

		IF (FOUND_TEST_SUITE)

                        STRING(REGEX REPLACE ".*\\( *([A-Za-z_0-9]+) *[\\),].*" "\\1" SUITE_NAME ${FOUND_TEST_SUITE})
                        MESSAGE(STATUS "Found test suite ${SUITE_NAME}")
			
                        STRING (TOLOWER ${MODULE_NAME}${SUITE_NAME} TARGET_EXE)
                        IF (BUILD_IOS)
                            SET (TEST_EXEC_CMD ${BINDIR}/${TARGET_EXE}.app/${TARGET_EXE})
                        ELSE ()
                            SET (TEST_EXEC_CMD ${TARGET_EXE})
                        ENDIF()
						    
			IF (${BUILD_TYPE} STREQUAL "DEBUG")
				SET (MEMORY_LEAKS --detect_memory_leaks=0)
			ENDIF()
			
			LIST (FIND TEST_SUITES ${SUITE_NAME} FOUND_IDX)
			IF (${FOUND_IDX} EQUAL -1)
				
				MESSAGE(STATUS "Adding new test suite ${SUITE_NAME}")

				LIST (APPEND TEST_SUITES ${SUITE_NAME})

                                MESSAGE(STATUS "Adding ${SOURCE_FILE_NAME} to test ${TARGET_EXE}")
                                ADD_EXECUTABLE(${TARGET_EXE} ${SOURCE_FILE_NAME} ${SOURCES} ${HATN_TEST_THREAD_SOURCES})
                                TARGET_INCLUDE_DIRECTORIES(${TARGET_EXE} PRIVATE ${TEST_BINARY_DIR})
                                IF(NOT "${MODULE_TEST_LIB}" STREQUAL "")
                                    TARGET_LINK_LIBRARIES(${TARGET_EXE} PRIVATE ${MODULE_TEST_LIB})
                                ENDIF()
                                TARGET_COMPILE_DEFINITIONS(${TARGET_EXE} PRIVATE -DHATN_MODULE=${MODULE_NAME} ${CTESTS_COMPILE_DEFINITIONS})

                                # MESSAGE(STATUS "Adding for target ${TARGET_EXE} the following HATN_MODULES=${HATN_MODULES}")
                                ADD_HATN_MODULES(${TARGET_EXE} PRIVATE ${HATN_MODULES})
				ADD_DEPENDENCIES(${PROJECT_NAME} ${TARGET_EXE})
				
                                LINK_HATN_PLUGINS(${TARGET_EXE} ${MODULE_NAME})
				
                                ADD_HATN_CTEST(${SUITE_NAME}-all "ALL" "")
                                ADD_HATN_CTEST(${SUITE_NAME} "SUITE" --run_test=${SUITE_NAME})

                                IF (NOT MSVC)
                                    TARGET_COMPILE_OPTIONS(${TARGET_EXE} PRIVATE -Wall -Wno-sign-compare)
                                ELSE()
                                    TARGET_COMPILE_OPTIONS(${TARGET_EXE} PRIVATE "/bigobj")
                                ENDIF()

			ELSE()	
				MESSAGE(STATUS "Using existing test suite ${SUITE_NAME}")
				TARGET_SOURCES(${TARGET_EXE} PRIVATE ${SOURCE_FILE_NAME})
			ENDIF()

			SET(FOUND_TESTS "")
			STRING(REGEX MATCHALL "BOOST_FIXTURE_TEST_CASE\\( *([A-Za-z_0-9]+) *\\," FIXTURE_TESTS "${SOURCE_FILE_CONTENTS}")
			FOREACH(HIT ${FIXTURE_TESTS})
				STRING(REGEX REPLACE ".*\\( *([A-Za-z_0-9]+) *\\,.*" "\\1" TEST_CASE ${HIT})
				LIST (APPEND FOUND_TESTS ${TEST_CASE})
			ENDFOREACH()
			STRING(REGEX MATCHALL "BOOST_AUTO_TEST_CASE\\( *([A-Za-z_0-9]+) *\\)" AUTO_TESTS "${SOURCE_FILE_CONTENTS}")
			FOREACH(HIT ${AUTO_TESTS})
				STRING(REGEX REPLACE ".*\\( *([A-Za-z_0-9]+) *\\).*" "\\1" TEST_CASE ${HIT})
				LIST (APPEND FOUND_TESTS ${TEST_CASE})
			ENDFOREACH()
						
			FOREACH(TEST_CASE ${FOUND_TESTS})
				LIST (FIND TEST_CASES ${TEST_CASE} FOUND_IDX)
				IF (${FOUND_IDX} EQUAL -1)
					MESSAGE(STATUS "Adding test ${SUITE_NAME}/${TEST_CASE}")
					LIST (APPEND TEST_CASES ${TEST_CASE})
                                        ADD_HATN_CTEST(${SUITE_NAME}/${TEST_CASE} "CASE" --run_test=${SUITE_NAME}/${TEST_CASE})
				ENDIF()				
			ENDFOREACH()
		ENDIF()

	ENDFOREACH()

        FOREACH(SOURCE_FILE_NAME ${TEST_SOURCES})

            FILE(READ "${SOURCE_FILE_NAME}" SOURCE_FILE_CONTENTS)

            SET(FOUND_SUITES "")
            STRING(REGEX MATCHALL "// HATN_TEST_SUITE [A-Za-z_0-9]+" HATN_SUITES "${SOURCE_FILE_CONTENTS}")
            FOREACH(HIT ${HATN_SUITES})

                    STRING(REGEX REPLACE "// HATN_TEST_SUITE ([A-Za-z_0-9]+)" "\\1" SUITE_NAME ${HIT})
                    IF (${SUITE_NAME} IN_LIST TEST_SUITES)

                        STRING (TOLOWER ${MODULE_NAME}${SUITE_NAME} TARGET_EXE)
                        IF (BUILD_IOS)
                            SET (TEST_EXEC_CMD ${BINDIR}/${TARGET_EXE}.app/${TARGET_EXE})
                        ELSE ()
                            SET (TEST_EXEC_CMD ${TARGET_EXE})
                        ENDIF()

                        TARGET_SOURCES(${TARGET_EXE} PRIVATE ${SOURCE_FILE_NAME})

                        MESSAGE(STATUS "Adding ${SOURCE_FILE_NAME} to test ${TARGET_EXE}")
                    ENDIF()

            ENDFOREACH()

        ENDFOREACH()

	SET_PROPERTY(GLOBAL PROPERTY HATN_TEST_SUITES "${TEST_SUITES}")

ENDFUNCTION(ADD_HATN_CTESTS)
