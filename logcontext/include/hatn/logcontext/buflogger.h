/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.
    
    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)
      
*/

/****************************************************************************/

/** @file logcontext/buflogger.h
  *
  *  Defines formatter of logs to buffer in plain text.
  *
  */

/****************************************************************************/

#ifndef HATNCTXBUFLOGGER_H
#define HATNCTXBUFLOGGER_H

#include <fmt/chrono.h>

#include <hatn/common/apierror.h>
#include <hatn/common/format.h>

#include <hatn/logcontext/context.h>
#include <hatn/logcontext/logger.h>

HATN_LOGCONTEXT_NAMESPACE_BEGIN

template <typename ContextT=Context>
class TextLogFormatterT
{
    public:

        template <typename BufT,
                  typename ErrorT=hana::false_,
                  typename CloseT=hana::false_,
                  typename WithApiStatusT=hana::false_>
        static void format(
            BufT& buf,
            LogLevel level,
            const ContextT* ctx,
            const common::pmr::string& msg,
            const lib::string_view& module,
            const common::pmr::vector<Record>& records,
            ErrorT ec=ErrorT{},
            CloseT =CloseT{},
            WithApiStatusT =WithApiStatusT{}
        )
        {
    #if __cplusplus >= 201703L
            auto tp=std::chrono::floor<std::chrono::microseconds>(ctx->taskCtx()->now());
    #else
            auto tp=ctx->taskCtx()->now();
    #endif
            fmt::format_to(std::back_inserter(buf),"{:%m%dT%H:%M:%S}"
                                                    " lvl={}"
                                                    " ctx=",
                           tp,
                           logLevelName(level)
                        );
            buf.append(ctx->taskCtx()->id());

            hana::eval_if(
                std::is_same<CloseT,hana::false_>{},
                [](auto){},
                [&](auto _){
                    fmt::format_to(std::back_inserter(_(buf))," DONE={}",_(ctx)->taskCtx()->finishMicroseconds());
                    hana::eval_if(
                        std::is_same<WithApiStatusT,hana::true_>{},
                        [&](auto _)
                        {
                            const char* status=common::ApiError::DefaultStatus;
                            if (_(ec))
                            {
                                if (_(ec).apiError()!=nullptr)
                                {
                                    status=_(ec).apiError()->apiStatus();
                                }
                                else
                                {
                                    status=_(ec).error();
                                }
                            }
                            _(buf).append(lib::string_view(" api_s="));
                            _(buf).append(lib::string_view(status));
                        },
                        [](auto){}
                    );
                }
            );

            hana::eval_if(
                std::is_same<ErrorT,hana::false_>{},
                [](auto){},
                [&](auto _)
                {
                    _(buf).append(lib::string_view(" err="));
                    _(ec).codeString(_(buf));
                }
            );

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
        }
};
using TextLogFormatter=TextLogFormatterT<>;

template <typename Traits, typename ContextT=Context>
class BufLoggerT : public LoggerHandlerT<ContextT>,
                   public common::WithTraits<Traits>
{
    public:

        using common::WithTraits<Traits>::WithTraits;

        void log(
            LogLevel level,
            const ContextT* ctx,
            common::pmr::string msg,
            lib::string_view module=lib::string_view{},
            common::pmr::vector<Record> records=common::pmr::vector<Record>{}
            ) override
        {
            auto bufWrapper=this->traits().prepareBuf();
            TextLogFormatterT<ContextT>::format(bufWrapper.buf(),
                                                level,ctx,msg,std::move(module),std::move(records)
                                                );
            this->traits().logBuf(bufWrapper);
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
            auto bufWrapper=this->traits().prepareBuf();
            TextLogFormatterT<ContextT>::format(bufWrapper.buf(),
                                                level,ctx,msg,std::move(module),std::move(records),
                                                ec
                                                );
            this->traits().logBufError(bufWrapper);
        }

        void logClose(
            LogLevel level,
            const Error& ec,
            const ContextT* ctx,
            common::pmr::string msg,
            lib::string_view module=lib::string_view{},
            common::pmr::vector<Record> records=common::pmr::vector<Record>{}
            ) override
        {
            auto bufWrapper=this->traits().prepareBuf();
            TextLogFormatterT<ContextT>::format(bufWrapper.buf(),
                                                level,ctx,msg,std::move(module),std::move(records),
                                                ec,
                                                hana::true_{}
                                                );
            this->traits().logBuf(bufWrapper);
        }

        void logCloseApi(
            LogLevel level,
            const Error& ec,
            const ContextT* ctx,
            common::pmr::string msg,
            lib::string_view module=lib::string_view{},
            common::pmr::vector<Record> records=common::pmr::vector<Record>{}
            ) override
        {
            auto bufWrapper=this->traits().prepareBuf();
            TextLogFormatterT<ContextT>::format(bufWrapper.buf(),
                                                level,ctx,msg,std::move(module),std::move(records),
                                                ec,
                                                hana::true_{},
                                                hana::true_{}
                                                );
            this->traits().logBuf(bufWrapper);
        }
};

HATN_LOGCONTEXT_NAMESPACE_END

#endif // HATNCTXBUFLOGGER_H
