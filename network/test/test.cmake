SET (TEST_SOURCES
     ${NETWORK_TEST_SRC}/testtcp.cpp
     # ${NETWORK_TEST_SRC}/testudp.cpp
)

# IF(NOT BUILD_ANDROID)
#     SET (TEST_SOURCES
#          ${TEST_SOURCES}
#          ${NETWORK_TEST_SRC}/testresolver.cpp
#     )
# ENDIF()

ADD_HATN_CTESTS(network ${TEST_SOURCES} ${TEST_HEADERS})

FUNCTION(TestNetwork)
    COPY_LIBRARY_HERE(hatncommon${LIB_POSTFIX} ../common/)
    COPY_LIBRARY_HERE(hatnnetwork${LIB_POSTFIX} ../network/)
ENDFUNCTION(TestNetwork)

IF ($ENV{DNS_RESOLVE_LOCAL_HOSTS})
    ADD_DEFINITIONS(-DDNS_RESOLVE_LOCAL_HOSTS)
    MESSAGE(STATUS "Test DNS resolving using hosts file")
ENDIF()

ADD_CUSTOM_TARGET(network-src SOURCES ${TEST_HEADERS} ${TEST_SOURCES} ${SOURCES})
