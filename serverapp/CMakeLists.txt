PROJECT(hatnserverapp)

SET (HEADERS
    include/hatn/serverapp/serverappdefs.h
    include/hatn/serverapp/serverapp.h
)

SET (SOURCES
    src/serverapp.cpp
)

BUILD_HATN_MODULE()

TARGET_LINK_LIBRARIES(${PROJECT_NAME} PUBLIC args)
