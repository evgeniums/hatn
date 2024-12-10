/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)
    
  */

/****************************************************************************/

/** @file network/detail/asynchandler.ipp
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

namespace detail {

template <typename Fn>
bool enterAsyncHandler(const common::WeakPtr<common::TaskContext>& wptr, const Fn& callback)
{
    auto ctx=wptr.lock();
    if (!ctx)
    {
        callback(commonError(CommonError::ABORTED));
        return false;
    }
    ctx->onAsyncHandlerEnter();
    return true;
}

template <typename Fn>
bool enterAsyncHandler(const common::WeakPtr<common::TaskContext>& wptr, const Fn& callback, size_t size)
{
    auto ctx=wptr.lock();
    if (!ctx)
    {
        callback(commonError(CommonError::ABORTED),size);
        return false;
    }
    ctx->onAsyncHandlerEnter();
    return true;
}

template <typename Fn>
bool enterAsyncHandler(const common::WeakPtr<common::TaskContext>& wptr, const Fn& callback, int size)
{
    return enterAsyncHandler(wptr,callback,static_cast<size_t>(size));
}

template <typename Fn, typename ArgT>
bool enterAsyncHandler(const common::WeakPtr<common::TaskContext>& wptr, const Fn& callback, ArgT&& arg)
{
    auto ctx=wptr.lock();
    if (!ctx)
    {
        callback(commonError(CommonError::ABORTED),0,std::forward<ArgT>(arg));
        return false;
    }
    ctx->onAsyncHandlerEnter();
    return true;
}

inline void leaveAsyncHandler(common::TaskContext& ctx)
{
    ctx.onAsyncHandlerExit();
}

}

HATN_NETWORK_NAMESPACE_END

#endif // HATNASIOHANDLER_IPP
