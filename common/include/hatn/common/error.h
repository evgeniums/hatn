/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.
    
    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)
      
*/

/****************************************************************************/
/** @file common/error.h
  *
  *     Base error classes.
  *
  */

/****************************************************************************/

#ifndef HATNERROR_H
#define HATNERROR_H

#include <string>
#include <memory>

#include <boost/system/error_code.hpp>

#include <hatn/common/common.h>

HATN_COMMON_NAMESPACE_BEGIN

class ByteArray;
class Error;

//! Base class for error categories
class HATN_COMMON_EXPORT ErrorCategory
{
    public:

        //! Ctor
        ErrorCategory()=default;

        virtual ~ErrorCategory()=default;
        ErrorCategory(const ErrorCategory&)=delete;
        ErrorCategory(ErrorCategory&&) =delete;
        ErrorCategory& operator=(const ErrorCategory&)=delete;
        ErrorCategory& operator=(ErrorCategory&&) =delete;

        //! Name of the category
        virtual const char *name() const noexcept = 0;

        //! Get description for the code
        virtual std::string message(int code, const std::string& nativeMessage=std::string()) const = 0;

        inline bool operator==(const ErrorCategory &rhs) const noexcept { return this == &rhs; }
        inline bool operator!=(const ErrorCategory &rhs) const noexcept { return this != &rhs; }
        inline bool operator<( const ErrorCategory &rhs ) const noexcept
        {
            return std::less<const ErrorCategory*>()( this, &rhs );
        }
};

//! Common errors
enum class CommonError : int
{
    UNKNOWN=-1,
    OK=0,
    INVALID_SIZE=1,
    INVALID_ARGUMENT=2,
    UNSUPPORTED=3,
    INVALID_FILENAME=4,
    FILE_FLUSH_FAILED=5,
    FILE_ALREADY_OPEN=6,
    FILE_WRITE_FAILED=7,
    FILE_READ_FAILED=8,
    FILE_NOT_OPEN=9,
    TIMEOUT=10
};

//! Generic error category
class HATN_COMMON_EXPORT CommonErrorCategory : public ErrorCategory
{
    public:

        //! Name of the category
        virtual const char *name() const noexcept override
        {
            return "hatn.common";
        }

        //! Get description for the code
        virtual std::string message(int code, const std::string& nativeMessage=std::string()) const override;

        //! Get category
        static const CommonErrorCategory& getCategory() noexcept;
};

//! Base class for native errors
class HATN_COMMON_EXPORT NativeError
{
    public:

        //! Ctor
        NativeError()=default;

        virtual ~NativeError()=default;
        NativeError(const NativeError&)=default;
        NativeError(NativeError&&) =default;
        NativeError& operator=(const NativeError&)=default;
        NativeError& operator=(NativeError&&) =default;

        //! Message
        virtual std::string message() const
        {
            return std::string();
        }

        //! Code
        virtual int value() const noexcept
        {
            return 0;
        }

        //! Check if error is NULL
        virtual bool isNull() const noexcept
        {
            return true;
        }

        //! Bool operator
        inline operator bool() const noexcept
        {
            return !isNull();
        }

        //! Get category
        virtual const ErrorCategory* getCategory() const noexcept
        {
            return &CommonErrorCategory::getCategory();
        }

        //! Compare with other error
        inline bool isEqual(const NativeError& other) const noexcept
        {
            if (other.getCategory()!=this->getCategory()
               ||
                other.value()!=this->value()
               )
            {
                return false;
            }
            return compareContent(other);
        }

        bool operator ==(const NativeError& other) const noexcept
        {
            return isEqual(other);
        }
        bool operator !=(const NativeError& other) const noexcept
        {
            return !isEqual(other);
        }

        virtual Error serializeAppend(ByteArray& buf) const;

    protected:

        //! Compare self content with content of other error
        virtual bool compareContent(const NativeError& other) const noexcept
        {
            std::ignore=other;
            return false;
        }
};

