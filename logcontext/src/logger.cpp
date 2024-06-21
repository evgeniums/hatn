/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/****************************************************************************/
/*

*/
/** @file logcontext/logger.сpp
  *
  *      Contains definition of logger.
  *
  */

#include <hatn/logcontext/logger.h>

HATN_LOGCONTEXT_NAMESPACE_BEGIN

//---------------------------------------------------------------

template class HATN_LOGCONTEXT_EXPORT LoggerBaseT<Context>;
template class HATN_LOGCONTEXT_EXPORT LoggerTraitsHandlerT<Context>;
template class HATN_LOGCONTEXT_EXPORT LoggerT<LoggerTraitsHandlerT,Context>;

//---------------------------------------------------------------

HATN_LOGCONTEXT_NAMESPACE_END
