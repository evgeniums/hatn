/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.
    
    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)
      
*/

/****************************************************************************/
/** @file common/asiotimer.сpp
  *
  *     Timer classes with asio backend
  *
  */

#include <hatn/common/thread.h>
#include <hatn/common/weakptr.h>
#include <hatn/common/asiotimer.h>

HATN_COMMON_NAMESPACE_BEGIN

namespace
{

inline void expiresFromNow(boost::asio::deadline_timer& timer,uint64_t period)
{
    timer.expires_from_now(boost::posix_time::microseconds(period));
}

inline void expiresFromNow(boost::asio::high_resolution_timer& timer,uint64_t period)
{
    timer.expires_from_now(std::chrono::microseconds(period));
}

}

//---------------------------------------------------------------
template <typename NativeT,typename TimerT>
void AsioTimerTraits<NativeT,TimerT>::invokeHandler(const boost::system::error_code& ec)
{
    m_running=false;
    if (ec)
    {
        if (m_timer->handler())
        {
            m_timer->handler()(TimerT::Status::Cancel);
        }
    }
    else
    {
        if (m_timer->handler())
        {
            m_timer->handler()(TimerT::Status::Timeout);
            if (!m_timer->isSingleShot())
            {
                m_timer->start();
            }
        }
    }
}

//---------------------------------------------------------------
template <typename NativeT,typename TimerT>
void AsioTimerTraits<NativeT,TimerT>::start()
{
    m_running=true;
    expiresFromNow(m_native,m_periodUs);

    if (m_autoAsyncGuard)
    {
        auto sharedTimer=m_timer->sharedFromThis();
        Assert(sharedTimer,"Asio timer must be used within SharedPtr only");
        m_native.async_wait(
                [this,timer{toWeakPtr(sharedTimer)}](const boost::system::error_code& ec)
                {
                    auto t=timer.lock();
                    if (t.isNull())
                    {
                        return;
                    }
                    invokeHandler(ec);
                }
            );
    }
    else
    {
        m_native.async_wait(
            [this](const boost::system::error_code& ec)
            {
                invokeHandler(ec);
            }
        );
    }
}

//---------------------------------------------------------------

template class HATN_COMMON_EXPORT AsioTimerTraits<boost::asio::deadline_timer,AsioDeadlineTimer>;
template class HATN_COMMON_EXPORT AsioTimerTraits<boost::asio::high_resolution_timer,AsioHighResolutionTimer>;

//---------------------------------------------------------------
HATN_COMMON_NAMESPACE_END
