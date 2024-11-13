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
#include <hatn/common/runonscopeexit.h>

#include <hatn/logcontext/logcontext.h>
#include <hatn/logcontext/record.h>

HATN_LOGCONTEXT_NAMESPACE_BEGIN

enum class LogLevel : int8_t
{
    Default=-1,
    None=0,
    Fatal=1,
    Error=2,
    Warn=3,
    Info=4,
    Debug=5,
    Trace=6,

    Any=100
};

//---------------------------------------------------------------
inline const char* logLevelName(LogLevel level) noexcept
{
    switch (level)
    {
        case (LogLevel::Info): return "INFO";
        case (LogLevel::Error): return "ERROR";
        case (LogLevel::Warn): return "WARN";
        case (LogLevel::Debug): return "DEBUG";
        case (LogLevel::Trace): return "TRACE";
        case (LogLevel::Any): return "ANY";
        case (LogLevel::Fatal): return "FATAL";
        case (LogLevel::Default): return "DEFAULT";
        case (LogLevel::None): return "NONE";
    }
    return "UNKNOWN";
}

constexpr size_t MaxVarStackSize=16;
constexpr size_t MaxVarMapSize=8;
constexpr size_t MaxScopeDepth=16;
constexpr size_t MaxThreadDepth=4;
constexpr size_t MaxTagLength=8;
constexpr size_t MaxTagSetSize=8;

struct ScopeCursorData
{
    size_t scopeStackOffset=0;
    size_t varStackOffset=0;
    common::ThreadId threadId;
    const char* error;
};

using ScopeCursor=std::pair<const char*,ScopeCursorData>;

struct DefaultConfig
{
    constexpr static const size_t ValueLength=PreallocatedValueSize;
    constexpr static const size_t KeyLength=MaxKeyLength;
    constexpr static const size_t VarStackSize=MaxVarStackSize;
    constexpr static const size_t VarMapSize=MaxVarMapSize;
    constexpr static const size_t ScopeDepth=MaxScopeDepth;
    constexpr static const size_t ThreadDepth=MaxThreadDepth;
    constexpr static const size_t TagLength=MaxTagLength;
    constexpr static const size_t TagSetSize=MaxTagSetSize;
};

struct ThreadCursorData
{
    size_t scopeStackOffset=0;
};

template <typename CursorDataT=ThreadCursorData>
using ThreadCursorT=std::pair<common::ThreadId,CursorDataT>;

template <class T, std::size_t N>
using ContextAlloc=common::AllocatorOnStack<T,N>;

// template <class T, std::size_t N>
// using ContextAlloc=std::allocator<T>;

template <typename Config=DefaultConfig,
         typename ThreadCursorDataT=ThreadCursorData>
class ContextT : public common::TaskContextValue
{
    public:

        using config=Config;

        using valueT=ValueT<config::ValueLength>;
        using keyT=KeyT<config::KeyLength>;
        using recordT=RecordT<valueT,keyT>;
        using scopeCursorDataT=ScopeCursorData;
        using scopeCursorT=ScopeCursor;
        using threadCursorT=ThreadCursorT<ThreadCursorDataT>;
        using tagT=common::FixedByteArray<config::TagLength>;
        using tagRecordT=std::pair<tagT,LogLevel>;

        using varStackAllocatorT=ContextAlloc<recordT,config::VarStackSize>;
        using varMapAllocatorT=ContextAlloc<recordT,config::VarMapSize>;
        using scopeStackAllocatorT=ContextAlloc<scopeCursorT,config::ScopeDepth>;
        using threadStackAllocatorT=ContextAlloc<threadCursorT,config::ThreadDepth>;
        using tagSetAllocatorT=ContextAlloc<tagT,config::TagSetSize>;

        ContextT(common::TaskContext* taskCtx)
            :   TaskContextValue(taskCtx),
                m_currentScopeIdx(0),
                m_lockStack(false),
                m_logLevel(LogLevel::Default)
            ,
                m_scopeStackArena(),
                m_varStackArena(),
                m_threadStackArena(),
                m_varMapArena(),
                m_tagSetArena(),
                m_scopeStack(m_scopeStackArena),
                m_varStack(m_varStackArena),
                m_threadStack(m_threadStackArena),
                m_globalVarMap(m_varMapArena),
                m_tags(m_tagSetArena)
        {}

