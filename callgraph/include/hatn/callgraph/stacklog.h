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
#include <hatn/common/thread.h>
#include <hatn/common/taskcontext.h>

#include <hatn/callgraph/callgraph.h>
#include <hatn/callgraph/stacklogrecord.h>

HATN_CALLGRAPH_NAMESPACE_BEGIN

namespace stacklog
{

constexpr size_t MaxVarStackSize=32;
constexpr size_t MaxVarMapSize=16;
constexpr size_t MaxFnDepth=16;
constexpr size_t MaxFnNameLength=32;
constexpr size_t MaxThreadDepth=8;

struct DefaultConfig
{
    constexpr static const size_t ValueLength=MaxValueLength;
    constexpr static const size_t KeyLength=MaxKeyLength;
    constexpr static const size_t VarStackSize=MaxVarStackSize;
    constexpr static const size_t VarMapSize=MaxVarMapSize;
    constexpr static const size_t FnDepth=MaxFnDepth;
    constexpr static const size_t FnNameLength=MaxFnNameLength;
    constexpr static const size_t ThreadDepth=MaxThreadDepth;
};

struct FnCursorData
{
    size_t fnStackOffset=0;
    size_t varStackOffset=0;
};

template <typename FnCursorDataT=FnCursorData, size_t NameLength=MaxFnNameLength>
using FnCursorT=std::pair<common::FixedByteArray<NameLength>,FnCursorDataT>;

struct ThreadCursorData
{
    size_t fnStackOffset=0;
};

template <typename CursorDataT=ThreadCursorData>
using ThreadCursorT=std::pair<common::ThreadId,CursorDataT>;

template <typename Config=DefaultConfig,
         typename FnCursorDataT=FnCursorData,
         typename ThreadCursorDataT=ThreadCursorData>
class StackLogT
{
    public:

        using config=Config;

        using valueT=ValueT<config::ValueLength>;
        using keyT=KeyT<config::KeyLength>;
        using recordT=RecordT<valueT,keyT>;
        using fnCursorDataT=FnCursorDataT;
        using fnCursorT=FnCursorT<FnCursorDataT,config::FnNameLength>;
        using threadCursorT=ThreadCursorT<ThreadCursorDataT>;

        using varStackAllocatorT=common::AllocatorOnStack<recordT,config::VarStackSize>;
        using varMapAllocatorT=common::AllocatorOnStack<recordT,config::VarMapSize>;
        using fnStackAllocatorT=common::AllocatorOnStack<fnCursorT,config::FnDepth>;
        using threadStackAllocatorT=common::AllocatorOnStack<threadCursorT,config::ThreadDepth>;

        StackLogT()
        {
            m_varStack.reserve(config::VarStackSize);
            m_globalVarMap.reserve(config::VarMapSize);
            m_fnStack.reserve(config::FnDepth);
        }

        void enterFn(const char* name)
        {
            Assert(!m_error,"Must not continue stack in error state");
            m_fnStack.emplace_back(std::make_pair(name,fnCursorDataT{m_fnStack.size(),m_varStack.size()}));
        }

        void leaveFn()
        {
            if (m_error)
            {
                return;
            }

            const auto& fnCursor=m_fnStack.back();
            bool freeFn=true;

            if (!m_threadStack.empty())
            {
                const auto& threadCursor=m_threadStack.back();
                freeFn=fnCursor.second.fnStackOffset>threadCursor.second.fnStackOffset;
            }
            if (freeFn)
            {
                m_varStack.resize(fnCursor.second.varStackOffset);
                m_fnStack.pop_back();
            }
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

        void acquireThread(const common::ThreadId& id)
        {
            m_threadStack.emplace_back(
                std::make_pair(id,ThreadCursorDataT{m_fnStack.size()})
            );
        }

        void releaseThread(const common::ThreadId& id)
        {
            if (m_threadStack.empty())
            {
                return;
            }

            int pos=int(m_threadStack.size())-1;
            for (;pos>=0;pos--)
            {
                if (m_threadStack[pos].first==id)
                {
                    break;
                }
            }

            Assert(pos>=0,"Can not release thread that was not aquired");
            m_threadStack.resize(pos);
        }

        void setEc(const Error& ec)
        {
            m_error=ec;
        }

        void setEc(Error&& ec)
        {
            m_error=std::move(ec);
        }

        void resetEc() noexcept
        {
            m_error.reset();
        }

        Error& ec() noexcept
        {
            return m_error;
        }

    private:

        std::vector<fnCursorT,fnStackAllocatorT> m_fnStack;
        std::vector<recordT,varStackAllocatorT> m_varStack;
        std::vector<threadCursorT,threadStackAllocatorT> m_threadStack;
        common::FlatMap<keyT,valueT,std::less<keyT>,varMapAllocatorT> m_globalVarMap;

        Error m_error;
};
using StackLog=StackLogT<>;

template <typename StackLogT=StackLog>
class StackLogWrapperT : public common::TaskContextWrapper<StackLogT>
{
    public:

        const StackLogT* value() const noexcept
        {
            return &m_stackLog;
        }

        StackLogT* value() noexcept
        {
            return &m_stackLog;
        }

    private:

        StackLogT m_stackLog;
};
using StackLogWrapper=StackLogWrapperT<>;

} // namespace stacklog

HATN_CALLGRAPH_NAMESPACE_END

HATN_TASK_CONTEXT_DECLARE(HATN_CALLGRAPH_NAMESPACE::stacklog::StackLog,HATN_CALLGRAPH_EXPORT)

#endif // HATNSTACKLOG_H
