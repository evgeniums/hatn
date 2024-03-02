/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.
    
    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)
      
*/

/****************************************************************************/
/** @file common/memorypool/memblocksize.cpp
 *
 *
 */
/****************************************************************************/


#include <hatn/common/memorypool/memblocksize.h>

HATN_COMMON_NAMESPACE_BEGIN
namespace memorypool {

size_t MemBlockSize::DEFAULT_MIN_BLOCK_SIZE=32;
size_t MemBlockSize::DEFAULT_MAX_BLOCK_SIZE=64*1024;

//---------------------------------------------------------------
        } // namespace memorypool
HATN_COMMON_NAMESPACE_END
