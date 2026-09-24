PROJECT(hatnapp)

SET (HEADERS
    include/hatn/app/appdefs.h
    include/hatn/app/apperror.h
    include/hatn/app/apperrorcodes.h
    include/hatn/app/appenv.h
    include/hatn/app/app.h
    include/hatn/app/appname.h
    include/hatn/app/eventdispatcher.h
)

SET (SOURCES
    src/apperror.cpp
    src/app.cpp
    src/appenv.cpp
    src/eventdispatcher.cpp
)

BUILD_HATN_MODULE()

IF(MSVC)
    TARGET_COMPILE_OPTIONS(${PROJECT_NAME} PRIVATE /bigobj)
ENDIF()

IF (HATN_PLUGIN_rocksdb)
    SET_TARGET_PROPERTIES(${PROJECT_NAME} PROPERTIES CXX_STANDARD 20 CXX_STANDARD_REQUIRED ON)
    TARGET_LINK_LIBRARIES(${PROJECT_NAME} PRIVATE hatnrocksdbschema)
ENDIF()
