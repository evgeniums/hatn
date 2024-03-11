INCLUDE (hatn/Translate)

FUNCTION(CREATE_CONFIG_FILE MODULE_NAME MODULE_HEADERS_PATH)

    STRING(REGEX REPLACE "^hatn" "" MODULE_NAME ${MODULE_NAME})
    SET (CONFIG_FILE "include/hatn/${MODULE_HEADERS_PATH}/config.h")
    STRING (TOUPPER ${MODULE_NAME} UPPER_MODULE_NAME)

    IF(BUILD_STATIC)
        SET(LINKING "STATIC")
    ELSE(BUILD_STATIC)
        SET(LINKING "DYNAMIC")
    ENDIF(BUILD_STATIC)

    SET (CONFIG_H_TEXT ${${UPPER_MODULE_NAME}_CONFIG_H_TEXT})

    IF(BUILD_ANDROID)
        SET (CONFIG_H_TEXT
"#ifndef ANDROID \n\
    #define ANDROID 1 \n\
#endif \n\
${CONFIG_H_TEXT}
"
            )
    ENDIF()

    FILE(WRITE ${HATN_BINARY_DIR}/${CONFIG_FILE}
"/***************************************************/\n\
/*****This file is auto-generated. Do not edit.*****/\n\
/***************************************************/\n\
\n\
#ifndef HATN_${UPPER_MODULE_NAME}_${LINKING}_BUILD \n\
    #define HATN_${UPPER_MODULE_NAME}_${LINKING}_BUILD 1 \n\
#endif \n\
#ifndef HATN_${UPPER_MODULE_NAME}_EXPORT \n\
    #ifdef HATN_${UPPER_MODULE_NAME}_STATIC_BUILD \n\
        #define HATN_${UPPER_MODULE_NAME}_EXPORT \n\
    #endif \n\
#endif \n\
${CONFIG_H_TEXT}
"
        )

ENDFUNCTION(CREATE_CONFIG_FILE)

FUNCTION(SETUP_HATN_VERSION)

    SET (VERSION_FILE ${HATN_BINARY_DIR}/include/hatn/version.h)

    FILE(READ ${HATN_SOURCE_DIR}/VERSION HATN_VERSION)
    STRING(REPLACE "." ";" VERSION_LIST ${HATN_VERSION})
    list(GET VERSION_LIST 0 HATN_VERSION_MAJOR)
    list(GET VERSION_LIST 1 HATN_VERSION_MINOR)
    list(GET VERSION_LIST 2 HATN_VERSION_PATCH)

    SET(HATN_VERSION_MAJOR ${HATN_VERSION_MAJOR} PARENT_SCOPE)
    SET(HATN_VERSION_MINOR ${HATN_VERSION_MINOR} PARENT_SCOPE)
    SET(HATN_VERSION_PATCH ${HATN_VERSION_PATCH} PARENT_SCOPE)
    SET(HATN_VERSION ${HATN_VERSION} PARENT_SCOPE)

    ADD_CUSTOM_TARGET(dep ALL DEPENDS
        ${HATN_SOURCE_DIR}/VERSION
        ${VERSION_FILE}
    )
    ADD_CUSTOM_COMMAND(OUTPUT ${VERSION_FILE}
      COMMAND ${CMAKE_COMMAND} -DVERSION_FILE=${VERSION_FILE} -DWRITE_HATN_VERSION_HEADER=1 -DHATN_VERSION=${HATN_VERSION} -DHATN_VERSION_MAJOR=${HATN_VERSION_MAJOR} -DHATN_VERSION_MINOR=${HATN_VERSION_MINOR} -DHATN_VERSION_PATCH=${HATN_VERSION_PATCH} -P ${HATN_SOURCE_DIR}/cmake/hatn/GenerateVersion.cmake
      COMMENT STATUS "Generate version.h"
      ${HATN_SOURCE_DIR}/VERSION
    )

ENDFUNCTION(SETUP_HATN_VERSION)

FUNCTION(BUILD_MODULE)
    STRING(REPLACE "hatn" "" NAME ${PROJECT_NAME})
    STRING(TOUPPER ${NAME} NAME)
    CREATE_CONFIG_FILE(${MODULE_NAME} ${MODULE_NAME})    

    IF (${HATN_${MODULE_NAME}_HEADER_ONLY})

        SET (EXPORT_MODE INTERFACE)
        ADD_LIBRARY(${PROJECT_NAME} INTERFACE)

        TARGET_COMPILE_DEFINITIONS(${PROJECT_NAME} INTERFACE -DHATN_${MODULE}_EXPORT=)

    ELSE ()

        SET (EXPORT_MODE PUBLIC)
        ADD_LIBRARY(${PROJECT_NAME} ${LINK_TYPE} ${SOURCES} ${HEADERS})

        TARGET_COMPILE_DEFINITIONS(${PROJECT_NAME} PRIVATE -DBUILD_HATN_${NAME})

        IF (MSVC)
            TARGET_LINK_DIRECTORIES(${PROJECT_NAME} PRIVATE
              ${HATN_BINARY_DIR}/${name}/$<CONFIGURATION>
            )
        ELSE (MSVC)
            TARGET_LINK_DIRECTORIES(${PROJECT_NAME} PRIVATE
              ${HATN_BINARY_DIR}/${name}
            )
        ENDIF (MSVC)

    ENDIF()

    TARGET_INCLUDE_DIRECTORIES(${PROJECT_NAME} ${EXPORT_MODE}
         ${HATN_INCLUDE_DIRECTORIES}
         $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
         $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/thirdparty>
         $<BUILD_INTERFACE:${HATN_BINARY_DIR}/include>
         $<INSTALL_INTERFACE:include>
         $<INSTALL_INTERFACE:include/hatn/thirdparty>
    )

