MACRO(FIND_DLL UPPER_NAME name)
    FIND_FILE(HATN${UPPER_NAME}_DYNAMIC
            NAMES hatn${name}.dll hatn${name}.so hatn${name}.dlyb libhatn${name}.dll libhatn${name}.so libhatn${name}.dlyb
                  hatn${name}d.dll hatn${name}d.so hatn${name}d.dlyb libhatn${name}d.dll libhatn${name}d.so libhatn${name}d.dlyb
            PATHS
              ${HATN_${UPPER_NAME}_LIB}
              ${HATN_${UPPER_NAME}_ROOT}/lib
              $ENV{HATN_${UPPER_NAME}_LIB}
              $ENV{HATN_${UPPER_NAME}_ROOT}/lib
    )
ENDMACRO(FIND_DLL)

MACRO(HATN_MODULE name)
    SET(Args ${ARGN})
    LIST(LENGTH Args NumArgs)
    IF(NumArgs GREATER 0)
        SET(self ${ARGV1})
    ENDIF()

    STRING (TOUPPER ${name} UPPER_NAME)

    IF (HATN_${UPPER_NAME}_SRC)
      SET(HATN${UPPER_NAME}_LIBRARY hatn${name})
      SET(HATN${UPPER_NAME}_INCLUDE_DIR ${HATN_${UPPER_NAME}_SRC})
      IF(NOT EXISTS ${HATN_${UPPER_NAME}_SRC}/src)
        MESSAGE(STATUS "Header only module ${name}")
        SET(HATN_${name}_HEADER_ONLY TRUE CACHE BOOL "Header only module ${name}")
      ENDIF()
    ELSE (HATN_${UPPER_NAME}_SRC)
        FIND_PATH(HATN${UPPER_NAME}_INCLUDE_DIR hatn/${name}/${name}.h
                PATHS
                  ${HATN_${UPPER_NAME}_SRC}/include
                  ${HATN_${UPPER_NAME}_INCLUDE}
                  ${HATN_${UPPER_NAME}_ROOT}/include
                  $ENV{HATN_${UPPER_NAME}_INCLUDE}
                  $ENV{HATN_${UPPER_NAME}_ROOT}/include
                DOC "hatn${name} headers directory"
        )
       IF(NOT HATN${UPPER_NAME}_INCLUDE_DIR)
         MESSAGE(FATAL_ERROR "hatn${name} headers not found")
       ENDIF()
       IF(EXISTS HATN${UPPER_NAME}_INCLUDE_DIR/hatn/${name}/headeronly.cfg)
         MESSAGE(STATUS "Header only module ${name}")
         SET(HATN_${name}_HEADER_ONLY TRUE CACHE BOOL "Header only module ${name}")
       ENDIF()
       FIND_LIBRARY(HATN${UPPER_NAME}_LIBRARY hatn${name}
              PATHS
                ${CMAKE_CURRENT_BINARY_DIR}/${name}
                ${HATN_${UPPER_NAME}_LIB}
                ${HATN_${UPPER_NAME}_ROOT}/lib
                $ENV{HATN_${UPPER_NAME}_LIB}
                $ENV{HATN_${UPPER_NAME}_ROOT}/lib
                DOC "hatn${name} library"
      )
      IF(NOT HATN${UPPER_NAME}_LIBRARY)
        MESSAGE(FATAL_ERROR "hatn${name} library not found")
      ENDIF()
    ENDIF (HATN_${UPPER_NAME}_SRC)

    # for DLL copying
    FIND_DLL(${UPPER_NAME} ${name})

    # normalize library name
    STRING(SUBSTRING ${name} 0 1 FIRST_LETTER)
    STRING(TOUPPER ${FIRST_LETTER} FIRST_LETTER)
    STRING(REGEX REPLACE "^.(.*)" "${FIRST_LETTER}\\1" name "${name}")

    # include extra cmake for module if it exists
    SET (ExtraCmake ${HATN_SOURCE_DIR}/cmake/hatn/modules/${name}.cmake)
    IF (NOT ${self})
        IF (EXISTS ${ExtraCmake})
            MESSAGE(STATUS "Including extra cmake module ${ExtraCmake}")
            INCLUDE(${ExtraCmake})
        ENDIF()
    ENDIF(NOT ${self})

    # Print report
    IF (
        ("${HATN_${name}_HEADER_ONLY}" STREQUAL "")
        OR
        (NOT ${HATN_${name}_HEADER_ONLY})
        )    
        MESSAGE(STATUS "hatn ${name} libs: ${HATN${UPPER_NAME}_LIBRARY}")
    ENDIF()
    MESSAGE(STATUS "hatn ${name} headers: ${HATN${UPPER_NAME}_INCLUDE_DIR}")

