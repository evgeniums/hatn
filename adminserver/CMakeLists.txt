PROJECT(hatnadminserver)

SET (HEADERS
    include/hatn/adminserver/adminserver.h
    include/hatn/adminserver/admindb.h
    include/hatn/adminserver/dbmodels.h
    include/hatn/adminserver/localadmincontroller.h
    include/hatn/adminserver/apiadmincontroller.h
    include/hatn/adminserver/appadmincontroller.h
)

SET (HEADERS
    ${HEADERS}
    include/hatn/adminserver/ipp/localadmincontroller.ipp
)

SET (SOURCES
    src/admindb.cpp
    src/dbmodels.cpp
)

BUILD_HATN_MODULE()

GET_PROPERTY(PLUGIN_LIST GLOBAL PROPERTY PLUGIN_LIST)
IF ("\"rocksdb\"" IN_LIST PLUGIN_LIST)
    MESSAGE(STATUS "Linking ${PROJECT_NAME} with hatnrocksdbschema")
    TARGET_LINK_LIBRARIES(${PROJECT_NAME} PUBLIC hatnrocksdbschema)
    LINK_ROCKSDB(${PROJECT_NAME})
ENDIF()
