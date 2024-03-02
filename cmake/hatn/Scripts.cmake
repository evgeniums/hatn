IF (HATN_COMMON_SRC)
SET(HATNCOMMON_SCRIPTS_DIR "${HATN_COMMON_SRC}/../scripts" CACHE PATH "Path to scripts")
ELSE(HATN_COMMON_SRC)
FIND_PATH(HATNCOMMON_SCRIPTS_DIR hatncommon.scripts
        PATHS          
          ${HATN_COMMON_SCRIPTS}
          ${HATN_COMMON_ROOT}/../scripts
          ${HATN_COMMON_ROOT}/lib/hatn/scripts
          $ENV{HATN_COMMON_SCRIPTS}
          $ENV{HATN_COMMON_ROOT}/../scripts
          $ENV{HATN_COMMON_ROOT}/lib/hatn/scripts
        DOC "hatncommon scripts folder"
)
ENDIF(HATN_COMMON_SRC)
SET(HATNCOMMON_PYTHON_SCRIPTS_DIR "${HATNCOMMON_SCRIPTS_DIR}/python" CACHE PATH "Path to python scripts")
MESSAGE(STATUS "hatn common scripts folder: ${HATNCOMMON_SCRIPTS_DIR}")
MESSAGE(STATUS "HATNCOMMON_PYTHON_SCRIPTS_DIR: ${HATNCOMMON_PYTHON_SCRIPTS_DIR}")
