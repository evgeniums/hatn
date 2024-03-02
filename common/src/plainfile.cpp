/*
   Copyright (c) 2020 - current, Evgeny Sidorov (dracosha.com), All rights reserved.
    
    
  */

/****************************************************************************/
/*
    
*/
/** @file common/plainfile.cpp
 *
 *     Class for writing and reading plain files
 *
 */

#ifdef _MSC_VER
#include <windows.h>
#include <fileapi.h>
#endif

#include <boost/filesystem.hpp>

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
        return Error(CommonError::FILE_ALREADY_OPEN);
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
            return Error(CommonError::FILE_FLUSH_FAILED);
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
    bool closeFile=false;
    boost::system::error_code ec;
    if (!m_file.is_open())
    {
        const_cast<PlainFile*>(this)->m_file.open(filename(),Mode::scan,ec);
        if (ec)
        {
            throw ErrorException(makeBoostError(ec));
        }
        closeFile=true;
    }
    auto ret=m_file.size(ec);
    if (closeFile)
    {
        boost::system::error_code ec1;
        const_cast<PlainFile*>(this)->m_file.close(ec1);
    }
    if (ec)
    {
        throw ErrorException(makeBoostError(ec));
    }
    return ret;
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
    HATN_COMMON_NAMESPACE_END
