/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.
    
    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)
      
*/

/****************************************************************************/
/** @file common/stdwrappers.h
  *
  *      Wrappers of filesystem framework that can be present or missing in std lib.
  *
  */

/****************************************************************************/

#ifndef HATNFILESYSTEM_H
#define HATNFILESYSTEM_H

#if __cplusplus < 201703L
    #include <boost/filesystem.hpp>
#else
    #include <filesystem>
#endif

#include <hatn/common/common.h>

HATN_COMMON_NAMESPACE_BEGIN
namespace lib
{

#if __cplusplus < 201703L
namespace filesystem=boost::filesystem;
#else
namespace filesystem=std::filesystem;
#endif

} // namespace lib
HATN_COMMON_NAMESPACE_END

#endif // HATNFILESYSTEM_H
