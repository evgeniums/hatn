/*
    Copyright (c) 2020 - current, Evgeny Sidorov (dracosha.com), All rights reserved.
    
    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)
      
*/

/****************************************************************************/
/*
    
*/
/** @file common/utils.h
  *
  *      Utils.
  *
  */

/****************************************************************************/

#ifndef HATNUTILS_H
#define HATNUTILS_H

#include <cassert>
#include <cstdint>
#include <cmath>
#include <functional>

#include <boost/any.hpp>
#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string/trim.hpp>

#include <hatn/common/common.h>

#ifdef ANDROID

#include <sstream>

namespace std {
template <typename T>
string to_string(T&& value,
                 std::enable_if_t<
                    std::is_integral<std::decay_t<T>>::value || std::is_floating_point<std::decay_t<T>>::value
                 ,void*> =nullptr)
{
    ostringstream os;
    os << value;
    return os.str();
}
}

#endif

HATN_COMMON_NAMESPACE_BEGIN

//! Common utils
struct HATN_COMMON_EXPORT Utils final
{
    Utils()=delete;
    ~Utils()=delete;
    Utils(const Utils&)=delete;
    Utils(Utils&&) =delete;
    Utils& operator=(const Utils&)=delete;
    Utils& operator=(Utils&&) =delete;

    //! Get random value uniformelly distributed in range
    static uint32_t uniformRand(uint32_t min, uint32_t max) noexcept;

    //! Safe any to bool conversion
    template <typename T>
    static T safeAny(
            const boost::any& val,
            const T& defaultVal=T()
        ) noexcept
    {
        T result=defaultVal;
        try {
            if (!val.empty())
            {
                result=boost::any_cast<T>(val);
            }
        }
        catch (...)
        {
        }
        return result;
    }

    //! Compare two values of type "any"
    template <typename T>
    static bool boostAnyEqual(
            const boost::any& val1,
            const boost::any& val2
        ) noexcept
    {
        return safeAny<T>(val1)==safeAny<T>(val2);
    }

    //! Check if two boost any are equal, only std::string and integral types are supported
    static bool compareBoostAny(
        const boost::any& val1,
        const boost::any& val2
    ) noexcept;

    template <typename ContainerT>
    static void split(ContainerT& target,const std::string& str, char ch)
    {
        boost::algorithm::split(target,str,[ch](char c){return c==ch;});
    }

    template <typename ContainerT>
    static void trimSplit(ContainerT& target,std::string& str, char ch)
    {
        boost::algorithm::trim(str);
        split(target,str,ch);
    }

    static int safeStrCompare(const char* l, const char *r) noexcept
    {
        if (l==nullptr)
        {
            return r==nullptr ? 0 : -1;
        }
        else if (r==nullptr)
        {
            return l==nullptr ? 0 : 1;
        }
        return strcmp(l,r);
    }
};

//! Run handler on scope exiting
struct RunOnScopeExit final
{
    /**
     * @brief Constructor
     * @param handler handler to run when object is destroyed
     */
    RunOnScopeExit(std::function<void ()> handler, bool enable=true) noexcept
        : m_handler(std::move(handler)),
          m_enable(enable)
    {}

    //! Dtor
    ~RunOnScopeExit()
    {
        if (m_enable && m_handler)
        {
            m_handler();
        }
    }

    RunOnScopeExit(const RunOnScopeExit&)=delete;
    RunOnScopeExit(RunOnScopeExit&&) =delete;
    RunOnScopeExit& operator=(const RunOnScopeExit&)=delete;
    RunOnScopeExit& operator=(RunOnScopeExit&&) =delete;

    //! Enable handler
    inline void setEnable(bool enable) noexcept
    {
        m_enable=enable;
    }

    //! Check if handler is enabled
    inline bool isEnabled() const noexcept
    {
        return m_enable;
    }

    private:

        std::function<void ()> m_handler;
        bool m_enable;
};

#define Assert(condition,message) assert((condition) && message); if (!(condition)) throw std::runtime_error(message);
#define AssertThrow(condition,message) if (!(condition)) {throw std::runtime_error(message);}
#define AssertThrowEx(condition,message,ex) if (!(condition)) {throw ex(message);}

//---------------------------------------------------------------
    HATN_COMMON_NAMESPACE_END
#endif // HATNUTILS_H
