/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.
    
    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)
      
*/

/****************************************************************************/
/** @file common/objecttraits.h
  *
  *     Base class template for class templates with traits
  *
  */

/****************************************************************************/

#ifndef DRACOSHOBJECTTRAITS_H
#define DRACOSHOBJECTTRAITS_H

#include <hatn/common/common.h>

HATN_COMMON_NAMESPACE_BEGIN

template <typename Traits, typename=void>
class WithTraits
{};

/**
 * @brief Base class template for class templates with traits
 */
template <typename Traits>
class WithTraits<Traits, 
                std::enable_if_t<std::is_copy_constructible<Traits>::value && std::is_move_constructible<Traits>::value>
                >
{
    public:

        using TraitsT=Traits;

        //! Ctor
        template <typename ... Args>
        WithTraits(Args&& ...traitsArgs) : m_traits(std::forward<Args>(traitsArgs)...)
        {}

        ~WithTraits() = default;
        WithTraits(WithTraits&& other) noexcept
            : m_traits(std::move(other.m_traits))
        {}
        WithTraits& operator=(WithTraits&& other) noexcept
        {
            if (&other != this)
            {
                m_traits=std::move(other.m_traits);
            }
            return *this;
        }
        WithTraits(const WithTraits& other)
            : m_traits(other.m_traits)
        {}
        WithTraits& operator=(const WithTraits& other)
        {
            if (&other != this)
            {
                m_traits=other.m_traits;
            }
            return *this;
        }

    protected:

        //! Get traits
        inline Traits& traits() noexcept
        {
            return m_traits;
        }

        //! Get traits
        inline const Traits& traits() const noexcept
        {
            return m_traits;
        }

    private:

        Traits m_traits;
};

template <typename Traits>
class WithTraits<Traits,
    std::enable_if_t<!std::is_copy_constructible<Traits>::value && std::is_move_constructible<Traits>::value>
>
{
public:

    using TraitsT=Traits;

    //! Ctor
    template <typename ... Args>
    WithTraits(Args&& ...traitsArgs) : m_traits(std::forward<Args>(traitsArgs)...)
    {}

    ~WithTraits() = default;
    WithTraits(WithTraits&& other) noexcept
        : m_traits(std::move(other.m_traits))
    {}
    WithTraits& operator=(WithTraits&& other) noexcept
    {
        if (&other != this)
        {
            m_traits = std::move(other.m_traits);
        }
        return *this;
    }
    WithTraits(const WithTraits& other) = delete;
    WithTraits& operator=(const WithTraits& other) = delete;

protected:

    //! Get traits
    inline Traits& traits() noexcept
    {
        return m_traits;
    }

    //! Get traits
    inline const Traits& traits() const noexcept
    {
        return m_traits;
    }

private:

    Traits m_traits;
};

template <typename Traits>
class WithTraits<Traits,
    std::enable_if_t<!std::is_copy_constructible<Traits>::value && !std::is_move_constructible<Traits>::value>
>
{
public:

    using TraitsT=Traits;

    //! Ctor
    template <typename ... Args>
    WithTraits(Args&& ...traitsArgs) : m_traits(std::forward<Args>(traitsArgs)...)
    {}

    ~WithTraits() = default;
    WithTraits(WithTraits&& other) = delete;
    WithTraits& operator=(WithTraits&& other) = delete;
    WithTraits(const WithTraits& other) = delete;
    WithTraits& operator=(const WithTraits& other) = delete;

protected:

    //! Get traits
    inline Traits& traits() noexcept
    {
        return m_traits;
    }

    //! Get traits
    inline const Traits& traits() const noexcept
    {
        return m_traits;
    }

private:

    Traits m_traits;
};

/**
 * @brief Base tempate class for classes that incapsulate implementer (impl) object
 */
template <typename ImplT>
class WithImpl
{
    public:

        template <typename ...Args>
        WithImpl(Args&&... args)
            : m_impl(std::forward<Args>(args)...)
        {}

        ImplT& impl() noexcept
        {
            return m_impl;
        }

        const ImplT& impl() const noexcept
        {
            return m_impl;
        }

    private:

        ImplT m_impl;
};

//---------------------------------------------------------------
HATN_COMMON_NAMESPACE_END

#endif // DRACOSHOBJECTTRAITS_H
