/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.
    
    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)
      
*/

/****************************************************************************/
/*
    
*/
/** @file dataunit/datauniterror.h
  *
  *  Declares error classes.
  *
  */

/****************************************************************************/

#ifndef HATNDATAUNITERROR_H
#define HATNDATAUNITERROR_H

#include <string>

#include <hatn/common/error.h>
#include <hatn/common/nativeerror.h>

#include <hatn/dataunit/dataunit.h>

HATN_DATAUNIT_NAMESPACE_BEGIN

struct HATN_DATAUNIT_EXPORT RawError
{
    int code=0;
    std::string message;
    int field=-1;

    void reset() noexcept
    {
        code=0;
        message.clear();
        field=-1;
    }

    static RawError& threadLocal();
};

HATN_DATAUNIT_NAMESPACE_END

#endif // HATNDATAUNITERROR_H
