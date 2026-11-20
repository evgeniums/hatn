/*
    Copyright (c) 2024 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    {{LICENSE}}
*/

/****************************************************************************/
/*

*/
/** @file grpcclient/grpclog.cpp
  *
  *  Forwards the gRPC library's abseil log messages to a hatn logger. See grpclog.h.
  *
  */

/****************************************************************************/

#include <algorithm>
#include <atomic>
#include <cstdlib>
#include <map>
#include <mutex>

#include <grpc/grpc.h>

#include <absl/log/globals.h>
#include <absl/log/initialize.h>
#include <absl/log/log_entry.h>
#include <absl/log/log_sink.h>
#include <absl/log/log_sink_registry.h>

#include <hatn/logcontext/contextlogger.h>

#include <hatn/grpcclient/grpclog.h>

HATN_GRPCCLIENT_NAMESPACE_BEGIN

namespace common=HATN_COMMON_NAMESPACE;
namespace log=HATN_LOGCONTEXT_NAMESPACE;

namespace {

constexpr const char* LogModule="grpc";

log::LogLevel mapSeverity(absl::LogSeverity severity) noexcept
{
    switch (severity)
    {
    case absl::LogSeverity::kInfo: return log::LogLevel::Info;
    case absl::LogSeverity::kWarning: return log::LogLevel::Warn;
    case absl::LogSeverity::kError: return log::LogLevel::Error;
    case absl::LogSeverity::kFatal: return log::LogLevel::Fatal;
    }
    return log::LogLevel::Info;
}

/**
 * Abseil sink that hands every message to a hatn logger.
 *
 * Send() is called synchronously on whatever thread produced the message, usually a gRPC worker thread
 * that has no hatn log context. A short-lived context is therefore created for each message.
 * Nothing in Send() may log through abseil, otherwise the message would loop back into the sink.
 */
class GrpcLogSink : public absl::LogSink
{
    public:

        void Send(const absl::LogEntry& entry) override
        {
            const auto level=mapSeverity(entry.log_severity());
            if (static_cast<int>(level)>m_level.load(std::memory_order_relaxed))
            {
                return;
            }

            std::shared_ptr<log::Logger> logger;
            {
                std::lock_guard<std::mutex> l{m_mutex};
                logger=m_logger;
            }
            if (!logger)
            {
                return;
            }

            auto taskCtx=log::makeLogCtx();
            auto& logCtx=taskCtx->get<log::Context>();
            logCtx.setLogger(logger.get());

            const common::lib::string_view module{LogModule};
            if (!log::Logger::passLog(*logger,level,&logCtx,module))
            {
                return;
            }

            // the entry is valid only during this call
            const auto textView=entry.text_message();
            const auto fileView=entry.source_basename();
            const std::string text(textView.data(),textView.size());
            const std::string source=std::string(fileView.data(),fileView.size())+":"+std::to_string(entry.source_line());

            logger->log(
                level,
                &logCtx,
                text.c_str(),
                {
                    {"src",common::lib::string_view{source}}
                },
                module
            );
        }

        void setLogger(std::shared_ptr<log::Logger> logger)
        {
            std::lock_guard<std::mutex> l{m_mutex};
            m_logger=std::move(logger);
        }

        void setLevel(log::LogLevel level) noexcept
        {
            m_level.store(static_cast<int>(level),std::memory_order_relaxed);
        }

    private:

        std::mutex m_mutex;
        std::shared_ptr<log::Logger> m_logger;
        std::atomic<int> m_level{static_cast<int>(log::LogLevel::Warn)};
};

/**
 * Process-wide state. Allocated once and never destroyed on purpose: gRPC threads may still log
 * while the process shuts down, so the sink and the mutex must outlive static destruction.
 */
struct State
{
    std::mutex mutex;

    std::unique_ptr<GrpcLogSink> sink;
    log::LogLevel baseLevel=log::LogLevel::Warn;

    int traceScopes=0;
    std::map<std::string,int> tracerRefs;

    // must be called under the mutex
    log::LogLevel effectiveLevel() const noexcept
    {
        if (traceScopes>0 && static_cast<int>(baseLevel)<static_cast<int>(log::LogLevel::Info))
        {
            return log::LogLevel::Info;
        }
        return baseLevel;
    }

