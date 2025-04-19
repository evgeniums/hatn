SET (TEST_SOURCES
    ${ACL_TEST_SRC}/testacl.cpp
)

SET (TEST_HEADERS
    ${HEADERS}
)

ADD_HATN_CTESTS(acl ${TEST_SOURCES} ${TEST_HEADERS})

FUNCTION(TestAcl)
    COPY_LIBRARY_HERE(hatncommon${LIB_POSTFIX} ../common/)
    COPY_LIBRARY_HERE(hatndataunit${LIB_POSTFIX} ../dataunit/)
    COPY_LIBRARY_HERE(hatnbase${LIB_POSTFIX} ../base/)
    COPY_LIBRARY_HERE(hatnnetwork${LIB_POSTFIX} ../network/)
    COPY_LIBRARY_HERE(hatncrypt${LIB_POSTFIX} ../crypt/)
    COPY_LIBRARY_HERE(hatnacl${LIB_POSTFIX} ../acl/)
    COPY_LIBRARY_HERE(hatnlogcontext${LIB_POSTFIX} ../logcontext/)
    COPY_LIBRARY_HERE(hatndb${LIB_POSTFIX} ../db/)
ENDFUNCTION(TestAcl)

ADD_CUSTOM_TARGET(acltest-src SOURCES ${TEST_HEADERS} ${TEST_SOURCES} ${SOURCES})