ENDFUNCTION(BUILD_MODULE)

FUNCTION(BUILD_EXE)
    ADD_EXECUTABLE(${PROJECT_NAME} ${SOURCES} ${HEADERS})
ENDFUNCTION(BUILD_EXE)

FUNCTION(INSTALL_MODULE)

    STRING(REGEX REPLACE "^hatn" "" MODULE_NAME ${PROJECT_NAME})
    IF (
        ("${HATN_${MODULE_NAME}_HEADER_ONLY}" STREQUAL "")
        OR
        (NOT ${HATN_${MODULE_NAME}_HEADER_ONLY})
        )
        MESSAGE(STATUS "Install ${MODULE_NAME} library")
        IF (BUILD_STATIC)
            INSTALL(TARGETS ${PROJECT_NAME}
                    ARCHIVE DESTINATION lib
            )
        ELSE (BUILD_STATIC)
            IF (INSTALL_DEV)
                INSTALL(TARGETS ${PROJECT_NAME}
                    RUNTIME DESTINATION bin
                    LIBRARY DESTINATION lib
                    ARCHIVE DESTINATION lib
                )
            ELSE(INSTALL_DEV)
                INSTALL(TARGETS ${PROJECT_NAME}
                    RUNTIME DESTINATION bin
                    LIBRARY DESTINATION lib
                )
            ENDIF(INSTALL_DEV)
        ENDIF(BUILD_STATIC)
    ELSE()
        MESSAGE(STATUS "Skip installing header only ${MODULE_NAME} library")
    ENDIF()

    IF (INSTALL_DEV
            OR (
                (NOT ("${HATN_${MODULE_NAME}_HEADER_ONLY}" STREQUAL ""))
                AND
                (${HATN_${MODULE_NAME}_HEADER_ONLY})
               )
        )
        INSTALL(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/include/hatn DESTINATION include)
        INSTALL(DIRECTORY ${HATN_BINARY_DIR}/include/hatn DESTINATION include)
    ENDIF()

    IF(ENABLE_TRANSLATIONS)
        COMPILE_TRANSLATIONS()
        INSTALL_TRANSLATIONS()
    ENDIF(ENABLE_TRANSLATIONS)

ENDFUNCTION(INSTALL_MODULE)

FUNCTION(INIT_MODULE)
    IF(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/include/hatn/${MODULE_NAME}/headeronly.cfg)
        MESSAGE(STATUS "Exists ${CMAKE_CURRENT_SOURCE_DIR}/include/hatn/${MODULE_NAME}/headeronly.cfg")
        SET(HATN_${MODULE_NAME}_HEADER_ONLY TRUE CACHE BOOL "Header only module ${MODULE_NAME}")
    ELSE ()
        MESSAGE(STATUS "Not exists ${CMAKE_CURRENT_SOURCE_DIR}/include/hatn/${MODULE_NAME}/headeronly.cfg")
        SET(HATN_${MODULE_NAME}_HEADER_ONLY FALSE CACHE BOOL "Header only module ${MODULE_NAME}")        
    ENDIF()
ENDFUNCTION(INIT_MODULE)

FUNCTION(BUILD_HATN_MODULE)
    MESSAGE(STATUS "Build hatn module ${PROJECT_NAME}")
    STRING(REGEX REPLACE "^hatn" "" MODULE_NAME ${PROJECT_NAME})
    UNSET(HATN_MODULE_DEPS)
    READ_HATN_MODULE_DEPS(${MODULE_NAME})
    MESSAGE(STATUS "Module ${PROJECT_NAME} depends on ${HATN_MODULE_DEPS}")
    INIT_MODULE()
    BUILD_MODULE()
    ADD_HATN_MODULES(${MODULE_NAME} ${HATN_MODULE_DEPS})
    INSTALL_MODULE()
	MESSAGE(DEBUG "ADD_HATN_MODULE_PLUGINS(${BUILD_PLUGINS})")
    ADD_HATN_MODULE_PLUGINS(${BUILD_PLUGINS})
