PROJECT(hatnacl)

SET (HEADERS
    include/hatn/acl/acl.h
    include/hatn/acl/aclerror.h
    include/hatn/acl/aclerrorcodes.h
)

SET (SOURCES
    src/aclerror.cpp
)

BUILD_HATN_MODULE()
