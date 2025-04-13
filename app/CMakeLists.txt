PROJECT(hatnapp)

SET (HEADERS
    include/hatn/app/app.h
    include/hatn/app/apperror.h
    include/hatn/app/apperrorcodes.h
    include/hatn/app/appenv.h
    include/hatn/app/appname.h
    include/hatn/app/baseapp.h
)

SET (SOURCES
    src/apperror.cpp
    src/baseapp.cpp
)

BUILD_HATN_MODULE()
