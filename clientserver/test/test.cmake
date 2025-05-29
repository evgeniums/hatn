IF (NO_DYNAMIC_HATN_PLUGINS)
    INCLUDE_DIRECTORIES(
        ${INCLUDE_DIRECTORIES}
        ${HATN_CRYPT_SRC}/plugins/openssl/include
    )
ENDIF()

SET(MODULE_TEST_LIB clientservertestlib)

SET(TEST_LIB_SOURCES
    ${HATN_CRYPT_SRC}/test/initcryptplugin.cpp
)

SET(TEST_LIB_HEADERS
    ${HATN_CRYPT_SRC}/test/initcryptplugin.h
)

ADD_LIBRARY(${MODULE_TEST_LIB} STATIC ${TEST_LIB_SOURCES} ${TEST_LIB_HEADERS})
TARGET_LINK_LIBRARIES(${MODULE_TEST_LIB} PUBLIC hatncrypt)
TARGET_INCLUDE_DIRECTORIES(${MODULE_TEST_LIB} PUBLIC ${TEST_BINARY_DIR} ${PLUGIN_SRC_DIRS} ${HATN_CRYPT_SRC}/test)

SET (TEST_SOURCES
    ${CLIENTSERVER_TEST_SRC}/testaccountconfig.cpp
)

ADD_HATN_CTESTS(clientserver ${TEST_SOURCES} ${TEST_HEADERS})

ADD_CUSTOM_TARGET(clientservertest-src SOURCES ${TEST_HEADERS} ${TEST_SOURCES} ${SOURCES})

IF (BUILD_STATIC)
    MESSAGE(STATUS "Linking ${MODULE_TEST_LIB} with crypt plugins for static build")
    LINK_HATN_PLUGINS(${MODULE_TEST_LIB} crypt)

    IF (HATN_PLUGIN_openssl)
        SET (OPENSSL_PLUGIN_SRC_DIR "${HATN_SOURCE_DIR}/crypt/plugins/openssl/include")
        MESSAGE(STATUS "Add include directory for static build for ${MODULE_TEST_LIB}: ${OPENSSL_PLUGIN_SRC_DIR}")
        ADD_DEPENDENCIES(${MODULE_TEST_LIB} hatnopenssl)
        TARGET_INCLUDE_DIRECTORIES(${MODULE_TEST_LIB} PRIVATE ${OPENSSL_PLUGIN_SRC_DIR})
    ENDIF()
ENDIF()

FUNCTION(TestClientServer)
    COPY_LIBRARY_HERE(hatncommon${LIB_POSTFIX} ../common/)
    COPY_LIBRARY_HERE(hatncrypt${LIB_POSTFIX} ../crypt/)
    COPY_LIBRARY_HERE(hatnlogcontext${LIB_POSTFIX} ../logcontext/)
    COPY_LIBRARY_HERE(hatndataunit${LIB_POSTFIX} ../dataunit/)
    COPY_LIBRARY_HERE(hatnclientserver${LIB_POSTFIX} ../clientserver/)

    IF (NOT BUILD_STATIC)
        IF (HATN_PLUGIN_openssl)
            MESSAGE(STATUS "Copying openssl plugin to test folder")
            COPY_LIBRARY(hatnopenssl${LIB_POSTFIX} ../crypt/plugins/openssl plugins/crypt)
        ENDIF()
    ENDIF()

ENDFUNCTION(TestClientServer)
