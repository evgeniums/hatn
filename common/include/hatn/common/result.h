/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/****************************************************************************/

/** @file common/result.h
  *
  * Contains simple alternative to std::expected returning either value or hatn::common::Error.
  *
  */

/****************************************************************************/

#ifndef HATNRESULT_H
#define HATNRESULT_H

#include <type_traits>

#include <boost/hana.hpp>

#include <hatn/common/error.h>
#include <hatn/common/utils.h>

namespace hana=boost::hana;

HATN_COMMON_NAMESPACE_BEGIN

namespace result_detail
{

/**
 * @brief Tag for Result types.
 */
struct ResultTag
{
    template <typename T>
    friend bool operator == (const T&, const ResultTag&) noexcept
    {
        return true;
    }
    template <typename T>
    friend bool operator != (const T&, const ResultTag&) noexcept
    {
        return false;
    }

    template <typename T>
    bool operator == (const T&) noexcept
    {
        return true;
    }
    template <typename T>
    bool operator != (const T&) noexcept
    {
        return false;
    }
};

//! Helper class for reference binding in Result.
template <typename T, typename T1=void>
struct DefaultResult
{};

//! @note This specialization is used when type is default constructible only for reference binding in case of error. Thus, this value must never be used.
template <typename T>
struct DefaultResult<T,std::enable_if_t<std::is_default_constructible<std::decay_t<T>>::value>>
{
    using storageType=std::decay_t<T>;
    static storageType* value()
    {
        static storageType storage;
        return &storage;
    }
};

//! @note This specialization is used when type is not default constructible only for reference binding in case of error. Thus, this value must never be used.
template <typename T>
struct DefaultResult<T,std::enable_if_t<!std::is_default_constructible<std::decay_t<T>>::value>>
{
    using storageType=std::decay_t<T>;
    static storageType* value()
    {
        static std::aligned_storage_t<sizeof(storageType), alignof(storageType)> storage;
        return reinterpret_cast<storageType*>(&storage);
    }
};
}


/**
 * @brief The Result class is a simple alternative to std::expected or similar classes.
 * In addition to success value it holds hana::common::Error variable.
 */
template <typename T, typename T1=void>
class Result
{
    public:

        using hana_tag=result_detail::ResultTag;
        using type=T;

        /**
         * @brief Constructor with value.
         * @param value Result value to wrap.
         */
        Result(
                T&& value
            ) : m_value(std::forward<T>(value))
        {}

        /**
         * @brief Constructor with error.
         * @param error Result error.
         *
         * @throws ErrorException if error is null.
         */
        Result(
            Error&& error
            ) : m_error(std::move(error))
        {
            if (m_error.isNull())
            {
                throw ErrorException{commonError(CommonError::RESULT_NOT_ERROR)};
            }
        }

        /**
         * @brief Constructor with error.
         * @param error Result error.
         *
         * @throws ErrorException if error is null.
         */
        Result(
            const Error& error
            ) : m_error(error)
        {
            if (m_error.isNull())
            {
                throw ErrorException{commonError(CommonError::RESULT_NOT_ERROR)};
            }
        }

        /**
         * @brief Move constructor for the same type.
         * @param other Other result of the same type.
         */
        Result(
            Result&& other
            ) :
                m_value(std::move(other.m_value)),
                m_error(std::move(other.m_error))
        {}

        /**
         * @brief Move constructor from other error result.
         * @param other Other result with error to move from.
         *
         * @throws ErrorException if other result is valid.
         */
        template <typename T2>
        explicit Result(
            Result<T2>&& other
            ) : m_error(std::move(other.m_error))
        {
            Assert(m_error.isNull(),"cannot move not error result");
        }

        /**
         * @brief Constructor with value emplacement.
         * @param args Arguments to value constructor.
         */
        template <typename ...Args>
        Result(Args&& ...args) : m_value(std::forward<Args>(args)...)
        {}

