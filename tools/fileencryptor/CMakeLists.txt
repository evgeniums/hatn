CMAKE_MINIMUM_REQUIRED(VERSION 3.15)
PROJECT(hatn-file-encryptor-project)

SET(FILEENCRYPTOR_CORE_SOURCES
    src/filecryptbase.cpp
    src/fileencryptor.cpp
    src/filedecryptor.cpp
)

SET(FILEENCRYPTOR_CORE_HEADERS
    include/hatn/fileencryptor/fileencryptor.h
    include/hatn/fileencryptor/filedecryptor.h
)

SET(FILEENCRYPTOR_CAPI_SOURCES
    src/fileencryptor_capi.cpp
    src/filedecryptor_capi.cpp
)

SET(FILEENCRYPTOR_CAPI_HEADERS
    include/hatn/fileencryptor/fileencryptor_capi.h
    include/hatn/fileencryptor/filedecryptor_capi.h
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
# Macro: add a file-crypt CLI executable
# --------------------------------------------------------------------------

MACRO(ADD_FILECRYPT_EXE target_name main_src)
    ADD_EXECUTABLE(${target_name}
        ${main_src}
        $<TARGET_OBJECTS:hatnfileencryptorcore>
    )
    SET_PROPERTY(TARGET ${target_name} PROPERTY CXX_STANDARD 17)
    TARGET_INCLUDE_DIRECTORIES(${target_name} PRIVATE
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<BUILD_INTERFACE:${HATN_BINARY_DIR}/include>
        ${HATN_INCLUDE_DIRECTORIES}
    )
    TARGET_COMPILE_DEFINITIONS(${target_name} PRIVATE ${HATN_COMPILE_DEFINITIONS})
    TARGET_COMPILE_OPTIONS(${target_name}     PRIVATE ${HATN_COMPILE_OPTIONS})
    ADD_HATN_MODULES(${target_name} PRIVATE app crypt)
    TARGET_LINK_DIRECTORIES(${target_name} PRIVATE ${HATN_LINK_DIRECTORIES})
    TARGET_LINK_LIBRARIES(${target_name} PRIVATE
        ${CMAKE_THREAD_LIBS_INIT}
        ${Boost_LIBRARIES}
        ${HATN_SYSTEM_LIBS}
    )
ENDMACRO()

# --------------------------------------------------------------------------
# Executables
# --------------------------------------------------------------------------

ADD_FILECRYPT_EXE(hatn-file-encryptor src/main.cpp)
ADD_FILECRYPT_EXE(hatn-file-decryptor src/maindecrypt.cpp)
