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

template <typename T,typename ValueT>
Error setValue(const ConfigTree& t, const ConfigTreePath& path, T& field)
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

template <typename T>
struct FieldTraits<T,hana::when<decltype(dataunit::types::IsScalar<T::typeId>)::value>>
{
    static Error set(const ConfigTree& t, const ConfigTreePath& path, T& field)
    {
        return setValue<T,typename T::type>(t,path,field);
    }
};

template <typename T>
struct FieldTraits<T,hana::when<decltype(dataunit::types::IsString<T::typeId>)::value>>
{
    static Error set(const ConfigTree& t, const ConfigTreePath& path, T& field)
    {
        return setValue<T,std::string>(t,path,field);
    }
};

} // namespace config_object_detail

//---------------------------------------------------------------

template <typename Traits>
Error ConfigObject<Traits>::loadConfig(const ConfigTree& configTree, const ConfigTreePath& path)
{
    // skip section if not set
    if (!configTree.isSet(path))
    {
        return OK;
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
        auto err=std::make_shared<common::NativeError>(fmt::format(_TR("object {}: parameter {}"), _CFG.name(),failedField));
        err->setPrevError(std::move(ec));
        return baseError(BaseError::CONFIG_OBJECT_LOAD_ERROR,std::move(err));
    }

    // done
    return OK;
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
        auto ec=std::make_shared<common::NativeError>(fmt::format(_TR("{} at {}: {}"), _CFG.name(), path.path(), err.message()));
        return baseError(BaseError::CONFIG_OBJECT_VALIDATE_ERROR,std::move(ec));
    }

    // done
    return OK;
}

//---------------------------------------------------------------

HATN_BASE_NAMESPACE_END

#endif // HATNCONFIGOBJECT_IPP
