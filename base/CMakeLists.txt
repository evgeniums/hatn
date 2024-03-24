PROJECT(hatnbase)

SET (HEADERS
    include/hatn/base/base.h
    include/hatn/base/basetranslate.h
    include/hatn/base/baseerror.h
    include/hatn/base/baseerrorcodes.h
    include/hatn/base/configtreevalue.h
    include/hatn/base/detail/configtreevalue.ipp
    include/hatn/base/configtree.h
    include/hatn/base/configtreepath.h
    include/hatn/base/configtreeio.h
    include/hatn/base/configtreejson.h
    include/hatn/base/configtreeloader.h
)

SET (SOURCES
    src/baseerror.cpp
    src/configtree.cpp
    src/configtreepath.cpp
    src/configtreeio.cpp
    src/configtreeloader.cpp
    src/configtreejson.cpp
)

BUILD_HATN_MODULE()
