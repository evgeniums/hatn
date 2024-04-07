SET (TEST_SOURCES
    # ${DATAUNIT_TEST_SRC}/testsyntax.cpp
    # ${DATAUNIT_TEST_SRC}/testjson.cpp
    # ${DATAUNIT_TEST_SRC}/testfields.cpp
    # ${DATAUNIT_TEST_SRC}/testgetset.cpp
    # ${DATAUNIT_TEST_SRC}/testfieldpath.cpp
    # ${DATAUNIT_TEST_SRC}/testprevalidate.cpp
    # ${DATAUNIT_TEST_SRC}/testperformance.cpp
    # ${DATAUNIT_TEST_SRC}/testserialization.cpp
    # ${DATAUNIT_TEST_SRC}/testwirebuf.cpp
    # ${DATAUNIT_TEST_SRC}/testmeta.cpp
    # ${DATAUNIT_TEST_SRC}/testdefault.cpp
    # ${DATAUNIT_TEST_SRC}/testinheritance.cpp
    ${DATAUNIT_TEST_SRC}/testerrors.cpp
)

SET (TEST_HEADERS
    ${HEADERS}
    ${DATAUNIT_TEST_SRC}/testunitlib.h
    ${DATAUNIT_TEST_SRC}/simpleunitdeclaration.h
    ${DATAUNIT_TEST_SRC}/testfieldpath.h
    ${DATAUNIT_TEST_SRC}/testunitdeclarations.h
    ${DATAUNIT_TEST_SRC}/testunitdeclarations1.h
    ${DATAUNIT_TEST_SRC}/testunitdeclarations2.h
    ${DATAUNIT_TEST_SRC}/testunitdeclarations3.h
    ${DATAUNIT_TEST_SRC}/testunitdeclarations4.h
    ${DATAUNIT_TEST_SRC}/testunitdeclarations5.h
    ${DATAUNIT_TEST_SRC}/testunitdeclarations6.h
    ${DATAUNIT_TEST_SRC}/testunitdeclarations7.h
    ${DATAUNIT_TEST_SRC}/testunitdeclarations8.h
)

SET(MODULE_TEST_LIB dataunittestlib)

# SET(TEST_LIB_SOURCES ${TEST_LIB_SOURCES} ${DATAUNIT_TEST_SRC}/testunitinstantiations1.cpp)
# SET(TEST_LIB_SOURCES ${TEST_LIB_SOURCES} ${DATAUNIT_TEST_SRC}/testunitinstantiations2.cpp)
# SET(TEST_LIB_SOURCES ${TEST_LIB_SOURCES} ${DATAUNIT_TEST_SRC}/testunitinstantiations3.cpp)
# SET(TEST_LIB_SOURCES ${TEST_LIB_SOURCES} ${DATAUNIT_TEST_SRC}/testunitinstantiations4.cpp)
# SET(TEST_LIB_SOURCES ${TEST_LIB_SOURCES} ${DATAUNIT_TEST_SRC}/testunitinstantiations5.cpp)
# SET(TEST_LIB_SOURCES ${TEST_LIB_SOURCES} ${DATAUNIT_TEST_SRC}/testunitinstantiations6.cpp)
# SET(TEST_LIB_SOURCES ${TEST_LIB_SOURCES} ${DATAUNIT_TEST_SRC}/testunitinstantiations7.cpp)
# SET(TEST_LIB_SOURCES ${TEST_LIB_SOURCES} ${DATAUNIT_TEST_SRC}/testunitinstantiations8.cpp)

IF (MINGW)
    SET(MODULE_TEST_LIB_LINK STATIC)
ELSE()
    SET(MODULE_TEST_LIB_LINK ${LINK_TYPE})
ENDIF()

ADD_LIBRARY(${MODULE_TEST_LIB} ${MODULE_TEST_LIB_LINK} ${HATN_TEST_THREAD_SOURCES} ${TEST_LIB_SOURCES})
TARGET_INCLUDE_DIRECTORIES(${MODULE_TEST_LIB} PRIVATE ${TEST_BINARY_DIR})

IF (MSVC OR MINGW)
    TARGET_COMPILE_DEFINITIONS(${MODULE_TEST_LIB} PRIVATE "-DBUILD_TEST_UNIT")
ELSE()
    TARGET_COMPILE_DEFINITIONS(${MODULE_TEST_LIB} PRIVATE -DBUILD_TEST_UNIT)
ENDIF()

IF ("${MODULE_TEST_LIB_LINK}" STREQUAL "STATIC")
    SET(HATN_TEST_THREAD_SOURCES "")
ENDIF()

ADD_HATN_MODULES(${MODULE_TEST_LIB} PRIVATE dataunit)

IF (MSVC)
    SET_SOURCE_FILES_PROPERTIES(${TEST_LIB_SOURCES} PROPERTIES COMPILE_FLAGS /bigobj)
ENDIF()

IF (MINGW AND BUILD_DEBUG)
    # Fix string table overflow when compiling in debug mode
    SET_SOURCE_FILES_PROPERTIES(${TEST_SOURCES} PROPERTIES COMPILE_FLAGS -Os)
    SET_SOURCE_FILES_PROPERTIES(${TEST_LIB_SOURCES} PROPERTIES COMPILE_FLAGS -Os)
ENDIF ()

ADD_HATN_CTESTS(dataunit ${TEST_SOURCES})

FUNCTION(TestDataunit)
    COPY_LIBRARY_HERE(hatncommon${LIB_POSTFIX} ../common/)
    COPY_LIBRARY_HERE(hatndataunit${LIB_POSTFIX} ../dataunit/)
ENDFUNCTION(TestDataunit)

ADD_CUSTOM_TARGET(dataunit-src SOURCES ${TEST_HEADERS} ${TEST_SOURCES} ${SOURCES})