        ~ContextT()=default;
        ContextT(ContextT&&)=delete;
        ContextT(const ContextT&)=delete;
        ContextT& operator=(ContextT&&)=delete;
        ContextT& operator=(const ContextT&)=delete;

        /**
         * @brief Enter scope.
         * @param name Name of the scope. Must be constexpr. Do not use temporary variable.
         */
        void enterScope(const char* name)
        {
            m_currentScopeIdx++;
            Assert(m_currentScopeIdx<=config::ScopeDepth,"Reached depth of scope stack");
            m_scopeStack.emplace_back(std::make_pair(name,scopeCursorDataT{m_scopeStack.size(),m_varStack.size(),common::Thread::currentThreadID(),nullptr}));
        }

        /**
         * @brief Describe scope error.
         * @param err Error description. Must be constexpr. Do not use temporary variable.
         * @param lockStack Do not pop scope stack when leavinf the scope.
         */
        void describeScopeError(const char* err, bool lockStack=true)
        {
            if (lockStack)
            {
                m_lockStack=true;
            }
            auto* scopeCursor=currentScope();
            Assert(scopeCursor!=nullptr,"Forbidden in empty scope stack");
            scopeCursor->second.error=err;
        }

        void leaveScope()
        {
            const auto* scopeCursor=currentScope();
            Assert(scopeCursor!=nullptr,"Forbidden in empty scope stack");
            bool freeScope=true;

            if (!m_threadStack.empty())
            {
                const auto& threadCursor=m_threadStack.back();
                freeScope=scopeCursor->second.scopeStackOffset>threadCursor.second.scopeStackOffset;
            }
            if (freeScope)
            {
                m_currentScopeIdx--;
                Assert(m_currentScopeIdx<=config::ScopeDepth,"Mismatched number of enter/leace scope calls");

                if (!m_lockStack)
                {
                    m_varStack.resize(scopeCursor->second.varStackOffset);
                    m_scopeStack.pop_back();
                }
            }
        }

        template <typename T>
        void pushStackVar(const lib::string_view& key, T&& value)
        {
            m_varStack.emplace_back(key,std::forward<T>(value));
        }

        void popStackVar() noexcept
        {
            if (!m_lockStack)
            {
                m_varStack.pop_back();
            }
        }

        template <typename T>
        void setGlobalVar(const lib::string_view& key, T&& value)
        {
            auto&& val=valueT{std::forward<T>(value)};
            auto&& k=keyT{key};
            m_globalVarMap.emplace(key,std::move(val));
        }

        void unsetGlobalVar(const lib::string_view& key)
        {
            m_globalVarMap.erase(key);
        }

