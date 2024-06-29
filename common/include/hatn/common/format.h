/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.
    
    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)
      
*/

/****************************************************************************/
/** @file common/format.h
  *
  *     String formating wrappers to use with fmtlib
  *
  */

/****************************************************************************/

#ifndef HATNFORMAT_H
#define HATNFORMAT_H

#ifdef fmt
#undef fmt
#endif

#include <fmt/format.h>

#include <hatn/common/common.h>
#include <hatn/common/pmr/pmrtypes.h>

HATN_COMMON_NAMESPACE_BEGIN

    template <typename T=char>
    using FormatAllocator=pmr::polymorphic_allocator<T>;

    template <size_t Size=fmt::inline_buffer_size,typename T=char>
    using FmtAllocatedBuffer=fmt::basic_memory_buffer<T,Size,FormatAllocator<T>>;

    using FmtAllocatedBufferChar=FmtAllocatedBuffer<>;
    using FmtAllocator=FormatAllocator<>;

    template <typename BufT>
    std::string fmtBufToString(const BufT& buffer)
    {
        return std::string(buffer.data(),buffer.size());
    }

//---------------------------------------------------------------
HATN_COMMON_NAMESPACE_END

#endif // HATNFORMAT_H