#if __cplusplus >= 201703L && !(defined (__GNUC__) && defined (_WIN32))
    #define HATN_NODISCARD [[nodiscard]]
#else
    #define HATN_NODISCARD
#endif

//! Base class for errors
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
              m_category(category),
              m_systemCat(systemCat),
              m_boostCat(boostCat)
       {}

        //! Constructor
        Error(
                std::shared_ptr<NativeError> error
              ) noexcept
            : m_code(error->value()),
              m_category(error->getCategory()),
              m_native(std::move(error)),
              m_systemCat(nullptr),
              m_boostCat(nullptr)
        {}

        //! Ctor from code
        Error(CommonError code):Error(static_cast<int>(code))
        {}

        //! Get message
        std::string message() const
        {
            if (m_systemCat!=nullptr)
            {
                return m_systemCat->message(m_code);
            }
            else if (m_boostCat!=nullptr)
            {
                return m_boostCat->message(m_code);
            }
            else if (m_native)
            {
                return m_category->message(m_code,m_native->message());
            }
            return m_category->message(m_code);
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
            if (m_systemCat!=nullptr)
            {
                return m_systemCat->default_error_condition(m_code).value();
            }
            if (m_boostCat!=nullptr)
            {
                return m_boostCat->default_error_condition(m_code).value();
            }
            return m_code;
        }

        //! Get category
        inline const ErrorCategory* category() const noexcept
        {
            return m_category;
        }

        //! Get native error
        inline std::shared_ptr<NativeError> native() const noexcept
        {
            return m_native;
        }

        //! Set native error
        inline void setNative(std::shared_ptr<NativeError> error) noexcept
        {
            m_native=std::move(error);
            if (m_code==static_cast<int>(CommonError::OK))
            {
                m_code=m_native->value();
            }
        }

        //! Bool operator
        inline operator bool() const noexcept
        {
            return m_code!=static_cast<int>(CommonError::OK);
        }

        inline bool operator ==(const Error& other) const noexcept
        {
            if (other.category()!=this->category()
               ||
                other.value()!=this->value()
               )
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
            m_native.reset();
            m_systemCat=nullptr;
            m_boostCat=nullptr;
            m_category=&CommonErrorCategory::getCategory();
            m_code=static_cast<int>(CommonError::OK);
        }

        Error serialize(ByteArray& buf) const;

    private:

        int32_t m_code;
        const ErrorCategory* m_category;
        std::shared_ptr<NativeError> m_native;

        const std::error_category* m_systemCat;
        const boost::system::error_category* m_boostCat;
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

//! System error category
class HATN_COMMON_EXPORT SystemErrorCategory final : public CommonErrorCategory
{
    public:

        //! Name of the category
        virtual const char *name() const noexcept override
        {
            return "system";
        }

        //! Get category
        static const SystemErrorCategory& getCategory() noexcept;
};

//! Boost error category
class HATN_COMMON_EXPORT BoostErrorCategory final : public CommonErrorCategory
{
    public:

        //! Name of the category
        virtual const char *name() const noexcept override
        {
            return "boost";
        }

        //! Get category
        static const BoostErrorCategory& getCategory() noexcept;
};

//! Create Error object from std system error
inline common::Error makeSystemError(std::error_code ec) noexcept
{
    return common::Error(ec.value(),&SystemErrorCategory::getCategory(),&ec.category(),nullptr);
}

//! Create Error object from std system error code
inline common::Error makeSystemError(std::errc ec) noexcept
{
    return common::Error(static_cast<int>(ec),&SystemErrorCategory::getCategory(),&std::generic_category(),nullptr);
}

//! Create Error object from boost system error
inline common::Error makeBoostError(boost::system::error_code ec) noexcept
{
    return common::Error(ec.value(),&BoostErrorCategory::getCategory(),nullptr,&ec.category());
}

//! Create Error object from std system error code
inline common::Error makeBoostError(boost::system::errc::errc_t ec)
{
    return common::Error(static_cast<int>(ec),&BoostErrorCategory::getCategory(),nullptr,&boost::system::generic_category());
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
