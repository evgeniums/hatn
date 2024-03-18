/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/****************************************************************************/
/*

*/
/** @file base/result.h
  *
  * Contains simple alternative to std::expected returning either value or error.
  *
  */

/****************************************************************************/

#ifndef HATNBASERESULT_H
#define HATNBASERESULT_H

#include <type_traits>

#include <boost/hana.hpp>

#include <hatn/common/error.h>

#include <hatn/base/base.h>
#include <hatn/base/baseerror.h>

namespace hana=boost::hana;

HATN_BASE_NAMESPACE_BEGIN

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

template <typename T, typename T1=void>
struct DefaultResult
{
    static const std::decay_t<T>* value()
    {
        static auto storage=std::decay_t<T>{};
        return &storage;
    }
};

template <typename T>
struct DefaultResult<T,std::enable_if_t<std::is_reference<T>::value && !std::is_const<std::remove_reference_t<T>>::value>>
{
    static std::decay_t<T>* value()
    {
        static auto storage=std::decay_t<T>{};
        return &storage;
    }
};

template <typename T, typename T1=void> struct MoveReference
{
    template <typename T2>
    constexpr static auto f(T2&& v) -> decltype(auto)
    {
        return static_cast<T2&&>(v);
    }
};

template <typename T> struct MoveReference<T,std::enable_if_t<std::is_lvalue_reference<T>::value>>
{
    template <typename T2>
    constexpr static T f(T2&& v)
    {
        return v;
    }
};

template <typename T>
class Result
{
    public:

        using hana_tag=ResultTag;
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
         */
        Result(
            common::Error error
            ) : m_value(*DefaultResult<T>::value()),
                m_error(std::move(error))
        {
            if (m_error.isNull())
            {
                throw common::ErrorException{makeError(ErrorCode::RESULT_NOT_ERROR)};
            }
        }

        /**
         * @brief Move constructor from other error result.
         * @param other Other result with error to move from.
         */
        template <typename T1>
        Result(
            Result<T1>&& other
            ) : m_value(*DefaultResult<T>::value()),
                m_error(std::move(other.m_error))
        {
            if (m_error.isNull())
            {
                throw common::ErrorException{makeError(ErrorCode::RESULT_NOT_ERROR)};
            }
        }

        /**
         * @brief Move constructor.
         * @param other Other result of the same type.
         */
        Result(
            Result&& other
            ) : m_value(MoveReference<decltype(m_value)>::f(std::move(other.m_value))),
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
                m_value=MoveReference<decltype(m_value)>::f(std::move(other.m_value));
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
            return MoveReference<decltype(m_value)>::f(std::move(m_value));
        }

        /**
         * @brief Take wrapped value.
         * @return Wrapped value if result is not error, throws otherwise.
         */
        T takeValue()
        {
            if (!isValid())
            {
                throw common::ErrorException{makeError(ErrorCode::RESULT_ERROR)};
            }
            return takeWrappedValue();
        }

        //! Check if result is valid.
        bool isValid() const noexcept
        {
            return m_error.isNull();
        }

        //! Get error.
        const common::Error& error() const noexcept
        {
            return m_error;
        }

        //! Bool operator true if result holds error.
        operator bool() const noexcept
        {
            return static_cast<bool>(m_error);
        }

    private:

        T m_value;
        common::Error m_error;

        template <typename T1> friend class Result;
};

template <>
class Result<common::Error>
{
public:

    using hana_tag=ResultTag;
    using type=common::Error;

    /**
     * @brief Constructor with error.
     * @param error Result error.
     */
    Result(
            common::Error error
        ) : m_error(std::move(error))
    {
        if (m_error.isNull())
        {
            throw common::ErrorException{makeError(ErrorCode::RESULT_NOT_ERROR)};
        }
    }

    /**
     * @brief Move constructor from other error result.
     * @param other Other result with error to move from.
     */
    template <typename T1>
    Result(
        Result<T1>&& other
      ) : m_error(std::move(other.m_error))
    {
        if (m_error.isNull())
        {
            throw common::ErrorException{makeError(ErrorCode::RESULT_NOT_ERROR)};
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
    common::Error& takeWrappedValue()
    {
        return m_error;
    }

    /**
     * @brief Take wrapped value.
     * @return Wrapped value if result is not error, throws otherwise.
     */
    common::Error& takeValue()
    {
        if (!isValid())
        {
            throw common::ErrorException{makeError(ErrorCode::RESULT_ERROR)};
        }
        return takeWrappedValue();
    }

    //! Check if result is valid.
    bool isValid() const noexcept
    {
        return false;
    }

    //! Get error.
    const common::Error& error() const noexcept
    {
        return m_error;
    }

    //! Bool operator true if result holds error.
    operator bool() const noexcept
    {
        return true;
    }

private:

    common::Error m_error;

    template <typename T1> friend class Result;
};
using ErrorResult=Result<common::Error>;

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
constexpr MakeResultImpl makeResult{};

//! Make error result.
ErrorResult errorResult(ErrorCode code) noexcept
{
    return ErrorResult{makeError(code)};
}

HATN_BASE_NAMESPACE_END

#endif // HATNBASERESULT_H
