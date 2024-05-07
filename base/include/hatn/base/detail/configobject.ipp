/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.
    
    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)
      
*/

/****************************************************************************/
/*
    
*/
/** @file base/detail/configobject.ipp
  *
  * Contains definitions of ConfigObject template methods.
  *
  */

/****************************************************************************/

#ifndef HATNCONFIGOBJECT_IPP
#define HATNCONFIGOBJECT_IPP

#include <hatn/common/error.h>
#include <hatn/common/nativeerror.h>
#include <hatn/common/translate.h>

#include <hatn/validator/validate.hpp>

#include <hatn/dataunit/valuetypes.h>
#include <hatn/dataunit/unit.h>
#include <hatn/dataunit/visitors.h>

#include <hatn/base/baseerrorcodes.h>
#include <hatn/base/configtree.h>
#include <hatn/base/configobject.h>

HATN_BASE_NAMESPACE_BEGIN

//---------------------------------------------------------------

namespace config_object_detail {

template <typename T, typename=hana::when<true>>
struct FieldTraits
{
    static Error set(const ConfigTree&,const ConfigTreePath&, T&)
    {
        return BaseError::UNSUPPORTED_TYPE;
    }
};

template <typename T, typename=hana::when<true>>
struct FieldValue
{
};

template <typename T>
struct FieldValue<T,hana::when<decltype(dataunit::types::IsScalar<T::typeId>)::value>>
{
    using type=typename T::type;
};

template <typename T>
struct FieldValue<T,hana::when<decltype(dataunit::types::IsString<T::typeId>)::value>>
{
    using type=std::string;
};

template <typename T>
struct FieldTraits<T,hana::when<!T::isRepeatedType::value>>
{
    using ValueT=typename FieldValue<T>::type;

    static Error set(const ConfigTree& t, const ConfigTreePath& path, T& field)
    {
        auto p=path.copyAppend(field.name());
        if (t.isSet(p,true))
        {
            auto val=t.get(p);
            if (val)
            {
                return val.takeError();
            }

            auto v=val->template as<ValueT>();
            if (v)
            {
                return v.takeError();
            }

            field.set(v.value());
        }

        return OK;
    }
};

template <typename T>
struct FieldTraits<T,hana::when<T::isRepeatedType::value>>
{
    using ValueT=typename FieldValue<T>::type;

    static Error set(const ConfigTree& t, const ConfigTreePath& path, T& field)
    {
        auto p=path.copyAppend(field.name());
        if (t.isSet(p,true))
        {
            field.clearArray();

            auto val=t.get(p);
            if (val)
            {
                return val.takeError();
            }

            auto arr=val->template asArray<ValueT>();
            if (arr)
            {
                return arr.takeError();
            }

            for (size_t i=0;i<arr->size();i++)
            {
                field.addValue(arr->at(i));
            }
        }

        return OK;
    }
};

} // namespace config_object_detail

//---------------------------------------------------------------

template <typename Traits>
Error ConfigObject<Traits>::loadConfig(const ConfigTree& configTree, const ConfigTreePath& path, bool checkRequired)
{
    auto requiredHandler=[this,&checkRequired,&path]()
    {
        // check required fields
        if (checkRequired)
        {
            auto reqResult=HATN_DATAUNIT_NAMESPACE::visitors::checkRequiredFields(_CFG);
            if (reqResult.first!=-1)
            {
                auto errMsg=fmt::format(_TR("required parameter {} not set","base"),reqResult.second);
                auto ec=std::make_shared<common::NativeError>(fmt::format(_TR("{} at path {}: {}"), _CFG.name(), path.path(), errMsg));
                return baseError(BaseError::CONFIG_OBJECT_VALIDATE_ERROR,std::move(ec));
            }
        }
        return Error{};
    };

    // skip section if not set
    if (!configTree.isSet(path))
    {
        return requiredHandler();
    }

    // set each field
    std::string failedField;
    auto predicate=[](const Error& ec)
    {
        return !ec;
    };
    auto handler=[&configTree,&path,&failedField](auto& field, auto&&)
    {
        auto ec=config_object_detail::FieldTraits<std::decay_t<decltype(field)>>::set(configTree,path,field);
        if (ec)
        {
            failedField=field.name();
        }
        return ec;
    };
    auto ec=_CFG.each(predicate,handler);
    if (ec)
    {
        auto err=std::make_shared<common::NativeError>(fmt::format(_TR("{} at path {}: parameter {}","base"), _CFG.name(),path.path(),failedField));
        err->setPrevError(std::move(ec));
        return baseError(BaseError::CONFIG_OBJECT_LOAD_ERROR,std::move(err));
    }


    // done
    return requiredHandler();
}

//---------------------------------------------------------------

template <typename Traits>
template <typename ValidatorT>
Error ConfigObject<Traits>::loadConfig(const ConfigTree& configTree, const ConfigTreePath& path, const ValidatorT& validator)
{
    // load configuration
    auto ec=loadConfig(configTree,path);
    if (ec)
    {
        return ec;
    }

    // validate object
    HATN_VALIDATOR_NAMESPACE::error_report err;
    HATN_VALIDATOR_NAMESPACE::validate(_CFG,validator,err);
    if (err)
    {
        auto ec=std::make_shared<common::NativeError>(fmt::format(_TR("{} at path {}: {}","base"), _CFG.name(), path.path(), err.message()));
        return baseError(BaseError::CONFIG_OBJECT_VALIDATE_ERROR,std::move(ec));
    }

    // done
    return OK;
}

//---------------------------------------------------------------

HATN_BASE_NAMESPACE_END

#endif // HATNCONFIGOBJECT_IPP
