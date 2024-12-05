/*
   Copyright (c) 2019 - current, Evgeny Sidorov (esid1976@gmail.com), All rights reserved
    
    
  */

/****************************************************************************/
/*
    
*/
/** \file network/error.h
  *
  *  Error classes for Hatn Network Library.
  *
  */

/****************************************************************************/

#ifndef HATNNETWORKERROR_H
#define HATNNETWORKERROR_H

#include <hatn/common/error.h>

#include <hatn/network/errorcodes.h>
#include <hatn/network/network.h>

namespace hatn {
    namespace network {

    //! Network error category
    class HATN_NETWORK_EXPORT NetworkErrorCategory : public common::ErrorCategory
    {
        public:

            //! Name of the category
            virtual const char *name() const noexcept
            {
                return "hatn.network";
            }

            //! Get description for the code
            virtual std::string message(int code) const;

            //! Get string representation of the code.
            virtual const char* codeString(int code) const
            {
                //! @todo Implement
                std::ignore=code;
                return nullptr;
            }

            //! Get category
            static const NetworkErrorCategory& getCategory() noexcept;
    };

    //! Make network error object from code
    inline ::hatn::common::Error makeError(ErrorCode code) noexcept
    {
        return ::hatn::common::Error(static_cast<int>(code),&NetworkErrorCategory::getCategory());
    }
}
}
#endif // HATNNETWORKERROR_H
