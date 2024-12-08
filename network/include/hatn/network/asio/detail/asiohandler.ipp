/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)
    
  */

/****************************************************************************/

/** @file network/asio/detail/asiohandler.ipp
  *
  */

/****************************************************************************/

#ifndef HATNASIOHANDLER_IPP
#define HATNASIOHANDLER_IPP

#include <hatn/common/taskcontext.h>
#include <hatn/common/weakptr.h>
#include <hatn/common/error.h>

#include <hatn/network/network.h>

HATN_NETWORK_NAMESPACE_BEGIN

namespace asio {

namespace detail {

template <typename Fn>
bool enterHandler(const common::WeakPtr<common::TaskContext>& wptr, const Fn& callback)
{
    auto ctx=wptr.lock();
    if (!ctx)
    {
        callback(commonError(CommonError::ABORTED));
        return false;
    }
    ctx->enterAsyncHandler();
    return true;
}

template <typename Fn>
bool enterHandler(const common::WeakPtr<common::TaskContext>& wptr, const Fn& callback, size_t size)
{
    auto ctx=wptr.lock();
    if (!ctx)
    {
        callback(commonError(CommonError::ABORTED),size);
        return false;
    }
    ctx->enterAsyncHandler();
    return true;
}

template <typename Fn>
bool enterHandler(const common::WeakPtr<common::TaskContext>& wptr, const Fn& callback, int size)
{
    return enterHandler(wptr,callback,static_cast<size_t>(size));
}

template <typename Fn, typename BuffersT>
bool enterHandler(const common::WeakPtr<common::TaskContext>& wptr, const Fn& callback, BuffersT&& buffers)
{
    auto ctx=wptr.lock();
    if (!ctx)
    {
        callback(commonError(CommonError::ABORTED),0,std::forward<BuffersT>(buffers));
        return false;
    }
    ctx->enterAsyncHandler();
    return true;
}

}

} // namespace asio

HATN_NETWORK_NAMESPACE_END

#endif // HATNASIOHANDLER_IPP
