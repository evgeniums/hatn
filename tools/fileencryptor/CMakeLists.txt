CMAKE_MINIMUM_REQUIRED(VERSION 3.15)
PROJECT(hatn-file-encryptor-project)

SET(FILEENCRYPTOR_CORE_SOURCES
    src/fileencryptor.cpp
)

SET(FILEENCRYPTOR_CORE_HEADERS
    include/hatn/fileencryptor/fileencryptor.h
)

SET(FILEENCRYPTOR_CAPI_SOURCES
    src/fileencryptor_capi.cpp
)

SET(FILEENCRYPTOR_CAPI_HEADERS
    include/hatn/fileencryptor/fileencryptor_capi.h
)

# --------------------------------------------------------------------------
# Core object library (shared between the shared lib and the exe)
# --------------------------------------------------------------------------

ADD_LIBRARY(hatnfileencryptorcore OBJECT
    ${FILEENCRYPTOR_CORE_SOURCES}
    ${FILEENCRYPTOR_CORE_HEADERS}
)

SET_PROPERTY(TARGET hatnfileencryptorcore PROPERTY CXX_STANDARD 17)
SET_PROPERTY(TARGET hatnfileencryptorcore PROPERTY POSITION_INDEPENDENT_CODE ON)

TARGET_INCLUDE_DIRECTORIES(hatnfileencryptorcore PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<BUILD_INTERFACE:${HATN_BINARY_DIR}/include>
    ${HATN_INCLUDE_DIRECTORIES}
)

TARGET_COMPILE_DEFINITIONS(hatnfileencryptorcore PUBLIC ${HATN_COMPILE_DEFINITIONS})
TARGET_COMPILE_OPTIONS(hatnfileencryptorcore     PUBLIC ${HATN_COMPILE_OPTIONS})

ADD_HATN_MODULES(hatnfileencryptorcore PUBLIC app crypt)

# --------------------------------------------------------------------------
# Shared library: libhatnfileencryptor (.dylib / .so)
# --------------------------------------------------------------------------

ADD_LIBRARY(hatnfileencryptor SHARED
    ${FILEENCRYPTOR_CAPI_SOURCES}
    ${FILEENCRYPTOR_CAPI_HEADERS}
    $<TARGET_OBJECTS:hatnfileencryptorcore>
)

SET_PROPERTY(TARGET hatnfileencryptor PROPERTY CXX_STANDARD 17)
SET_PROPERTY(TARGET hatnfileencryptor PROPERTY POSITION_INDEPENDENT_CODE ON)

TARGET_INCLUDE_DIRECTORIES(hatnfileencryptor PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<BUILD_INTERFACE:${HATN_BINARY_DIR}/include>
    ${HATN_INCLUDE_DIRECTORIES}
)

TARGET_COMPILE_DEFINITIONS(hatnfileencryptor PUBLIC ${HATN_COMPILE_DEFINITIONS})
TARGET_COMPILE_OPTIONS(hatnfileencryptor     PUBLIC ${HATN_COMPILE_OPTIONS})

# Link hatn modules; ADD_HATN_MODULES also pulls in static plugin archives.
ADD_HATN_MODULES(hatnfileencryptor PRIVATE app crypt)

TARGET_LINK_DIRECTORIES(hatnfileencryptor PRIVATE ${HATN_LINK_DIRECTORIES})
TARGET_LINK_LIBRARIES(hatnfileencryptor PRIVATE
    ${CMAKE_THREAD_LIBS_INIT}
    ${Boost_LIBRARIES}
    ${HATN_SYSTEM_LIBS}
)

# On macOS, force-load the openssl plugin static archive so the PluginInfo
# registration symbol is not stripped by the linker.
IF(APPLE)
    IF(HATN_PLUGIN_openssl)
        TARGET_LINK_OPTIONS(hatnfileencryptor PRIVATE
            "LINKER:-force_load,$<TARGET_FILE:hatnopenssl>"
        )
    ENDIF()
ELSEIF(UNIX)
    IF(HATN_PLUGIN_openssl)
        TARGET_LINK_OPTIONS(hatnfileencryptor PRIVATE
            "LINKER:--whole-archive,$<TARGET_FILE:hatnopenssl>,--no-whole-archive"
        )
    ENDIF()
ENDIF()

# --------------------------------------------------------------------------
# Executable: hatn-file-encryptor
# --------------------------------------------------------------------------

ADD_EXECUTABLE(hatn-file-encryptor
    src/main.cpp
    $<TARGET_OBJECTS:hatnfileencryptorcore>
)

SET_PROPERTY(TARGET hatn-file-encryptor PROPERTY CXX_STANDARD 17)

TARGET_INCLUDE_DIRECTORIES(hatn-file-encryptor PRIVATE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<BUILD_INTERFACE:${HATN_BINARY_DIR}/include>
    ${HATN_INCLUDE_DIRECTORIES}
)

TARGET_COMPILE_DEFINITIONS(hatn-file-encryptor PRIVATE ${HATN_COMPILE_DEFINITIONS})
TARGET_COMPILE_OPTIONS(hatn-file-encryptor     PRIVATE ${HATN_COMPILE_OPTIONS})

ADD_HATN_MODULES(hatn-file-encryptor PRIVATE app crypt)

TARGET_LINK_DIRECTORIES(hatn-file-encryptor PRIVATE ${HATN_LINK_DIRECTORIES})
TARGET_LINK_LIBRARIES(hatn-file-encryptor PRIVATE
    ${CMAKE_THREAD_LIBS_INIT}
    ${Boost_LIBRARIES}
    ${HATN_SYSTEM_LIBS}
)
