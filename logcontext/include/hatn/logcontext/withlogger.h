/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.
    
    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)
      
*/

/****************************************************************************/
/*
    
*/
/** @file logcontext/withlogger.h
  *
  */

/****************************************************************************/

#ifndef HATNCONTEXTWITHLOGGER_H
#define HATNCONTEXTWITHLOGGER_H

#include <memory>

#include <hatn/logcontext/logcontext.h>
#include <hatn/logcontext/context.h>

HATN_LOGCONTEXT_NAMESPACE_BEGIN

template <typename HandlerT>
auto makeLogger(std::shared_ptr<HandlerT> handler)
{
    return std::make_shared<Logger>(std::move(handler));
}

class WithLogger
{
    public:

        WithLogger()=default;

        WithLogger(std::shared_ptr<Logger> logger) : m_logger(std::move(logger))
        {}

        template <typename HandlerT>
        WithLogger(std::shared_ptr<HandlerT> handler) : m_logger(makeLogger(std::move(handler)))
        {}

        Logger* logger() const noexcept
        {
            return m_logger.get();
        }

        std::shared_ptr<Logger> loggerShared() const
        {
            return m_logger;
        }

        void setLogger(std::shared_ptr<Logger> logger)
        {
            m_logger=std::move(logger);
        }

        template <typename HandlerT>
        void setHandler(std::shared_ptr<HandlerT> handler)
        {
            m_logger=makeLogger(std::move(handler));
        }

    private:

        std::shared_ptr<Logger> m_logger;
};

HATN_LOGCONTEXT_NAMESPACE_END

#endif // HATNCONTEXTWITHLOGGER_H
