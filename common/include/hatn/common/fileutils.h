/*
   Copyright (c) 2020 - current, Evgeny Sidorov (dracosha.com), All rights reserved.
    
    
  */

/****************************************************************************/
/*
    
*/
/** @file common/fileutils.h
  *
  *      Utils for file operations.
  *
  */

/****************************************************************************/

#ifndef HATNFILEUTILS_H
#define HATNFILEUTILS_H

#ifdef __GNUC__
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wnon-virtual-dtor"
#endif

#include <boost/beast/core/file.hpp>
#include <boost/filesystem.hpp>

#ifdef __GNUC__
#pragma GCC diagnostic pop
#endif

#include <hatn/common/common.h>
#include <hatn/common/error.h>

HATN_COMMON_NAMESPACE_BEGIN

//! Utils for file operations
struct FileUtils final
{
    FileUtils()=delete;
    ~FileUtils()=delete;
    FileUtils(const FileUtils&)=delete;
    FileUtils(FileUtils&&) =delete;
    FileUtils& operator=(const FileUtils&)=delete;
    FileUtils& operator=(FileUtils&&) =delete;

    //! Load container from file
    template <typename ContainerT>
    static Error loadFromFile(
            ContainerT& container,
            const char* fileName
        )
    {
        boost::system::error_code ec;
        boost::beast::file f;
        f.open(fileName,boost::beast::file_mode::read,ec);
        if (ec)
        {
            return makeBoostError(ec);
        }
        size_t size=static_cast<size_t>(f.size(ec));
        if (ec)
        {
            return makeBoostError(ec);
        }
        container.resize(size);
        f.read(container.data(),size,ec);
        if (ec)
        {
            return makeBoostError(ec);
        }
        return Error();
    }

    //! Save container to file
    template <typename ContainerT>
    static Error saveToFile(
            const ContainerT& container,
            const char* fileName
        ) noexcept
    {
        boost::system::error_code ec;
        boost::beast::file f;
        f.open(fileName,boost::beast::file_mode::write,ec);
        if (ec)
        {
            return makeBoostError(ec);
        }
        f.write(container.data(),container.size(),ec);
        if (ec)
        {
            return makeBoostError(ec);
        }
        return Error();
    }

    template <typename PathT>
    static Error remove(const PathT& fileName) noexcept
    {
        boost::system::error_code ec;
        boost::filesystem::remove(fileName,ec);
        return makeBoostError(ec);
    }

    template <typename PathT>
    static Error copy(const PathT& from, const PathT& to)
    {
        boost::system::error_code ec;
        boost::filesystem::copy_file(from,to,ec);
        return makeBoostError(ec);
    }
};

//---------------------------------------------------------------
    HATN_COMMON_NAMESPACE_END
#endif // HATNFILEUTILS_H