    // must be called under the mutex
    void applyLevel() noexcept
    {
        if (sink)
        {
            sink->setLevel(effectiveLevel());
        }
    }
};

State& state()
{
    static State* s=new State();
    return *s;
}

} // anonymous namespace

//--------------------------------------------------------------------------

void installGrpcLogSink(std::shared_ptr<log::Logger> logger, log::LogLevel minLevel)
{
    // Abseil prints every message to stderr until it is initialized; after that only the messages at
    // or above the stderr threshold (ERROR) are printed there and the rest go to the sinks only.
    static std::once_flag initOnce;
    std::call_once(initOnce,[]()
    {
        absl::InitializeLog();
        absl::SetStderrThreshold(absl::LogSeverityAtLeast::kError);
    });

    auto& st=state();
    std::lock_guard<std::mutex> l{st.mutex};

    st.baseLevel=minLevel;
    const char* trace=std::getenv("HATN_GRPC_TRACE");
    if (trace!=nullptr && trace[0]!='\0' && static_cast<int>(st.baseLevel)<static_cast<int>(log::LogLevel::Info))
    {
        st.baseLevel=log::LogLevel::Info;
    }

    if (!st.sink)
    {
        st.sink=std::make_unique<GrpcLogSink>();
        absl::AddLogSink(st.sink.get());
    }
    st.sink->setLogger(std::move(logger));
    st.applyLevel();
}

//--------------------------------------------------------------------------

void uninstallGrpcLogSink()
{
    auto& st=state();
    std::lock_guard<std::mutex> l{st.mutex};

    if (!st.sink)
    {
        return;
    }

    // returns after the messages that are being sent right now are done, so the sink can be destroyed
    absl::RemoveLogSink(st.sink.get());
    st.sink.reset();
}

//--------------------------------------------------------------------------

void setGrpcLogLevel(log::LogLevel minLevel)
{
    auto& st=state();
    std::lock_guard<std::mutex> l{st.mutex};

    st.baseLevel=minLevel;
    st.applyLevel();
}

//--------------------------------------------------------------------------

log::LogLevel grpcLogLevel()
{
    auto& st=state();
    std::lock_guard<std::mutex> l{st.mutex};

    return st.effectiveLevel();
}

//--------------------------------------------------------------------------

std::vector<std::string> defaultGrpcDiagnosticTracers()
{
    return {
        "dns_resolver",
        "cares_resolver",
        "client_channel",
        "connectivity_state",
        "subchannel",
        "pick_first",
        "handshaker",
        "tcp",
        "call_error"
    };
}

/*****************************ScopedGrpcTrace*********************************/

//--------------------------------------------------------------------------

ScopedGrpcTrace::ScopedGrpcTrace(std::vector<std::string> tracers)
    : m_tracers(std::move(tracers))
{
    // one reference per name for this object
    std::sort(m_tracers.begin(),m_tracers.end());
    m_tracers.erase(std::unique(m_tracers.begin(),m_tracers.end()),m_tracers.end());

    auto& st=state();
    std::lock_guard<std::mutex> l{st.mutex};

    st.traceScopes++;
    st.applyLevel();

    for (const auto& name : m_tracers)
    {
        if (++st.tracerRefs[name]==1)
        {
            grpc_tracer_set_enabled(name.c_str(),1);
        }
    }
}

//--------------------------------------------------------------------------

ScopedGrpcTrace::~ScopedGrpcTrace()
{
    auto& st=state();
    std::lock_guard<std::mutex> l{st.mutex};

    for (const auto& name : m_tracers)
    {
        auto it=st.tracerRefs.find(name);
        if (it==st.tracerRefs.end())
        {
            continue;
        }
        if (--it->second<=0)
        {
            st.tracerRefs.erase(it);
            // one name per call: gRPC applies the "-" prefix only to the first item of a comma-separated list
            grpc_tracer_set_enabled(name.c_str(),0);
        }
    }

    if (st.traceScopes>0)
    {
        st.traceScopes--;
    }
    st.applyLevel();
}

HATN_GRPCCLIENT_NAMESPACE_END
