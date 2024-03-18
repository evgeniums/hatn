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
#include <system_error>

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

        //! Default constructor for std error category.
        Error(
                int code=static_cast<int>(CommonError::OK),
                const std::error_category* category=&CommonErrorCategory::getCategory()
            ) noexcept
            : m_code(code),
              m_extended(category)
       {}

        //! Constructor for boost error category.
        Error(
                int code,
                const boost::system::error_category* category
              ) noexcept
            : m_code(code),
              m_extended(category)
        {}

        //! Constructor from native error.
        Error(
                std::shared_ptr<NativeError> error
              ) noexcept
            : m_code(error->value()),
              m_extended(std::move(error))
        {}

        ~Error()=default;
        Error(const Error&)=default;
        Error(Error&&) =default;
        Error& operator=(const Error&)=default;
        Error& operator=(Error&&) =default;

        //! Get message
        inline std::string message() const
        {
            switch (lib::variantIndex(m_extended))
            {
                case(0):
                {
                    auto systemCat=lib::variantGet<const std::error_category*>(m_extended);
                    return systemCat->message(m_code);
                }

                case(1):
                {
                    auto boostCat=lib::variantGet<const boost::system::error_category*>(m_extended);
                    return boostCat->message(m_code);
                }

                case(2):
                {
                    auto nativeError=lib::variantGet<std::shared_ptr<NativeError>>(m_extended);
                    if (nativeError)
                    {
                        return nativeError->message();
                    }
                }
            }

            return CommonErrorCategory::getCategory().message(m_code);
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
            if (lib::variantIndex(m_extended)==0)
            {
                auto systemCat=lib::variantGet<const std::error_category*>(m_extended);
                return systemCat->default_error_condition(m_code).value();
            }

            if (lib::variantIndex(m_extended)==1)
            {
                auto boostCat=lib::variantGet<const boost::system::error_category*>(m_extended);
                return boostCat->default_error_condition(m_code).value();
            }

            return m_code;
        }

        //! Get native error
        inline std::shared_ptr<NativeError> native() const noexcept
        {
            if (lib::variantIndex(m_extended)==2)
            {
                return lib::variantGet<std::shared_ptr<NativeError>>(m_extended);
            }
            return std::shared_ptr<NativeError>{};
        }

        //! Set native error
        inline void setNative(std::shared_ptr<NativeError> error) noexcept
        {
            if (m_code==static_cast<int>(CommonError::OK))
            {
                m_code=error->value();
            }
            m_extended=std::move(error);
        }

        //! Get system error category.
        inline const std::error_category* category() const
        {
            switch (lib::variantIndex(m_extended))
            {
                case(0):
                {
                    auto systemCat=lib::variantGet<const std::error_category*>(m_extended);
                    return systemCat;
                }

                case(2):
                {
                    auto nativeError=lib::variantGet<std::shared_ptr<NativeError>>(m_extended);
                    if (nativeError)
                    {
                        return nativeError->category();
                    }
                }
            }

            return nullptr;
        }

        //! Get boost error category.
        inline const boost::system::error_category* boostCategory() const
        {
            if (lib::variantIndex(m_extended)==1)
            {
                auto boostCat=lib::variantGet<const boost::system::error_category*>(m_extended);
                return boostCat;
            }
            return nullptr;
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

            if (lib::variantIndex(m_extended)!=2 && other.m_extended!=this->m_extended)
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

        lib::variant<const std::error_category*,
                     const boost::system::error_category*,
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

//---------------------------------------------------------------
HATN_COMMON_NAMESPACE_END

HATN_NAMESPACE_BEGIN

using Error=common::Error;

//! Create Error object from common error code.
inline common::Error commonError(ErrorCodes code) noexcept
{
    return common::Error(static_cast<int>(code));
}

//! Create Error object from std system error.
inline common::Error makeSystemError(const std::error_code& ec) noexcept
{
    return common::Error(ec.value(),&ec.category());
}

//! Create Error object from std system error code.
inline common::Error makeSystemError(std::errc ec) noexcept
{
    return common::Error(static_cast<int>(ec),&std::generic_category());
}

//! Create Error object from boost system error.
inline common::Error makeBoostError(const boost::system::error_code& ec) noexcept
{
    return common::Error(ec.value(),&ec.category());
}

//! Create Error object from std system error code.
inline common::Error makeBoostError(boost::system::errc::errc_t ec)
{
    return common::Error(static_cast<int>(ec),&boost::system::generic_category());
}

HATN_NAMESPACE_END

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
