SET (TEST_SOURCES
    ${MEDIA_TEST_SRC}/testwaveform.cpp
    ${MEDIA_TEST_SRC}/testpcmring.cpp
    ${MEDIA_TEST_SRC}/testoggopus.cpp
    ${MEDIA_TEST_SRC}/testvoicerecorder.cpp
    ${MEDIA_TEST_SRC}/testvoiceplayer.cpp
    ${MEDIA_TEST_SRC}/testvoicecrop.cpp
)

SET (TEST_HEADERS
    ${HEADERS}
    ${MEDIA_TEST_SRC}/testmediautils.h
)

ADD_HATN_CTESTS(media ${TEST_SOURCES} ${TEST_HEADERS})

FUNCTION(TestMedia)
    COPY_LIBRARY_HERE(hatncommon${LIB_POSTFIX} ../common/)
    COPY_LIBRARY_HERE(hatndataunit${LIB_POSTFIX} ../dataunit/)
    COPY_LIBRARY_HERE(hatnbase${LIB_POSTFIX} ../base/)
    COPY_LIBRARY_HERE(hatnlogcontext${LIB_POSTFIX} ../logcontext/)
    COPY_LIBRARY_HERE(hatnmedia${LIB_POSTFIX} ../media/)
ENDFUNCTION(TestMedia)

ADD_CUSTOM_TARGET(mediatest-src SOURCES ${TEST_HEADERS} ${TEST_SOURCES} ${SOURCES})
