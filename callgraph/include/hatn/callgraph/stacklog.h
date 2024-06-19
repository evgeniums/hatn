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
    size_t fnStackOffset=0;
    size_t varStackOffset=0;
};

template <typename FnCursorDataT=FnCursorData, size_t NameLength=MaxFnNameLength>
using FnCursorT=std::pair<common::FixedByteArray<NameLength>,FnCursorDataT>;

template <typename Config=DefaultConfig, typename FnCursorDataT=FnCursorData>
class StackLogT
{
    public:

        using config=Config;

        using valueT=ValueT<config::ValueLength>;
        using keyT=KeyT<config::KeyLength>;
        using recordT=RecordT<valueT,keyT>;
        using fnCursorDataT=FnCursorDataT;
        using fnCursorT=FnCursorT<FnCursorDataT,config::FnNameLength>;

        using varStackAllocatorT=common::AllocatorOnStack<recordT,config::VarStackSize>;
        using varMapAllocatorT=common::AllocatorOnStack<recordT,config::VarMapSize>;
        using fnStackAllocatorT=common::AllocatorOnStack<fnCursorT,config::FnDepth>;

        StackLogT()
        {
            m_varStack.reserve(config::VarStackSize);
            m_globalVarMap.reserve(config::VarMapSize);
            m_fnStack.reserve(config::FnDepth);
        }

        void enterFn(const char* name)
        {
            m_fnStack.emplace_back(std::make_pair(name,fnCursorDataT{m_fnStack.size(),m_varStack.size()}));
        }

        void leaveFn()
        {
            const auto& fnCursor=m_fnStack.back();
            m_varStack.resize(fnCursor.second.varStackOffset);
            m_fnStack.pop_back();
        }

        template <typename T>
        void pushStackVar(const lib::string_view& key, T&& value)
        {
            m_varStack.emplace_back(key,std::forward<T>(value));
        }

        void popStackVar() noexcept
        {
            m_varStack.pop_back();
        }

        template <typename T>
        void setGlobalVar(const lib::string_view& key, T&& value)
        {
            m_globalVarMap.emplace(key,std::forward<T>(value));
        }

        void unsetGlobalVar(const lib::string_view& key)
        {
            m_globalVarMap.erase(key);
        }

    private:

        std::vector<fnCursorT,fnStackAllocatorT> m_fnStack;
        std::vector<recordT,varStackAllocatorT> m_varStack;
        common::FlatMap<keyT,valueT,std::less<keyT>,varMapAllocatorT> m_globalVarMap;
};

using StackLog=StackLogT<>;

} // namespace stacklog

HATN_CALLGRAPH_NAMESPACE_END

#endif // HATNSTACKLOG_H
