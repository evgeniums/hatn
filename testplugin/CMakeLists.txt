PROJECT(hatn-testplugin)

SET (HEADERS
)

SET (SOURCES
    testplugin.cpp
)

ADD_LIBRARY(${PROJECT_NAME} ${LINK_TYPE} ${SOURCES} ${HEADERS})

ADD_HATN_MODULES(testplugin common)
