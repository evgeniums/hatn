/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.
    
    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)
      
*/

/****************************************************************************/
/** @file common/withsharedvalue.h
 *
 *
 */
/****************************************************************************/

#ifndef HATNWITHSHAREDVALUE_H
#define HATNWITHSHAREDVALUE_H

#include <memory>

#include <hatn/common/common.h>
#include <hatn/common/sharedptr.h>
#include <hatn/common/makeshared.h>

HATN_COMMON_NAMESPACE_BEGIN

template <typename T>
class WithStdSharedValue
{
    public:

        WithStdSharedValue(
            std::shared_ptr<T> value={}
        ) : m_value(std::move(value))
        {}

        const T& value() const noexcept
        {
            return *m_value;
        }

        T& value() noexcept
        {
            return *m_value;
        }

        const T& operator *() const noexcept
        {
            return *m_value;
        }

        T& operator *() noexcept
        {
            return *m_value;
        }

        const T* operator ->() const noexcept
        {
            return m_value.get();
        }

        T* operator ->() noexcept
        {
            return m_value.get();
        }

        const T* get() const noexcept
        {
            return m_value.get();
        }

        T* get() noexcept
        {
            return m_value.get();
        }

        bool isNull() const noexcept
        {
            return m_value.isNull();
        }

        operator bool() const noexcept
        {
            return !isNull();
        }

        std::shared_ptr<T> sharedValue() const noexcept
        {
            return m_value;
        }

        void setValue(std::shared_ptr<T> value)
        {
            m_value=std::move(value);
        }

    private:

        std::shared_ptr<T> m_value;
};

template <typename T>
class WithSharedValue
{
    public:

        WithSharedValue(
                common::SharedPtr<T> value={}
            ) : m_value(std::move(value))
        {}

        const T& value() const noexcept
        {
            return *m_value;
        }

        T& value() noexcept
        {
            return *m_value;
        }

        const T& operator *() const noexcept
        {
            return *m_value;
        }

        T& operator *() noexcept
        {
            return *m_value;
        }

        const T* operator ->() const noexcept
        {
            return m_value.get();
        }

        T* operator ->() noexcept
        {
            return m_value.get();
        }

        const T* get() const noexcept
        {
            return m_value.get();
        }

        T* get() noexcept
        {
            return m_value.get();
        }

        bool isNull() const noexcept
        {
            return m_value.isNull();
        }

        operator bool() const noexcept
        {
            return !isNull();
        }

        common::SharedPtr<T> sharedValue() const
        {
            return m_value;
        }

        common::SharedPtr<T> takeValue()
        {
            return std::move(m_value);
        }

        void setValue(common::SharedPtr<T> value)
        {
            m_value=std::move(value);
        }

    private:

        common::SharedPtr<T> m_value;
};

template <typename T>
class WithSharedValueAuto : public WithSharedValue<T>
{
    public:

        WithSharedValueAuto(
                common::SharedPtr<T> value
            ) : WithSharedValue<T>(std::move(value))
        {}

        WithSharedValueAuto() : WithSharedValue<T>(common::makeShared<T>())
        {}
};

HATN_COMMON_NAMESPACE_END

#endif // HATNWITHSHAREDVALUE_H
