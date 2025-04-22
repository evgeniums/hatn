PROJECT(hatnjournal)

SET (HEADERS
    include/hatn/journal/journal.h
    include/hatn/journal/journalerror.h
    include/hatn/journal/journalerrorcodes.h    
    include/hatn/journal/journalmodels.h
    include/hatn/journal/journaldbmodels.h
    include/hatn/journal/journaldbmodelsprovider.h
    # include/hatn/journal/journalcontroller.h
    # include/hatn/journal/localjournalcontroller.h
)

SET (HEADERS
    ${HEADERS}
    # include/hatn/journal/ipp/localjournalcontroller.ipp
)

SET (SOURCES
    src/journalerror.cpp
    src/journaldbmodelsprovider.cpp
)

BUILD_HATN_MODULE()
HATN_LIB_LINK_ROCKSDB()
