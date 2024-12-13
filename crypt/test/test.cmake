IF (NO_DYNAMIC_HATN_PLUGINS)
    INCLUDE_DIRECTORIES(
        ${INCLUDE_DIRECTORIES}
        ${HATN_CRYPT_SRC}/plugins/openssl/include
    )
ENDIF()

SET(MODULE_TEST_LIB crypttestlib)

SET(TEST_LIB_SOURCES
    ${CRYPT_TEST_SRC}/initcryptplugin.cpp
)

SET(TEST_LIB_HEADERS
    ${CRYPT_TEST_SRC}/initcryptplugin.h
)

ADD_LIBRARY(${MODULE_TEST_LIB} STATIC ${TEST_LIB_SOURCES} ${TEST_LIB_HEADERS})
TARGET_LINK_LIBRARIES(${MODULE_TEST_LIB} PUBLIC hatncrypt)
TARGET_INCLUDE_DIRECTORIES(${MODULE_TEST_LIB} PRIVATE ${TEST_BINARY_DIR} ${PLUGIN_SRC_DIRS})
TARGET_COMPILE_DEFINITIONS(${MODULE_TEST_LIB} PRIVATE -DBUILD_TEST_CRYPT)

SET (TEST_SOURCES
    ${CRYPT_TEST_SRC}/testrandomgen.cpp
    ${CRYPT_TEST_SRC}/testdigest.cpp
    ${CRYPT_TEST_SRC}/testcipher.cpp
    ${CRYPT_TEST_SRC}/testpbkdf.cpp
    # ${CRYPT_TEST_SRC}/testhmac.cpp
    # ${CRYPT_TEST_SRC}/testencrypthmac.cpp
    ${CRYPT_TEST_SRC}/testhkdf.cpp
    # ${CRYPT_TEST_SRC}/testdh.cpp
    ${CRYPT_TEST_SRC}/testecdh.cpp
    ${CRYPT_TEST_SRC}/testaead.cpp
    ${CRYPT_TEST_SRC}/testmac.cpp
    ${CRYPT_TEST_SRC}/testsignature.cpp
    ${CRYPT_TEST_SRC}/testcryptcontainer.cpp
    ${CRYPT_TEST_SRC}/testcryptfile.cpp
    ${CRYPT_TEST_SRC}/testkeyexportimport.cpp
    ${CRYPT_TEST_SRC}/testx509.cpp
    ${CRYPT_TEST_SRC}/testtls.cpp
)

ADD_HATN_CTESTS(crypt ${TEST_SOURCES} ${TEST_HEADERS})

ADD_CUSTOM_TARGET(crypt-src SOURCES ${TEST_HEADERS} ${TEST_SOURCES} ${SOURCES})

FUNCTION(TestCrypt)
    COPY_LIBRARY_HERE(hatncommon${LIB_POSTFIX} ../common/)
    COPY_LIBRARY_HERE(hatncrypt${LIB_POSTFIX} ../crypt/)
    COPY_LIBRARY_HERE(hatnlogcontext${LIB_POSTFIX} ../logcontext/)
    COPY_LIBRARY_HERE(hatndataunit${LIB_POSTFIX} ../dataunit/)
ENDFUNCTION(TestCrypt)
