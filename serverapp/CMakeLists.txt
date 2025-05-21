PROJECT(hatnserverapp)

SET (HEADERS
    include/hatn/serverapp/serverappdefs.h
    include/hatn/serverapp/localusercontroller.h
    include/hatn/serverapp/userdbmodels.h
    include/hatn/serverapp/userdbmodelsprovider.h
)

SET (HEADERS
    ${HEADERS}
    include/hatn/serverapp/ipp/localusercontroller.ipp
)

SET (SOURCES
    src/serverapp.cpp
    src/userdbmodelsprovider.cpp
)

BUILD_HATN_MODULE()

TARGET_LINK_LIBRARIES(${PROJECT_NAME} PUBLIC args)

IF (HATN_PLUGIN_rocksdb)
    TARGET_LINK_LIBRARIES(${PROJECT_NAME} PUBLIC hatnrocksdbschema)
ENDIF()
