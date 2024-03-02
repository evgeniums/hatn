# Copy file A to file B post build
FUNCTION(COPY A B)
    ADD_CUSTOM_COMMAND(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "${A}" "${B}")
ENDFUNCTION(COPY)

# Copy file A to file B post build
FUNCTION(COPY_DIRECTORY A B)
    ADD_CUSTOM_COMMAND(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
                "${A}" "${B}")
ENDFUNCTION(COPY_DIRECTORY)

# Copy from current source directory to binary directory with dll/exe
FUNCTION(COPY_SOURCE_FILE SOURCE DEST)
    COPY(${CMAKE_CURRENT_SOURCE_DIR}/${SOURCE}
         ${BINDIR}/${DEST})
ENDFUNCTION(COPY_SOURCE_FILE)

# Copy from current source directory to binary directory with dll/exe
# destname is the same as the original filename
FUNCTION(COPY_SOURCE_FILE1 SOURCE)
    COPY_SOURCE_FILE("${SOURCE}" "${SOURCE}")
ENDFUNCTION(COPY_SOURCE_FILE1)

FUNCTION(CONSTRUCT_LIBNAMES LIBNAME SOURCE_DIR DEST_DIR)
    IF(WIN32)
        IF(MSVC)
            SET (SRC_LIB ${HATN_BINARY_DIR}/${SOURCE_DIR}/$<CONFIGURATION>/${LIBNAME}.dll PARENT_SCOPE)
            SET (DST_LIB ${CMAKE_CURRENT_BINARY_DIR}/$<CONFIGURATION>/${DEST_DIR}/${LIBNAME}.dll PARENT_SCOPE)
        ELSE(MSVC)
            SET (SRC_LIB ${HATN_BINARY_DIR}/${SOURCE_DIR}/lib${LIBNAME}.dll PARENT_SCOPE)
            SET (DST_LIB ${CMAKE_CURRENT_BINARY_DIR}/${DEST_DIR}/lib${LIBNAME}.dll PARENT_SCOPE)
        ENDIF(MSVC)
    ELSE(WIN32)
        IF (APPLE)
            SET (SRC_LIB ${HATN_BINARY_DIR}/${SOURCE_DIR}/lib${LIBNAME}.dylib PARENT_SCOPE)
            SET (DST_LIB ${CMAKE_CURRENT_BINARY_DIR}/${DEST_DIR}/lib${LIBNAME}.dylib PARENT_SCOPE)
        ELSE (APPLE)
            SET (SRC_LIB ${HATN_BINARY_DIR}/${SOURCE_DIR}/lib${LIBNAME}.so PARENT_SCOPE)
            SET (DST_LIB ${CMAKE_CURRENT_BINARY_DIR}/${DEST_DIR}/lib${LIBNAME}.so PARENT_SCOPE)
        ENDIF(APPLE)
    ENDIF(WIN32)
ENDFUNCTION(CONSTRUCT_LIBNAMES)

# Copy library from binary directory (relative to current binary directory)
# to the current binary directory
FUNCTION(COPY_LIBRARY LIBNAME SOURCE_DIR DEST_DIR)
    IF (NOT BUILD_STATIC)
    IF(WIN32)
        IF(MSVC)
            COPY(${CMAKE_CURRENT_BINARY_DIR}/${SOURCE_DIR}/$<CONFIGURATION>/${LIBNAME}.dll
                 ${CMAKE_CURRENT_BINARY_DIR}/$<CONFIGURATION>/${DEST_DIR}/${LIBNAME}.dll)
        ELSE(MSVC)
            COPY(${CMAKE_CURRENT_BINARY_DIR}/${SOURCE_DIR}/lib${LIBNAME}.dll
                 ${CMAKE_CURRENT_BINARY_DIR}/${DEST_DIR}/lib${LIBNAME}.dll)
        ENDIF(MSVC)
    ELSE(WIN32)
        IF (APPLE)
            COPY(${CMAKE_CURRENT_BINARY_DIR}/${SOURCE_DIR}/lib${LIBNAME}.dylib
                ${CMAKE_CURRENT_BINARY_DIR}/${DEST_DIR}/lib${LIBNAME}.dylib)
        ELSE (APPLE)
            COPY(${CMAKE_CURRENT_BINARY_DIR}/${SOURCE_DIR}/lib${LIBNAME}.so
                ${CMAKE_CURRENT_BINARY_DIR}/${DEST_DIR}/lib${LIBNAME}.so)
        ENDIF(APPLE)
    ENDIF(WIN32)
    ENDIF()
ENDFUNCTION(COPY_LIBRARY)

FUNCTION(COPY_LIBRARY_HERE LIBNAME SOURCE_DIR)
    COPY_LIBRARY("${LIBNAME}" "${SOURCE_DIR}" "")
ENDFUNCTION(COPY_LIBRARY_HERE)

# Copy any file to the current binary directory
FUNCTION(COPY_PATH_BINDIR SOURCE_DIR DEST_DIR)
    IF(MSVC)
        SET(DST ${CMAKE_CURRENT_BINARY_DIR}/${BUILD_TYPE_CONFIGURATION}/${DEST_DIR})
    ELSE(MSVC)
        SET(DST ${CMAKE_CURRENT_BINARY_DIR}/${DEST_DIR})
    ENDIF(MSVC)
    IF (NOT EXISTS ${DST})
        FILE(MAKE_DIRECTORY ${DST})
    ENDIF()
    COPY_DIRECTORY(${SOURCE_DIR} ${DST})
ENDFUNCTION(COPY_PATH_BINDIR)

# Copy any file from build directory of a library (relative to current binary directory)
# to the current binary directory
FUNCTION(COPY_FILE_BINDIR FILENAME SOURCE_DIR DEST_DIR)
    IF(MSVC)
        SET(DST ${CMAKE_CURRENT_BINARY_DIR}/$<CONFIGURATION>/${DEST_DIR}/${FILENAME})
    ELSE(MSVC)
        SET(DST ${CMAKE_CURRENT_BINARY_DIR}/${DEST_DIR}/${FILENAME})
    ENDIF(MSVC)
    COPY(${CMAKE_CURRENT_BINARY_DIR}/${SOURCE_DIR}/${FILENAME} ${DST})
ENDFUNCTION(COPY_FILE_BINDIR)

# Get list of subdirectories
MACRO(SUBDIRLIST result curdir)
  FILE(GLOB children RELATIVE ${curdir} ${curdir}/*)
  SET(dirlist "")
  FOREACH(child ${children})
    IF(IS_DIRECTORY ${curdir}/${child})
      LIST(APPEND dirlist ${child})
    ENDIF()
  ENDFOREACH()
  SET(${result} ${dirlist})
ENDMACRO()
