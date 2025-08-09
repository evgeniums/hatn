/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.
    
    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)
      
*/

/****************************************************************************/
/*
    
*/
/** @file dataunit/field.h
  *
  *      Common templates for field types
  *
  */

/****************************************************************************/

#ifndef HATNDATAUNITFIELDIMPL_H
#define HATNDATAUNITFIELDIMPL_H

#include <boost/endian/conversion.hpp>
#include <boost/intrusive/avl_set.hpp>

#include <hatn/common/meta/constructwithargordefault.h>
#include <hatn/common/bytearray.h>
#include <hatn/common/pmr/pmrtypes.h>
#include <hatn/common/sharedptr.h>
#include <hatn/common/logger.h>
#include <hatn/common/makeshared.h>
#include <hatn/common/classuid.h>
#include <hatn/common/format.h>
#include <hatn/common/meta/hasmethod.h>

#include <hatn/dataunit/dataunit.h>
#include <hatn/dataunit/allocatorfactory.h>
#include <hatn/dataunit/types.h>
#include <hatn/dataunit/wiredata.h>
#include <hatn/dataunit/stream.h>
#include <hatn/dataunit/fieldserialization.h>
#include <hatn/dataunit/unit.h>
#include <hatn/dataunit/rapidjsonsaxhandlers.h>

HATN_DATAUNIT_NAMESPACE_BEGIN

HATN_PREPARE_HAS_METHOD(sharedFromThis)

using namespace types;

struct FieldTag
{
    bool operator ==(const FieldTag&) const
    {
        return true;
    }
    bool operator !=(const FieldTag&) const
    {
        return false;
    }
};

template <typename T, typename =void>
struct DummyConst
{
    static const T& f()
    {
        static T dummy;
        return dummy;
    }
};
template <typename T>
struct DummyConst<T,std::enable_if_t<std::is_integral<T>::value>>
{
    static const T& f()
    {
        static T dummy=0;
        return dummy;
    }
};

template <typename Type,bool Shared=false>
struct FTraits
{
    using type=typename Type::type;
    using base=typename Type::type;

    template <typename UnitT>
    static void setV(UnitT*, common::SharedPtr<Unit>)
    {
        Assert(false,"Only shared subunits can be set with setV()");
    }
};
template <typename Type>
struct FTraits<Type,true>
{
    using type=typename Type::shared_type;
    using base=typename Type::base_shared_type;

    template <typename UnitT>
    static void setV(UnitT* self, common::SharedPtr<Unit> val,
              std::enable_if_t<
                  decltype(has_sharedFromThis<typename UnitT::managed>())::value
                >* =nullptr
        )
    {
        using vType=typename UnitT::type::element_type;
        Assert(strcmp(val->name(),vType::unitName())==0,"Mismatched unit types");

        static vType sample;
        vType* casted=sample.castToManagedUnit(val.get());
        self->set(casted->sharedFromThis());
    }

    template <typename UnitT>
    static void setV(UnitT*, common::SharedPtr<Unit>,
              std::enable_if_t<
                  !std::is_base_of<common::ManagedObject,typename UnitT::managed>::value
                  >* =nullptr
              )
    {
        Assert(false,"Cannot set unmanaged unit");
    }
};

template <typename T,typename,typename=void>
struct JsonR
{
};
template <typename T,typename Repeated>
struct JsonR<T,Repeated,
                    std::enable_if_t<
                        T::isBytesType::value
                        ||
                        Repeated::value
                        ||
                        T::isUnitType::value
                    >
                >
{
    template <typename T1> inline static void push(
            Unit*,
            T1*
        )
    {
        // use overriden method in derived class
    }
};
template <typename T,typename Repeated>
struct JsonR<T,Repeated,
                    std::enable_if_t<
                        !T::isBytesType::value
                        &&
                        !Repeated::value
                        &&
                        !T::isUnitType::value
                    >
                >
{
    template <typename T1>
    static void push(
            Unit* topUnit,
            T1* field
        )
    {
        json::pushHandler<T1,json::FieldReader<T,T1>>(topUnit,field);
    }
};

template <typename T, int Id, typename FieldName, typename Tag, bool Required>
struct FieldConf : public T
{
    constexpr static const int ID=Id;
    using hana_tag=Tag;

    using T::T;

    //! Get static field id.
    constexpr static int fieldId() noexcept
    {
        return Id;
    }

    //! Get static field name.
    constexpr static const char* fieldName() noexcept
    {
        return FieldName::name;
    }

    //! Get field required statically.
    constexpr static bool fieldRequired() noexcept
    {
        return Required;
    }

