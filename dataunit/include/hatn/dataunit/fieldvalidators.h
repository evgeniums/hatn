/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.
    
    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)
      
*/

/****************************************************************************/
/*
    
*/
/** \file dataunit/fieldvalidators.h
  *
  *  Definitions of field validators
  *
  */

/****************************************************************************/

#ifndef HATNDATAUNITVALIDATORS_H
#define HATNDATAUNITVALIDATORS_H

#include <type_traits>
#include <functional>

#include <boost/algorithm/string/predicate.hpp>

#include <hatn/common/translate.h>
#include <hatn/dataunit/unit.h>

HATN_NAMESPACE_BEGIN
using HATN_COMMON_NS::_TR;
namespace HATN_DATAUNIT_NS {
namespace validators {

/********************** Elementary validators **************************/

//---------------------------------------------------------------
struct pass
{
    template <typename T>
    struct type
    {
        explicit type(const T& =T())
        {}
        bool operator() (const T&) const noexcept
        {
            return true;
        }
        std::string errorStr() const
        {
            return _TR("OK","validator");
        }
    };
};

//---------------------------------------------------------------
struct fail
{
    template <typename T>
    struct type
    {
        explicit type(const T& =T())
        {}
        bool operator() (const T&) const noexcept
        {
            return false;
        }
        std::string errorStr() const
        {
            return _TR("unconditional failure","validator");
        }
    };
};

//---------------------------------------------------------------
struct exists
{
    template <typename T>
    struct type
    {
        type()
        {}
        bool operator() (bool val) const noexcept
        {
            return val;
        }
        std::string errorStr() const
        {
            return _TR("must exist","validator");
        }
    };
};

//---------------------------------------------------------------
struct not_exists
{
    template <typename T>
    struct type
    {
        type()
        {}
        bool operator() (bool val) const noexcept
        {
            return !val;
        }
        std::string errorStr() const
        {
            return _TR("must not exist","validator");
        }
    };
};

//---------------------------------------------------------------
struct eq
{
    template <typename T>
    struct type
    {
        explicit type(const T& val):m_val(val)
        {}
        bool operator() (const T& val) const noexcept
        {
            return val==m_val;
        }
        std::string errorStr() const
        {
            return fmt::format(_TR("must be equal to {}","validator"),m_val);
        }
        T m_val;
    };
};

//---------------------------------------------------------------
struct ne
{
    template <typename T>
    struct type
    {
        explicit type(const T& val):m_val(val)
        {}
        bool operator() (const T& val) const noexcept
        {
            return val!=m_val;
        }
        std::string errorStr() const
        {
            return fmt::format(_TR("must be not equal to {}","validator"),m_val);
        }
        T m_val;
    };
};

//---------------------------------------------------------------
struct lt
{
    template <typename T>
    struct type
    {
        explicit type(const T& val):m_val(val)
        {}
        bool operator() (const T& val) const noexcept
        {
            return val<m_val;
        }
        std::string errorStr() const
        {
            return fmt::format(_TR("must be less than {}","validator"),m_val);
        }
        T m_val;
    };
};

//---------------------------------------------------------------
struct lte
{
    template <typename T>
    struct type
    {
        explicit type(const T& val):m_val(val)
        {}
        bool operator() (const T& val) const noexcept
        {
            return val<=m_val;
        }
        std::string errorStr() const
        {
            return fmt::format(_TR("must be less than or equal to {}","validator"),m_val);
        }
        T m_val;
    };
};

//---------------------------------------------------------------
struct gt
{
    template <typename T>
    struct type
    {
        explicit type(const T& val):m_val(val)
        {}
        bool operator() (const T& val) const noexcept
        {
            return val>m_val;
        }
        std::string errorStr() const
        {
            return fmt::format(_TR("must be greater than {}","validator"),m_val);
        }
        T m_val;
    };
};

//---------------------------------------------------------------
struct gte
{
    template <typename T>
    struct type
    {
        explicit type(const T& val):m_val(val)
        {}
        bool operator() (const T& val) const noexcept
        {
            return val>=m_val;
        }
        std::string errorStr() const
        {
            return fmt::format(_TR("must be greater than or equal to {}","validator"),m_val);
        }
        T m_val;
    };
};

/********************** Configuration wrappers of elementary validators **************************/

struct ValueBase
{
};
template <typename T>
struct Value : public ValueBase
{
    using type=T;
};
struct BufSizeBase
{
};
template <typename T>
struct BufSize : public BufSizeBase
{
    using type=T;
};
struct ArrSizeBase
{
};
template <typename T>
struct ArrSize : public ArrSizeBase
{
    using type=T;
};
struct PresenseBase
{
};
template <typename T>
struct Presence : public PresenseBase
{
    using type=T;
};

/********************** Helper classes for processing configuration wrappers of elementary validators **************************/

template <
          typename BaseT,
          typename DefaultT,
          template <typename ...> class GetTmpl,
          typename CurrentT,
          typename=void>
struct SelectType
{
};
template <
          typename BaseT,
          typename DefaultT,
          template <typename ...> class GetTmpl,
          typename CurrentT
          >
struct SelectType<
        BaseT,
        DefaultT,
        GetTmpl,
        CurrentT,
        std::enable_if_t<std::is_base_of<BaseT,CurrentT>::value>
        >
{
    template <typename ...Types>
    using type=CurrentT;
};
template <
        typename BaseT,
        typename DefaultT,
        template <typename ...> class GetTmpl,
        typename CurrentT
         >
struct SelectType<
                BaseT,
                DefaultT,
                GetTmpl,
                CurrentT,
                std::enable_if_t<!std::is_base_of<BaseT,CurrentT>::value>>
{
    template <typename ...Types>
    using type=typename GetTmpl<BaseT,DefaultT,Types...>::type;
};

template <typename BaseT, typename DefaultT, typename ... Validators>
struct GetType
{
};
template <typename BaseT, typename DefaultT, typename ValidatorT, typename ... Validators>
struct GetType<BaseT,DefaultT,ValidatorT,Validators...>
{
    using type=typename SelectType<BaseT,DefaultT,GetType,ValidatorT>::template type<Validators...>;
};
template <typename BaseT, typename DefaultT, typename ValidatorT>
struct GetType<BaseT,DefaultT,ValidatorT>
{
    using type=typename SelectType<BaseT,DefaultT,GetType,ValidatorT>::template type<>;
};
template <typename BaseT, typename DefaultT>
struct GetType<BaseT,DefaultT>
{
    using type=DefaultT;
};

/********************** FieldValidator **************************/

/**
 * @brief Validator of data unit field
 */
template <typename T,typename ... Validators>
struct FieldValidator
{
    using presenceT=typename GetType<PresenseBase,Presence<pass>,Validators...>::type::type::template type<T>;
    using valueT=typename GetType<ValueBase,Value<pass>,Validators...>::type::type::template type<T>;
    using bufSizeT=typename GetType<BufSizeBase,BufSize<pass>,Validators...>::type::type::template type<size_t>;
    using arrSizeT=typename GetType<ArrSizeBase,ArrSize<pass>,Validators...>::type::type::template type<size_t>;

