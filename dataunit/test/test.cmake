SET (TEST_SOURCES
    ${DATAUNIT_TEST_SRC}/testsyntax.cpp
    ${DATAUNIT_TEST_SRC}/testjson.cpp
    ${DATAUNIT_TEST_SRC}/testfields.cpp
    ${DATAUNIT_TEST_SRC}/testgetset.cpp
    ${DATAUNIT_TEST_SRC}/testfieldpath.cpp
    ${DATAUNIT_TEST_SRC}/testprevalidate.cpp
)

SET (TEST_HEADERS
    ${HEADERS}
    ${DATAUNIT_TEST_SRC}/testunitdeclarations.h
    ${DATAUNIT_TEST_SRC}/testfieldpath.h
)

IF (MINGW)
    # Fix string table overflow when compiling in debug mode
    SET_SOURCE_FILES_PROPERTIES(${DATAUNIT_TEST_SOURCES} PROPERTIES COMPILE_FLAGS -Os)
ENDIF (MINGW)

SET (SOURCES
    ${SOURCES}
    ${DATAUNIT_TEST_SRC}/testunitinstantiations.cpp
)

ADD_HATN_CTESTS(dataunit ${TEST_SOURCES})

FUNCTION(TestDataunit)
    COPY_LIBRARY_HERE(hatncommon${LIB_POSTFIX} ../common/)
    COPY_LIBRARY_HERE(hatndataunit${LIB_POSTFIX} ../dataunit/)
ENDFUNCTION(TestDataunit)

ADD_CUSTOM_TARGET(dataunit-src SOURCES ${TEST_HEADERS} ${TEST_SOURCES} ${SOURCES})
