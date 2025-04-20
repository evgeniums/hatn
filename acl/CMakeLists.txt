PROJECT(hatnacl)

SET (HEADERS
    include/hatn/acl/acl.h
    include/hatn/acl/aclerror.h
    include/hatn/acl/aclerrorcodes.h
    include/hatn/acl/aclconstants.h
    include/hatn/acl/aclmodels.h
    include/hatn/acl/acldbmodels.h
    include/hatn/acl/acldbmodelsprovider.h
    include/hatn/acl/accesschecker.h
    include/hatn/acl/aclcontroller.h
    include/hatn/acl/localaclcontroller.h
)

SET (HEADERS
    ${HEADERS}
    include/hatn/acl/ipp/accesschecker.ipp
    include/hatn/acl/ipp/localaclcontroller.ipp
)

SET (SOURCES
    src/aclerror.cpp
    src/acldbmodelsprovider.cpp
)

BUILD_HATN_MODULE()
HATN_LIB_LINK_ROCKSDB()
