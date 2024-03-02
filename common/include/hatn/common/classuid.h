/*
    Copyright (c) 2020 - current, Evgeny Sidorov (dracosha.com), All rights reserved.
    
    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)
      
*/
/****************************************************************************/
/** @file common/classuid.h
 *
 *     Class UID wrapper.
 *
 */
/****************************************************************************/

#ifndef HATNCLASSUID_H
#define HATNCLASSUID_H

#include <tuple>
#include <string>
#include <set>

#include <hatn/common/common.h>

HATN_COMMON_NAMESPACE_BEGIN

using CUID_TYPE=uintptr_t;

template <typename TargetType, typename ... Classes>
struct CollectClassUids
{
};
template <typename TargetType, typename T>
struct CollectClassUids<TargetType,T>
{
    static_assert(std::is_base_of<T,TargetType>::value,"Class T must be base class of TargetType");
    static void fillCuids(std::set<CUID_TYPE>& s)
    {
        T::fillCuids(s);
    }
};
template <typename TargetType, typename T, typename ... Classes>
struct CollectClassUids<TargetType,T,Classes...>
{
    static_assert(std::is_base_of<T,TargetType>::value,"Class T must be base class of TargetType");
    static void fillCuids(std::set<CUID_TYPE>& s)
    {
        T::fillCuids(s);
        CollectClassUids<TargetType,Classes...>::fillCuids(s);
    }
};

/**
 * @brief Base class for getting typeID
 */
class HATN_COMMON_EXPORT EnableTypeIDBase
{
    public:

        virtual ~EnableTypeIDBase()=default;
        EnableTypeIDBase(const EnableTypeIDBase&)=default;
        EnableTypeIDBase(EnableTypeIDBase&&) =default;
        EnableTypeIDBase& operator=(const EnableTypeIDBase&)=default;
        EnableTypeIDBase& operator=(EnableTypeIDBase&&) =default;

        //! Get type ID
        virtual CUID_TYPE typeID() const noexcept =0;
};

/**
 * @brief Class for getting typeID
 */
template <typename T1, typename T2>
class EnableTypeID : public T1, public T2
{
    public:

        using T1::T1;

        //! Get type ID
        virtual CUID_TYPE typeID() const noexcept override
        {
            return this->cuid();
        }
};

//! Base class with interfaces for human readable type name
class HumanReadableType
{
	public:

        HumanReadableType()=default;
        virtual ~HumanReadableType()=default;
        HumanReadableType(const HumanReadableType&)=default;
        HumanReadableType(HumanReadableType&&) =default;
        HumanReadableType& operator=(const HumanReadableType&)=default;
        HumanReadableType& operator=(HumanReadableType&&) =default;

		/**
			\brief Set human readable type
		**/
        virtual void setHumanReadableType(std::string type) noexcept
		{
			std::ignore = type;
		}

		/**
			\brief Set human readable type
			\return Human readable type intended to be used for all objects of the same type
		**/
        virtual std::string humanReadableType() const noexcept
		{
			return "_unknown_";
        }
};

//! Base class for classes with UID
template <typename T, typename ...>
class ClassUid
{
    using type=typename T::invalid_template_instantiation;
};

//! Base class for classes with UID
template <typename T>
class ClassUid<T> : public HumanReadableType
{
    public:

        /**
         * @brief Get UID of the class
         *
         * Caution! Use with care with dynamic libraries.
         * A new instance of a class might be created per dynamic unit.
         * To avoid this a workaround must be used - use HATN_CUID_DECLARE() in class declaration and HATN_CUID_INIT(ClassName) in source (*.cpp).
         *
         */
        static CUID_TYPE cuid() noexcept
        {
            static int dummy;
            return reinterpret_cast<CUID_TYPE>(&dummy);
        }

        //! Get UID of this class and all parent classes
        static std::set<CUID_TYPE> cuids()
        {
            std::set<CUID_TYPE> res;
            res.insert(cuid());
            return res;
        }

        /**
         * @brief Fill cuids of this class. Here there is only one cuid
         * @param s Set of CUIDs
         */
        static void fillCuids(std::set<CUID_TYPE>& s)
        {
            s.insert(cuid());
        }
};

/**
 *  @brief Base template for classes that have unique identifier (UID) that can be accessed statically by class name in runtime.
 *
 *  The UID is generated as an address of static variable unique per class.
 *
 *  \note Using across dynamic libraries is not well supported.
 *  A separate instance of static cuid() can be created for each library.
 *  To use with dynamic libs put HATN_CUID_DECLARE_MULTI() in class declaration and HATN_CUID_INIT_MULTI(ClassName) in source file (*.cpp)
 */
template <typename T, typename T1>
class ClassUid<T,T1> : public HumanReadableType
{
    public:

        /**
         * @brief Get UID of the class
         */
        static CUID_TYPE cuid() noexcept
        {
            static int dummy;
            return reinterpret_cast<CUID_TYPE>(&dummy);
        }

        //! Get UID of this class and all parent classes
        static std::set<CUID_TYPE> cuids()
        {
            std::set<CUID_TYPE> res;
            fillCuids(res);
            return res;
        }

