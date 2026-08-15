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

#include <iostream>
#include <sstream>
#include <mutex>
#include <memory>

#include <hatn/common/flatmap.h>
#include <hatn/common/allocatoronstack.h>
#include <hatn/common/thread.h>
#include <hatn/common/taskcontext.h>
#include <hatn/common/weakptr.h>
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
    size_t id;

    BarrierCursorData(const char* name="", size_t scopeStackOffset=0, size_t id=0)
        : name(name),
          scopeStackOffset(scopeStackOffset),
          id(id)
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
        using self=ContextT<Config>;

        // Hard cap on m_scopeStack growth while locked (see enterScope()). Generous headroom
        // over normal nesting depth (config::ScopeDepth) so it never engages during legitimate
        // deep-but-still-locked scenarios, but bounds the otherwise-unbounded growth if a lock
        // is left set across many enter/leave cycles on a reused context (a missing
        // HATN_CTX_SCOPE_UNLOCK()/setStackLocked(false) call).
        constexpr static const size_t MaxLockedScopeStackSize=8*config::ScopeDepth;

        // Same idea for m_barrierStack: a stack barrier (HATN_CTX_STACK_BARRIER_ON /
        // HATN_CTX_SCOPE_WITH_BARRIER) that is never lifted with a matching OFF pins every
        // scope pushed at or below it forever, and on a long-lived/reused context each missed
        // OFF adds up. Bound the growth the same way enterScope() bounds m_scopeStack.
        constexpr static const size_t MaxBarrierStackSize=8*config::BarrierDepth;

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
                m_scopeStackCapWarned(false),
                m_barrierStackCapWarned(false),
                m_nextBarrierId(1),
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

            // Defensive cap: while m_lockStack is true, leaveScope() skips pop_back() (see
            // below), so a context reused across many enter/leave cycles without an
            // intervening HATN_CTX_SCOPE_UNLOCK()/setStackLocked(false) call would otherwise
            // grow m_scopeStack unboundedly (this was the root cause of a real multi-minute
            // hang destroying a scope stack with thousands of entries at context teardown; see
            // whitem/docs — fixed at the actual lock trigger too, this is belt-and-suspenders).
            // currentScope()'s existing idx>size() clamp already tolerates m_currentScopeIdx
            // outrunning m_scopeStack.size(), so simply not growing past the cap is safe.
            if (m_scopeStack.size()>=MaxLockedScopeStackSize)
            {
                if (!m_scopeStackCapWarned)
                {
                    m_scopeStackCapWarned=true;
                    std::ostringstream oss;
                    oss << "logcontext scope stack exceeded " << MaxLockedScopeStackSize
                        << " entries (lockStack=" << m_lockStack
                        << ", currentScopeIdx=" << m_currentScopeIdx
                        << ") - capping growth; likely a missing scope-unlock on a "
                           "repeatedly-locked, reused context";
                    emitDiagnostic(oss.str());
                }
                return;
            }

            m_scopeStack.emplace_back(std::make_pair(name,scopeCursorDataT{m_scopeStack.size(),m_varStack.size(),m_varStack.size(),common::Thread::currentThreadID(),nullptr}));
        }

        /**
         * @brief Describe scope error.
         * @param err Error description. Must be constexpr. Do not use temporary variable.
         * @param lockStack Do not pop scope stack when leaving the scope.
         */
        void describeScopeError(const char* err, bool lockStack=true)
        {
            //! @todo Use error stack

            if (lockStack && m_enableStackLocking)
            {
                m_lockStack=true;
                if (m_lockScopeIdx==0)
                {
                    m_lockScopeIdx=m_currentScopeIdx;
                }
            }
            auto* scopeCursor=currentScope();
            if (scopeCursor==nullptr)
            {
                std::cerr << "describeScopeError() forbidden in empty scope stack" << std::endl;
                return;
            }
            scopeCursor->second.error=err;
        }

        void leaveScope()
        {
            const auto* scopeCursor=currentScope();
            if (scopeCursor==nullptr)
            {
                // scope cursor can be nullptr only after resetting/closing API, ensure context's reset
                reset();
                return;
            }
            bool freeScope=true;

            if (!m_barrierStack.empty())
            {
                freeScope=scopeCursor->second.scopeStackOffset >= m_barrierStack.back().scopeStackOffset;
            }
            if (freeScope)
            {
                if (m_currentScopeIdx>0)
                {
                    m_currentScopeIdx--;
                }

                if (!m_lockStack)
                {
                    m_varStack.resize(scopeCursor->second.varStackOffset);
                    m_scopeStack.pop_back();
                }

                // The real invariant is that the index tracks the stack size exactly - but
                // only once m_scopeStack has actually been popped above (a plain depth
                // heuristic like m_currentScopeIdx>config::ScopeDepth both false-fires on
                // legitimate deep nesting and stays silent on a real mismatch that keeps the
                // index low, so this checks the actual invariant instead). Checking this
                // BEFORE the pop_back() above compares a not-yet-decremented stack size
                // against the already-decremented index, so it would be off by one on every
                // single normal, correctly-paired call - that was a real bug in an earlier
                // version of this check, not a heuristic false-positive: it fired on every
                // shallow, barrier-free leaveScope() (e.g. plain scopes with scope stack (1)-
                // (4) and barrier stack (0), as seen from TestFiles2Queue), which is exactly
                // the class of call this diagnostic must stay silent on.
                if (!m_lockStack && m_barrierStack.empty() && m_currentScopeIdx!=m_scopeStack.size())
                {
                    dumpScopeMismatch();
                }
            }
        }

        template <typename T>
        void pushStackVar(const lib::string_view& key, T&& value)
        {
            auto* scopeCursor=currentScope();
            if (scopeCursor==nullptr)
            {
                return;
            }
            m_varStack.emplace_back(key,std::forward<T>(value));
            scopeCursor->second.varStackSize=m_varStack.size();
        }

        void popStackVar() noexcept
        {
            if (!m_lockStack)
            {
                if (m_varStack.empty())
                {
                    return;
                }
                m_varStack.pop_back();
                auto* scopeCursor=currentScope();
                if (scopeCursor!=nullptr)
                {
                    scopeCursor->second.varStackSize=m_varStack.size();
                }
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

        // Returns the id of the newly pushed barrier, or 0 if the barrier stack is capped (see
        // MaxBarrierStackSize) and the barrier was not tracked at all. 0 is never a real id
        // (m_nextBarrierId starts at 1), so callers of stackBarrierOffId() can treat it as a
        // harmless no-op sentinel.
        size_t stackBarrierOn(const char* name)
        {
            if (m_barrierStack.size()>=MaxBarrierStackSize)
            {
                if (!m_barrierStackCapWarned)
                {
                    m_barrierStackCapWarned=true;
                    std::ostringstream oss;
                    oss << "logcontext barrier stack exceeded " << MaxBarrierStackSize
                        << " entries - capping growth; likely a barrier that is never lifted "
                           "with a matching HATN_CTX_STACK_BARRIER_OFF on a long-lived/reused "
                           "context";
                    emitDiagnostic(oss.str());
                }
                return 0;
            }

            auto id=m_nextBarrierId++;
            m_barrierStack.emplace_back(name,m_currentScopeIdx,id);
            return id;
        }

        void stackBarrierOff(const char* name)
        {
            if (m_barrierStack.empty())
            {
                return;
            }

            bool restore=false;
            int idx=static_cast<int>(m_barrierStack.size())-1;
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

        // Id-based counterpart of stackBarrierOff(): matches the barrier pushed by the
        // stackBarrierOn() call that returned this id, regardless of how many other barriers
        // share the same name. This is what ScopeBarrier (see below) uses, so releasing one of
        // several identically-named nested barriers (e.g. repeated
        // "grpctransport::sendrequest" retries on a reused context) always collapses the
        // correct frame instead of the topmost name match.
        void stackBarrierOffId(size_t id)
        {
            if (id==0 || m_barrierStack.empty())
            {
                return;
            }

            bool restore=false;
            int idx=static_cast<int>(m_barrierStack.size())-1;
            for (;idx>=0;idx--)
            {
                if (m_barrierStack[idx].id==id)
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
            int idx=static_cast<int>(m_barrierStack.size())-1;
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
            //! @todo Use error stack

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
            return const_cast<self*>(this)->currentScope();
        }

        scopeCursorT* currentScope()
        {
            auto idx=m_currentScopeIdx;
            if (idx>m_scopeStack.size())
            {
                idx=m_scopeStack.size();
            }
            if (idx==0)
            {
                return nullptr;
            }
            return &m_scopeStack[idx-1];
        }

        void resetStacks()
        {
            m_currentScopeIdx=0;
            m_lockScopeIdx=0;
            m_scopeStack.clear();
            m_varStack.clear();
            m_barrierStack.clear();

            // A context reset while still locked previously stayed locked forever - reset()
            // is meant to bring the context back to a clean, reusable state, so it must also
            // clear the lock and the one-shot cap warnings, and restart barrier id allocation
            // (1, not 0: 0 is the reserved "not tracked" sentinel returned by stackBarrierOn()
            // when the barrier stack is capped, see ScopeBarrier).
            m_lockStack=false;
            m_scopeStackCapWarned=false;
            m_barrierStackCapWarned=false;
            m_nextBarrierId=1;
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
        void resetParentCtx(const HATN_COMMON_NAMESPACE::SharedPtr<ParentContextT>& parentCtx)
        {
            if (!parentCtx)
            {
                m_parentLogCtx=nullptr;
                return;
            }
            m_parentLogCtx=&parentCtx->template get<ContextT>();
        }

        void resetParentCtx(const HATN_COMMON_NAMESPACE::SharedPtr<HATN_COMMON_NAMESPACE::TaskContext>&)
        {
            auto pCtx=common::ThreadSubcontext<common::TaskSubcontextT<ContextT>>::value();
            if (pCtx!=nullptr)
            {
                m_parentLogCtx=pCtx;
            }
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

        // Single, mutex-guarded write so a multi-line diagnostic dump is never interleaved
        // with concurrent writes to the same stream (e.g. the logger thread writing the
        // console sink) and never shows up shredded mid-token in the log file.
        static void emitDiagnostic(const std::string& msg)
        {
            static std::mutex mutex;
            std::lock_guard<std::mutex> lk(mutex);
            std::cerr << msg << std::endl;
        }

        void dumpScopeMismatch()
        {
            std::ostringstream oss;
            oss << "Mismatched number of enter/leave scope calls: currentScopeIdx=" << m_currentScopeIdx
                << " ctx=" << mainCtx().id()
                << " lockStack=" << m_lockStack
                << "\n  scope stack (" << m_scopeStack.size() << "):";
            for (size_t i=0;i<m_scopeStack.size();i++)
            {
                oss << "\n    [" << i << "] " << m_scopeStack[i].first;
                if (m_scopeStack[i].second.error!=nullptr)
                    oss << "  error=" << m_scopeStack[i].second.error;
            }
            oss << "\n  barrier stack (" << m_barrierStack.size() << "):";
            for (size_t i=0;i<m_barrierStack.size();i++)
            {
                oss << "\n    [" << i << "] " << m_barrierStack[i].name
                    << "  id=" << m_barrierStack[i].id
                    << "  scopeOffset=" << m_barrierStack[i].scopeStackOffset;
            }
            emitDiagnostic(oss.str());
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
        bool m_scopeStackCapWarned;
        bool m_barrierStackCapWarned;
        size_t m_nextBarrierId;

        ContextT* m_parentLogCtx;

        Logger* m_logger;
};
using Context=ContextT<>;
using Subcontext=Context;
using LogContext=Context;

/**
 * @brief RAII counterpart of HATN_CTX_STACK_BARRIER_ON()/HATN_CTX_STACK_BARRIER_OFF().
 *
 * A stack barrier pins its own scope frame and everything pushed below it until a matching
 * OFF is issued, so an async continuation can resume logging at that frame later. In practice
 * every OFF has to be reached by hand on every exit path - including error early-returns and a
 * callback that is captured but never invoked - and a single missed one pins the frame (and
 * everything under it) on that context forever. Nothing bounds how many times this can happen
 * on a long-lived or reused context, which is exactly the "scope stack (28): ..." growth this
 * type exists to prevent.
 *
 * ScopeBarrier turns the barrier into a movable, non-copyable token: construct it where the
 * barrier is raised, capture it by value (its copy ctor is deleted, so this really means move
 * or shared_ptr) into every continuation that can run instead of the original, and the barrier
 * is released exactly once - whichever copy is destroyed last - regardless of which path was
 * taken to get there. Release is id-based (see ContextT::stackBarrierOffId()), so it collapses
 * the exact frame it raised even when several identically-named barriers are nested (e.g.
 * repeated retries reusing one context).
 *
 * The last token can also outlive the context itself: a pending callback that captured it may
 * be destroyed - without ever running - after the task context is gone (cancelled request,
 * transport shutdown draining its queues). ScopeBarrier therefore keeps a weak reference to
 * the SharedPtr-owned TaskContext of the log context and silently skips the release when that
 * context is already destroyed, instead of dereferencing a dangling pointer.
 */
class ScopeBarrier
{
    public:

        ScopeBarrier() noexcept : m_ctx(nullptr), m_id(0), m_guarded(false)
        {}

        ScopeBarrier(Context* ctx, const char* name) : m_ctx(ctx), m_id(0), m_guarded(false)
        {
            if (m_ctx!=nullptr)
            {
                m_id=m_ctx->stackBarrierOn(name);

                // m_ctx is a raw pointer into a log subcontext owned by a SharedPtr-managed
                // TaskContext, while the last barrier token typically dies inside an async
                // callback whose destruction the context owner does not control - the pending
                // callback can be dropped (cancelled request, transport shutdown) long after
                // the task context itself is gone. Take a weak reference to the owning
                // TaskContext so release() can detect that and skip the (otherwise
                // use-after-free) stackBarrierOffId() call.
                if (m_ctx->hasMainCtx())
                {
                    auto mainCtx=m_ctx->sharedMainCtx();
                    if (!mainCtx.isNull())
                    {
                        m_mainCtxGuard=mainCtx;
                        m_guarded=true;
                    }
                }
            }
        }

        ~ScopeBarrier()
        {
            release();
        }

        ScopeBarrier(const ScopeBarrier&)=delete;
        ScopeBarrier& operator=(const ScopeBarrier&)=delete;

        ScopeBarrier(ScopeBarrier&& other) noexcept
            : m_ctx(other.m_ctx), m_id(other.m_id),
              m_guarded(other.m_guarded), m_mainCtxGuard(std::move(other.m_mainCtxGuard))
        {
            other.m_ctx=nullptr;
            other.m_id=0;
            other.m_guarded=false;
        }

        ScopeBarrier& operator=(ScopeBarrier&& other) noexcept
        {
            if (this!=&other)
            {
                release();
                m_ctx=other.m_ctx;
                m_id=other.m_id;
                m_guarded=other.m_guarded;
                m_mainCtxGuard=std::move(other.m_mainCtxGuard);
                other.m_ctx=nullptr;
                other.m_id=0;
                other.m_guarded=false;
            }
            return *this;
        }

        //! @brief Release the barrier now instead of waiting for the destructor. Idempotent.
        void release() noexcept
        {
            if (m_ctx!=nullptr)
            {
                if (m_guarded)
                {
                    // lock() keeps the TaskContext alive for the duration of the call;
                    // if it fails the context is already destroyed and there is nothing
                    // to release.
                    auto mainCtx=m_mainCtxGuard.lock();
                    if (!mainCtx.isNull())
                    {
                        m_ctx->stackBarrierOffId(m_id);
                    }
                }
                else
                {
                    // Context without a SharedPtr-owned main TaskContext (e.g. created on
                    // the stack): its creator controls the lifetime, keep the direct call.
                    m_ctx->stackBarrierOffId(m_id);
                }
                m_mainCtxGuard.reset();
                m_ctx=nullptr;
                m_id=0;
                m_guarded=false;
            }
        }

    private:

        Context* m_ctx;
        size_t m_id;
        bool m_guarded;
        common::WeakPtr<common::TaskContext> m_mainCtxGuard;
};

struct makeScopeBarrierT
{
    //! @brief Returns nullptr if ctx is null, so it is safe to call unconditionally from macros.
    std::shared_ptr<ScopeBarrier> operator()(Context* ctx, const char* name) const
    {
        if (ctx==nullptr)
        {
            return std::shared_ptr<ScopeBarrier>{};
        }
        return std::make_shared<ScopeBarrier>(ctx,name);
    }
};
constexpr makeScopeBarrierT makeScopeBarrier{};

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

#define HATN_CTX_CURRENT() \
    HATN_THREAD_SUBCONTEXT(HATN_LOGCONTEXT_NAMESPACE::Context)

#define HATN_CTX_IF() \
    if (HATN_CTX_CURRENT()!=nullptr)

#define HATN_CTX_SET_VAR(Name,Value) \
    HATN_CTX_IF() \
        HATN_CTX_CURRENT()->setGlobalVar(Name,Value);

#define HATN_CTX_PUSH_VAR(Name,Value) \
    HATN_CTX_IF() \
        HATN_CTX_CURRENT()->setGlobalVar(Name,Value);

#define HATN_CTX_PUSH_FIXED_VAR(Name,Value) \
    HATN_CTX_IF() \
        HATN_CTX_CURRENT()->pushFixedVar(Name,Value);

#define HATN_CTX_UNSET_VAR(Name) \
    HATN_CTX_IF() \
        ScopeCtx->unsetGlobalVar(Name);

#define HATN_CTX_SCOPE_PUSH(Name,Value) \
    HATN_CTX_IF() \
        HATN_CTX_CURRENT()->pushStackVar(Name,Value);

#define HATN_CTX_SCOPE_PUSH_(Name,Value) \
    HATN_CTX_IF() \
    _(ScopeCtx)->pushStackVar(Name,Value);

#define HATN_CTX_SCOPE_POP() \
    HATN_CTX_IF() \
        HATN_CTX_CURRENT()->popStackVar();

#define HATN_CTX_RESET() \
    HATN_CTX_IF() \
        HATN_CTX_CURRENT()->reset();

#define HATN_CTX_RESET_STACKS() \
    HATN_CTX_IF() \
        HATN_CTX_CURRENT()->resetStacks();

#define HATN_CTX_SCOPE_ERROR(Error) \
    HATN_CTX_IF() \
        HATN_CTX_CURRENT()->describeScopeError(Error);

#define HATN_CTX_SCOPE_LOCK() \
    HATN_CTX_IF() \
        HATN_CTX_CURRENT()->setStackLocked(true);

#define HATN_CTX_SCOPE_UNLOCK() \
    HATN_CTX_IF() \
        HATN_CTX_CURRENT()->setStackLocked(false);

#define HATN_CTX_CHECK_EC(ec) \
    if (ec) \
    { \
        HATN_CTX_IF() \
            HATN_CTX_CURRENT()->setStackLocked(true); \
        return ec; \
    }

#define HATN_CTX_CHECK_EC_MSG(ec,msg) \
    if (ec) \
    { \
        HATN_CTX_SCOPE_ERROR(msg) \
        return ec; \
    }

#define HATN_CTX_CHECK_EC_LOG(ec,msg) \
    if (ec) \
    { \
        HATN_CTX_SCOPE_LOCK() \
        HATN_CTX_ERROR(ec,msg) \
        return ec; \
    }

#define HATN_CTX_CHECK_EC_LOG_MSG(ec,msg) \
    if (ec) \
    { \
        HATN_CTX_SCOPE_ERROR(msg) \
        HATN_CTX_ERROR(ec,"") \
        return ec; \
    }

#define HATN_CTX_EC_LOG(ec,msg) \
    HATN_CTX_SCOPE_ERROR(msg) \
    HATN_CTX_ERROR(ec,msg)

#define HATN_CTX_STACK_BARRIER_ON(Name) \
    HATN_CTX_IF() \
        HATN_CTX_CURRENT()->stackBarrierOn(Name);

#define HATN_CTX_STACK_BARRIER_OFF(Name) \
    HATN_CTX_IF() \
        HATN_CTX_CURRENT()->stackBarrierOff(Name);

#define HATN_CTX_STACK_BARRIER_RESTORE(Name) \
    HATN_CTX_IF() \
        HATN_CTX_CURRENT()->stackBarrierRestore(Name);

#define HATN_CTX_STACK_BARRIER_LAST_OFF() \
    HATN_CTX_IF() \
        HATN_CTX_CURRENT()->stackBarrierLastOff();

#define HATN_CTX_SCOPE_WITH_BARRIER(Name) \
    HATN_CTX_SCOPE(Name) \
    HATN_CTX_IF() \
        HATN_CTX_CURRENT()->stackBarrierOn(Name);

// RAII counterpart of HATN_CTX_SCOPE_WITH_BARRIER(): the barrier is released by ScopeBarrier's
// destructor instead of a hand-written HATN_CTX_STACK_BARRIER_OFF(). _ctxBarrier is declared
// after HATN_CTX_SCOPE(Name)'s own scope guard, so on a purely synchronous exit it is
// destroyed first - barrier lifted, then leaveScope() can actually pop the scope instead of
// being blocked by its own barrier. For an async continuation, capture _ctxBarrier by value
// (it is move-only/shared, never copied implicitly) into every lambda that can run instead of
// falling off the end of the current scope; whichever copy is destroyed last releases the
// barrier exactly once, on every path - including early returns, exceptions, and a callback
// that ends up never being invoked.
#define HATN_CTX_SCOPE_WITH_BARRIER_GUARD(Name) \
    HATN_CTX_SCOPE(Name) \
    auto _ctxBarrier=HATN_LOGCONTEXT_NAMESPACE::makeScopeBarrier(ScopeCtx,Name); \
    std::ignore=_ctxBarrier;


#endif // HATNLOGCONTEXT_H