    FieldValidator() : bufSize(0),arrSize(0)
    {}

    template <typename T1>
    FieldValidator(T1 val,
                            typename std::enable_if<std::numeric_limits<T1>::is_integer,void*>::type =nullptr)
        : value(val),bufSize(static_cast<size_t>(val)),arrSize(static_cast<size_t>(val))
    {}

    template <typename T1>
    FieldValidator(T1 val,
                            typename std::enable_if<!std::numeric_limits<T1>::is_integer,void*>::type =nullptr)
        : value(val),bufSize(0),arrSize(0)
    {}

    template <typename T1>
    FieldValidator(T1 val, size_t size)
        : value(val),bufSize(size),arrSize(size)
    {}

    template <typename T1>
    FieldValidator(T1 val, size_t bufSize, size_t arraySize)
        : value(val),bufSize(bufSize),arrSize(arraySize)
    {}

    valueT value;
    bufSizeT bufSize;
    arrSizeT arrSize;
    presenceT exists;

//    bool validateExists(Field* field,const ValidateCb& validateCb=ValidateCb())
//    {
//        if (exists(field->isSet()))
//        {
//            return true;
//        }
//        //! \todo process callback
//        return false;
//    }
//    bool validateValue(Field* field,const T& val,const ValidateCb& validateCb=ValidateCb())
//    {
//        if (value(val))
//        {
//            return true;
//        }
//        //! \todo process callback
//        return false;
//    }
//    bool validateValue(Field* field,const ValidateCb& validateCb=ValidateCb())
//    {
//        T val;
//        field->getValue(val);
//        return validateValue(field,val,validateCb);
//    }
//    bool validateBufSize(Field* field,const size_t& size,const ValidateCb& validateCb=ValidateCb())
//    {
//        if (bufSize(size))
//        {
//            return true;
//        }
//        //! \todo process callback
//        return false;
//    }
//    bool validateBufSize(Field* field,const ValidateCb& validateCb=ValidateCb())
//    {
//        return validateBufSize(field->bufSize(),validateCb);
//    }
//    bool validateArraySize(Field* field,const size_t& size,const ValidateCb& validateCb=ValidateCb())
//    {
//        if (arraySize(size))
//        {
//            return true;
//        }
//        //! \todo process callback
//        return false;
//    }
//    bool validateArraySize(Field* field,const ValidateCb& validateCb=ValidateCb())
//    {
//        return validateArraySize(field->arraySize(),validateCb);
//    }
//    bool validateArrayBufSize(Field* field,const ValidateCb& validateCb=ValidateCb())
//    {
//        for (size_t idx=0;idx<field->arraySize();idx++)
//        {
//            if (!validateBufSize(field,field->arrayBufSize(idx),validateCb))
//            {
//                return false;
//            }
//        }
//        return true;
//    }
};

template <typename T>
struct FieldValidatorAndID
{
    int id;
    T validator;
};

} // namespace validators

//using namespace validators;

//FieldValidator<uint32_t,Value<gt>> v1(10);
//FieldValidator<uint32_t,BufSize<lte>> v2(100);
//FieldValidator<uint32_t,Value<gt>,ArrSize<lte>> v3(100,1000);
//FieldValidator<float,Presence<exists>,Value<gt>> v4(10.0f);
//BufValidator<Value<str_eq>,BufSize<eq>> v5("hello",5);
//FieldValidator<Unit*,Presence<not_exists>> v6;

HATN_DATAUNIT_NAMESPACE_END

#endif // HATNDATAUNITVALIDATORS_H
