SET(NO_DYNAMIC_HATN_PLUGINS OFF CACHE BOOL "Do not use dymanic plugins")

SET(LINK_TYPE SHARED)

IF (NOT BUILD_DEBUG)
    IF (NOT WIN32)
        IF (NOT BUILD_IOS)
            # hidden visibility breaks plugins scheme
            # MESSAGE(STATUS "Set visibility hidden")
            # SET (HATN_PRIVATE_COMPILE_OPTIONS -fvisibility=hidden -fvisibility-inlines-hidden)
        ENDIF()
    ENDIF()
ENDIF()
