PROJECT(hatnapp)

SET (HEADERS
    include/hatn/app/appdefs.h
    include/hatn/app/apperror.h
    include/hatn/app/apperrorcodes.h
    include/hatn/app/appenv.h
    include/hatn/app/app.h
    include/hatn/app/appname.h
)

SET (SOURCES
    src/apperror.cpp
    src/app.cpp
    src/appenv.cpp
)

BUILD_HATN_MODULE()

IF(MSVC)
    TARGET_COMPILE_OPTIONS(${PROJECT_NAME} PRIVATE /bigobj)
ENDIF()

IF (HATN_PLUGIN_rocksdb)
    TARGET_LINK_LIBRARIES(${PROJECT_NAME} PRIVATE hatnrocksdbschema)
ENDIF()
