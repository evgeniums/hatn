/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.
    
    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)
      
*/

/****************************************************************************/
/*
    
*/
/** @file logcontext/context.h
  *
  *  Contains stack log types and helpers.
  *
  */

/****************************************************************************/

#ifndef HATNLOGCONTEXT_H
#define HATNLOGCONTEXT_H

#include <vector>

#include <hatn/common/flatmap.h>
#include <hatn/common/allocatoronstack.h>
#include <hatn/common/thread.h>
#include <hatn/common/taskcontext.h>

#include <hatn/logcontext/logcontext.h>
#include <hatn/logcontext/record.h>

HATN_LOGCONTEXT_NAMESPACE_BEGIN

enum class LogLevel : int8_t
{
    Default=-1,
    None=0,
    Fatal=1,
    Err=2,
    Warn=3,
    Info=4,
    Debug=5,
    Trace=6
};

constexpr size_t MaxVarStackSize=32;
constexpr size_t MaxVarMapSize=16;
constexpr size_t MaxFnDepth=16;
constexpr size_t MaxFnNameLength=32;
constexpr size_t MaxThreadDepth=8;
constexpr size_t MaxTagLength=16;
constexpr size_t MaxTagSetSize=8;

struct DefaultConfig
{
    constexpr static const size_t ValueLength=MaxValueLength;
    constexpr static const size_t KeyLength=MaxKeyLength;
    constexpr static const size_t VarStackSize=MaxVarStackSize;
    constexpr static const size_t VarMapSize=MaxVarMapSize;
    constexpr static const size_t FnDepth=MaxFnDepth;
    constexpr static const size_t FnNameLength=MaxFnNameLength;
    constexpr static const size_t ThreadDepth=MaxThreadDepth;
    constexpr static const size_t TagLength=MaxTagLength;
    constexpr static const size_t TagSetSize=MaxTagSetSize;
};

struct FnCursorData
{
    size_t fnStackOffset=0;
    size_t varStackOffset=0;
    common::ThreadId threadId;
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
class ContextT
{
    public:

        using config=Config;

        using valueT=ValueT<config::ValueLength>;
        using keyT=KeyT<config::KeyLength>;
        using recordT=RecordT<valueT,keyT>;
        using fnCursorDataT=FnCursorDataT;
        using fnCursorT=FnCursorT<FnCursorDataT,config::FnNameLength>;
        using threadCursorT=ThreadCursorT<ThreadCursorDataT>;
        using tagT=common::FixedByteArray<config::TagLength>;
        using tagRecordT=std::pair<tagT,LogLevel>;

        using varStackAllocatorT=common::AllocatorOnStack<recordT,config::VarStackSize>;
        using varMapAllocatorT=common::AllocatorOnStack<recordT,config::VarMapSize>;
        using fnStackAllocatorT=common::AllocatorOnStack<fnCursorT,config::FnDepth>;
        using threadStackAllocatorT=common::AllocatorOnStack<threadCursorT,config::ThreadDepth>;
        using tagSetAllocatorT=common::AllocatorOnStack<tagT,config::TagSetSize>;

        ContextT() : m_logLevel(LogLevel::Default)
        {
            m_varStack.reserve(config::VarStackSize);
            m_globalVarMap.reserve(config::VarMapSize);
            m_fnStack.reserve(config::FnDepth);
            m_tags.reserve(config::TagSetSize);
        }

        void enterFn(const char* name)
        {
            Assert(!m_error,"Must not continue stack in error state");
            m_fnStack.emplace_back(
                std::make_pair(name,fnCursorDataT{m_fnStack.size(),m_varStack.size(),common::Thread::currentThreadID()}));
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

        void setTag(tagT tag)
        {
            m_tags.insert(std::move(tag));
        }

        void unsetTag(const common::lib::string_view& tag)
        {
            m_tags.erase(tag);
        }

        bool containsTag(const common::lib::string_view& tag) const
        {
            auto it=m_tags.find(tag);
            return it!=m_tags.end();
        }

        LogLevel logLevel() const noexcept
        {
            return m_logLevel;
        }

        void setLogLevel(LogLevel level) noexcept
        {
            m_logLevel=level;
        }

        const std::vector<fnCursorT,fnStackAllocatorT>& fnStack() const noexcept
        {
            return m_fnStack;
        }

    private:

        std::vector<fnCursorT,fnStackAllocatorT> m_fnStack;
        std::vector<recordT,varStackAllocatorT> m_varStack;
        std::vector<threadCursorT,threadStackAllocatorT> m_threadStack;
        common::FlatMap<keyT,valueT,std::less<keyT>,varMapAllocatorT> m_globalVarMap;
        common::FlatSet<tagT,std::less<tagT>,tagSetAllocatorT> m_tags;

        Error m_error;
        LogLevel m_logLevel;
};
using Context=ContextT<>;

template <typename ContextT=Context>
class ContextWrapperT : public common::TaskContextWrapper<ContextT>
{
    public:

        const ContextT* value() const noexcept
        {
            return &m_context;
        }

        ContextT* value() noexcept
        {
            return &m_context;
        }

    private:

        ContextT m_context;
};
using ContextWrapper=ContextWrapperT<>;

HATN_LOGCONTEXT_NAMESPACE_END

HATN_TASK_CONTEXT_DECLARE(HATN_LOGCONTEXT_NAMESPACE::Context,HATN_LOGCONTEXT_EXPORT)

#endif // HATNLOGCONTEXT_H
