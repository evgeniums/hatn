/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/****************************************************************************/
/*

*/
/** @file api/genericerror.h
  *
  */

/****************************************************************************/

#ifndef HATNAPIGENERICERROR_H
#define HATNAPIGENERICERROR_H

#include <hatn/common/apierror.h>

#include <hatn/api/api.h>
#include <hatn/api/protocol.h>

HATN_API_NAMESPACE_BEGIN

//! Error category for hatnapi.
class HATN_API_EXPORT ApiGenericErrorCategory : public common::ApiErrorCategory
{
    public:

        //! Get string representation of the code.
        virtual const char* status(int code) const;

        //! Name of the category.
        virtual const char *family() const noexcept
        {
            return "hatn.api.generic";
        }

        //! Get description for the code.
        virtual std::string message(int code,const common::Translator* translator=nullptr) const;

        //! Get category
        static const ApiGenericErrorCategory& getCategory() noexcept;
};

//! Error codes of hatnapi lib.
using ApiGenericError = protocol::ResponseStatus;

inline const char* apiGenericErrorString(ApiGenericError code)
{
    return errorString(code,protocol::ResponseStatusStrings);
}

HATN_API_NAMESPACE_END

#endif // HATNAPIGENERICERROR_H