ENDFUNCTION(BUILD_HATN_MODULE)

FUNCTION(BUILD_HATN_PLUGIN module_name)
    MESSAGE(STATUS "Build hatn plugin ${PROJECT_NAME}")
    STRING(REPLACE "hatn" "" NAME ${PROJECT_NAME})
    SET(small_name ${NAME})
    STRING(TOUPPER ${NAME} NAME)
    ADD_LIBRARY(${PROJECT_NAME} ${LINK_TYPE} ${SOURCES} ${HEADERS})
    IF(WIN32)
        TARGET_COMPILE_DEFINITIONS(${PROJECT_NAME} PUBLIC -DBUILD_HATN_${NAME})
    ENDIF(WIN32)
    READ_HATN_MODULE_PLUGIN_DEPS(${module_name} ${small_name})
    ADD_HATN_MODULES(${PROJECT_NAME} ${HATN_PLUGIN_DEPS})
    IF(BUILD_STATIC)
        INSTALL(TARGETS ${PROJECT_NAME}
            ARCHIVE DESTINATION lib/hatn/plugins
        )
    ELSE(BUILD_STATIC)
        INSTALL(TARGETS ${PROJECT_NAME}
            RUNTIME DESTINATION lib/hatn/plugins
            LIBRARY DESTINATION lib/hatn/plugins
        )
    ENDIF(BUILD_STATIC)

    IF(ENABLE_TRANSLATIONS)
        COMPILE_TRANSLATIONS()
        IF (EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/translations)
            INSTALL(DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/translations/"
                    DESTINATION share/locale
                    )
        ENDIF()
    ENDIF(ENABLE_TRANSLATIONS)
    IF (INSTALL_DEV AND BUILD_STATIC)
        INSTALL(DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}" DESTINATION include/hatn/plugins/${small_name}
            FILES_MATCHING PATTERN "*.h")
    ENDIF()
    IF(BUILD_STATIC)
        CREATE_CONFIG_FILE(${small_name} ${module_name}/plugins/${small_name})
    ENDIF(BUILD_STATIC)
ENDFUNCTION(BUILD_HATN_PLUGIN)

FUNCTION(COPY_HATN_DYNAMIC name target)
    STRING(TOUPPER ${name} UPPER_NAME)
    COPY(${HATN${UPPER_NAME}_DYNAMIC} ${target})
ENDFUNCTION(COPY_HATN_DYNAMIC)

FUNCTION(COPY_PREBUILD_THIRDPARTY_HEADERS SRC)
    FILE(COPY ${SRC} DESTINATION "${HATN_BINARY_DIR}/include/hatn/thirdparty" FILES_MATCHING PATTERN "*.h*")
ENDFUNCTION(COPY_PREBUILD_THIRDPARTY_HEADERS)

FUNCTION(LINK_STATIC_HATN_PLUGIN TARGET_NAME PLUGIN_NAME)
	MESSAGE(STATUS "LINK_STATIC_HATN_PLUGIN ${TARGET_NAME} ${MODULE}")
	ADD_DEPENDENCIES(${TARGET_NAME} hatn${PLUGIN_NAME})
    IF (BUILD_STATIC)
        TARGET_LINK_LIBRARIES(${TARGET_NAME} PRIVATE hatn${PLUGIN_NAME})
    ENDIF(BUILD_STATIC)
ENDFUNCTION()

FUNCTION(LINK_STATIC_HATN_PLUGINS TARGET_NAME MODULE)
	MESSAGE(STATUS "LINK_STATIC_HATN_PLUGINS ${TARGET_NAME} ${MODULE}")
    STRING (TOLOWER ${MODULE} MODULE_NAME)
    SET (MODULE_PLUGINS_PATH "${HATN_SOURCE_DIR}/${MODULE_NAME}/plugins")
	MESSAGE(STATUS "MODULE_PLUGINS_PATH ${MODULE_PLUGINS_PATH}")
	MESSAGE(STATUS "Check plugins with names: ${BUILD_PLUGINS}")
    FOREACH(plugin_name ${BUILD_PLUGINS})
        IF ("${plugin_name}" STREQUAL "all")
            IF (EXISTS ${MODULE_PLUGINS_PATH})
                SUBDIRLIST(PLUGIN_LIST ${MODULE_PLUGINS_PATH})
                FOREACH(plugin ${PLUGIN_LIST})
                    LINK_STATIC_HATN_PLUGIN(${TARGET_NAME} ${plugin})
                ENDFOREACH()
            ENDIF()
            BREAK()
        ELSE ()
            IF (EXISTS "${MODULE_PLUGINS_PATH}/${plugin_name}")
				LINK_STATIC_HATN_PLUGIN(${TARGET_NAME} ${plugin_name})
            ENDIF()
        ENDIF()
    ENDFOREACH()
ENDFUNCTION()
