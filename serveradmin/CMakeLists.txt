PROJECT(hatnserveradmin)

SET (HEADERS
    include/hatn/serveradmin/serveradmin.h
    include/hatn/serveradmin/admindb.h
    include/hatn/serveradmin/admin.h
    include/hatn/serveradmin/dbmodels.h
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
