/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/****************************************************************************/
/*

*/
/** @file mq/mqerror.сpp
  *
  *      Contains definition of error category for hatn API library.
  *
  */

#include <hatn/common/translate.h>

#include <hatn/mq/mqerror.h>

HATN_MQ_NAMESPACE_BEGIN

/********************** BaseErrorCategory **************************/

static MqErrorCategory MqErrorCategoryInstance;

//---------------------------------------------------------------
const MqErrorCategory& MqErrorCategory::getCategory() noexcept
{
    return MqErrorCategoryInstance;
}

//---------------------------------------------------------------
std::string MqErrorCategory::message(int code) const
{
    std::string result;
    switch (code)
    {
        HATN_MQ_ERRORS(HATN_ERROR_MESSAGE)

        default:
            result=_TR("unknown error");
    }

    return result;
}

//---------------------------------------------------------------
const char* MqErrorCategory::codeString(int code) const
{
    return errorString(code,MqErrorStrings);
}

//---------------------------------------------------------------

HATN_MQ_NAMESPACE_END
