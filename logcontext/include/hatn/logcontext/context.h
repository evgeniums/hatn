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
    size_t varStackSize=0;
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

    ThreadCursorData(size_t scopeStackOffset=0) : scopeStackOffset(scopeStackOffset)
    {}
};

template <typename CursorDataT=ThreadCursorData>
using ThreadCursorT=CursorDataT;

template <class T, std::size_t N>
using ContextAlloc=common::AllocatorOnStack<T,N>;

template <typename Config=DefaultConfig,
         typename ThreadCursorDataT=ThreadCursorData>
class ContextT
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

        ContextT()
            :   m_currentScopeIdx(0),
                m_lockStack(false),
                m_logLevel(LogLevel::Default),
                m_enableStackLocking(true)
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
            m_scopeStack.emplace_back(std::make_pair(name,scopeCursorDataT{m_scopeStack.size(),m_varStack.size(),m_varStack.size(),common::Thread::currentThreadID(),nullptr}));
        }

        /**
         * @brief Describe scope error.
         * @param err Error description. Must be constexpr. Do not use temporary variable.
         * @param lockStack Do not pop scope stack when leaving the scope.
         */
        void describeScopeError(const char* err, bool lockStack=true)
        {
            if (lockStack && m_enableStackLocking)
            {
                m_lockStack=true;
            }
            auto* scopeCursor=currentScope();
            Assert(scopeCursor!=nullptr,"describeScopeError() forbidden in empty scope stack");
            scopeCursor->second.error=err;
        }

        void leaveScope()
        {
            const auto* scopeCursor=currentScope();
            Assert(scopeCursor!=nullptr,"leaveScope() forbidden in empty scope stack");
            bool freeScope=true;

            if (!m_threadStack.empty())
            {
                const auto& threadCursor=m_threadStack.back();
                freeScope=scopeCursor->second.scopeStackOffset>threadCursor.scopeStackOffset;
            }
            if (freeScope)
            {
                m_currentScopeIdx--;
                Assert(m_currentScopeIdx<=config::ScopeDepth,"Mismatched number of enter/leave scope calls");

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
            auto* scopeCursor=currentScope();
            scopeCursor->second.varStackSize=m_varStack.size();
        }

        void popStackVar() noexcept
        {
            if (!m_lockStack)
            {
                m_varStack.pop_back();
                auto* scopeCursor=currentScope();
                scopeCursor->second.varStackSize=m_varStack.size();
            }
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

        void acquireThread()
        {
            m_threadStack.emplace_back(
                m_scopeStack.size()
            );
        }

        void releaseThread()
        {
            if (!m_threadStack.empty())
            {
                m_threadStack.pop_back();
            }
        }

        inline void acquireAsyncHandler()
        {
            acquireThread();
        }

        inline void releaseAsyncHandler()
        {
            releaseThread();
        }

        inline void enterLoop()
        {
            Assert(!m_loopScopeIdx,"Nested loops not suported by LogContext");
            m_loopScopeIdx=m_currentScopeIdx;
        }

        inline void leaveLoop()
        {
            if (!m_loopScopeIdx)
            {
                m_currentScopeIdx=0;
            }
            else
            {
                m_currentScopeIdx=m_loopScopeIdx.value();
                m_loopScopeIdx.reset();
            }
            restoreStackCursors();            
        }

        void setStackLockingEnabled(bool enable) noexcept
        {
            m_enableStackLocking=enable;
        }

        bool isStackLockingEnabled() const noexcept
        {
            return m_enableStackLocking;
        }

        void setStackLocked(bool enable)
        {
            if (!m_enableStackLocking)
            {
                return;
            }

            bool locked=m_lockStack;
            m_lockStack=enable;

            // restore stack cursors to current scope
            if (locked)
            {
                restoreStackCursors();
            }
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

        const auto& threadStack() const noexcept
        {
            return m_threadStack;
        }

    private:

        void restoreStackCursors()
        {
            if (!m_lockStack)
            {
                m_scopeStack.resize(m_currentScopeIdx);
                const auto* scopeCursor=currentScope();
                if (scopeCursor!=nullptr)
                {
                    m_varStack.resize(scopeCursor->second.varStackSize);
                }
                else
                {
                    m_varStack.clear();
                }
            }
        }

        size_t m_currentScopeIdx;
        bool m_lockStack;
        LogLevel m_logLevel;

        HATN_COMMON_NAMESPACE::VectorOnStack<scopeCursorT,config::ScopeDepth> m_scopeStack;
        HATN_COMMON_NAMESPACE::VectorOnStack<recordT,config::VarStackSize> m_varStack;
        HATN_COMMON_NAMESPACE::VectorOnStack<threadCursorT,config::ThreadDepth> m_threadStack;
        HATN_COMMON_NAMESPACE::FlatMapOnStack<keyT,valueT,config::VarMapSize,std::less<keyT>> m_globalVarMap;
        HATN_COMMON_NAMESPACE::FlatSetOnStack<tagT,config::TagSetSize,std::less<tagT>> m_tags;

        bool m_enableStackLocking;
        lib::optional<size_t> m_loopScopeIdx;
};
using Context=ContextT<>;
using Subcontext=HATN_COMMON_NAMESPACE::TaskSubcontextT<Context>;

struct makeLogCtxT
{
    template <typename ...BaseArgs>
    auto operator()(BaseArgs&&... args) const
    {
        return common::makeTaskContext<Context>(
                common::subcontexts(
                    common::subcontext()
                ),
                std::forward<BaseArgs>(args)...
            );
    }

    auto operator()() const
    {
        return common::makeTaskContext<Context>();
    }
};
constexpr makeLogCtxT makeLogCtx{};
using LogCtxType=common::TaskContextType<Context>;

HATN_LOGCONTEXT_NAMESPACE_END

HATN_TASK_CONTEXT_DECLARE(HATN_LOGCONTEXT_NAMESPACE::Context,HATN_LOGCONTEXT_EXPORT)

#define HATN_CTX_IF() \
    if (HATN_THREAD_SUBCONTEXT(HATN_LOGCONTEXT_NAMESPACE::Context)!=nullptr)

#define HATN_CTX_SCOPE_DEFER() \
    auto _ctxOnExit=[ScopeCtx]{ \
        ScopeCtx->leaveScope();\
    }; \
    auto _ctxScopeGuard=HATN_COMMON_NAMESPACE::makeScopeGuard(std::move(_ctxOnExit),ScopeCtx!=nullptr);\
    std::ignore=_ctxScopeGuard;

#define HATN_CTX_SCOPE(Name) \
    auto ScopeCtx=HATN_THREAD_SUBCONTEXT(HATN_LOGCONTEXT_NAMESPACE::Context); \
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
        HATN_THREAD_SUBCONTEXT(HATN_LOGCONTEXT_NAMESPACE::Context)->pushStackVar(Name,Value);

#define HATN_CTX_SCOPE_PUSH_(Name,Value) \
    HATN_CTX_IF() \
    _(ScopeCtx)->pushStackVar(Name,Value);

#define HATN_CTX_SCOPE_POP() \
    HATN_CTX_IF() \
        HATN_THREAD_SUBCONTEXT(HATN_LOGCONTEXT_NAMESPACE::Context)->popStackVar();

#define HATN_CTX_RESET() \
    HATN_CTX_IF() \
        HATN_THREAD_SUBCONTEXT(HATN_LOGCONTEXT_NAMESPACE::Context)->reset();

#define HATN_CTX_RESET_STACKS() \
    HATN_CTX_IF() \
        HATN_THREAD_SUBCONTEXT(HATN_LOGCONTEXT_NAMESPACE::Context)->resetStacks();

#define HATN_CTX_SCOPE_ERROR(Error) \
    HATN_CTX_IF() \
        HATN_THREAD_SUBCONTEXT(HATN_LOGCONTEXT_NAMESPACE::Context)->describeScopeError(Error);

#define HATN_CTX_SCOPE_LOCK() \
    HATN_CTX_IF() \
        HATN_THREAD_SUBCONTEXT(HATN_LOGCONTEXT_NAMESPACE::Context)->setStackLocked(true);

#define HATN_CTX_SCOPE_UNLOCK() \
    HATN_CTX_IF() \
        HATN_THREAD_SUBCONTEXT(HATN_LOGCONTEXT_NAMESPACE::Context)->setStackLocked(false);

#endif // HATNLOGCONTEXT_H
