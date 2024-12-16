/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.
    
    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)
      
*/

/****************************************************************************/
/** @file common/plainfile.cpp
 *
 *     Class for writing and reading plain files
 *
 */

#ifdef _WIN32
#include <windows.h>
#include <fileapi.h>
#else
#include <fcntl.h>
#include <unistd.h>
#endif

#include <hatn/common/filesystem.h>

#include <hatn/common/plainfile.h>

HATN_COMMON_NAMESPACE_BEGIN

/********************** PlainFile **************************/

//---------------------------------------------------------------
PlainFile::~PlainFile()
{
    try
    {
        doClose();
    }
    catch (const ErrorException&)
    {
    }
}

//---------------------------------------------------------------
Error PlainFile::open(const char *filename, Mode mode)
{
    if (m_file.is_open())
    {
        return commonError(CommonError::FILE_ALREADY_OPEN);
    }
    boost::system::error_code ec;
    m_file.open(filename,mode,ec);
    return makeBoostError(ec);
}

//---------------------------------------------------------------
bool PlainFile::isOpen() const noexcept
{
    return m_file.is_open();
}

//---------------------------------------------------------------
Error PlainFile::flush() noexcept
{
    if (m_file.is_open())
    {
#if BOOST_BEAST_USE_WIN32_FILE
        if (!FlushFileBuffers(m_file.native_handle())) // WinApi flush
#elif BOOST_BEAST_USE_POSIX_FILE
        if (false) // no flush needed for posix write()
#else
        if (fflush(m_file.native_handle())!=0) // stdio flush
#endif
        {
            return commonError(CommonError::FILE_FLUSH_FAILED);
        }
    }
    return Error();
}

//---------------------------------------------------------------
void PlainFile::close()
{
    doClose();
}

//---------------------------------------------------------------
void PlainFile::doClose()
{
    boost::system::error_code ec;
    m_file.close(ec);
    if (ec)
    {
        throw ErrorException(makeBoostError(ec));
    }
}

//---------------------------------------------------------------
Error PlainFile::seek(uint64_t pos)
{
    boost::system::error_code ec;
    m_file.seek(pos,ec);
    return makeBoostError(ec);
}

//---------------------------------------------------------------
uint64_t PlainFile::pos() const
{
    boost::system::error_code ec;
    auto ret=const_cast<PlainFile*>(this)->m_file.pos(ec);
    if (ec)
    {
        throw ErrorException(makeBoostError(ec));
    }
    return ret;
}

//---------------------------------------------------------------
uint64_t PlainFile::size() const
{
    if (!m_file.is_open())
    {
        return fileSize();
    }
    boost::system::error_code ec;
    auto size=m_file.size(ec);
    if (ec)
    {
        throw ErrorException(makeBoostError(ec));
    }
    return size;
}

//---------------------------------------------------------------
uint64_t PlainFile::size(Error &ec) const
{
    if (!m_file.is_open())
    {
        return fileSize();
    }
    boost::system::error_code e;
    auto size=m_file.size(e);
    if (e)
    {
        ec=makeBoostError(e);
    }
    return size;
}

//---------------------------------------------------------------
uint64_t PlainFile::fileSize(Error& ec) const noexcept
{
    lib::fs_error_code e;
    auto size=lib::filesystem::file_size(filename(),e);
    if (e)
    {
        ec=makeSystemError(e);
    }
    return size;
}

//---------------------------------------------------------------
uint64_t PlainFile::fileSize() const
{
    Error ec;
    auto size=fileSize(ec);
    if (ec)
    {
        throw ErrorException(ec);
    }
    return size;
}

//---------------------------------------------------------------
size_t PlainFile::write(const char *data, size_t size)
{
    if (size==0)
    {
        return 0;
    }
    boost::system::error_code ec;
    auto ret=m_file.write(data,size,ec);
    if (ec)
    {
        throw ErrorException(makeBoostError(ec));
    }
    return ret;
}

//---------------------------------------------------------------
size_t PlainFile::read(char *data, size_t maxSize)
{
    if (maxSize==0)
    {
        return 0;
    }
    boost::system::error_code ec;
    auto ret=m_file.read(data,maxSize,ec);
    if (ec)
    {
        throw ErrorException(makeBoostError(ec));
    }
    return ret;
}

//---------------------------------------------------------------
Error PlainFile::truncate(size_t size, bool /*backupCopy*/)
{

    //! @todo Use native errors

#if BOOST_BEAST_USE_WIN32_FILE

    FILE_END_OF_FILE_INFO endOfFile;
    endOfFile.EndOfFile.QuadPart = size;

    if (!SetFileInformationByHandle(m_file.native_handle(), FileEndOfFileInfo, &endOfFile, sizeof(FILE_END_OF_FILE_INFO)))
    {
        return CommonError::FILE_TRUNCATE_FAILED;
    }

#elif BOOST_BEAST_USE_POSIX_FILE

    int r = ftruncate(m_file.native_handle(), size);
    if (r < 0)
    {
        return CommonError::FILE_TRUNCATE_FAILED;
    }

#endif

    return CommonError::NOT_IMPLEMENTED;
}

//---------------------------------------------------------------
Error PlainFile::sync() noexcept
{
    //! @todo Use native errors
    if (m_file.is_open())
    {
#if BOOST_BEAST_USE_WIN32_FILE
        if (!FlushFileBuffers(m_file.native_handle()))
        {
            return commonError(CommonError::FILE_SYNC_FAILED);
        }
#elif BOOST_BEAST_USE_POSIX_FILE

#ifdef HATN_HAVE_FULLFSYNC
        if (::fcntl(m_file.native_handle(), F_FULLFSYNC) < 0)
        {
            return commonError(CommonError::FILE_SYNC_FAILED);
        }
#else
        if (fdatasync(m_file.native_handle()) < 0)
        {
            return commonError(CommonError::FILE_SYNC_FAILED);
        }
#endif  // HAVE_FULLFSYNC

#endif
    }
    return OK;
}

//---------------------------------------------------------------
Error PlainFile::fsync() noexcept
{
    //! @todo Use native errors
    if (m_file.is_open())
    {
#if BOOST_BEAST_USE_WIN32_FILE
        if (!FlushFileBuffers(m_file.native_handle()))
        {
            return commonError(CommonError::FILE_FSYNC_FAILED);
        }
#elif BOOST_BEAST_USE_POSIX_FILE

#ifdef HATN_HAVE_FULLFSYNC
        if (::fcntl(m_file.native_handle(), F_FULLFSYNC) < 0)
        {
            return commonError(CommonError::FILE_FSYNC_FAILED);
        }
#else
        if (fsync(m_file.native_handle()) < 0)
        {
            return commonError(CommonError::FILE_FSYNC_FAILED);
        }
#endif  // HAVE_FULLFSYNC

#endif
    }
    return OK;
}

//---------------------------------------------------------------
HATN_COMMON_NAMESPACE_END
