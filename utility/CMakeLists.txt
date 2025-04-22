PROJECT(hatnutility)

SET (HEADERS
    include/hatn/utility/utility.h
    include/hatn/utility/utilityerror.h
    include/hatn/utility/utilityerrorcodes.h
    include/hatn/utility/aclconstants.h
    include/hatn/utility/aclmodels.h
    include/hatn/utility/acldbmodels.h
    include/hatn/utility/acldbmodelsprovider.h
    include/hatn/utility/accesschecker.h
    include/hatn/utility/aclcontroller.h
    include/hatn/utility/localaclcontroller.h
)

SET (HEADERS
    ${HEADERS}
    include/hatn/utility/ipp/accesschecker.ipp
    include/hatn/utility/ipp/localaclcontroller.ipp
)

SET (SOURCES
    src/utilityerror.cpp
    src/acldbmodelsprovider.cpp
)

BUILD_HATN_MODULE()
HATN_LIB_LINK_ROCKSDB()
