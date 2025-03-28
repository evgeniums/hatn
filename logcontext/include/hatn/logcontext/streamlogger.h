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

#include <hatn/logcontext/context.h>
#include <hatn/logcontext/buflogger.h>

HATN_LOGCONTEXT_NAMESPACE_BEGIN

class BufToStream
{
    public:

        BufToStream(
                std::vector<std::ostream*> couts,
                std::vector<std::ostream*> cerrs
            ) : m_couts(std::move(couts)),
                m_cerrs(std::move(cerrs))
        {}

        BufToStream(
            std::ostream* cout=&std::cout,
            std::ostream* cerr=&std::cerr
            ) : BufToStream(std::vector<std::ostream*>{cout},std::vector<std::ostream*>{cerr})
        {}

        template <typename BufT>
        void sendTo(
            const BufT& buf,
            std::vector<std::ostream*>& streams
        )
        {
            for (auto&& os: streams)
            {
                std::copy(buf.begin(),buf.end(),std::ostream_iterator<char>(*os));
                *os<<std::endl;
            }
        }

        template <typename BufT>
        void log(
            const BufT& buf
        )
        {
            sendTo(buf,m_couts);
        }

        template <typename BufT>
        void logError(
            const BufT& buf
        )
        {
            sendTo(buf,m_cerrs);
        }

    private:

        std::vector<std::ostream*> m_couts;
        std::vector<std::ostream*> m_cerrs;
};

class StreamLoggerTraits : public BufToStream
{
    public:

        using BufToStream::BufToStream;

        struct BufWrapper
        {
            BufWrapper()
            {}

            common::FmtAllocatedBufferChar& buf()
            {
                return m_buf;
            }

            const common::FmtAllocatedBufferChar& buf() const
            {
                return m_buf;
            }

            common::FmtAllocatedBufferChar m_buf;
        };

        static BufWrapper prepareBuf()
        {
            return BufWrapper{};
        }

        void logBuf(const BufWrapper& bufWrapper)
        {
            this->log(bufWrapper.buf());
        }

        void logBufError(const BufWrapper& bufWrapper)
        {
            this->logError(bufWrapper.buf());
        }
};

constexpr const char* StreamLoggerName="streamlogger";

template <typename ContextT=Subcontext>
class StreamLoggerT : public BufLoggerT<StreamLoggerTraits,ContextT>
{
    public:

        template <typename ...TraitsArgs>
        StreamLoggerT(TraitsArgs&& ...traitsArgs) :
            BufLoggerT<StreamLoggerTraits,ContextT>(StreamLoggerName,std::forward<TraitsArgs>(traitsArgs)...)
        {}
};

using StreamLogger=StreamLoggerT<>;
using StreamLogHandler=StreamLogger;

HATN_LOGCONTEXT_NAMESPACE_END

#endif // HATNCTXSTREAMLOGGER_H
