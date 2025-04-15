CMAKE_MINIMUM_REQUIRED (VERSION 3.12)
PROJECT(hatn)

# Check if it is a master project

GET_DIRECTORY_PROPERTY(HasParent PARENT_DIRECTORY)
IF(NOT HasParent)
SET(HATN_MASTER_PROJECT True)
ENDIF()

# Include cmake scripts

SET (HATN_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR} CACHE STRING "hatn source dir")
SET (HATN_BINARY_DIR ${CMAKE_CURRENT_BINARY_DIR} CACHE STRING "hatn binary dir")

LIST(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
INCLUDE(hatn/Utils)
INCLUDE(hatn/Config)
INCLUDE(hatn/Module)
INCLUDE(hatn/Build)

INCLUDE(hatn/Libs)

IF ($ENV{HATN_SERVER_LIBS})
    SET(BUILD_SEVER_LIBS $ENV{HATN_SERVER_LIBS})
ENDIF()

IF (BUILD_SEVER_LIBS)
    INCLUDE(hatn/ServerLibs)
ENDIF()

SETUP_HATN_VERSION()

# Init variables for hatn modules without actually building them

INIT_HATN_SRC_PATHS(${HATN_MODULES})

# Setup modules and dependencies

IF ("${DEV_MODULE}" STREQUAL "" OR "${DEV_MODULE}" STREQUAL "all")
    MESSAGE(STATUS "Build all modules")
ELSE()
    STRING (TOUPPER ${DEV_MODULE} UPPER_DEV_MODULE_NAME)
    MESSAGE(STATUS "Build module ${DEV_MODULE} and modules it depends on")
    READ_HATN_MODULE_DEPS(${DEV_MODULE})
    READ_HATN_PLUGINS_DEPS(${DEV_MODULE} ${BUILD_PLUGINS})
    SET(HATN_MODULES ${HATN_MODULE_DEPS} ${DEV_MODULE})
ENDIF()

LIST(REMOVE_DUPLICATES HATN_MODULES)
MESSAGE(STATUS "Building modules ${HATN_MODULES}")
FOREACH(Module ${HATN_MODULES})
    READ_HATN_MODULE_DEPS(${Module})
    READ_HATN_PLUGINS_DEPS(${Module} ${BUILD_PLUGINS})
ENDFOREACH()
LIST(REMOVE_DUPLICATES HATN_MODULES)
IF (HATN_MODULE_DEPS)
    LIST(REMOVE_DUPLICATES HATN_MODULE_DEPS)
ENDIF()

# Build thirdparty libraries
ADD_SUBDIRECTORY(thirdparty)

# Build all requested modules
ADD_HATN_SUBDIRS(${HATN_MODULES})

IF (BUILD_PLUGINS)
	# testplugin must be built either for all libs or for common
	IF("${DEV_MODULE}" STREQUAL "" OR "${DEV_MODULE}" STREQUAL "all" OR "${DEV_MODULE}" STREQUAL "common")
		LIST (FIND BUILD_PLUGINS testplugin FOUND_IDX)
		IF (${FOUND_IDX} EQUAL -1)
			LIST (PREPEND BUILD_PLUGINS testplugin)
		ENDIF()
	ENDIF()
    MESSAGE(STATUS "Try to build plugins with names: ${BUILD_PLUGINS}")
ELSE()
	# testplugin must be built either for all libs or for common
	IF("${DEV_MODULE}" STREQUAL "" OR "${DEV_MODULE}" STREQUAL "all" OR "${DEV_MODULE}" STREQUAL "common")
		SET (BUILD_PLUGINS testplugin)
		MESSAGE(STATUS "Try to build plugins with names: ${BUILD_PLUGINS}")		
	ELSE()
		MESSAGE(STATUS "Not building plugins")
	ENDIF()
ENDIF()

# Build tests only if this is a first level project
IF (HATN_MASTER_PROJECT)
    IF(BUILD_TESTS)
      ADD_SUBDIRECTORY(test)
    ENDIF(BUILD_TESTS)
ENDIF(HATN_MASTER_PROJECT)

INSTALL_HATN_FILES()