        /**
         * @brief Move assignment operator.
         * @param other Other result of the same type.
         */
        Result& operator =(Result&& other)
        {
            if (&other!=this)
            {
                m_value=std::move(other.m_value);
                m_error=std::move(other.m_error);
            }
            return *this;
        }

        ~Result()=default;

        Result()=delete;
        Result(const Result&)=delete;
        Result& operator =(const Result&)=delete;

        /**
         * @brief Take wrapped value.
         * @return Moves wrapped value to caller.
         */
        T takeWrappedValue()
        {
            return std::move(m_value);
        }

        /**
         * @brief Take wrapped value.
         * @return Wrapped value if result is not error, throws otherwise.
         *
         * @throws ErrorException if result is not valid.
         */
        T takeValue()
        {
            checkThrowInvalid();
            return takeWrappedValue();
        }

        /**
         * @brief Dereference operator ->.
         * @return Poniter to wrapped value if result is not error, throws otherwise.
         * @throws ErrorException if result is not valid.
         */
        auto operator->() const -> decltype(auto)
        {
            checkThrowInvalid();
            return &m_value;
        }

        /**
         * @brief Dereference operator *.
         * @return Wrapped value if result is not error, throws otherwise.
         * @throws ErrorException if result is not valid.
         */
        const T& operator*() const
        {
            checkThrowInvalid();
            return m_value;
        }

        /**
         * @brief Dereference operator ->.
         * @return Poniter to wrapped value if result is not error, throws otherwise.
         * @throws ErrorException if result is not valid.
         */
        auto operator->() -> decltype(auto)
        {
            checkThrowInvalid();
            return &m_value;
        }

        /**
         * @brief Dereference operator *.
         * @return Wrapped value if result is not error, throws otherwise.
         * @throws ErrorException if result is not valid.
         */
        T& operator*()
        {
            checkThrowInvalid();
            return m_value;
        }

        /**
         * @brief Get const reference to value.
         * @return Const reference to wrapped value if result is not error, throws otherwise.
         * @throws ErrorException if result is not valid.
         */
        const T& value() const
        {
            checkThrowInvalid();
            return m_value;
        }

        /**
         * @brief Get reference to value.
         * @return Reference to wrapped value if result is not error, throws otherwise.
         * @throws ErrorException if result is not valid.
         */
        T& value()
        {
            checkThrowInvalid();
            return m_value;
        }

        //! Check if result is valid.
        bool isValid() const noexcept
        {
            return m_error.isNull();
        }

        //! Get error.
        const Error& error() const noexcept
        {
            return m_error;
        }

        //! Take error.
        Error takeError() noexcept
        {
            return std::move(m_error);
        }

        //! Bool operator true if result holds error.
        operator bool() const noexcept
        {
            return static_cast<bool>(m_error);
        }

        //! Convert to error.
        operator Error() const noexcept
        {
            return m_error;
        }

    private:

        void checkThrowInvalid() const
        {
            if (!isValid())
            {
                throw ErrorException{commonError(CommonError::RESULT_ERROR)};
            }
        }

        T m_value;
        Error m_error;

        template <typename T3, typename T4> friend class Result;
};

/**
 * @brief The Result class specialization when value type is lvalue reference.
 */
template <typename T>
class Result<T,std::enable_if_t<std::is_lvalue_reference<T>::value>>
{
    public:

        using hana_tag=result_detail::ResultTag;
        using type=T;

        /**
         * @brief Constructor with value.
         * @param value Result value to wrap.
         */
        Result(
            T&& value
            ) : m_value(std::forward<T>(value))
        {}

        /**
         * @brief Constructor with error.
         * @param error Result error.
         *
         * @throws ErrorException if error is null.
         */
        Result(
            Error error
            ) : m_value(*result_detail::DefaultResult<T>::value()),
                m_error(std::move(error))
        {
            if (m_error.isNull())
            {
                throw ErrorException{commonError(CommonError::RESULT_NOT_ERROR)};
            }
        }

