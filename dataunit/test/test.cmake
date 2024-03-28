SET (TEST_SOURCES
    # ${DATAUNIT_TEST_SRC}/testsyntax.cpp
    # ${DATAUNIT_TEST_SRC}/testjson.cpp
    # ${DATAUNIT_TEST_SRC}/testfields.cpp
    # ${DATAUNIT_TEST_SRC}/testgetset.cpp
    # ${DATAUNIT_TEST_SRC}/testfieldpath.cpp
    # ${DATAUNIT_TEST_SRC}/testprevalidate.cpp
    ${DATAUNIT_TEST_SRC}/testperformance.cpp
    ${DATAUNIT_TEST_SRC}/testserialization.cpp
)

SET (TEST_HEADERS
    ${HEADERS}
    ${DATAUNIT_TEST_SRC}/testunitdeclarations.h
    ${DATAUNIT_TEST_SRC}/testfieldpath.h
)
# @todo Fix it
# IF (MINGW AND BUILD_DEBUG)
#     # Fix string table overflow when compiling in debug mode
#     SET_SOURCE_FILES_PROPERTIES(${TEST_SOURCES} PROPERTIES COMPILE_FLAGS -Os)
#     SET_SOURCE_FILES_PROPERTIES(${SOURCES} PROPERTIES COMPILE_FLAGS -Os)
#     SET_SOURCE_FILES_PROPERTIES(${DATAUNIT_TEST_SRC}/testunitinstantiations.cpp PROPERTIES COMPILE_FLAGS -Os)
# ENDIF ()

SET(MODULE_TEST_LIB dataunittestlib)
# SET(TEST_LIB_SOURCES ${DATAUNIT_TEST_SRC}/testunitinstantiations.cpp)
ADD_LIBRARY(${MODULE_TEST_LIB} STATIC  ${HATN_TEST_THREAD_SOURCES} ${TEST_LIB_SOURCES})
TARGET_INCLUDE_DIRECTORIES(${MODULE_TEST_LIB} PRIVATE ${TEST_BINARY_DIR})
ADD_HATN_MODULES(${MODULE_TEST_LIB} PRIVATE dataunit)

SET(HATN_TEST_THREAD_SOURCES "")

ADD_HATN_CTESTS(dataunit ${TEST_SOURCES})

FUNCTION(TestDataunit)
    COPY_LIBRARY_HERE(hatncommon${LIB_POSTFIX} ../common/)
    COPY_LIBRARY_HERE(hatndataunit${LIB_POSTFIX} ../dataunit/)
ENDFUNCTION(TestDataunit)

ADD_CUSTOM_TARGET(dataunit-src SOURCES ${TEST_HEADERS} ${TEST_SOURCES} ${SOURCES})
