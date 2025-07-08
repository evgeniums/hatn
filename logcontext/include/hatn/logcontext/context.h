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

#include <hatn/common/flatmap.h>
#include <hatn/common/allocatoronstack.h>
#include <hatn/common/thread.h>
#include <hatn/common/taskcontext.h>
#include <hatn/common/runonscopeexit.h>

#include <hatn/logcontext/logcontext.h>
#include <hatn/logcontext/record.h>
#include <hatn/logcontext/logger.h>

HATN_LOGCONTEXT_NAMESPACE_BEGIN

constexpr size_t MaxVarStackSize=16;
constexpr size_t MaxVarMapSize=8;
constexpr size_t MaxScopeDepth=16;
constexpr size_t MaxBarrierDepth=4;
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
    constexpr static const size_t BarrierDepth=MaxBarrierDepth;
    constexpr static const size_t TagLength=MaxTagLength;
    constexpr static const size_t TagSetSize=MaxTagSetSize;
};

struct BarrierCursorData
{
    const char* name;
    size_t scopeStackOffset;

    BarrierCursorData(const char* name="", size_t scopeStackOffset=0)
        : name(name),
          scopeStackOffset(scopeStackOffset)
    {}
};

using BarrierCursor=BarrierCursorData;

template <class T, std::size_t N>
using ContextAlloc=common::AllocatorOnStack<T,N>;

template <typename Config=DefaultConfig>
class ContextT : public HATN_COMMON_NAMESPACE::TaskSubcontext
{
    public:

        using config=Config;

        using LoggerHandler=LoggerHandlerT<ContextT<Config>>;
        using Logger=LoggerWithHandler<ContextT<Config>>;

        using valueT=ValueT<config::ValueLength>;
        using keyT=KeyT<config::KeyLength>;
        using recordT=RecordT<valueT,keyT>;
        using scopeCursorDataT=ScopeCursorData;
        using scopeCursorT=ScopeCursor;
        using barrierCursorT=BarrierCursor;
        using tagT=common::FixedByteArray<config::TagLength>;
        using tagRecordT=std::pair<tagT,LogLevel>;

