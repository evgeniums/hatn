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
#include <hatn/common/format.h>

HATN_COMMON_NAMESPACE_BEGIN

class ByteArray;
class NativeError;
class ApiError;

/**
 * @brief The Error class.
 *
 */
class HATN_COMMON_EXPORT HATN_NODISCARD Error final
{
    public:

        /**
         * @brief Type of the error.
         */
        enum class Type : uint8_t
        {
            Default, //!< Default type with ErrorCategory
            System, //!< std::error_code
            Boost, //!< boost::system::error_code
            Native //!< Native error with extended data.
        };

        //! Default constructor.
        template <typename T>
        Error(
                T code,
                const ErrorCategory* category=&CommonErrorCategory::getCategory()
            ) noexcept
            : m_code(static_cast<int>(code)),
              m_data(category)
        {}

        //! Constructor for std error category.
        template <typename T>
        Error(
            T code,
            const std::error_category* category
            ) noexcept
            : m_code(static_cast<int>(code)),
              m_data(category)
        {}

        //! Constructor for boost error category.
        template <typename T>
        Error(
                T code,
                const boost::system::error_category* category
              ) noexcept
            : m_code(static_cast<int>(code)),
              m_data(category)
        {}

        //! Constructor from native error.
        template <typename T>
        Error(
                T code,
                std::shared_ptr<NativeError>&& error
              ) noexcept
            : m_code(static_cast<int>(code)),
              m_data(std::move(error))
        {}

        Error(CommonError code=CommonError::OK) : Error(static_cast<int>(code))
        {}

        ~Error()=default;
        Error(const Error&)=default;
        Error(Error&&) =default;
        Error& operator=(const Error&)=default;
        Error& operator=(Error&&) =default;

        /**
         * @brief Check if this error of some type.
         * @param t Type to check for.
         * @return Operation result.
         */
        bool isType(Type t) const noexcept
        {
            return type()==t;
        }

        /**
         * @brief Get error type.
         * @return Error type.
         */
        Type type() const noexcept
        {
            return static_cast<Type>(lib::variantIndex(m_data));
        }

        //! Get error message.
        inline std::string message() const
        {
            FmtAllocatedBufferChar buf;
            message(buf);
            return fmtBufToString(buf);
        }

        /**
         * @brief Get single level code string if error is of default type.
         * @return String representation of error code.
         */
        inline const char* error() const
        {
            if (isType(Type::Default))
            {

                auto cat=lib::variantGet<const ErrorCategory*>(m_data);
                return cat->codeString(m_code);
            }

            return "";
        }

        /**
         * @brief Append string represetnation of error code to the buffer.
         * @param buf Buffer to append to.
         *
         * String representation depends on the error type and in some cases may be nested.
         */
        template <typename BufT>
        inline void codeString(BufT& buf) const
        {
            switch (type())
            {
                case(Type::Default):
                {
                    auto cat=lib::variantGet<const ErrorCategory*>(m_data);
                    defaultCatCodeString(cat,buf);
                }
                break;

                case(Type::System):
                {
                    auto cat=lib::variantGet<const std::error_category*>(m_data);
                    systemCatCodeString(cat,buf);
                }
                break;

                case(Type::Boost):
                {
                    auto cat=lib::variantGet<const boost::system::error_category*>(m_data);
                    boostCatCodeString(cat,buf);
                }
                break;

                case(Type::Native):
                {
                    const auto& nativeError=lib::variantGet<std::shared_ptr<NativeError>>(m_data);
                    if (nativeError)
                    {
                        nativeCodeString(nativeError,buf);
                    }
                }
                break;
            }
        }

        /**
         * @brief Get string representation of error code.
         * @return Operation result.
         */
        inline std::string codeString() const
        {
            FmtAllocatedBufferChar buf;
            codeString(buf);
            return fmtBufToString(buf);
        }