        void acquireThread(const common::ThreadId& id)
        {
            m_threadStack.emplace_back(
                std::make_pair(id,ThreadCursorDataT{m_scopeStack.size()})
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

        void setStackLocked(bool enable)
        {
            m_lockStack=enable;
        }

        bool stackLocked() const noexcept
        {
            return m_lockStack;
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

        const scopeCursorT* currentScope() const
        {
            if (m_currentScopeIdx==0)
            {
                return nullptr;
            }
            return &m_scopeStack.at(m_currentScopeIdx-1);
        }

        scopeCursorT* currentScope()
        {
            if (m_currentScopeIdx==0)
            {
                return nullptr;
            }
            return &m_scopeStack.at(m_currentScopeIdx-1);
        }

        void resetStacks()
        {
            m_currentScopeIdx=0;
            m_scopeStack.clear();
            m_varStack.clear();
            m_threadStack.clear();
        }

        void reset()
        {
            resetStacks();
            m_globalVarMap.clear();
            m_tags.clear();
        }

        const auto& scopeStack() const noexcept
        {
            return m_scopeStack;
        }

        const auto& stackVars() const noexcept
        {
            return m_varStack;
        }

        const auto& globalVars() const noexcept
        {
            return m_globalVarMap;
        }

        const auto& tags() const noexcept
        {
            return m_tags;
        }

    private:

        size_t m_currentScopeIdx;
        bool m_lockStack;
        LogLevel m_logLevel;

        typename scopeStackAllocatorT::arena_type m_scopeStackArena;
        typename varStackAllocatorT::arena_type m_varStackArena;
        typename threadStackAllocatorT::arena_type m_threadStackArena;
        typename varMapAllocatorT::arena_type m_varMapArena;
        typename tagSetAllocatorT::arena_type m_tagSetArena;

        std::vector<scopeCursorT,scopeStackAllocatorT> m_scopeStack;
        std::vector<recordT,varStackAllocatorT> m_varStack;
        std::vector<threadCursorT,threadStackAllocatorT> m_threadStack;
        common::FlatMap<keyT,valueT,std::less<keyT>,varMapAllocatorT> m_globalVarMap;
        common::FlatSet<tagT,std::less<tagT>,tagSetAllocatorT> m_tags;
};
using Context=ContextT<>;

template <typename ContextT=Context>
class ContextWrapperT : public common::TaskContextWrapper<ContextT>
{
    public:

        using common::TaskContextWrapper<ContextT>::TaskContextWrapper;
};
using ContextWrapper=ContextWrapperT<>;

HATN_LOGCONTEXT_NAMESPACE_END

HATN_TASK_CONTEXT_DECLARE(HATN_LOGCONTEXT_NAMESPACE::Context,HATN_LOGCONTEXT_EXPORT)

#define HATN_CTX_IF() \
    if (HATN_COMMON_NAMESPACE::ThreadLocalContext<HATN_LOGCONTEXT_NAMESPACE::Context>::value()!=nullptr)

#define HATN_CTX_SCOPE_DEFER() \
    auto _ctxOnExit=[ScopeCtx]{ \
        ScopeCtx->leaveScope();\
    }; \
    auto _ctxScopeGuard=HATN_COMMON_NAMESPACE::makeScopeGuard(std::move(_ctxOnExit),ScopeCtx!=nullptr);\
    std::ignore=_ctxScopeGuard;

#define HATN_CTX_SCOPE(Name) \
    auto ScopeCtx=HATN_COMMON_NAMESPACE::ThreadLocalContext<HATN_LOGCONTEXT_NAMESPACE::Context>::value(); \
    HATN_CTX_IF() \
    { ScopeCtx->enterScope(Name); } \
    HATN_CTX_SCOPE_DEFER()

#define HATN_CTX_SET_VAR(Name,Value) \
    HATN_CTX_IF() \
    ScopeCtx->setGlobalVar(Name,Value);

#define HATN_CTX_UNSET_VAR(Name) \
    HATN_CTX_IF() \
        ScopeCtx->unsetGlobalVar(Name);

#define HATN_CTX_SCOPE_PUSH(Name,Value) \
    HATN_CTX_IF() \
        HATN_COMMON_NAMESPACE::ThreadLocalContext<HATN_LOGCONTEXT_NAMESPACE::Context>::value()->pushStackVar(Name,Value);

#define HATN_CTX_SCOPE_PUSH_(Name,Value) \
    HATN_CTX_IF() \
    _(ScopeCtx)->pushStackVar(Name,Value);

#define HATN_CTX_SCOPE_POP() \
    HATN_CTX_IF() \
        ScopeCtx->popStackVar();

#define HATN_CTX_RESET() \
    HATN_CTX_IF() \
        HATN_COMMON_NAMESPACE::ThreadLocalContext<HATN_LOGCONTEXT_NAMESPACE::Context>::value()->reset();

#define HATN_CTX_RESET_STACKS() \
    HATN_CTX_IF() \
        HATN_COMMON_NAMESPACE::ThreadLocalContext<HATN_LOGCONTEXT_NAMESPACE::Context>::value()->resetStacks();

#define HATN_CTX_SCOPE_ERROR(Error) \
    HATN_CTX_IF() \
        HATN_COMMON_NAMESPACE::ThreadLocalContext<HATN_LOGCONTEXT_NAMESPACE::Context>::value()->describeScopeError(Error);

#define HATN_CTX_SCOPE_LOCK() \
    HATN_CTX_IF() \
        ScopeCtx->setStackLocked(true);

#endif // HATNLOGCONTEXT_H