        /**
         * @brief Move constructor from other error result.
         * @param other Other result with error to move from.
         *
         * @throws ErrorException if error is null.
         */
        template <typename T1>
        explicit Result(
            Result<T1>&& other
            ) : m_value(*result_detail::DefaultResult<T>::value()),
            m_error(std::move(other.m_error))
        {
            if (m_error.isNull())
            {
                throw ErrorException{commonError(CommonError::RESULT_NOT_ERROR)};
            }
        }

        /**
         * @brief Move constructor.
         * @param other Other result of the same type.
         */
        Result(
            Result&& other
            ) :
            m_value(other.m_value),
            m_error(std::move(other.m_error))
        {}

        /**
             * @brief Move assignment operator.
             * @param other Other result of the same type.
             */
        Result& operator =(Result&& other)
        {
            if (&other!=this)
            {
                m_value=other.m_value;
                m_error=std::move(other.m_error);
            }
            return *this;
        }

        ~Result()=default;

        Result()=delete;
        Result(const Result&)=delete;
        Result& operator =(const Result&)=delete;

        /**
         * @brief Take wrapped value.
         * @return Reference value as is.
         */
        T takeWrappedValue()
        {
            return m_value;
        }

        /**
         * @brief Take wrapped value.
         * @return Wrapped value if result is not error, throws otherwise.
         *
         * @throws ErrorException if result is not valid.
         */
        T takeValue()
        {
            checkThrowInvalid();
            return takeWrappedValue();
        }

        /**
         * @brief Dereference operator ->.
         * @return Poniter to wrapped value if result is not error, throws otherwise.
         * @throws ErrorException if result is not valid.
         */
        auto operator->() const -> decltype(auto)
        {
            checkThrowInvalid();
            return &m_value;
        }

        /**
         * @brief Dereference operator *.
         * @return Wrapped value if result is not error, throws otherwise.
         * @throws ErrorException if result is not valid.
         */
        auto operator*() const -> decltype(auto)
        {
            checkThrowInvalid();
            return m_value;
        }

        /**
         * @brief Dereference operator ->.
         * @return Poniter to wrapped value if result is not error, throws otherwise.
         * @throws ErrorException if result is not valid.
         */
        auto operator->() -> decltype(auto)
        {
            checkThrowInvalid();
            return &m_value;
        }

        /**
         * @brief Dereference operator *.
         * @return Wrapped value if result is not error, throws otherwise.
         * @throws ErrorException if result is not valid.
         */
        auto operator*() -> decltype(auto)
        {
            checkThrowInvalid();
            return m_value;
        }

        /**
         * @brief Get value.
         * @return Wrapped reference value if result is not error, throws otherwise.
         * @throws ErrorException if result is not valid.
         */
        auto value() const -> decltype(auto)
        {
            checkThrowInvalid();
            return m_value;
        }

        /**
         * @brief Get value.
         * @return Wrapped reference value if result is not error, throws otherwise.
         * @throws ErrorException if result is not valid.
         */
        auto value() -> decltype(auto)
        {
            checkThrowInvalid();
            return m_value;
        }

        //! Check if result is valid.
        bool isValid() const noexcept
        {
            return m_error.isNull();
        }

        //! Get error.
        const Error& error() const noexcept
        {
            return m_error;
        }

        //! Take error.
        Error takeError() noexcept
        {
            return std::move(m_error);
        }

        //! Bool operator true if result holds error.
        operator bool() const noexcept
        {
            return static_cast<bool>(m_error);
        }

        //! Convert to error.
        operator Error() const noexcept
        {
            return m_error;
        }

    private:

        void checkThrowInvalid() const
        {
            if (!isValid())
            {
                throw ErrorException{commonError(CommonError::RESULT_ERROR)};
            }
        }

        T m_value;
        Error m_error;

        template <typename T1, typename T2> friend class Result;
};

/**
 * @brief The Result class specialization for errors.
 */
template <>
class Result<Error>
{
    public:

        using hana_tag=result_detail::ResultTag;
        using type=Error;

