IF (GENERATE_VERSION)

    ADD_CUSTOM_TARGET(dep ALL DEPENDS
        ${PROJECT_BINARY_DIR}/hatn_version.h
        ${PROJECT_SOURCE_DIR}/hatn_version.h.in
    )

    ADD_DEFINITIONS(-DDCS_GENERATE_VERSION)
    ADD_CUSTOM_COMMAND(OUTPUT ${PROJECT_BINARY_DIR}/hatn_version.h
      COMMAND ${PYTHON_EXE} ${HATNCOMMON_PYTHON_SCRIPTS_DIR}/makeversion.py ${PROJECT_BINARY_DIR}/df_version.h ${PROJECT_SOURCE_DIR} ${BUILD_VERSION}
      WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
      COMMENT STATUS "Generating version"
      DEPENDS ${PROJECT_SOURCE_DIR}/hatn_version.h.in
    )
    IF (WIN32)
      ADD_CUSTOM_COMMAND(TARGET dep
                POST_BUILD
                COMMAND copy /b hatn_version.h.in +,,
                WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
      )
    ELSE (WIN32)
        ADD_CUSTOM_COMMAND(TARGET dep
            POST_BUILD
            COMMAND touch hatn_version.h.in
            WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
        )
    ENDIF (WIN32)
ENDIF (GENERATE_VERSION)

IF(WRITE_HATN_VERSION_HEADER)
FILE(WRITE ${VERSION_FILE}
"/***************************************************/\n\
/*****This file is auto-generated. Do not edit.*****/\n\
/***************************************************/\n\
\n\
#ifndef HATN_VERSION\n\
\n\
//! @internal\n\
//! Transforms a (version, revision, patchlevel) triple into a number of the\n\
//! form 0xVVRRPPPP to allow comparing versions in a normalized way.\n\
//!\n\
//! See http://sourceforge.net/p/predef/wiki/VersionNormalization.\n\
#define HATN_CONFIG_VERSION(version, revision, patch) (((version) << 24) + ((revision) << 16) + (patch))\n\
\n\
#define HATN_VERSION \"${HATN_VERSION}\" \n\
#define HATN_VERSION_MAJOR ${HATN_VERSION_MAJOR} \n\
#define HATN_VERSION_MINOR ${HATN_VERSION_MINOR} \n\
#define HATN_VERSION_PATCH ${HATN_VERSION_PATCH} \n\
\n\
//! @ingroup group-config \n\
//! Macro expanding to the full version of the library, in hexadecimal \n\
//! representation. \n\
#define HATN_VALIDATOR_VERSION HATN_VALIDATOR_CONFIG_VERSION(HATN_VERSION_MAJOR,HATN_MINOR_VERSION,HATN_PATCH_VERSION)\n\
#endif
"
)
ENDIF(WRITE_HATN_VERSION_HEADER)
