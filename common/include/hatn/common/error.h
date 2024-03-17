/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.
    
    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)
      
*/

/****************************************************************************/
/** @file common/error.h
  *
  *     Defines error class.
  *
  */

/****************************************************************************/

#ifndef HATNERROR_H
#define HATNERROR_H

#include <string>
#include <memory>

#include <fmt/format.h>
#include <boost/system/error_code.hpp>

#include <hatn/common/common.h>
#include <hatn/common/stdwrappers.h>
#include <hatn/common/errorcategory.h>
#include <hatn/common/nativeerror.h>

HATN_COMMON_NAMESPACE_BEGIN

class ByteArray;

//! Error class.
class HATN_COMMON_EXPORT HATN_NODISCARD Error final
{
    public:

        //! Constructor
        Error(int code=static_cast<int>(CommonError::OK),
              const ErrorCategory* category=&CommonErrorCategory::getCategory(),
              const std::error_category* systemCat=nullptr,
              const boost::system::error_category* boostCat=nullptr
            ) noexcept
            : m_code(code),
              m_extended(category)
       {
           if (systemCat!=nullptr)
           {
               m_extended=systemCat;
           }
           else if (boostCat!=nullptr)
           {
               m_extended=boostCat;
           }
       }

        //! Constructor
        Error(
                std::shared_ptr<NativeError> error
              ) noexcept
            : m_code(error->value()),
              m_extended(std::move(error))
        {}

        //! Ctor from code
        Error(CommonError code):Error(static_cast<int>(code))
        {}

        //! Get message
        std::string message() const
        {
            switch (lib::variantIndex(m_extended))
            {
                case(0):
                {
                    auto category=lib::variantGet<const ErrorCategory*>(m_extended);
                    return category->message(m_code);
                }

                case(1):
                {
                    auto boostCat=lib::variantGet<const boost::system::error_category*>(m_extended);
                    return boostCat->message(m_code);
                }

                case(2):
                {
                    auto systemCat=lib::variantGet<const std::error_category*>(m_extended);
                    return systemCat->message(m_code);
                }

                case(3):
                {
                    auto nativeError=lib::variantGet<std::shared_ptr<NativeError>>(m_extended);
                    if (nativeError)
                    {
                        return nativeError->message();
                    }
                }
            }

            return fmt::format("error code {}",m_code);
        }

        //! Get value
        inline int value() const noexcept
        {
            return m_code;
        }

        //! Set value
        inline void setValue(int code) noexcept
        {
            m_code=code;
        }

        //! Map to platform independent error code
        inline int errorCondition() const noexcept
        {
            if (lib::variantIndex(m_extended)==1)
            {
                auto boostCat=lib::variantGet<const boost::system::error_category*>(m_extended);
                return boostCat->default_error_condition(m_code).value();
            }

            if (lib::variantIndex(m_extended)==2)
            {
                auto systemCat=lib::variantGet<const std::error_category*>(m_extended);
                return systemCat->default_error_condition(m_code).value();
            }

            return m_code;
        }

        //! Get native error
        inline std::shared_ptr<NativeError> native() const noexcept
        {
            if (lib::variantIndex(m_extended)==3)
            {
                return lib::variantGet<std::shared_ptr<NativeError>>(m_extended);
            }
            return std::shared_ptr<NativeError>{};
        }

        //! Set native error
        inline void setNative(std::shared_ptr<NativeError> error) noexcept
        {
            m_extended=std::move(error);
            if (m_code==static_cast<int>(CommonError::OK))
            {
                m_code=error->value();
            }
        }

        //! Bool operator
        inline operator bool() const noexcept
        {
            return m_code!=static_cast<int>(CommonError::OK);
        }

        //! Comparison operator
        inline bool operator ==(const Error& other) const noexcept
        {
            if (
                other.value()!=this->value()
                    ||
                lib::variantIndex(other.m_extended)!=lib::variantIndex(this->m_extended)
               )
            {
                return false;
            }

            if (lib::variantIndex(m_extended)!=3 && other.m_extended!=this->m_extended)
            {
                return false;
            }

            if (other.native()&&this->native())
            {
                return *other.native()==*this->native();
            }

            return true;
        }

        inline bool operator !=(const Error& other) const noexcept
        {
            return !(*this==other);
        }

        inline bool isNull() const noexcept
        {
            return m_code==static_cast<int>(CommonError::OK);
        }

        inline void reset() noexcept
        {
            m_extended=&CommonErrorCategory::getCategory();
            m_code=static_cast<int>(CommonError::OK);
        }

        Error serialize(ByteArray& buf) const;

    private:

        int32_t m_code;

        lib::variant<const ErrorCategory*,
                     const boost::system::error_category*,
                     const std::error_category*,
                     std::shared_ptr<NativeError>
                     > m_extended;
};

//! hatn error exception
class HATN_COMMON_EXPORT ErrorException final : public std::runtime_error
{
    public:

        //! Ctor
        explicit ErrorException(
                Error error,
                std::string message=std::string()
            ) noexcept :
                std::runtime_error(message.empty()?error.message():message),
                m_error(std::move(error))
        {
        }

        //! Get error
        inline Error error() const noexcept
        {
            return m_error;
        }

    private:

        Error m_error;
};

//! Create Error object from std system error
inline common::Error makeSystemError(std::error_code ec) noexcept
{
    return common::Error(ec.value(),nullptr,&ec.category(),nullptr);
}

//! Create Error object from std system error code
inline common::Error makeSystemError(std::errc ec) noexcept
{
    return common::Error(static_cast<int>(ec),nullptr,&std::generic_category(),nullptr);
}

//! Create Error object from boost system error
inline common::Error makeBoostError(boost::system::error_code ec) noexcept
{
    return common::Error(ec.value(),nullptr,nullptr,&ec.category());
}

//! Create Error object from std system error code
inline common::Error makeBoostError(boost::system::errc::errc_t ec)
{
    return common::Error(static_cast<int>(ec),nullptr,nullptr,&boost::system::generic_category());
}

//---------------------------------------------------------------
HATN_COMMON_NAMESPACE_END

#define HATN_CHECK_RETURN(Eval) \
{\
    auto ec=Eval;\
    if (ec)\
    {\
        return ec;\
    }\
}

#define HATN_CHECK_EC(ec) \
{\
    if (ec)\
    {\
        return ec;\
    }\
}

#define HATN_CHECK_THROW(Eval) \
{\
    auto ec=Eval;\
    if (ec)\
    {\
        throw ::hatn::common::ErrorException(ec);\
    }\
}

#endif // HATNERROR_H
