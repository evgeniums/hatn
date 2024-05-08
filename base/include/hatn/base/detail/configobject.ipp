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
#include <hatn/common/runonscopeexit.h>

#include <hatn/validator/validate.hpp>

#include <hatn/dataunit/valuetypes.h>
#include <hatn/dataunit/unit.h>
#include <hatn/dataunit/visitors.h>
#include <hatn/dataunit/unitmeta.h>

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
    return validate(path,validator);
}

//---------------------------------------------------------------

template <typename Traits>
template <typename ValidatorT>
Error ConfigObject<Traits>::validate(const ConfigTreePath& path, const ValidatorT& validator)
{
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

template <typename Traits>
Error ConfigObject<Traits>::loadLogConfig(const ConfigTree& configTree, const ConfigTreePath& path, config_object::LogRecords& records, const config_object::LogSettings& logSettings)
{
    auto onExit=common::makeScopeGuard(
        [this,&logSettings,&records]()
        {
            fillLogRecords(logSettings,records);
        }
    );
    std::ignore=onExit;

    // load configuration
    auto ec=loadConfig(configTree,path);
    if (ec)
    {
        return ec;
    }

    // done
    return OK;
}

//---------------------------------------------------------------

template <typename Traits>
template <typename ValidatorT>
Error ConfigObject<Traits>::loadLogConfig(const ConfigTree& configTree, const ConfigTreePath& path, config_object::LogRecords& records, const ValidatorT& validator, const config_object::LogSettings& logSettings)
{
    auto onExit=common::makeScopeGuard(
        [this,&logSettings,&records]()
        {
            fillLogRecords(logSettings,records);
        }
        );
    std::ignore=onExit;

    // load configuration
    auto ec=loadConfig(configTree,path);
    if (ec)
    {
        return ec;
    }

    // validate object
    return validate(path,validator);
}

//---------------------------------------------------------------

template <typename Traits>
void ConfigObject<Traits>::fillLogRecords(const config_object::LogSettings& logSettings, config_object::LogRecords& records)
{
    auto handler=[&logSettings,&records](auto&& field, auto&&)
    {
        using T=typename std::decay_t<decltype(field)>;

        auto value=hana::eval_if(
            typename T::isRepeatedType{},
            [&](auto _)
            {
                return hana::eval_if(
                    dataunit::types::IsString<T::typeId>,
                    [&](auto _)
                    {
                        const auto& vector=_(field).value();
                        auto count=vector.size();
                        if (_(logSettings).CompactArrays && vector.size()>_(logSettings).MaxArrayElements)
                        {
                            count=_(logSettings).MaxArrayElements;
                        }

                        auto out = fmt::memory_buffer();
                        fmt::format_to(std::back_inserter(out),"[");
                        for (size_t i=0;i<count;i++)
                        {
                            if (i!=0)
                            {
                                fmt::format_to(std::back_inserter(out),",");
                            }

                            std::string v{_(field).at(i).c_str()};
                            _(logSettings).mask(_(field).name(),v);
                            fmt::format_to(std::back_inserter(out),"\"{}\"",v);
                        }

                        if (count!=vector.size())
                        {
                            fmt::format_to(std::back_inserter(out),",...]");
                        }
                        else
                        {
                            fmt::format_to(std::back_inserter(out),"]");
                        }
                        return fmt::to_string(out);
                    },
                    [&](auto _)
                    {
                        const auto& vector=_(field).value();
                        size_t endOffset{0};
                        if (_(logSettings).CompactArrays && vector.size()>_(logSettings).MaxArrayElements)
                        {
                            endOffset=vector.size()-_(logSettings).MaxArrayElements;
                        }

                        auto out = fmt::memory_buffer();
                        fmt::format_to(std::back_inserter(out),"[{}",fmt::join(std::begin(vector),std::end(vector)-endOffset,","));
                        if (endOffset!=0)
                        {
                            fmt::format_to(std::back_inserter(out),",...]");
                        }
                        else
                        {
                            fmt::format_to(std::back_inserter(out),"]");
                        }
                        return fmt::to_string(out);
                    }
                );
            },
            [&](auto _)
            {
                return hana::eval_if(
                    dataunit::types::IsString<T::typeId>,
                    [&](auto _)
                    {
                        std::string v{_(field).c_str()};
                        _(logSettings).mask(_(field).name(),v);
                        return fmt::format("\"{}\"",v);
                    },
                    [&](auto _)
                    {
                        return fmt::format("{}",_(field).value());
                    }
                );
            }
        );

        records.emplace_back(field.name(),std::move(value));

        return true;
    };
    _CFG.each(HATN_DATAUNIT_NAMESPACE::meta::true_predicate,handler);
}

//---------------------------------------------------------------

HATN_BASE_NAMESPACE_END

#endif // HATNCONFIGOBJECT_IPP