    //! Get field ID
    virtual int getID() const noexcept override
    {
        return Id;
    }

    //! Get field name.
    virtual const char* name() const noexcept override
    {
        return FieldName::name;
    }

    //! Check if field is required
    virtual bool isRequired() const noexcept override
    {
        return Required;
    }    
};

struct DefaultValueTag{};

/**  Template to set default values */
template <typename T>
struct DefaultValue
{
    using hana_tag=DefaultValueTag;

    using HasDefV=std::false_type;
    static typename T::type value() {return typename T::type(static_cast<typename T::type>(0));}
};

template <typename Base,typename Type,typename DefaultV,typename =void>
struct FieldDefault
{
};
template <typename Base,typename Type,typename DefaultV>
struct FieldDefault<Base,Type,
            DefaultV,
                    std::enable_if_t<(Type::isBytesType::value && !Type::isStringType::value) || Type::isUnitType::value>
        >  : public Base
{
    using Base::Base;
};

/**  Base class with constructor with default value */
template <typename Base,typename Type,typename DefaultV>
struct FieldDefault<Base,Type,
        DefaultV,
        std::enable_if_t<!Type::isBytesType::value && !Type::isUnitType::value>
    > : public Base
{
    using vtype=typename Type::type;

    FieldDefault(Unit* unit) : Base(unit,DefaultV::value())
    {}

    vtype defaultValue() const
    {
        return fieldDefaultValue();
    }

    virtual bool hasDefaultValue() const noexcept override
    {
        return fieldHasDefaultValue();
    }

    bool fieldHasDefaultValue() const noexcept
    {
        return DefaultV::HasDefV::value;
    }

    vtype fieldDefaultValue() const
    {
        return static_cast<vtype>(DefaultV::value());
    }

    //! Clear field
    virtual void clear() override
    {
        fieldClear();
    }

    //! Reset field
    virtual void reset() override
    {
        fieldReset();
    }

    //! Clear field
    void fieldClear()
    {
        this->m_value=DummyConst<vtype>::f();
    }

    //! Reset field
    void fieldReset()
    {
        this->m_value=fieldDefaultValue();
        this->markSet(false);
    }
};

/**  Base class with constructor with default value */
template <typename Base,typename Type,typename DefaultV>
struct FieldDefault<Base,Type,
                    DefaultV,
                    std::enable_if_t<Type::isStringType::value>
                    > : public Base
{
    using vtype=decltype(DefaultV::value());

    FieldDefault(Unit* unit) : Base(unit)
    {
        fillDefault();
    }

    void fillDefault()
    {
        if (DefaultV::HasDefV::value)
        {
            this->buf(false)->load(DefaultV::value());
        }
    }

    auto defaultValue() const
    {
        return fieldDefaultValue();
    }

    virtual bool hasDefaultValue() const noexcept override
    {
        return fieldHasDefaultValue();
    }

    bool fieldHasDefaultValue() const noexcept
    {
        return DefaultV::HasDefV::value;
    }

    auto fieldDefaultValue() const
    {
        return DefaultV::value();
    }

    void fieldReset()
    {
        this->m_value.reset();
        this->markSet(false);
        fillDefault();
    }

    virtual void reset() override
    {
        fieldReset();
    }
};

/**  Base template of dataunit field with default value */
template <typename Type>
    struct FieldTmpl
{
};

/**  Template class of optional dataunit field */
template <typename FieldName,typename Type,int Id>
    struct OptionalField : public FieldConf<FieldTmpl<Type>,Id,FieldName,Type,false>
{
    using FieldConf<FieldTmpl<Type>,Id,FieldName,Type,false>::FieldConf;
};

/**  Template class of required dataunit field */
template <typename FieldName,typename Type,int Id>
    struct RequiredField : public FieldConf<FieldTmpl<Type>,Id,FieldName,Type,true>
{
    using FieldConf<FieldTmpl<Type>,Id,FieldName,Type,true>::FieldConf;
};

/**  Template class of optional dataunit field with default value */
template <typename FieldName,typename Type,int Id,typename DefaultAlias>
    struct DefaultField : public FieldConf<FieldDefault<FieldTmpl<Type>,Type,DefaultAlias>,Id,FieldName,Type,false>
{
    using FieldConf<FieldDefault<FieldTmpl<Type>,Type,DefaultAlias>,Id,FieldName,Type,false>::FieldConf;
};

HATN_DATAUNIT_NAMESPACE_END

#endif // HATNDATAUNITFIELDIMPL_H
