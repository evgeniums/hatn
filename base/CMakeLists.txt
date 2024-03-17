PROJECT(hatnbase)

SET (HEADERS
    include/hatn/base/base.h
    include/hatn/base/baseerror.h
    include/hatn/base/baseerrorcodes.h
    include/hatn/base/result.h
    include/hatn/base/configtreevalue.h
    include/hatn/base/detail/configtreevalue.ipp
    include/hatn/base/configtree.h
)

SET (SOURCES
    src/baseerror.cpp
    src/configtree.cpp
)

BUILD_HATN_MODULE()
