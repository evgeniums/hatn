/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.
    
    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)
      
*/

/****************************************************************************/
/*
    
*/
/** @file callgraph/stacklog.h
  *
  *  Contains stack log types and helpers.
  *
  */

/****************************************************************************/

#ifndef HATNSTACKLOG_H
#define HATNSTACKLOG_H

#include <vector>

#include <hatn/common/flatmap.h>
#include <hatn/common/allocatoronstack.h>

#include <hatn/callgraph/callgraph.h>
#include <hatn/callgraph/stacklogrecord.h>

HATN_CALLGRAPH_NAMESPACE_BEGIN

namespace stacklog
{

constexpr size_t MaxVarStackSize=32;
constexpr size_t MaxVarMapSize=16;
constexpr size_t MaxFnDepth=16;
constexpr size_t MaxFnNameLength=32;

struct DefaultConfig
{
    constexpr static const size_t ValueLength=MaxValueLength;
    constexpr static const size_t KeyLength=MaxKeyLength;
    constexpr static const size_t VarStackSize=MaxVarStackSize;
    constexpr static const size_t VarMapSize=MaxVarMapSize;
    constexpr static const size_t FnDepth=MaxFnDepth;
    constexpr static const size_t FnNameLength=MaxFnNameLength;
};

struct FnCursorData
{
    size_t varStackOffset=0;
    size_t fnStackOffset=0;
};

template <typename FnCursorDataT=FnCursorData, size_t NameLength=MaxFnNameLength>
using FnCursorT=std::pair<common::FixedByteArray<NameLength>,FnCursorDataT>;

template <typename Config=DefaultConfig, typename FnCursorDataT=FnCursorData>
struct StackLogT
{
    using config=Config;

    using valueT=ValueT<config::ValueLength>;
    using keyT=KeyT<config::KeyLength>;
    using recordT=RecordT<valueT,keyT>;
    using fnCursorT=FnCursorT<FnCursorDataT,config::FnNameLength>;

    using varStackAllocatorT=common::AllocatorOnStack<valueT,config::VarStackSize>;
    using varMapAllocatorT=common::AllocatorOnStack<recordT,config::VarMapSize>;
    using fnStackAllocatorT=common::AllocatorOnStack<fnCursorT,config::FnDepth>;

    std::vector<fnCursorT,fnStackAllocatorT> fnStack;
    std::vector<valueT,varStackAllocatorT> varStack;
    common::FlatMap<keyT,valueT,std::less<>,varMapAllocatorT> varMap;

    StackLogT()
    {
        varStack.reserve(config::VarStackSize);
        varMap.reserve(config::VarMapSize);
        fnStack.reserve(config::FnDepth);
    }
};

using StackLog=StackLogT<>;

} // namespace stacklog

HATN_CALLGRAPH_NAMESPACE_END

#endif // HATNSTACKLOG_H
