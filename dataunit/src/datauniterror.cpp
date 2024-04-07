/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.
    
    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)
      
*/

/****************************************************************************/
/*
    
*/
/** @file dataunit/datauniterror.сpp
  *
  * Defines dataunit error classes.
  *
  */

#include <hatn/dataunit/datauniterror.h>

HATN_DATAUNIT_NAMESPACE_BEGIN

/********************** Error **************************/

//---------------------------------------------------------------


RawError& RawError::threadLocal()
{
    static thread_local RawError inst;
    return inst;
}

//---------------------------------------------------------------
HATN_DATAUNIT_NAMESPACE_END