        /**
         * @brief Append error message to buffer.
         * @param buf Buffer to append to.
         */
        template <typename BufT>
        inline void message(BufT& buf) const
        {
            switch (type())
            {
                case(Type::Default):
                {
                    auto defaultCat=lib::variantGet<const ErrorCategory*>(m_data);
                    buf.append(defaultCat->message(m_code));
                }
                break;

                case(Type::System):
                {
                    auto systemCat=lib::variantGet<const std::error_category*>(m_data);
                    buf.append(systemCat->message(m_code));
                }
                break;

                case(Type::Boost):
                {
                    auto boostCat=lib::variantGet<const boost::system::error_category*>(m_data);
                    buf.append(lib::string_view(boostCat->message(m_code)));
                }
                break;

                case(Type::Native):
                {
                    const auto& nativeError=lib::variantGet<std::shared_ptr<NativeError>>(m_data);
                    if (nativeError)
                    {
                        nativeMessage(nativeError,buf);
                    }
                }
                break;
            }
        }

        //! Get error code.
        inline int value() const noexcept
        {
            return m_code;
        }

        //! Set error code.
        inline void setValue(int code) noexcept
        {
            m_code=code;
        }

        //! Get error code.
        inline int code() const noexcept
        {
            return m_code;
        }

        template <typename T>
        bool is(T code) const noexcept
        {
            return static_cast<int>(code)==m_code;
        }

        //! Set error code.
        template <typename T>
        inline void setCode(T code, const ErrorCategory* category=&CommonErrorCategory::getCategory()) noexcept
        {
            m_code=static_cast<int>(code);
            m_data=category;
        }

        //! Map to platform independent error code if applicable.
        inline int errorCondition() const noexcept
        {
            switch (type())
            {
                case(Type::Default):
                {
                    return m_code;
                }
                break;

                case(Type::System):
                {
                    auto systemCat=lib::variantGet<const std::error_category*>(m_data);
                    return systemCat->default_error_condition(m_code).value();
                }
                break;

                case(Type::Boost):
                {
                    auto boostCat=lib::variantGet<const boost::system::error_category*>(m_data);
                    return boostCat->default_error_condition(m_code).value();
                }
                break;

                case(Type::Native):
                {
                    const auto& nativeError=lib::variantGet<std::shared_ptr<NativeError>>(m_data);
                    if (nativeError)
                    {
                        return nativeErrorCondition(nativeError);
                    }
                }
                break;
            }

            return m_code;
        }

        //! Get native error.
        inline const NativeError* native() const noexcept
        {
            if (isType(Type::Native))
            {
                return lib::variantGet<std::shared_ptr<NativeError>>(m_data).get();
            }
            return nullptr;
        }

        //! Get native error.
        inline NativeError* native() noexcept
        {
            if (isType(Type::Native))
            {
                return lib::variantGet<std::shared_ptr<NativeError>>(m_data).get();
            }
            return nullptr;
        }

        //! Set native error.
        template <typename T>
        inline void setNative(T code, std::shared_ptr<NativeError>&& error) noexcept
        {
            m_code=static_cast<int>(code);
            m_data=std::move(error);
        }

        //! Set native error.
        inline void setNative(std::shared_ptr<NativeError>&& error) noexcept
        {
            m_data=std::move(error);
        }

        //! Get error category.
        inline const ErrorCategory* category() const
        {
            switch (type())
            {
                case(Type::Default):
                {
                    auto defaultCat=lib::variantGet<const ErrorCategory*>(m_data);
                    return defaultCat;
                }
                break;

                case(Type::Native):
                {
                    const auto& nativeError=lib::variantGet<std::shared_ptr<NativeError>>(m_data);
                    if (nativeError)
                    {
                        return nativeCategory(nativeError);
                    }
                }
                break;

                default:
                    return nullptr;
            }

            return nullptr;
        }

        //! Get system error category.
        inline const std::error_category* systemCategory() const
        {
            switch (type())
            {
                case(Type::Default):
                {
                    auto defaultCat=lib::variantGet<const ErrorCategory*>(m_data);
                    return defaultCat;
                }
                break;

                case(Type::System):
                {
                    auto systemCat=lib::variantGet<const std::error_category*>(m_data);
                    return systemCat;
                }
                break;

                case(Type::Native):
                {
                    const auto& nativeError=lib::variantGet<std::shared_ptr<NativeError>>(m_data);
                    if (nativeError)
                    {
                        return nativeSystemCategory(nativeError);
                    }
                }
                break;

                default:
                    return nullptr;
            }

            return nullptr;
        }