ENDMACRO(HATN_MODULE)

FUNCTION(LINK_MODULE target export_mode deps)
    TARGET_LINK_DIRECTORIES(${target} ${export_mode} ${HATN_LINK_DIRECTORIES})
    TARGET_LINK_LIBRARIES(${target} ${export_mode}
        ${deps}
        ${CMAKE_THREAD_LIBS_INIT}
        ${Boost_LIBRARIES}
        ${HATN_SYSTEM_LIBS}
    )
ENDFUNCTION(LINK_MODULE)

FUNCTION(INIT_HATN_SRC_PATHS)
    SET(Args ${ARGN})
    LIST(LENGTH Args NumArgs)
    IF(NumArgs GREATER 0)
        FOREACH(ARG ${Args})
            SET(MODULE_NAME ${ARG})
            STRING (TOUPPER ${MODULE_NAME} UPPER_NAME)
            SET (HATN_${UPPER_NAME}_SRC ${HATN_SOURCE_DIR}/${MODULE_NAME} CACHE STRING "")
        ENDFOREACH()
    ENDIF()
ENDFUNCTION(INIT_HATN_SRC_PATHS)

FUNCTION(ADD_HATN_SUBDIRS)
    SET(Args ${ARGN})
    LIST(LENGTH Args NumArgs)
    IF(NumArgs GREATER 0)
        FOREACH(ARG ${Args})
            SET(MODULE_NAME ${ARG})
            STRING (TOUPPER ${MODULE_NAME} UPPER_NAME)
            MESSAGE(STATUS "Entering subdirectory ${HATN_${UPPER_NAME}_SRC}")
            ADD_SUBDIRECTORY(${HATN_${UPPER_NAME}_SRC})
        ENDFOREACH()
    ENDIF()
ENDFUNCTION(ADD_HATN_SUBDIRS)

FUNCTION(ADD_HATN_MODULES target export_mode)
    SET(MODULE_DEPS "")
    SET(Args ${ARGN})
    LIST(LENGTH Args NumArgs)
    IF(NumArgs GREATER 0)
        FOREACH(ARG ${Args})
            SET(MODULE_NAME ${ARG})
            STRING(SUBSTRING ${MODULE_NAME} 0 1 FIRST_LETTER)
            STRING(TOUPPER ${FIRST_LETTER} FIRST_LETTER)
            STRING(REGEX REPLACE "^.(.*)" "${FIRST_LETTER}\\1" MODULE_NAME "${MODULE_NAME}")
            # MESSAGE(STATUS "TARGET_LINK_LIBRARIES(${target} ${EXPORT_MODE} hatn${ARG})")
            TARGET_LINK_LIBRARIES(${target} ${export_mode} hatn${ARG})
            IF (
                ("${HATN_${target}_HEADER_ONLY}" STREQUAL "")
                OR
                (NOT ${HATN_${target}_HEADER_ONLY})
                )
                LIST(APPEND MODULE_DEPS "hatn${ARG}")
            ELSE()
                MESSAGE(STATUS "Skip header-only module ${ARG} while linking")
            ENDIF()
            STRING(TOUPPER ${ARG} MODULE)
        ENDFOREACH()
    ENDIF()
    LIST(LENGTH MODULE_DEPS ModulesCount)
    IF (
        ("${HATN_${target}_HEADER_ONLY}" STREQUAL "")
        OR
        (NOT ${HATN_${target}_HEADER_ONLY})
        )
        # LINK_MODULE(${target} "${MODULE_DEPS}")
        LINK_MODULE(${target} ${export_mode} "${MODULE_DEPS}")
    ENDIF()
	TARGET_INCLUDE_DIRECTORIES(${target} ${export_mode} ${HATN_INCLUDE_DIRECTORIES})	
