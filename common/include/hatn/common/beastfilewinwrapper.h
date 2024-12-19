/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/****************************************************************************/
/** @file common/beastfilewinwrapper.h
  *
  * Declarations of date and time types.
  */

/****************************************************************************/

#ifndef HATNBEASTFILEWINWRAPPER_H
#define HATNBEASTFILEWINWRAPPER_H

#ifdef __GNUC__
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wnon-virtual-dtor"
#endif

#include <boost/beast/core/file_win32.hpp>

#ifdef __GNUC__
#pragma GCC diagnostic pop
#endif

#include <hatn/common/common.h>
#include <hatn/common/file.h>

HATN_COMMON_NAMESPACE_BEGIN

//! @todo Add boost patch to build scritps for all platforms

class BeastFileWinWrapper : public boost::beast::file_win32
{
    public:

        BeastFileWinWrapper()=default;

        void setShareMode(File::ShareMode mode) noexcept
        {
            m_shareMode=mode;
        }

        inline void open(char const* path, boost::beast::file_mode mode, boost::beast::error_code& ec);

    private:

        File::ShareMode m_shareMode=0;
};

inline void BeastFileWinWrapper::open(char const* path, boost::beast::file_mode mode, boost::beast::error_code& ec)
{
    if(h_ != boost::winapi::INVALID_HANDLE_VALUE_)
    {
        boost::winapi::CloseHandle(h_);
        h_ = boost::winapi::INVALID_HANDLE_VALUE_;
    }
    boost::winapi::DWORD_ share_mode = 0;
    boost::winapi::DWORD_ desired_access = 0;
    boost::winapi::DWORD_ creation_disposition = 0;
    boost::winapi::DWORD_ flags_and_attributes = 0;
    /*
                             |                    When the file...
    This argument:           |             Exists            Does not exist
    -------------------------+------------------------------------------------------
    CREATE_ALWAYS            |            Truncates             Creates
    CREATE_NEW         +-----------+        Fails               Creates
    OPEN_ALWAYS     ===| does this |===>    Opens               Creates
    OPEN_EXISTING      +-----------+        Opens                Fails
    TRUNCATE_EXISTING        |            Truncates              Fails
*/
    switch(mode)
    {
    default:
    case boost::beast::file_mode::read:
        desired_access = boost::winapi::GENERIC_READ_;
        share_mode = boost::winapi::FILE_SHARE_READ_;
        creation_disposition = boost::winapi::OPEN_EXISTING_;
        flags_and_attributes = 0x10000000; // FILE_FLAG_RANDOM_ACCESS
        break;

    case boost::beast::file_mode::scan:
        desired_access = boost::winapi::GENERIC_READ_;
        share_mode = boost::winapi::FILE_SHARE_READ_;
        creation_disposition = boost::winapi::OPEN_EXISTING_;
        flags_and_attributes = 0x08000000; // FILE_FLAG_SEQUENTIAL_SCAN
        break;

    case boost::beast::file_mode::write:
        desired_access = boost::winapi::GENERIC_READ_ |
                         boost::winapi::GENERIC_WRITE_;
        creation_disposition = boost::winapi::CREATE_ALWAYS_;
        flags_and_attributes = 0x10000000; // FILE_FLAG_RANDOM_ACCESS
        break;

    case boost::beast::file_mode::write_new:
        desired_access = boost::winapi::GENERIC_READ_ |
                         boost::winapi::GENERIC_WRITE_;
        creation_disposition = boost::winapi::CREATE_NEW_;
        flags_and_attributes = 0x10000000; // FILE_FLAG_RANDOM_ACCESS
        break;

    case boost::beast::file_mode::write_existing:
        desired_access = boost::winapi::GENERIC_READ_ |
                         boost::winapi::GENERIC_WRITE_;
        creation_disposition = boost::winapi::OPEN_EXISTING_;
        flags_and_attributes = 0x10000000; // FILE_FLAG_RANDOM_ACCESS
        break;

    case boost::beast::file_mode::append:
        desired_access = boost::winapi::GENERIC_READ_ |
                         boost::winapi::GENERIC_WRITE_;

        creation_disposition = boost::winapi::OPEN_ALWAYS_;
        flags_and_attributes = 0x08000000; // FILE_FLAG_SEQUENTIAL_SCAN
        break;

    case boost::beast::file_mode::append_existing:
        desired_access = boost::winapi::GENERIC_READ_ |
                         boost::winapi::GENERIC_WRITE_;
        creation_disposition = boost::winapi::OPEN_EXISTING_;
        flags_and_attributes = 0x08000000; // FILE_FLAG_SEQUENTIAL_SCAN
        break;
    }

    if (File::Sharing::hasFeature(m_shareMode,File::Share::Read))
    {
        share_mode |= boost::winapi::FILE_SHARE_READ_;
    }
    if (File::Sharing::hasFeature(m_shareMode,File::Share::Write))
    {
        share_mode |= boost::winapi::FILE_SHARE_WRITE_;
    }
    if (File::Sharing::hasFeature(m_shareMode,File::Share::Delete))
    {
        share_mode |= boost::winapi::FILE_SHARE_DELETE_;
    }

    boost::beast::detail::win32_unicode_path unicode_path(path, ec);
    if (ec)
        return;
    h_ = ::CreateFileW(
        unicode_path.c_str(),
        desired_access,
        share_mode,
        NULL,
        creation_disposition,
        flags_and_attributes,
        NULL);
    if (h_ == boost::winapi::INVALID_HANDLE_VALUE_)
    {
        ec.assign(boost::winapi::GetLastError(),
                  boost::beast::system_category());
        return;
    }
    if (mode == boost::beast::file_mode::append ||
        mode == boost::beast::file_mode::append_existing)
    {
        boost::winapi::LARGE_INTEGER_ in;
        in.QuadPart = 0;
        if (!boost::beast::detail::set_file_pointer_ex(h_, in, 0,
                                         boost::winapi::FILE_END_))
        {
            ec.assign(boost::winapi::GetLastError(),
                      boost::beast::system_category());
            boost::winapi::CloseHandle(h_);
            h_ = boost::winapi::INVALID_HANDLE_VALUE_;
            return;
        }
    }
    ec = {};
}

HATN_COMMON_NAMESPACE_END

#endif // HATNBEASTFILEWINWRAPPER_H
