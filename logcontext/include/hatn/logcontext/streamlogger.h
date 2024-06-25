/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.
    
    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)
      
*/

/****************************************************************************/

/** @file logcontext/consolelogger.h
  *
  *  Defines context logger to char stream.
  *
  */

/****************************************************************************/

#ifndef HATNCTXSTREAMLOGGER_H
#define HATNCTXSTREAMLOGGER_H

#include <fmt/chrono.h>

#include <hatn/common/format.h>

#include <hatn/logcontext/context.h>
#include <hatn/logcontext/logger.h>

HATN_LOGCONTEXT_NAMESPACE_BEGIN

template <typename ContextT=Context>
class StreamLoggerT : public LoggerHandlerT<ContextT>
{
    public:

        StreamLoggerT(
                std::vector<std::ostream*> couts,
                std::vector<std::ostream*> cerrs
            ) : m_couts(std::move(couts)),
                m_cerrs(std::move(cerrs))
        {}

        StreamLoggerT(
            std::ostream* cout=&std::cout,
            std::ostream* cerr=&std::cerr
            ) : StreamLoggerT(std::vector<std::ostream*>{cout},std::vector<std::ostream*>{cerr})
        {}

        void formatTo(
            std::vector<std::ostream*>& streams,
            LogLevel level,
            const ContextT* ctx,
            common::pmr::string msg,
            const Error* ec,
            lib::string_view module,
            common::pmr::vector<Record> records
        )
        {
            common::FmtAllocatedBufferChar buf;

            fmt::format_to(std::back_inserter(buf),"{:%Y%m%dT%H:%M:%S}"
                                                    " lvl={}"
                                                    " ctx={}",
                           ctx->taskCtx()->adjustTp(
                               ctx->taskCtx()->adjustTz(ctx->taskCtx()->startedAt())
                           ),
                           logLevelName(level),
                           ctx->taskCtx()->id().c_str()
                        );
            if (ec!=nullptr)
            {
                buf.append(lib::string_view(" err=\""));
                ec->codeString(buf);
                buf.append(lib::string_view("\""));
            }
            if (!msg.empty())
            {
                buf.append(lib::string_view(" msg=\""));
                buf.append(msg);
                buf.append(lib::string_view("\""));
            }
            if (!module.empty())
            {
                buf.append(lib::string_view(" mdl="));
                buf.append(module);
            }
            for (auto&& rec:records)
            {
                buf.append(lib::string_view(" "));
                buf.append(rec.first);
                buf.append(lib::string_view("="));
                serializeValue(buf,rec.second);
            }

            for (auto&& os: streams)
            {
                std::copy(buf.begin(),buf.end(),std::ostream_iterator<char>(*os));
                *os<<std::endl;
            }
        }

        void log(
            LogLevel level,
            const ContextT* ctx,
            common::pmr::string msg,
            lib::string_view module=lib::string_view{},
            common::pmr::vector<Record> records=common::pmr::vector<Record>{}
        ) override
        {
            formatTo(m_couts,level,ctx,msg,nullptr,std::move(module),std::move(records));
        }

        void logError(
            LogLevel level,
            const Error& ec,
            const ContextT* ctx,
            common::pmr::string msg,
            lib::string_view module=lib::string_view{},
            common::pmr::vector<Record> records=common::pmr::vector<Record>{}
        ) override
        {
            formatTo(m_cerrs,level,ctx,msg,&ec,std::move(module),std::move(records));
        }

    private:

        std::vector<std::ostream*> m_couts;
        std::vector<std::ostream*> m_cerrs;
};
using StreamLogger=StreamLoggerT<>;

HATN_LOGCONTEXT_NAMESPACE_END

#endif // HATNCTXSTREAMLOGGER_H
