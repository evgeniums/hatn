PROJECT(hatnopenssl)

IF(WIN32)
    MESSAGE(STATUS "Using OPENSSL_ROOT_DIR=${OPENSSL_ROOT_DIR}")
ENDIF()

FIND_PACKAGE(OpenSSL REQUIRED)

INCLUDE_DIRECTORIES(
    ${INCLUDE_DIRECTORIES}
    ${OPENSSL_INCLUDE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

SET (HEADERS
    include/hatn/crypt/plugins/openssl/opensslplugindef.h
    include/hatn/crypt/plugins/openssl/opensslplugin.h
    include/hatn/crypt/plugins/openssl/opensslerror.h
    include/hatn/crypt/plugins/openssl/opensslcontext.h
    include/hatn/crypt/plugins/openssl/opensslstream.h
    include/hatn/crypt/plugins/openssl/opensslx509.h
    include/hatn/crypt/plugins/openssl/opensslutils.h
    include/hatn/crypt/plugins/openssl/openssldh.h
    include/hatn/crypt/plugins/openssl/openssldh1.h
    include/hatn/crypt/plugins/openssl/opensslprivatekey.h
    include/hatn/crypt/plugins/openssl/opensslprivatekey.ipp
    include/hatn/crypt/plugins/openssl/opensslsessionticketkey.h
    include/hatn/crypt/plugins/openssl/opensslx509chain.h
    include/hatn/crypt/plugins/openssl/opensslsecretkey.h
    include/hatn/crypt/plugins/openssl/opensslsymmetric.h
    include/hatn/crypt/plugins/openssl/openssldigest.h
    include/hatn/crypt/plugins/openssl/opensslhmac.h
    include/hatn/crypt/plugins/openssl/opensslpbkdf.h
    include/hatn/crypt/plugins/openssl/opensslhkdf.h
    include/hatn/crypt/plugins/openssl/opensslsignature.h
    include/hatn/crypt/plugins/openssl/opensslpublickey.h
    include/hatn/crypt/plugins/openssl/opensslasymmetric.h
    include/hatn/crypt/plugins/openssl/opensslmackey.h
    include/hatn/crypt/plugins/openssl/opensslec.h
    include/hatn/crypt/plugins/openssl/opensslrsa.h
    include/hatn/crypt/plugins/openssl/openssldsa.h
    include/hatn/crypt/plugins/openssl/opensslrandomgenerator.h
    include/hatn/crypt/plugins/openssl/opensslpasswordgenerator.h
    include/hatn/crypt/plugins/openssl/opensslecdh.h
    include/hatn/crypt/plugins/openssl/opensslaead.h
    include/hatn/crypt/plugins/openssl/opensslmac.h
    include/hatn/crypt/plugins/openssl/opensslmac1.h
    include/hatn/crypt/plugins/openssl/openssldigestsign.h
    include/hatn/crypt/plugins/openssl/openssldigestsign.ipp
    include/hatn/crypt/plugins/openssl/openssled.h
    include/hatn/crypt/plugins/openssl/opensslx509certificatestore.h
)

SET (SOURCES
    opensslerror.cpp
    opensslcontext.cpp
    opensslplugin.cpp
    opensslx509.cpp
    opensslstream.cpp
    openssldh.cpp
    openssldh1.cpp
    opensslprivatekey.cpp
    opensslsessionticketkey.cpp
    opensslx509chain.cpp
    opensslsymmetric.cpp
    openssldigest.cpp
    opensslhmac.cpp
    opensslmac1.cpp
    opensslmac.cpp
    opensslpublickey.cpp
    opensslmackey.cpp
    opensslecdh.cpp
    opensslaead.cpp
    opensslsignature.cpp
    opensslhkdf.cpp
    opensslpbkdf.cpp
    openssldsa.cpp
    opensslec.cpp
    opensslrsa.cpp
    openssled.cpp
    opensslasymmetric.cpp
    opensslx509certificatestore.cpp
)

BUILD_HATN_PLUGIN(crypt)

IF(BUILD_ANDROID OR BUILD_IOS)

    LIST (GET OPENSSL_LIBRARIES 0 OPENSSL_LIB_PATH)
    SET (OPENSSL_LIBRARIES ${OPENSSL_LIB_PATH}/libssl.a ${OPENSSL_LIB_PATH}/libcrypto.a)

    MESSAGE(STATUS "Applying workaround for OpenSSL libs on Android/iOS: ${OPENSSL_LIBRARIES}")

ENDIF()

TARGET_LINK_LIBRARIES(${PROJECT_NAME} PUBLIC
    ${OPENSSL_LIBRARIES}
)
