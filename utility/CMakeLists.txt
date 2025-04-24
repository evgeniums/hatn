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
    include/hatn/utility/systemsection.h
    include/hatn/utility/accesstype.h
    include/hatn/utility/operation.h
    include/hatn/utility/journal.h
    include/hatn/utility/journalmodels.h
    include/hatn/utility/journaldbmodels.h
    include/hatn/utility/journaldbmodelsprovider.h
    include/hatn/utility/sectionmodels.h
    include/hatn/utility/sectiondbmodels.h
    include/hatn/utility/sectiondbmodelsprovider.h
    include/hatn/utility/sectioncontroller.h
    include/hatn/utility/topicsection.h
    include/hatn/utility/withmac.h
    include/hatn/utility/withmacdb.h
)

SET (HEADERS
    ${HEADERS}
    include/hatn/utility/ipp/accesschecker.ipp
    include/hatn/utility/ipp/localaclcontroller.ipp    
)

SET (SOURCES
    src/utilityerror.cpp
    src/acldbmodelsprovider.cpp
    src/operation.cpp
    src/acloperations.cpp
    src/journaldbmodelsprovider.cpp
    src/sectiondbmodelsprovider.cpp
)

BUILD_HATN_MODULE()
HATN_LIB_LINK_ROCKSDB()
