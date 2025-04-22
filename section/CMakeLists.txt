PROJECT(hatnsection)

SET (HEADERS
    include/hatn/section/section.h
    include/hatn/section/sectionerror.h
    include/hatn/section/sectionerrorcodes.h    
    include/hatn/section/sectionmodels.h
    include/hatn/section/sectiondbmodels.h
    include/hatn/section/sectiondbmodelsprovider.h
    include/hatn/section/sectioncontroller.h
    include/hatn/section/localsectioncontroller.h

    include/hatn/section/mac.h
    include/hatn/section/withmac.h
    include/hatn/section/withmacdb.h
)

SET (HEADERS
    ${HEADERS}
    include/hatn/section/ipp/localsectioncontroller.ipp
)

SET (SOURCES
    src/sectionerror.cpp
    src/sectiondbmodelsprovider.cpp
)

BUILD_HATN_MODULE()
HATN_LIB_LINK_ROCKSDB()