        /**
         * @brief Fill cuids of this class. Here we collect cuids of all parents
         * @param s Set of CUIDs
         * @param appendCurrent Append current cuid to the set or use only parent cuids
         */
        static void fillCuids(std::set<CUID_TYPE>& s, bool appendCurrent=true)
        {
            if (appendCurrent)
            {
                s.insert(cuid());
            }
            CollectClassUids<T,T1>::fillCuids(s);
        }
};

/**
 *  @brief Same as ClassUid<T,T1> but can have multiple UIDs collected from parent classes.
 *
 * \note Using across dynamic libraries is not well supported.
 *  A separate instance of static cuid() can be created for each library.
 *  To use with dynamic libs put HATN_CUID_DECLARE_MULTI() in class declaration and HATN_CUID_INIT_MULTI(ClassName) in source file (*.cpp)
 */
template <typename T, typename T1, typename ... Classes>
class ClassUid<T,T1,Classes...> : public HumanReadableType
{
    public:

        /**
         * @brief Get UID of the class
         */
        static CUID_TYPE cuid() noexcept
        {
            static int dummy;
            return reinterpret_cast<CUID_TYPE>(&dummy);
        }

        //! Get UID of this class and all parent classes
        static std::set<CUID_TYPE> cuids()
        {
            std::set<CUID_TYPE> res;
            fillCuids(res);
            return res;
        }

        /**
         * @brief Fill cuids of this class. Here we collect cuids of all parents
         * @param s Set of CUIDs
         * @param appendCurrent Append current cuid to the set or use only parent cuids
         */
        static void fillCuids(std::set<CUID_TYPE>& s, bool appendCurrent=true)
        {
            if (appendCurrent)
            {
                s.insert(cuid());
            }
            CollectClassUids<T,T1,Classes...>::fillCuids(s);
        }
};

template <typename T> class HasCUID
{
    typedef char one;
    typedef long two;

    template <typename C> static one test( decltype(&C::cuid) ) ;
    template <typename C> static two test(...);

  public:

    enum { value = sizeof(test<T>(0)) == sizeof(char) };
};

HATN_COMMON_NAMESPACE_END

//! Declare class UID, place it in class declaration
#define HATN_CUID_DECLARE() \
    static ::hatn::common::CUID_TYPE cuid() noexcept; \
    static std::set<::hatn::common::CUID_TYPE> cuids(); \
    static void fillCuids(std::set<::hatn::common::CUID_TYPE>& s); \
	private: \
		static std::string m_humanReadableType; \
	public: \
        virtual std::string humanReadableType() const noexcept override; \
        virtual void setHumanReadableType(std::string type) noexcept override;

//! Instantiate class UID, place it in compilation unit source (*.cpp)
#define HATN_CUID_INIT(ClassName) \
    ::hatn::common::CUID_TYPE ClassName::cuid() noexcept \
    { \
        static int dummy; \
        return reinterpret_cast<::hatn::common::CUID_TYPE>(&dummy); \
    } \
    std::set<::hatn::common::CUID_TYPE> ClassName::cuids() \
    { \
        std::set<::hatn::common::CUID_TYPE> res; \
        res.insert(cuid()); \
        return res; \
    } \
    void ClassName::fillCuids(std::set<::hatn::common::CUID_TYPE>& s) \
    { \
        s.insert(cuid()); \
    } \
    std::string ClassName::humanReadableType() const noexcept \
    { \
        return m_humanReadableType; \
    } \
    void ClassName::setHumanReadableType(std::string type) noexcept \
    { \
        m_humanReadableType = std::move(type); \
    } \
	std::string ClassName::m_humanReadableType = #ClassName;

//! Declare class UID, place it in class declaration
#define HATN_CUID_DECLARE_MULTI() \
    static ::hatn::common::CUID_TYPE cuid() noexcept; \
    static std::set<::hatn::common::CUID_TYPE> cuids(); \
	private: \
		static std::string m_humanReadableType; \
	public: \
        virtual std::string humanReadableType() const noexcept override; \
        virtual void setHumanReadableType(std::string type) noexcept override;

//! Instantiate class UID, place it in compilation unit source (*.cpp)
#define HATN_CUID_INIT_MULTI(ClassName) \
    ::hatn::common::CUID_TYPE ClassName::cuid() noexcept \
    { \
        static int dummy; \
        return reinterpret_cast<::hatn::common::CUID_TYPE>(&dummy); \
    } \
    std::set<::hatn::common::CUID_TYPE> ClassName::cuids() \
    { \
        std::set<CUID_TYPE> res; \
        fillCuids(res,false); \
        res.insert(cuid()); \
        return res; \
    } \
    std::string ClassName::humanReadableType() const noexcept \
    { \
        return m_humanReadableType; \
    } \
    void ClassName::setHumanReadableType(std::string type) noexcept \
    { \
        m_humanReadableType = std::move(type); \
    } \
	std::string ClassName::m_humanReadableType = #ClassName;

#endif // HATNCLASSUID_H
