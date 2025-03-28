PROJECT(hatnapp)

SET (HEADERS
    include/hatn/app/app.h
    include/hatn/app/baseapp.h
)

SET (SOURCES
    src/baseapp.cpp
)

BUILD_HATN_MODULE()
