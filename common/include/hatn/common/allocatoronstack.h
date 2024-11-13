/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.
    
    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)
      
*/
/****************************************************************************/
/** @file common/allocatoronstack.h
 *
 *     Defines AllocatorOnStack.
 *
 */
/****************************************************************************/

#ifndef HATNALLOCATORONSTACK_H
#define HATNALLOCATORONSTACK_H

#include <hatn/thirdparty/shortallocator/short_alloc.h>

#include <hatn/common/common.h>

HATN_COMMON_NAMESPACE_BEGIN

template <class T, std::size_t N, std::size_t Align = alignof(std::max_align_t)>
using AllocatorOnStack=salloc::short_alloc<T,N*Align,Align>;

HATN_COMMON_NAMESPACE_END

#endif // HATNALLOCATORONSTACK_H
