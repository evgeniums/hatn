SET (TEST_SOURCES
    ${SERVERADMIN_TEST_SRC}/testadmin.cpp
)

SET (TEST_HEADERS
    ${HEADERS}
)

ADD_HATN_CTESTS(serveradmin ${TEST_SOURCES} ${TEST_HEADERS})

FUNCTION(TestServerAdmin)
    COPY_LIBRARY_HERE(hatncommon${LIB_POSTFIX} ../common/)
    COPY_LIBRARY_HERE(hatndataunit${LIB_POSTFIX} ../dataunit/)
    COPY_LIBRARY_HERE(hatnbase${LIB_POSTFIX} ../base/)
    COPY_LIBRARY_HERE(hatnnetwork${LIB_POSTFIX} ../network/)
    COPY_LIBRARY_HERE(hatncrypt${LIB_POSTFIX} ../crypt/)
    COPY_LIBRARY_HERE(hatnapi${LIB_POSTFIX} ../api/)
    COPY_LIBRARY_HERE(hatnapp${LIB_POSTFIX} ../app/)
    COPY_LIBRARY_HERE(hatnlogcontext${LIB_POSTFIX} ../logcontext/)
    COPY_LIBRARY_HERE(hatnserveradmin${LIB_POSTFIX} ../serveradmin/)
ENDFUNCTION(TestServerAdmin)

ADD_CUSTOM_TARGET(serveradmintest-src SOURCES ${TEST_HEADERS} ${TEST_SOURCES} ${SOURCES})

# SET (TEST_JSON
#     ${API_TEST_SRC}/assets/microservices.jsonc
#     ${API_TEST_SRC}/assets/microservice-duplicate.jsonc
#     ${API_TEST_SRC}/assets/microservice-missing-ip-addr.jsonc
#     ${API_TEST_SRC}/assets/microservice-invalid-ip-addr.jsonc
#     ${API_TEST_SRC}/assets/microservice-unknown-dispatcher.jsonc
#     ${API_TEST_SRC}/assets/microservice-unknown-authdispatcher.jsonc
#     ${API_TEST_SRC}/assets/microservice-port-busy.jsonc
# )

# ADD_CUSTOM_TARGET(serveradmintest-json SOURCES ${TEST_JSON})
