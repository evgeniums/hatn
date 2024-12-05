/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.
    
    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)
      
*/

/****************************************************************************/

/** @file common/nativehandler.h
  *
  *   Helper templates to hold native handlers in outer classes
  *
  */

/****************************************************************************/

#ifndef HATNNATIVEHANDLER_H
#define HATNNATIVEHANDLER_H

#include <algorithm>
#include <hatn/common/common.h>

HATN_COMMON_NAMESPACE_BEGIN

template <typename NativeT> struct NativeHandlerTraits
{
    static void free(NativeT*)
    {}

    static NativeT* construct(NativeT* masterCopy)
    {
        return masterCopy;
    }
};

//! Template for native handler
template <typename NativeT, typename Traits=NativeHandlerTraits<NativeT>>
struct NativeHandler
{
    NativeT* handler;

    NativeHandler(NativeT* init=nullptr) noexcept
        : handler(init)
    {}

    template <typename ... Args> NativeHandler(Args&&... args)
        : handler(Traits::construct(std::forward<Args>(args)...))
    {}

    NativeHandler(const NativeHandler& other)=delete;
    NativeHandler& operator= (const NativeHandler& other)=delete;

    NativeHandler(NativeHandler&& other) noexcept : handler(other.handler)
    {
        other.handler=nullptr;
    }

    inline NativeHandler& operator= (NativeHandler&& other) noexcept
    {
        if (&other!=this)
        {
            handler=other.handler;
            other.handler=nullptr;
        }
        return *this;
    }

    ~NativeHandler()
    {
        if (handler!=nullptr)
        {
            Traits::free(handler);
        }
    }

    void reset()
    {
        if (handler!=nullptr)
        {
            Traits::free(handler);
        }
        handler=nullptr;
    }

    bool isNull() const noexcept
    {
        return handler==nullptr;
    }

    NativeT* take() noexcept
    {
        return std::exchange(handler,nullptr);
    }
};

//! Template class for container of some native handler
template <typename NativeT, typename Traits, typename BaseT, typename DerivedT>
class NativeHandlerContainer : public BaseT
{
    public:

        using Native=NativeHandler<NativeT,Traits>;

        using BaseT::BaseT;

        NativeHandlerContainer(NativeHandlerContainer&)=delete;
        NativeHandlerContainer& operator=(NativeHandlerContainer&)=delete;

        NativeHandlerContainer(NativeHandlerContainer&& other) noexcept : m_native(std::move(other.m_native))
        {}

        NativeHandlerContainer& operator=(NativeHandlerContainer&& other) noexcept
        {
            if (&other!=this)
            {
                m_native=std::move(other.m_native);
            }
            return *this;
        }

        /**
         * @brief Get native object
         * @return Native object
         */
        Native& nativeHandler() noexcept
        {
            return m_native;
        }

        /**
         * @brief Get native object
         * @return Native object
         */
        const Native& nativeHandler() const noexcept
        {
            return m_native;
        }

        //! Set native handler
        void setNativeHandler(Native&& native) noexcept
        {
            m_native=std::move(native);
        }

        //! Set native handler
        void setNativeHandler(NativeT* key) noexcept
        {
            m_native.handler=key;
        }

        static DerivedT& masterReference() noexcept
        {
            static DerivedT instance;
            return instance;
        }

        inline bool isValid() const noexcept
        {
            return !m_native.isNull();
        }

        void resetNative()
        {
            m_native.reset();
        }

    private:

        Native m_native;
};

HATN_COMMON_NAMESPACE_END

#endif // HATNNATIVEHANDLER_H
