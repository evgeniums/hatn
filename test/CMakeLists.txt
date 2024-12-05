PROJECT(hatnlibstest)

SET (TEST_BINARY_DIR ${CMAKE_CURRENT_BINARY_DIR})

ENABLE_TESTING()

INCLUDE(hatn/ConfigExe)
INCLUDE(hatn/ConfigTest)

SET(HEADERS
    ${HATN_COMMON_SRC}/include/hatn/test/pluginlist.h
)

SET(SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/main.cpp
)

IF (common IN_LIST HATN_MODULES)
    SET(HATN_TEST_THREAD_SOURCES
        ${HATN_COMMON_SRC}/include/hatn/test/multithreadfixture.cpp
    )
ENDIF()

IF (prograph IN_LIST HATN_MODULES)
    SET(HATN_TEST_THREADQ_SOURCES
        ${HATN_PROGRAPH_SRC}/include/hatn/test/threadqfixture.cpp
    )
ENDIF()

IF (HATN_TEST_WRAP_C)

    MESSAGE(STATUS "Building c-wrapped test library")
    MESSAGE(FATAL_ERROR "c-wrapped test library not supported currently")

    SET(HEADERS
        ${HEADERS}
        ${CMAKE_CURRENT_SOURCE_DIR}/testwrapper.h
    )

    SET(SOURCES
        ${SOURCES}
        ${CMAKE_CURRENT_SOURCE_DIR}/testwrapper.cpp
    )

    ADD_LIBRARY(${PROJECT_NAME} ${LINK_TYPE} ${SOURCES} ${HEADERS})

ELSE(HATN_TEST_WRAP_C)
    MESSAGE(STATUS "Building executable test application")
    ADD_CUSTOM_TARGET(${PROJECT_NAME} ALL SOURCES ${HEADERS} ${SOURCES})
ENDIF(HATN_TEST_WRAP_C)

IF (BUILD_ANDROID)
    SET(RESULT_XML_DIR /data/local/tmp/test/result-xml)
    MESSAGE(STATUS "RESULT_XML_DIR on Android ${RESULT_XML_DIR}")
ELSE()
    SET(RESULT_XML_DIR ${BINDIR}/result-xml)
    IF (NOT "$ENV{working_dir}" STREQUAL "")
        SET(RESULT_XML_DIR $ENV{working_dir}/result-xml)
    ENDIF()
    MESSAGE(STATUS "RESULT_XML_DIR ${RESULT_XML_DIR}")
ENDIF()

IF (BUILD_IOS)
    IF ("$ENV{HATN_TEST_IOS_DEVICE}" STREQUAL "")
        SET (HATN_TEST_IOS_DEVICE booted)
    ELSE()
        SET (HATN_TEST_IOS_DEVICE $ENV{HATN_TEST_IOS_DEVICE})
    ENDIF()
ENDIF()

TEST_HATN_MODULES(${HATN_MODULES})

CREATE_TEST_CONFIG_FILE()
