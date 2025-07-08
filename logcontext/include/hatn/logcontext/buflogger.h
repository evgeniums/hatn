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
#include <hatn/common/meta/cstrlength.h>

#include <hatn/logcontext/context.h>
#include <hatn/logcontext/logger.h>

HATN_LOGCONTEXT_NAMESPACE_BEGIN

class TextLogFormatterBase
{
    public:

        template <typename BufT, typename RecordT>
        static void appendRecord(BufT& buf, const RecordT& rec)
        {
            buf.append(lib::string_view(" "));
            buf.append(rec.first);
            buf.append(lib::string_view("="));
            serializeValue(buf,rec.second);
        }
};

template <typename ContextT=Subcontext>
class TextLogFormatterT : public TextLogFormatterBase
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
            const char* msg,
            const lib::string_view& module,
            ErrorT ec=ErrorT{},
            CloseT =CloseT{},
            WithApiStatusT =WithApiStatusT{}
        )
        {
            // add time point and context ID
    #if __cplusplus >= 201703L
            auto tp=std::chrono::floor<std::chrono::microseconds>(ctx->mainCtx().now());
    #else
            auto tp=ctx->mainCtx().now();
    #endif
            fmt::format_to(std::back_inserter(buf),"{:%m%dT%H:%M:%S}"
                                                    " lvl={}"
                                                    " ctx=",
                           tp,
                           logLevelName(level)
                        );
            buf.append(ctx->mainCtx().id());
            if (!ctx->mainCtx().name().empty())
            {
                buf.append(lib::string_view(" op="));
                buf.append(ctx->mainCtx().name());
            }

            auto addStack=[&](bool full)
            {
                // add scope stack
                if (!ctx->scopeStack().empty())
                {
                    size_t maxIdx=0;
                    if (!std::is_same<CloseT,hana::false_>::value)
                    {
                        maxIdx=ctx->lockScopeIdx();
                        if (full || maxIdx==0)
                        {
                            maxIdx=ctx->scopeStack().size();
                        }
                        else
                        {
                            maxIdx=std::min(maxIdx,ctx->scopeStack().size());
                        }
                    }
                    else
                    {
                        maxIdx=ctx->scopeStack().size();
                    }

                    buf.append(lib::string_view(" stack=\""));
                    for (size_t i=0;i<maxIdx;i++)
                    {
                        const auto& scope=ctx->scopeStack().at(i);
                        if (i!=0)
                        {
                            buf.append(lib::string_view("."));
                        }
                        buf.append(lib::string_view(scope.first));

                        hana::eval_if(
                            std::is_same<ErrorT,hana::false_>{},
                            [](auto){},
                            [&](auto _)
                            {
                                if (_(ec) && !common::StrEmpty(_(scope).second.error))
                                {
                                    _(buf).append(lib::string_view("("));
                                    _(buf).append(lib::string_view(_(scope).second.error));
                                    _(buf).append(lib::string_view(")"));
                                }
                            }
                            );
                    }
                    buf.append(lib::string_view("\""));
                }
            };

            // check if it is a CLOSE request and add DONE with elapsed microseconds and api status if applicable
            bool resetStacks=false;
            hana::eval_if(
                std::is_same<CloseT,hana::false_>{},
                [&](auto _){
                    _(addStack)(true);
                },
                [&](auto _){
                    fmt::format_to(std::back_inserter(_(buf))," us={}",_(ctx)->mainCtx().finishMicroseconds());
                    hana::eval_if(
                        std::is_same<WithApiStatusT,hana::true_>{},
                        [&](auto _)
                        {
                            const char* status=common::ApiError::DefaultStatus;
                            if (_(ec))
                            {
                                if (_(ec).apiError()!=nullptr)
                                {
                                    status=_(ec).apiError()->status();
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
                    _(resetStacks)=true;

                    if (_(ec))
                    {
                        // add stack for error
                        _(addStack)(false);
                    }
                }
            );

            // add error string
            hana::eval_if(
                std::is_same<ErrorT,hana::false_>{},
                [](auto){},
                [&](auto _)
                {
                    if (_(ec))
                    {
                        _(buf).append(lib::string_view(" err="));
                        _(ec).codeString(_(buf));
                        if (_(ec).isType(common::Error::Type::Native))
                        {
                            _(buf).append(lib::string_view(" err_msg=\""));
                            _(buf).append(_(ec).message());
                            _(buf).append(lib::string_view("\""));
                        }
                        _(resetStacks)=false;
                    }
                }
            );

            // add message
            if (!common::StrEmpty(msg))
            {
                buf.append(lib::string_view(" msg=\""));
                buf.append(lib::string_view(msg));
                buf.append(lib::string_view("\""));
            }

            // add module
            if (!module.empty())
            {
                buf.append(lib::string_view(" mdl="));
                buf.append(module);
            }

            // reset stacks
            if (resetStacks)
            {
                auto* c=const_cast<ContextT*>(ctx);
                c->resetStacks();
            }

            // add context's fixed vars
            for (const auto& rec:ctx->fixedVars())
            {
                appendRecord(buf,rec);
            }

            // add context's map vars
            for (const auto& rec:ctx->globalVars())
            {
                appendRecord(buf,rec);
            }

            // add context's stack vars
            for (const auto& rec:ctx->stackVars())
            {
                appendRecord(buf,rec);
            }
        }

        template <typename BufT,
                 typename ErrorT=hana::false_,
                 typename CloseT=hana::false_,
                 typename WithApiStatusT=hana::false_>
        static void format(
            BufT& buf,
            LogLevel level,
            const ContextT* ctx,
            const char* msg,
            const ImmediateRecords& records,
            const lib::string_view& module,
            ErrorT ec=ErrorT{},
            CloseT ct=CloseT{},
            WithApiStatusT api=WithApiStatusT{}
            )
        {
            format(buf,level,ctx,msg,module,ec,ct,api);

            // add immediate records
            for (auto&& rec:records)
            {
                appendRecord(buf,rec);
            }
        }
};
using TextLogFormatter=TextLogFormatterT<>;

template <typename Traits, typename ContextT=Subcontext>
class BufLoggerT : public LoggerHandlerT<ContextT>,
                   public common::WithTraits<Traits>
{
    public:

        template <typename ...TraitsArgs>
        BufLoggerT(std::string name, TraitsArgs&& ...traitsArgs) :
            LoggerHandlerT<ContextT>(std::move(name)),
            common::WithTraits<Traits>(std::forward<TraitsArgs>(traitsArgs)...)
        {}

        void log(
            LogLevel level,
            const ContextT* ctx,
            const char* msg,            
            const ImmediateRecords& records,
            lib::string_view module=lib::string_view{}
            ) override
        {
            auto bufWrapper=this->traits().prepareBuf();
            TextLogFormatterT<ContextT>::format(bufWrapper.buf(),
                                                level,ctx,msg,records,module
                                                );
            this->traits().logBuf(bufWrapper);
            this->traits().releaseBuf(bufWrapper);
        }

        void log(
            LogLevel level,
            const ContextT* ctx,
            const char* msg,
            lib::string_view module=lib::string_view{}
            ) override
        {
            auto bufWrapper=this->traits().prepareBuf();
            TextLogFormatterT<ContextT>::format(bufWrapper.buf(),
                                                level,ctx,msg,module
                                                );
            this->traits().logBuf(bufWrapper);
            this->traits().releaseBuf(bufWrapper);
        }

        void logError(
            LogLevel level,
            const Error& ec,
            const ContextT* ctx,
            const char* msg,            
            const ImmediateRecords& records,
            lib::string_view module=lib::string_view{}
            ) override
        {
            auto bufWrapper=this->traits().prepareBuf();
            TextLogFormatterT<ContextT>::format(bufWrapper.buf(),
                                                level,ctx,msg,records,module,
                                                ec
                                                );
            this->traits().logBufError(bufWrapper);
            this->traits().releaseBuf(bufWrapper);
        }

        void logError(
            LogLevel level,
            const Error& ec,
            const ContextT* ctx,
            const char* msg,
            lib::string_view module=lib::string_view{}
            ) override
        {
            auto bufWrapper=this->traits().prepareBuf();
            TextLogFormatterT<ContextT>::format(bufWrapper.buf(),
                                                level,ctx,msg,module,
                                                ec
                                                );
            this->traits().logBufError(bufWrapper);
            this->traits().releaseBuf(bufWrapper);
        }

        void logClose(
            LogLevel level,
            const Error& ec,
            const ContextT* ctx,
            const char* msg,
            const ImmediateRecords& records,
            lib::string_view module=lib::string_view{}
            ) override
        {
            auto bufWrapper=this->traits().prepareBuf();
            TextLogFormatterT<ContextT>::format(bufWrapper.buf(),
                                                level,ctx,msg,records,module,
                                                ec,
                                                hana::true_{}
                                                );
            this->traits().logBuf(bufWrapper);
            this->traits().releaseBuf(bufWrapper);
        }

        void logClose(
            LogLevel level,
            const Error& ec,
            const ContextT* ctx,
            const char* msg,
            lib::string_view module=lib::string_view{}
            ) override
        {
            auto bufWrapper=this->traits().prepareBuf();
            TextLogFormatterT<ContextT>::format(bufWrapper.buf(),
                                                level,ctx,msg,module,
                                                ec,
                                                hana::true_{}
                                                );
            this->traits().logBuf(bufWrapper);
            this->traits().releaseBuf(bufWrapper);
        }

        void logCloseApi(
            LogLevel level,
            const Error& ec,
            const ContextT* ctx,
            const char* msg,
            const ImmediateRecords& records,
            lib::string_view module=lib::string_view{}
            ) override
        {
            auto bufWrapper=this->traits().prepareBuf();
            TextLogFormatterT<ContextT>::format(bufWrapper.buf(),
                                                level,ctx,msg,records,module,
                                                ec,
                                                hana::true_{},
                                                hana::true_{}
                                                );
            this->traits().logBuf(bufWrapper);
            this->traits().releaseBuf(bufWrapper);
        }

        void logCloseApi(
            LogLevel level,
            const Error& ec,
            const ContextT* ctx,
            const char* msg,
            lib::string_view module=lib::string_view{}
            ) override
        {
            auto bufWrapper=this->traits().prepareBuf();
            TextLogFormatterT<ContextT>::format(bufWrapper.buf(),
                                                level,ctx,msg,module,
                                                ec,
                                                hana::true_{},
                                                hana::true_{}
                                                );
            this->traits().logBuf(bufWrapper);
            this->traits().releaseBuf(bufWrapper);
        }
};

HATN_LOGCONTEXT_NAMESPACE_END

#endif // HATNCTXBUFLOGGER_H
