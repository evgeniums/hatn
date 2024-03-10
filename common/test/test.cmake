SET(HEADERS
    ${HATN_COMMON_SRC}/include/hatn/test/multithreadfixture.h
)

SET(SOURCES
    ${HATN_COMMON_SRC}/include/hatn/test/multithreadfixture.cpp

    ${COMMON_TEST_SRC}/testplugins.cpp
    ${COMMON_TEST_SRC}/testmemorylockeddata.cpp
    ${COMMON_TEST_SRC}/testmetautils.cpp
    ${COMMON_TEST_SRC}/testsmartpointers.cpp
    ${COMMON_TEST_SRC}/testthread.cpp
    ${COMMON_TEST_SRC}/testlogger.cpp
    ${COMMON_TEST_SRC}/testenvironment.cpp
    ${COMMON_TEST_SRC}/testbytearray.cpp
    ${COMMON_TEST_SRC}/testfixedbytearray.cpp
    ${COMMON_TEST_SRC}/testfactory.cpp
    ${COMMON_TEST_SRC}/testfile.cpp
    ${COMMON_TEST_SRC}/testmemorypool.cpp
    ${COMMON_TEST_SRC}/testpoolmemoryresource.cpp
    ${COMMON_TEST_SRC}/testflatmap.cpp
)

IF(ENABLE_TRANSLATIONS)
  SET(SOURCES
     ${SOURCES}
     ${COMMON_TEST_SRC}/testtranslation.cpp
  )
ENDIF(ENABLE_TRANSLATIONS)

TARGET_SOURCES(${PROJECT_NAME} PRIVATE ${HEADERS} ${SOURCES})

IF(MINGW)
    IF(BUILD_STATIC)
        MESSAGE(WARNING "Disable timer thread test for MinGW static build due to unhandled bug")
        ADD_COMPILE_DEFINITIONS(HATN_THREAD_TIMER_TEST_DISABLE=1)
    ENDIF(BUILD_STATIC)
ENDIF(MINGW)

FUNCTION(TestCommon)

    COPY_LIBRARY_HERE(hatncommon${LIB_POSTFIX} ../common/)

    IF(NOT BUILD_STATIC)
        COPY_LIBRARY(hatn-testplugin${LIB_POSTFIX} ../testplugin plugins/)
        COPY_SOURCE_FILE(../testplugin/testplugin.dat testplugin.dat)
    ENDIF(NOT BUILD_STATIC)

    IF(ENABLE_TRANSLATIONS)

        INCLUDE(hatn/Translate)

        # Translation files
        ADD_CUSTOM_COMMAND(TARGET ${PROJECT_NAME} POST_BUILD
            COMMAND
                ${PYTHON_EXE} ${TRANSLATION_SCRIPTS}/compile-mo.py
                "${COMMON_TEST_SRC}/test-translations/test1.po"
                "${BINDIR}/translations/ru/LC_MESSAGES/test1.mo"
                "${MSGFMT_EXE}"
            COMMAND
                ${PYTHON_EXE} ${TRANSLATION_SCRIPTS}/compile-mo.py
                "${COMMON_TEST_SRC}/test-translations/test2.po"
                "${BINDIR}/translations/ru/LC_MESSAGES/test2.mo"
                "${MSGFMT_EXE}"
        )

        COPY_FILE_BINDIR("ru/LC_MESSAGES/hatncommon.mo" "../common/translations" "translations")

    ENDIF(ENABLE_TRANSLATIONS)

ENDFUNCTION(TestCommon)
