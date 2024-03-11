PROJECT(hatntestplugin)

SET (HEADERS
)

SET (SOURCES
    testplugin.cpp
)

# ADD_LIBRARY(hatn${PROJECT_NAME} ${LINK_TYPE} ${SOURCES} ${HEADERS})

# ADD_HATN_MODULES(hatn${PROJECT_NAME} common)

BUILD_HATN_PLUGIN(common)
