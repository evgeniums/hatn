PROJECT(hatncommon)

HATN_MODULE(common TRUE)

IF (HATN_SMARTPOINTERS_STD)
    SET (COMMON_CONFIG_H_TEXT "${COMMON_CONFIG_H_TEXT}\n#ifndef HATN_SMARTPOINTERS_STD \n    #define HATN_SMARTPOINTERS_STD\n#endif")
ENDIF()
IF (NOT HAVE_PMR)
    SET (COMMON_CONFIG_H_TEXT "${COMMON_CONFIG_H_TEXT}\n#ifndef HATN_PMR_BOOST \n    #define HATN_PMR_BOOST\n#endif")
ENDIF()

ADD_SUBDIRECTORY(thirdparty)
INCLUDE(thirdparty/sha1/sha1.cmake)
INCLUDE(thirdparty/crc32/crc32.cmake)
INCLUDE(thirdparty/base64/base64.cmake)
INCLUDE(thirdparty/fameta/fameta.cmake)
INCLUDE(thirdparty/shortallocator/shortallocator.cmake)

IF (NOT HATNCOMMON_PYTHON_SCRIPTS_DIR)
    INCLUDE(hatn/Scripts)
ENDIF (NOT HATNCOMMON_PYTHON_SCRIPTS_DIR)
IF (NOT HATNCOMMON_PYTHON_SCRIPTS_DIR)
    MESSAGE(FATAL_ERROR "HATNCOMMON_PYTHON_SCRIPTS_DIR cannot be empty")
ENDIF(NOT HATNCOMMON_PYTHON_SCRIPTS_DIR)

FILE(GLOB_RECURSE PythonSources "${HATNCOMMON_PYTHON_SCRIPTS_DIR}/*.py")
ADD_CUSTOM_TARGET(python-scripts SOURCES ${PythonSources})

SET (HEADERS
    ${HEADERS}
)

SET (HEADERS
    ${HEADERS}

    include/hatn/common/common.h
    include/hatn/common/ignorewarnings.h
    include/hatn/common/error.h
    include/hatn/common/nativeerror.h
    include/hatn/common/errorcodes.h
    include/hatn/common/commonerrorcodes.h
    include/hatn/common/errorcategory.h
    include/hatn/common/errorstack.h
    include/hatn/common/apierror.h
    include/hatn/common/result.h

    include/hatn/common/logger.h
    include/hatn/common/loggermodule.h
    include/hatn/common/loggermoduleimp.h
    include/hatn/common/options.h
    include/hatn/common/utils.h
    include/hatn/common/keyrange.h
    include/hatn/common/stdwrappers.h
    include/hatn/common/filesystem.h
    include/hatn/common/valueordefault.h
    include/hatn/common/pointerwithinit.h
    include/hatn/common/databuf.h
    include/hatn/common/appversion.h
    include/hatn/common/application.h
    include/hatn/common/classuid.h
    include/hatn/common/parameters.h
    include/hatn/common/translate.h
    include/hatn/common/plugin.h
    include/hatn/common/memorylockeddata.h
    include/hatn/common/bytearray.h
    include/hatn/common/fixedbytearray.h
    include/hatn/common/thread.h
    include/hatn/common/queue.h
    include/hatn/common/mutexqueue.h
    include/hatn/common/mpscqueue.h
    include/hatn/common/locker.h
    include/hatn/common/sharedlocker.h
    include/hatn/common/interface.h
    include/hatn/common/environment.h
    include/hatn/common/threadq.h
    include/hatn/common/threadwithqueue.h
    include/hatn/common/threadpoolwithqueues.h
    include/hatn/common/stream.h
    include/hatn/common/timer.h
    include/hatn/common/asiotimer.h
    include/hatn/common/syncinvoker.h
    include/hatn/common/taskcontext.h

    include/hatn/common/memorypool/pool.h
    include/hatn/common/memorypool/rawblock.h
    include/hatn/common/memorypool/bucket.h
    include/hatn/common/memorypool/poolcachegen.h
    include/hatn/common/memorypool/newdeletepool.h
    include/hatn/common/memorypool/memblocksize.h
    include/hatn/common/memorypool/multibucketpool.h
    include/hatn/common/memorypool/multibucketpool.ipp
    include/hatn/common/memorypool/preallocatedbucket.h
    include/hatn/common/memorypool/preallocatedbucket.ipp

    include/hatn/common/pmr/pmrtypes.h
    include/hatn/common/pmr/poolmemoryresource.h
    include/hatn/common/pmr/allocatorfactory.h
    include/hatn/common/pmr/uniqueptr.h
    include/hatn/common/pmr/withstaticallocator.h
    include/hatn/common/pmr/withstaticallocator.ipp
    include/hatn/common/pmr/singlepoolmemoryresource.h

    include/hatn/common/format.h
    include/hatn/common/threadcategory.h
    include/hatn/common/threadcategoriespool.h    
    include/hatn/common/multithreadenvironment.h
    include/hatn/common/builder.h
    include/hatn/common/factory.h

    include/hatn/common/managedobject.h
    include/hatn/common/sharedptr.h
    include/hatn/common/weakptr.h
    include/hatn/common/makeshared.h

    include/hatn/common/pointers/mempool/managedobject.h
    include/hatn/common/pointers/mempool/sharedptr.h
    include/hatn/common/pointers/mempool/weakptr.h
    include/hatn/common/pointers/mempool/weakctrl.h
    include/hatn/common/pointers/mempool/weakpool.h
    include/hatn/common/pointers/mempool/makeshared.h

    include/hatn/common/pointers/std/managedobject.h
    include/hatn/common/pointers/std/sharedptr.h
    include/hatn/common/pointers/std/weakptr.h
    include/hatn/common/pointers/std/makeshared.h

    include/hatn/common/elapsedtimer.h
    include/hatn/common/objectid.h

    include/hatn/common/fileutils.h
    include/hatn/common/nativehandler.h

    include/hatn/common/featureset.h
    include/hatn/common/containerutils.h
    include/hatn/common/objecttraits.h

    include/hatn/common/spanbuffer.h
    include/hatn/common/file.h
    include/hatn/common/plainfile.h

    include/hatn/common/cachelru.h
    include/hatn/common/flatmap.h

    include/hatn/common/singleton.h
    include/hatn/common/runonscopeexit.h

    include/hatn/common/date.h
    include/hatn/common/time.h
    include/hatn/common/datetime.h
    include/hatn/common/daterange.h

    include/hatn/common/meta/consttraits.h
    include/hatn/common/meta/pointertraits.h
    include/hatn/common/meta/movereference.h
    include/hatn/common/meta/interfacespack.h
    include/hatn/common/meta/cstrlength.h
    include/hatn/common/meta/constructwithargordefault.h
    include/hatn/common/meta/compilecounter.h
    include/hatn/common/meta/enumint.h
    include/hatn/common/meta/dynamiccastwithsample.h
    include/hatn/common/meta/tupletypec.h
    include/hatn/common/meta/decaytuple.h
    include/hatn/common/meta/replicatetotuple.h
    include/hatn/common/meta/errorpredicate.h
    include/hatn/common/meta/foreachif.h

    include/hatn/common/allocatoronstack.h

    include/hatn/common/crc32.h
    include/hatn/common/objectguard.h

    include/hatn/common/experimental/stats/stats.h
)

