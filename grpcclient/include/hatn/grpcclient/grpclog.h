/*
    Copyright (c) 2024 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    {{LICENSE}}
*/

/****************************************************************************/
/*

*/
/** @file grpcclient/grpclog.h
  *
  *  Routing of the gRPC library's own diagnostics into the application log.
  *
  *  gRPC writes its messages (and, when tracers are enabled, its connection traces) through
  *  abseil logging, which prints to stderr by default. On a deployed desktop or mobile app
  *  stderr is not visible, so a failed connection leaves nothing to look at. The functions below
  *  register an abseil log sink that forwards every message to a hatn logger, module "grpc".
  *
  *  The interface is free of gRPC and abseil types: both stay inside hatngrpcclient.
  */

/****************************************************************************/

#ifndef HATNGRPCLOG_H
#define HATNGRPCLOG_H

#include <memory>
#include <string>
#include <vector>

#include <hatn/logcontext/context.h>

#include <hatn/grpcclient/grpcclientdefs.h>

HATN_GRPCCLIENT_NAMESPACE_BEGIN

/**
 * @brief Start forwarding gRPC library messages to a hatn logger.
 * @param logger Destination logger. The sink keeps it alive until uninstallGrpcLogSink().
 * @param minLevel Least severe level that is forwarded; gRPC messages below it are dropped. Mapping from
 *        abseil: INFO -> Info, WARNING -> Warn, ERROR -> Error, FATAL -> Fatal. The logger's own
 *        level settings for module "grpc" still apply on top of this.
 *
 * Calling it again replaces the logger and the level. Also switches abseil logging to its initialized
 * state, which stops it from duplicating the messages on stderr below the ERROR severity.
 * If the environment variable HATN_GRPC_TRACE is set the level is raised to Info, so that the traces
 * requested through it reach the log.
 *
 * Thread-safe. Messages arrive on arbitrary gRPC threads.
 */
HATN_GRPCCLIENT_EXPORT void installGrpcLogSink(
    std::shared_ptr<HATN_LOGCONTEXT_NAMESPACE::Logger> logger,
    HATN_LOGCONTEXT_NAMESPACE::LogLevel minLevel=HATN_LOGCONTEXT_NAMESPACE::LogLevel::Warn
);

/**
 * @brief Stop forwarding and release the logger.
 *
 * Must be called before the logger passed to installGrpcLogSink() is destroyed. Does nothing if no sink is
 * installed. Active ScopedGrpcTrace objects stay valid; they just stop producing log lines.
 */
HATN_GRPCCLIENT_EXPORT void uninstallGrpcLogSink();

/**
 * @brief Change the least severe level that is forwarded while a sink is installed.
 */
HATN_GRPCCLIENT_EXPORT void setGrpcLogLevel(HATN_LOGCONTEXT_NAMESPACE::LogLevel minLevel);

/**
 * @brief Level currently used by the sink (the base level, or Info while a ScopedGrpcTrace is alive).
 */
HATN_GRPCCLIENT_EXPORT HATN_LOGCONTEXT_NAMESPACE::LogLevel grpcLogLevel();

/**
 * @brief Tracers that explain a failed connection: name resolution, connectivity state changes,
 *        subchannel and handshake progress, TCP connect results, load-balancing picks.
 *
 * Deliberately excludes the per-frame tracers (http, secure_endpoint), which are too noisy to enable
 * for a diagnostic of a few seconds.
 */
HATN_GRPCCLIENT_EXPORT std::vector<std::string> defaultGrpcDiagnosticTracers();

/**
 * @brief Enables gRPC tracers for the lifetime of the object.
 *
 * While at least one object is alive the sink level is Info, so the trace lines are written to the log.
 * Objects may overlap: a tracer is switched off only when the last object that requested it is destroyed,
 * and the sink level goes back to its base value when the last object is destroyed.
 * Unknown tracer names are reported by gRPC itself as an error line.
 */
class HATN_GRPCCLIENT_EXPORT ScopedGrpcTrace
{
    public:

        explicit ScopedGrpcTrace(std::vector<std::string> tracers=defaultGrpcDiagnosticTracers());
        ~ScopedGrpcTrace();

        ScopedGrpcTrace(const ScopedGrpcTrace&)=delete;
        ScopedGrpcTrace& operator=(const ScopedGrpcTrace&)=delete;
        ScopedGrpcTrace(ScopedGrpcTrace&&)=delete;
        ScopedGrpcTrace& operator=(ScopedGrpcTrace&&)=delete;

    private:

        std::vector<std::string> m_tracers;
};

HATN_GRPCCLIENT_NAMESPACE_END

#endif // HATNGRPCLOG_H
