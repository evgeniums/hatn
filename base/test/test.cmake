SET (TEST_SOURCES
    ${BASE_TEST_SRC}/testconfigtree.cpp
    ${BASE_TEST_SRC}/testconfigtreevalue.cpp
    ${BASE_TEST_SRC}/testconfigtreeio.cpp
    ${BASE_TEST_SRC}/testconfigobject.cpp
)

SET (TEST_HEADERS
    ${HEADERS}
)

ADD_HATN_CTESTS(base ${TEST_SOURCES} ${TEST_HEADERS})

FUNCTION(TestBase)
    COPY_LIBRARY_HERE(hatncommon${LIB_POSTFIX} ../common/)
    COPY_LIBRARY_HERE(hatndataunit${LIB_POSTFIX} ../dataunit/)
    COPY_LIBRARY_HERE(hatnbase${LIB_POSTFIX} ../base/)
ENDFUNCTION(TestBase)

ADD_CUSTOM_TARGET(basetest-src SOURCES ${TEST_HEADERS} ${TEST_SOURCES} ${SOURCES})

SET (TEST_CONFIG_FILES
    ${BASE_TEST_SRC}/assets/config1.jsonc
    ${BASE_TEST_SRC}/assets/config4.jsonc
    ${BASE_TEST_SRC}/assets/config_cycle3.jsonc
    ${BASE_TEST_SRC}/assets/config_inc3.jsonc
    ${BASE_TEST_SRC}/assets/config1.jsonc
    ${BASE_TEST_SRC}/assets/config5.jsonc
    ${BASE_TEST_SRC}/assets/config_err1.jsonc
    ${BASE_TEST_SRC}/assets/config_inc4.jsonc
    ${BASE_TEST_SRC}/assets/config1_with_inc.jsonc
    ${BASE_TEST_SRC}/assets/config6.xml
    ${BASE_TEST_SRC}/assets/config_err2.jsonc
    ${BASE_TEST_SRC}/assets/config_inc5.jsonc
    ${BASE_TEST_SRC}/assets/config2.jsonc
    ${BASE_TEST_SRC}/assets/config_cycle1.jsonc
    ${BASE_TEST_SRC}/assets/config_inc1.jsonc
    ${BASE_TEST_SRC}/assets/config_inc6.jsonc
    ${BASE_TEST_SRC}/assets/config2_inc3.jsonc
    ${BASE_TEST_SRC}/assets/config_cycle2.jsonc
    ${BASE_TEST_SRC}/assets/config_inc2.jsonc
    ${BASE_TEST_SRC}/assets/config_bool.jsonc
)

ADD_CUSTOM_TARGET(basetest-files SOURCES ${TEST_CONFIG_FILES})
