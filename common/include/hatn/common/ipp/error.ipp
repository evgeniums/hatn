/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.
    
    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)
      
*/

/****************************************************************************/
/** @file common/ipp/error.ipp
 *
 *   Definitions of some templates for error handling.
 *
 */

#ifndef HATNERROR_IPP
#define HATNERROR_IPP

#include <hatn/common/error.h>
#include <hatn/common/nativeerror.h>

HATN_COMMON_NAMESPACE_BEGIN

//---------------------------------------------------------------

template <typename BufT>
void Error::nativeMessage(const std::shared_ptr<NativeError>& nativeError, BufT& buf) const
{
    if (nativeError->category()!=nullptr)
    {
        buf.append(nativeError->category()->message(m_code));
        buf.append(lib::string_view(": "));
    }
    else if (nativeError->systemCategory()!=nullptr)
    {
        buf.append(nativeError->systemCategory()->message(m_code));
        buf.append(lib::string_view(": "));
    }
    else if (nativeError->boostCategory()!=nullptr)
    {
        buf.append(nativeError->boostCategory()->message(m_code));
        buf.append(lib::string_view(": "));
    }
    nativeError->message(buf);
}

//---------------------------------------------------------------

template <typename BufT>
void Error::nativeCodeString(const std::shared_ptr<NativeError>& nativeError, BufT& buf) const
{
    if (nativeError->category()!=nullptr)
    {
        defaultCatCodeString(nativeError->category(),buf);
    }
    else if (nativeError->systemCategory()!=nullptr)
    {
        systemCatCodeString(nativeError->systemCategory(),buf);
    }
    else if (nativeError->boostCategory()!=nullptr)
    {
        boostCatCodeString(nativeError->boostCategory(),buf);
    }
    nativeError->codeString(buf);
}

//---------------------------------------------------------------

HATN_COMMON_NAMESPACE_END

#endif // HATNERROR_IPP
