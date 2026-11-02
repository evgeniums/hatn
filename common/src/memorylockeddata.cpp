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

#include <atomic>

#include <hatn/common/memorylockeddata.h>
#include <hatn/common/logger.h>
#include <hatn/common/fileutils.h>

#include <hatn/common/loggermoduleimp.h>

INIT_LOG_MODULE(mlockdata,HATN_COMMON_EXPORT)

HATN_COMMON_NAMESPACE_BEGIN

std::map<MemoryLocker::pagenum_t, unsigned long> MemoryLocker::lockedCounter;
std::mutex MemoryLocker::mutex;

#ifdef HATN_MEMORY_LOCK_BEST_EFFORT
/*
 * Locking is attempted on every allocation of memory locked data, so on a host that refuses to
 * lock the failure would be reported for every resize. Report it once per process instead.
 */
static std::atomic<bool> LockFailureReported{false};
#endif

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
void MemoryLocker::unlockPages(pagenum_t first, pagenum_t endExclusive)
{
    for (pagenum_t page = first; page < endExclusive; page++)
    {
        auto it = lockedCounter.find(page);
        if (it == lockedCounter.end())
        {
            /*
             * The page was never counted, so it is not locked - see lockRegion(). Decrementing a
             * missing entry here used to wrap the counter around and leave the map growing.
             */
            continue;
        }
        if (--(it->second) == 0)
        {
            lockedCounter.erase(it);
            doUnlockRegion(pagenumFirstByte(page), pageSize);
        }
    }
}

//---------------------------------------------------------------
void MemoryLocker::lockRegion(void *p, size_t n)
{
    if (n == 0)
        return;

    std::unique_lock<std::mutex> lock(mutex);

    pagenum_t first = addr2pagenum(p);
    pagenum_t last = addr2pagenum((char*)p + n - 1);
    pagenum_t page = first;

    /*
     * A page is counted only once it is actually locked, so that the counter never claims pages
     * the OS refused to lock and unlockPages() never unlocks a page that was not locked.
     *
     * With best effort locking the counter is no longer an exact refcount of the buffers sharing
     * a page: if locking a page fails for one buffer and later succeeds for another on the same
     * page, releasing the first buffer unlocks the page while the second still holds it. Counts
     * stay balanced, the page is simply unlocked earlier than it could be - acceptable in a mode
     * that has already given up the guarantee. Exact tracking would have to be per allocation.
     */
    try
    {
        for (; page <= last; page++)
        {
            auto it = lockedCounter.find(page);
            if (it != lockedCounter.end())
            {
                ++(it->second);
                continue;
            }
            if (doLockRegion(pagenumFirstByte(page), pageSize))
            {
                lockedCounter.emplace(page, 1);
            }
        }
    }
    catch (...)
    {
        // page itself was not counted, so roll back the half open range before it
        unlockPages(first, page);
        throw;
    }
}

//---------------------------------------------------------------
void MemoryLocker::unlockRegion(void *p, size_t n)
{
    // without this guard (char*)p + n - 1 would wrap to the previous page
    if (n == 0)
        return;

    std::unique_lock<std::mutex> lock(mutex);

    unlockPages(addr2pagenum(p), addr2pagenum((char*)p + n - 1) + 1);
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
bool MemoryLocker::doLockRegion(void *p, size_t n)
{
    if (VirtualLock(p, n))
        return true;

    DWORD e = GetLastError();
#ifdef HATN_MEMORY_LOCK_BEST_EFFORT
    if (!LockFailureReported.exchange(true, std::memory_order_relaxed))
    {
        HATN_WARN(mlockdata, "VirtualLock() failed: LastError = " << e << " (" << FormatError(e)
                             << "), secret data will be kept in unlocked memory");
    }
    return false;
#else
    std::stringstream ss;
    ss << "VirtualLock() failed: LastError = " << e << " (" << FormatError(e) << ")";
    throw std::runtime_error(ss.str());
#endif
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
     * Show a warning here, leaving the memory region locked. The page stays counted against
     * the process limit, so later lock requests may fail - either by throwing, or, with best
     * effort locking, by falling back to unlocked memory.
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
bool MemoryLocker::doLockRegion(void *p, size_t n)
{
    if (mlock(p, n) == 0)
        return true;

    int e = errno;
#ifdef HATN_MEMORY_LOCK_BEST_EFFORT
    if (!LockFailureReported.exchange(true, std::memory_order_relaxed))
    {
        HATN_WARN(mlockdata, "mlock() failed: errno = " << e << " (" << strerror(e)
                             << "), secret data will be kept in unlocked memory");
    }
    return false;
#else
    std::stringstream ss;
    ss << "mlock() failed: errno = " << e << " (" << strerror(e) << ")";
    throw std::runtime_error(ss.str());
#endif
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
     * Show a warning here, leaving the memory region locked. The page stays counted against
     * the process limit, so later lock requests may fail - either by throwing, or, with best
     * effort locking, by falling back to unlocked memory.
     */
    HATN_ERROR(mlockdata,"munlock() failed: errno = " << e << " (" << strerror(e) << ")");
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