SET (HEADERS
    ${HEADERS}
    include/hatn/common/ipp/error.ipp
    include/hatn/common/ipp/threadq.ipp
    include/hatn/common/ipp/threadwithqueue.ipp
    include/hatn/common/ipp/threadcategoriespool.ipp
)

SET (HEADERS
    ${HEADERS}
    include/hatn/test/interfacetestdll.h
)

SET (SOURCES
    ${SOURCES}
    src/error.cpp
    src/logger.cpp
    src/options.cpp
    src/utils.cpp
    src/appversion.cpp
    src/application.cpp
    src/parameters.cpp
    src/translate.cpp
    src/plugin.cpp
    src/memorylockeddata.cpp
    src/classuid.cpp
    src/bytearray.cpp
    src/locker.cpp
    src/sharedlocker.cpp
    src/thread.cpp
    src/interface.cpp
    src/environment.cpp
    src/queue.cpp
    src/threadwithqueue.cpp
    src/threadpoolwithqueues.cpp
    src/timer.cpp
    src/asiotimer.cpp
    src/format.cpp
    src/stream.cpp

    src/memorypool/pool.cpp
    src/memorypool/chunk.cpp
    src/memorypool/bucket.cpp
    src/memorypool/poolcachegen.cpp
    src/memorypool/newdeletepool.cpp
    src/memorypool/memblocksize.cpp

    src/threadcategory.cpp
    src/multithreadenvironment.cpp
    src/factory.cpp

    src/pointers/mempool/managedobject.cpp
    src/pointers/mempool/weakctrl.cpp
    src/pointers/mempool/weakpool.cpp
    src/pointers/mempool/makeshared.cpp

    src/pointers/std/managedobject.cpp

    src/pmr/poolmemoryresource.cpp
    src/pmr/allocatorfactory.cpp

    src/elapsedtimer.cpp

    src/plainfile.cpp

    src/singleton.cpp
    src/datetime.cpp

    src/taskcontext.cpp
    src/objectguard.cpp
)

SET (SOURCES
    ${SOURCES}
    include/hatn/test/interfacetestdll.cpp
)

BUILD_MODULE()
LINK_MODULE(${PROJECT_NAME} PUBLIC "")
ADD_HATN_MODULE_PLUGINS(testplugin)

TARGET_INCLUDE_DIRECTORIES(${PROJECT_NAME} PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
    $<BUILD_INTERFACE:${HATN_BINARY_DIR}>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src>
)

# disable stacktrace on Apple platforms
IF (APPLE)
    TARGET_COMPILE_DEFINITIONS(${PROJECT_NAME} PUBLIC -DBOOST_STACKTRACE_USE_NOOP)
ENDIF(APPLE)

INSTALL_MODULE()

# @todo Fix fmt linking
INCLUDE(hatn/deps/fmt)
# IF (NOT FMT_HEADER_ONLY)
#     TARGET_LINK_LIBRARIES(${PROJECT_NAME} PUBLIC fmt)
# ELSE()
    TARGET_LINK_LIBRARIES(${PROJECT_NAME} PUBLIC fmt-header-only)
# ENDIF(NOT FMT_HEADER_ONLY)
IF (NOT SYSTEM_FMTLIB)
    TARGET_INCLUDE_DIRECTORIES(
        ${PROJECT_NAME} INTERFACE
            $<BUILD_INTERFACE:${HATN_SOURCE_DIR}/thirdparty/fmt/include>
    )
ENDIF()

IF (INSTALL_DEV)
    INCLUDE(hatn/Install)
ENDIF (INSTALL_DEV)