        //! Get boost error category.
        inline const boost::system::error_category* boostCategory() const
        {
            switch (type())
            {
                case(Type::Boost):
                {
                    auto boostCat=lib::variantGet<const boost::system::error_category*>(m_data);
                    return boostCat;
                }
                break;

                case(Type::Native):
                {
                    const auto& nativeError=lib::variantGet<std::shared_ptr<NativeError>>(m_data);
                    if (nativeError)
                    {
                        return nativeBoostCategory(nativeError);
                    }
                }
                break;

                default:
                    return nullptr;
            }

            return nullptr;
        }


        //! Bool operator.
        inline operator bool() const noexcept
        {
            return m_code!=static_cast<int>(CommonError::OK);
        }

        //! Comparison operator.
        inline bool operator ==(const Error& other) const noexcept
        {
            if (
                other.value()!=this->value()
                    ||
                other.type()!=this->type()
               )
            {
                return false;
            }

            if (!isType(Type::Native) && other.m_data!=this->m_data)
            {
                return false;
            }

            if (other.native()&&this->native())
            {
                return compareNative(other);
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
            m_data=&CommonErrorCategory::getCategory();
            m_code=static_cast<int>(CommonError::OK);
        }

        /**
         * @brief Get API error if applicable.
         * @return Pointer to API error object or nullptr.
         */
        const ApiError* apiError() const noexcept;

        /**
         * @brief Stack this error with next error.
         * @param next Next error.
         *
         * This error will be swapped with the next error with moving this error to the next error as a prev error.
         */
        void stackWith(Error&& next);

        void setPrevError(Error&& prev);

    private:

        bool compareNative(const Error& other) const noexcept;
        const ErrorCategory* nativeCategory(const std::shared_ptr<NativeError>& nativeError) const noexcept;
        const std::error_category* nativeSystemCategory(const std::shared_ptr<NativeError>& nativeError) const noexcept;
        const boost::system::error_category* nativeBoostCategory(const std::shared_ptr<NativeError>& nativeError) const noexcept;
        int nativeErrorCondition(const std::shared_ptr<NativeError>& nativeError) const noexcept;

        template <typename BufT>
        void nativeMessage(const std::shared_ptr<NativeError>& nativeError, BufT& buf) const;

        template <typename BufT>
        void nativeCodeString(const std::shared_ptr<NativeError>& nativeError, BufT& buf) const;

        int32_t m_code;

        lib::variant<const ErrorCategory*,
                     const std::error_category*,
                     const boost::system::error_category*,
                     std::shared_ptr<NativeError>
                     > m_data;

        template <typename BufT>
        inline void defaultCatCodeString(const ErrorCategory* cat, BufT& buf) const
        {
            buf.append(lib::string_view(cat->codeString(m_code)));
        }

        template <typename BufT>
        inline void systemCatCodeString(const std::error_category* cat, BufT& buf) const
        {
            fmt::format_to(std::back_inserter(buf),"std-{}-{}({})",cat->name(),m_code,cat->message(m_code));
        }

        template <typename BufT>
        inline void boostCatCodeString(const boost::system::error_category* cat, BufT& buf) const
        {
            fmt::format_to(std::back_inserter(buf),"boost-{}-{}({})",cat->name(),m_code,cat->message(m_code));
        }
};

//! hatn error exception
class HATN_COMMON_EXPORT ErrorException final : public std::runtime_error
{
    public:

        //! Ctor
        explicit ErrorException(
                Error error,
                const std::string& message=std::string()
            ) noexcept :
                std::runtime_error(message.empty()?error.message():message),
                m_error(std::move(error))
        {
        }

        //! Get error
        inline const Error& error() const noexcept
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
using CommonError=common::CommonError;
constexpr const CommonError OK{CommonError::OK};

//! Create Error object from common error code.
inline common::Error commonError(CommonError code) noexcept
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

//! Set error code.
template <typename T>
inline void setError(Error& ec, T code) noexcept
{
    ec.setCode(static_cast<int>(code));
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

#define HATN_BOOL_EC(ec) \
{\
    if (ec)\
    {\
        return false;\
    }\
}

#define HATN_BOOL_EC_MSG(ec,msg) \
{\
    if (ec)\
    {\
        msg=ec.message();\
        return !ec;\
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

#define HATN_CHECK_EMPTY_RETURN(ec) \
{\
    if (ec)\
    {\
        return;\
    }\
}

#endif // HATNERROR_H