ENDFUNCTION(ADD_HATN_MODULES)

FUNCTION (READ_HATN_MODULE_DEPS name)
    STRING (TOUPPER ${name} UPPER_MODULE_NAME)
    IF ("${HATN_${UPPER_MODULE_NAME}_SRC}" STREQUAL "")
        MESSAGE(FATAL_ERROR "Unknown module ${name}")
    ENDIF()
    SET(DEPS_CMAKE ${HATN_${UPPER_MODULE_NAME}_SRC}/depends.cmake)
    IF (EXISTS ${DEPS_CMAKE})    
        INCLUDE(${DEPS_CMAKE})
    ENDIF()        
ENDFUNCTION(READ_HATN_MODULE_DEPS)

FUNCTION (READ_HATN_MODULE_PLUGIN_DEPS module_name plugin_name)
    STRING (TOUPPER ${module_name} UPPER_MODULE_NAME)
    SET(PLUGIN_DEPS_CMAKE ${HATN_${UPPER_MODULE_NAME}_SRC}/plugins/${plugin_name}/depends.cmake)
    INCLUDE(${PLUGIN_DEPS_CMAKE})
ENDFUNCTION(READ_HATN_MODULE_PLUGIN_DEPS)

FUNCTION(ADD_HATN_MODULE_PLUGINS)
    SET(Args ${ARGN})
    LIST(LENGTH Args NumArgs)
    IF(NumArgs GREATER 0)
        FOREACH(ARG ${Args})
            SET(plugin_name ${ARG})
            IF ("${plugin_name}" STREQUAL "all")
                IF (EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/plugins)
                    SUBDIRLIST(PLUGIN_LIST ${CMAKE_CURRENT_SOURCE_DIR}/plugins)
                    FOREACH(plugin ${PLUGIN_LIST})
                        ADD_SUBDIRECTORY(${plugin} ${CMAKE_CURRENT_BINARY_DIR}/plugins/${plugin})
                    ENDFOREACH()
                ENDIF()
                BREAK()
            ELSE ()
                IF (EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/plugins/${plugin_name})
                    ADD_SUBDIRECTORY(${CMAKE_CURRENT_SOURCE_DIR}/plugins/${plugin_name} ${CMAKE_CURRENT_BINARY_DIR}/plugins/${plugin_name})
                ENDIF()
            ENDIF()
        ENDFOREACH()
    ENDIF()
ENDFUNCTION(ADD_HATN_MODULE_PLUGINS)

FUNCTION(READ_HATN_PLUGINS_DEPS module_name)
    SET(Args ${ARGN})
    LIST(LENGTH Args NumArgs)
    IF(NumArgs GREATER 0)
        FOREACH(ARG ${Args})
            SET(plugin_name ${ARG})
            IF (NOT "${plugin_name}" STREQUAL "${module_name}")
                IF ("${plugin_name}" STREQUAL "all")
                    IF (EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/plugins)
                        SUBDIRLIST(PLUGIN_LIST ${HATN_SOURCE_DIR}/${module_name}/plugins)
                        FOREACH(plugin ${PLUGIN_LIST})
                            READ_HATN_MODULE_PLUGIN_DEPS(${module_name} ${plugin})
                        ENDFOREACH()
                    ENDIF()
                    BREAK()
                ELSE ()
                    IF (EXISTS ${HATN_SOURCE_DIR}/${module_name}/plugins/${plugin_name})
                        READ_HATN_MODULE_PLUGIN_DEPS(${module_name} ${plugin_name})
                    ENDIF()
                ENDIF()
            ENDIF()
            SET(HATN_MODULE_DEPS ${HATN_MODULE_DEPS} ${HATN_PLUGIN_DEPS} PARENT_SCOPE)
        ENDFOREACH()
    ENDIF()
ENDFUNCTION(READ_HATN_PLUGINS_DEPS)