        /**
         * @brief Constructor with error.
         * @param error Result error.
         *
         * @throws ErrorException if error is null.
         */
        Result(
                Error error
            ) : m_error(std::move(error))
        {
            if (m_error.isNull())
            {
                throw ErrorException{commonError(CommonError::RESULT_NOT_ERROR)};
            }
        }

        /**
         * @brief Move constructor from other error result.
         * @param other Other result with error to move from.
         *
         * @throws ErrorException if other error is null.
         */
        template <typename T1>
        Result(
            Result<T1>&& other
          ) : m_error(std::move(other.m_error))
        {
            if (m_error.isNull())
            {
                throw ErrorException{commonError(CommonError::RESULT_NOT_ERROR)};
            }
        }

        /**
         * @brief Move constructor.
         * @param other Other result of the same type.
         */
        Result(
            Result&& other
            ) : m_error(std::move(other.m_error))
        {}

        /**
         * @brief Move assignment operator.
         * @param other Other result of the same type.
         */
        Result& operator =(Result&& other)
        {
            if (&other!=this)
            {
                m_error=std::move(other.m_error);
            }
            return *this;
        }

        ~Result()=default;

        Result()=delete;
        Result(const Result&)=delete;
        Result& operator =(const Result&)=delete;

        /**
         * @brief Take wrapped value.
         * @return Moves wrapped value to caller.
         */
        Error& takeWrappedValue()
        {
            return m_error;
        }

        /**
         * @brief Take wrapped value.
         * @return Wrapped value if result is not error, throws otherwise.
         *
         * @throws ErrorException always becaus result is not valid.
         */
        Error& takeValue()
        {
            checkThrowInvalid();
            return takeWrappedValue();
        }

        //! Check if result is valid.
        bool isValid() const noexcept
        {
            return false;
        }

        //! Get error.
        const Error& error() const noexcept
        {
            return m_error;
        }

        //! Take error.
        Error takeError() noexcept
        {
            return std::move(m_error);
        }

        //! Bool operator true if result holds error.
        operator bool() const noexcept
        {
            return true;
        }

        //! Convert to error.
        operator Error() const noexcept
        {
            return m_error;
        }

    private:

        void checkThrowInvalid() const
        {
            if (!isValid())
            {
                throw ErrorException{commonError(CommonError::RESULT_ERROR)};
            }
        }

        Error m_error;

        template <typename T1, typename T2> friend class Result;
};

namespace result_detail
{
/**
 * @brief Wrap a value into result.
 * @param v Value.
 * @return Result if value is not a Result already, otherwise value as is.
 */
struct MakeResultImpl
{
    template <typename T>
    auto operator () (T&& v) const -> decltype(auto)
    {
        return hana::eval_if(
            hana::is_a<ResultTag,T>,
            [&](auto&& _) -> decltype(auto)
            {
                return hana::id(_(v));
            },
            [&](auto&& _)
            {
                return Result<T>(std::forward<T>(_(v)));
            }
            );
    }
};

}

HATN_COMMON_NAMESPACE_END

HATN_NAMESPACE_BEGIN

template <typename T1, typename T2=void>
using Result=common::Result<T1,T2>;
using ErrorResult=common::Result<Error>;
constexpr common::result_detail::MakeResultImpl makeResult{};

template <typename T, typename ...Args>
auto emplaceResult(Args&& ...args) -> decltype(auto)
{
    return Result<T>(std::forward<Args>(args)...);
}

#define HATN_CHECK_RESULT(r) \
{\
    if (r)\
    {\
        return r.takeError();\
    }\
}

#define HATN_BOOL_RESULT(r) HATN_BOOL_EC(r)

#define HATN_BOOL_RESULT_MSG(r,msg) \
{\
    if (r)\
    {\
        msg=r.error().message();\
        return !r;\
    }\
}

#define HATN_RESULT_EC(r,ec) \
if (r) \
{\
    ec=r.error();\
    return r.takeWrappedValue();\
}

#define HATN_RESULT_THROW(r) \
if (r) \
{\
    throw common::ErrorException{r.error()};\
}

HATN_NAMESPACE_END

#endif // HATNRESULT_H