        ContextT()
            :   m_currentScopeIdx(0),                
                m_lockStack(false),
                m_lockScopeIdx(0),
                m_logLevel(LogLevel::Default),
                m_enableStackLocking(true),
                m_debugVerbosity(0),
                m_parentLogCtx(nullptr),
                m_logger(nullptr)
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
                if (m_lockScopeIdx==0)
                {
                    m_lockScopeIdx=m_currentScopeIdx;
                }
            }
            auto* scopeCursor=currentScope();
            Assert(scopeCursor!=nullptr,"describeScopeError() forbidden in empty scope stack");
            scopeCursor->second.error=err;
        }

        void leaveScope()
        {
            const auto* scopeCursor=currentScope();
#if 0
            Assert(scopeCursor!=nullptr,"leaveScope() forbidden in empty scope stack");
#else
            if (scopeCursor==nullptr)
            {
                // scope cursor can be nullptr only after resetting/closing API, ensure context's reset
                reset();
                return;
            }
#endif
            bool freeScope=true;

            if (!m_barrierStack.empty())
            {
                freeScope=scopeCursor->second.scopeStackOffset >= m_barrierStack.back().scopeStackOffset;
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
        void pushFixedVar(const lib::string_view& key, T&& value)
        {
            m_fixedVars.emplace_back(key,std::forward<T>(value));
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

        inline void stackBarrierOn(const char* name)
        {
            m_barrierStack.emplace_back(name,m_currentScopeIdx);
        }

        void stackBarrierOff(const char* name)
        {
            if (m_barrierStack.empty())
            {
                return;
            }

            bool restore=false;
            size_t idx=m_barrierStack.size()-1;
            for (;idx>=0;idx--)
            {
                if (std::strcmp(m_barrierStack[idx].name,name)==0)
                {
                    restore=true;
                    break;
                }
            }
            if (restore)
            {
                m_barrierStack.resize(idx);
                if (m_barrierStack.empty())
                {
                    m_currentScopeIdx=0;
                }
                else
                {
                    m_currentScopeIdx=m_barrierStack.back().scopeStackOffset;
                }
                restoreStackCursors();
            }
        }

        void stackBarrierRestore(const char* name)
        {
            if (m_barrierStack.empty())
            {
                return;
            }

            bool restore=false;
            size_t idx=m_barrierStack.size()-1;
            for (;idx>=0;idx--)
            {
                if (std::strcmp(m_barrierStack[idx].name,name)==0)
                {
                    restore=true;
                    break;
                }
            }
            if (restore)
            {
                m_barrierStack.resize(idx+1);
                if (m_barrierStack.empty())
                {
                    m_currentScopeIdx=0;
                }
                else
                {
                    m_currentScopeIdx=m_barrierStack.back().scopeStackOffset;
                }
                restoreStackCursors();
            }
        }

        inline void stackBarrierLastOff()
        {
            if (m_barrierStack.empty())
            {
                m_currentScopeIdx=0;
            }
            else
            {
                m_barrierStack.pop_back();
                if (m_barrierStack.empty())
                {
                    m_currentScopeIdx=0;
                }
                else
                {
                    m_currentScopeIdx=m_barrierStack.back().scopeStackOffset;
                }
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
            if (m_lockScopeIdx==0)
            {
                m_lockScopeIdx=m_currentScopeIdx;
            }

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

        uint8_t debugVerbosity() const noexcept
        {
            return m_debugVerbosity;
        }

        void setDebugVerbosity(uint8_t val) noexcept
        {
            m_debugVerbosity=val;
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
            m_lockScopeIdx=0;
            m_scopeStack.clear();
            m_varStack.clear();
            m_barrierStack.clear();
        }

        void reset()
        {
            resetStacks();
            m_globalVarMap.clear();
            m_tags.clear();
            m_fixedVars.clear();
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

        const auto& fixedVars() const noexcept
        {
            return m_fixedVars;
        }

        const auto& tags() const noexcept
        {
            return m_tags;
        }

        const auto& barrierStack() const noexcept
        {
            return m_barrierStack;
        }

        template <typename ParentContextT>
        void resetParentCtx(const HATN_COMMON_NAMESPACE::SharedPtr<ParentContextT>& parentCtx={})
        {
            if (!parentCtx)
            {
                m_parentLogCtx=nullptr;
                return;
            }
            m_parentLogCtx=&parentCtx->template get<ContextT>();
        }

        void resetParentCtx()
        {
            m_parentLogCtx=nullptr;
        }

        const ContextT* actualCtx() const noexcept
        {
            if (m_parentLogCtx!=nullptr)
            {
                return m_parentLogCtx;
            }
            return this;
        }

        ContextT* actualCtx() noexcept
        {
            if (m_parentLogCtx!=nullptr)
            {
                return m_parentLogCtx;
            }
            return this;
        }

        void setLogger(Logger* logger) noexcept
        {
            m_logger=logger;
        }

        Logger* logger() const noexcept
        {
            return m_logger;
        }

        size_t lockScopeIdx() const noexcept
        {
            return m_lockScopeIdx;
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
                m_lockScopeIdx=0;
            }
        }

        size_t m_currentScopeIdx;
        bool m_lockStack;
        size_t m_lockScopeIdx;
        LogLevel m_logLevel;

        HATN_COMMON_NAMESPACE::VectorOnStack<scopeCursorT,config::ScopeDepth> m_scopeStack;
        HATN_COMMON_NAMESPACE::VectorOnStack<recordT,config::VarStackSize> m_varStack;
        HATN_COMMON_NAMESPACE::VectorOnStack<barrierCursorT,config::BarrierDepth> m_barrierStack;
        std::map<keyT,valueT> m_globalVarMap;
        HATN_COMMON_NAMESPACE::FlatSetOnStack<tagT,config::TagSetSize,std::less<tagT>> m_tags;

        HATN_COMMON_NAMESPACE::VectorOnStack<recordT,config::VarStackSize> m_fixedVars;

        bool m_enableStackLocking;
        uint8_t m_debugVerbosity;

        ContextT* m_parentLogCtx;

        Logger* m_logger;
};
using Context=ContextT<>;
using Subcontext=Context;
using LogContext=Context;

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
using TaskLogContext=common::TaskContextType<Context>;

using LoggerHandler=typename Context::LoggerHandler;
using Logger=typename Context::Logger;

using LoggerHandlerBuilder=std::function<std::shared_ptr<LoggerHandler> ()>;

struct HATN_LOGCONTEXT_EXPORT ThreadLocalFallbackContext
{
    static void reset(Context* val=nullptr) noexcept;
    static void set(Context* val) noexcept
    {
        reset(val);
    }
};

HATN_LOGCONTEXT_NAMESPACE_END

HATN_COMMON_NAMESPACE_BEGIN

template <>
class HATN_LOGCONTEXT_EXPORT ThreadSubcontext<TaskSubcontextT<HATN_LOGCONTEXT_NAMESPACE::Context>>
{
    public:

        using Type=HATN_LOGCONTEXT_NAMESPACE::Context;

        static Type* value() noexcept;
        static void setValue(Type* val) noexcept;
        static void reset() noexcept;

        static void resetFallbackContext(Type* val=nullptr) noexcept;

};

HATN_COMMON_NAMESPACE_END

#define HATN_CTX_IF() \
    if (HATN_THREAD_SUBCONTEXT(HATN_LOGCONTEXT_NAMESPACE::Context)!=nullptr)

#define HATN_CTX_SET_VAR(Name,Value) \
    HATN_CTX_IF() \
        HATN_THREAD_SUBCONTEXT(HATN_LOGCONTEXT_NAMESPACE::Context)->setGlobalVar(Name,Value);

#define HATN_CTX_PUSH_VAR(Name,Value) \
    HATN_CTX_IF() \
        HATN_THREAD_SUBCONTEXT(HATN_LOGCONTEXT_NAMESPACE::Context)->setGlobalVar(Name,Value);

#define HATN_CTX_PUSH_FIXED_VAR(Name,Value) \
    HATN_CTX_IF() \
        HATN_THREAD_SUBCONTEXT(HATN_LOGCONTEXT_NAMESPACE::Context)->pushFixedVar(Name,Value);

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

#define HATN_CTX_CHECK_EC(ec) \
    if (ec) \
    { \
        HATN_CTX_IF() \
            HATN_THREAD_SUBCONTEXT(HATN_LOGCONTEXT_NAMESPACE::Context)->setStackLocked(true); \
        return ec; \
    }

#define HATN_CTX_CHECK_EC_MSG(ec,msg) \
    if (ec) \
    { \
        HATN_CTX_IF() \
            HATN_CTX_SCOPE_ERROR(msg) \
        return ec; \
    }

#define HATN_CTX_CHECK_EC_LOG(ec,msg) \
    if (ec) \
    { \
            HATN_CTX_IF() \
                HATN_CTX_SCOPE_LOCK() \
                HATN_CTX_ERROR(ec,msg) \
            return ec; \
    }

#define HATN_CTX_CHECK_EC_LOG_MSG(ec,msg) \
    if (ec) \
    { \
            HATN_CTX_IF() \
                HATN_CTX_SCOPE_ERROR(msg) \
                HATN_CTX_ERROR(ec,"") \
            return ec; \
    }


#define HATN_CTX_STACK_BARRIER_ON(Name) \
    HATN_CTX_IF() \
        HATN_THREAD_SUBCONTEXT(HATN_LOGCONTEXT_NAMESPACE::Context)->stackBarrierOn(Name);

#define HATN_CTX_STACK_BARRIER_OFF(Name) \
    HATN_CTX_IF() \
        HATN_THREAD_SUBCONTEXT(HATN_LOGCONTEXT_NAMESPACE::Context)->stackBarrierOff(Name);

#define HATN_CTX_STACK_BARRIER_RESTORE(Name) \
    HATN_CTX_IF() \
        HATN_THREAD_SUBCONTEXT(HATN_LOGCONTEXT_NAMESPACE::Context)->stackBarrierRestore(Name);

#define HATN_CTX_STACK_BARRIER_LAST_OFF() \
    HATN_CTX_IF() \
        HATN_THREAD_SUBCONTEXT(HATN_LOGCONTEXT_NAMESPACE::Context)->stackBarrierLastOff();

#endif // HATNLOGCONTEXT_H
