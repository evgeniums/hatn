/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.
    
    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)
      
*/
/****************************************************************************/
/** @file common/mlockdata.cpp
 *
 *     Types for secure data containers.
 *
 */
/****************************************************************************/

#include <hatn/common/memorylockeddata.h>
#include <hatn/common/logger.h>
#include <hatn/common/fileutils.h>

#include <hatn/common/loggermoduleimp.h>

INIT_LOG_MODULE(mlockdata,HATN_COMMON_EXPORT)

HATN_COMMON_NAMESPACE_BEGIN

std::map<MemoryLocker::pagenum_t, unsigned long> MemoryLocker::lockedCounter;
std::mutex MemoryLocker::mutex;

/********************** MemoryLocker **********************************/

//---------------------------------------------------------------
MemoryLocker::pagenum_t MemoryLocker::addr2pagenum(void *p)
{
    return reinterpret_cast<pagenum_t>(p) / pageSize;
}

//---------------------------------------------------------------
void* MemoryLocker::pagenumFirstByte(pagenum_t page)
{
    return reinterpret_cast<void*>(page * pageSize);
}

//---------------------------------------------------------------
void MemoryLocker::lockRegion(void *p, size_t n)
{
    if (n == 0)
        return;

    std::unique_lock<std::mutex> lock(mutex);

    pagenum_t first = addr2pagenum(p);
    pagenum_t last = addr2pagenum((char*)p + n - 1);
    pagenum_t page;

    try {
        for (page = first; page <= last; page++) {
            if (lockedCounter[page]++ == 0)
                doLockRegion(pagenumFirstByte(page), pageSize);
        }
    }
    catch (...) {
        for (page--; page >= first; page--) {
            if (--lockedCounter[page] == 0)
                doUnlockRegion(pagenumFirstByte(page), pageSize);
        }
        throw;
    }
}

//---------------------------------------------------------------
void MemoryLocker::unlockRegion(void *p, size_t n)
{
    std::unique_lock<std::mutex> lock(mutex);

    pagenum_t first = addr2pagenum(p);
    pagenum_t last = addr2pagenum((char*)p + n - 1);

    for (pagenum_t page = first; page <= last; page++) {
        if (--lockedCounter[page] == 0)
            doUnlockRegion(pagenumFirstByte(page), pageSize);
    }
}

#ifdef _WIN32
#include <windows.h>

//---------------------------------------------------------------
static size_t getPageSize()
{
    SYSTEM_INFO systemInfo;
    GetSystemInfo(&systemInfo);
    return systemInfo.dwPageSize;
}

const size_t MemoryLocker::pageSize = getPageSize();

//---------------------------------------------------------------
static std::string FormatError(DWORD error)
{
    LPVOID lpMsgBuf;

    FormatMessage(
        FORMAT_MESSAGE_ALLOCATE_BUFFER | 
        FORMAT_MESSAGE_FROM_SYSTEM |
        FORMAT_MESSAGE_IGNORE_INSERTS,
        NULL,
        error,
        MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US),
        (LPSTR) &lpMsgBuf,
        0, NULL);
    
    std::string s = (char*)lpMsgBuf;
    LocalFree(lpMsgBuf);
    return s;
}

//---------------------------------------------------------------
void MemoryLocker::doLockRegion(void *p, size_t n)
{
    if (VirtualLock(p, n))
        return;

    DWORD e = GetLastError();
    std::stringstream ss;
    ss << "VirtualLock() failed: LastError = " << e << " (" << FormatError(e) << ")";
    throw std::runtime_error(ss.str());
}

//---------------------------------------------------------------
void MemoryLocker::doUnlockRegion(void *p, size_t n)
{
    if (VirtualUnlock(p, n))
        return;

    DWORD e = GetLastError();

    /*
     * We cannot throw an exception from here as doUnlockRegion()
     * might be called from dtr.  An unhandled exception in dtr is UB.
     *
     * Show a warning here, leaving the memory region locked.
     * If we're out of locked memory due to the leak, doLockRegion() will fail.
     */
    HATN_ERROR(mlockdata, "VirtualUnlock() failed: LastError = " << e << " (" << FormatError(e) << ")");
}

#else // _WIN32

#include <sys/mman.h>
#include <unistd.h>

/*
 * From mlock(2):
 * Portable applications should employ sysconf(_SC_PAGESIZE) instead of getpagesize()
 */
const size_t MemoryLocker::pageSize = sysconf(_SC_PAGESIZE);

//---------------------------------------------------------------
void MemoryLocker::doLockRegion(void *p, size_t n)
{
    if (mlock(p, n) == 0)
        return;

    int e = errno;
    std::stringstream ss;
    ss << "mlock() failed: errno = " << e << " (" << strerror(e) << ")";
    throw std::runtime_error(ss.str());
}

//---------------------------------------------------------------
void MemoryLocker::doUnlockRegion(void *p, size_t n)
{
    if (munlock(p, n) == 0)
        return;

    int e = errno;

    /*
     * We cannot throw an exception from here as doUnlockRegion()
     * might be called from dtr.  An unhandled exception in dtr is UB.
     *
     * Show a warning here, leaving the memory region locked.
     * If we're out of locked memory due to the leak, doLockRegion() will fail.
     */
    HATN_ERROR(mlockdata,"mlock() failed: errno = " << e << " (" << strerror(e) << ")");
}
#endif // WIN32

/********************** MemoryLockedArray **********************************/

//---------------------------------------------------------------
Error MemoryLockedArray::loadFromFile(const char *fileName)
{
    return FileUtils::loadFromFile(d,fileName);
}

//---------------------------------------------------------------
Error MemoryLockedArray::saveToFile(const char *fileName) const noexcept
{
    return FileUtils::saveToFile(*this,fileName);
}

//---------------------------------------------------------------
HATN_COMMON_NAMESPACE_END
