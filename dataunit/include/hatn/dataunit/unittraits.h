/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.
    
    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)
      
*/

/****************************************************************************/
/*

*/
/** @file dataunit/unittraits.h
  *
  *      Declarations of DataUnit templates
  *
  */

/****************************************************************************/

#ifndef HATNDATAUNITS_H
#define HATNDATAUNITS_H

#include <boost/hana.hpp>

#include <hatn/common/flatmap.h>
#include <hatn/common/pmr/pmrtypes.h>
#include <hatn/common/pmr/withstaticallocator.h>
#include <hatn/common/stdwrappers.h>

#include <hatn/validator/utils/foreach_if.hpp>
#include <hatn/validator/validator.hpp>
#include <hatn/validator/utils/make_types_tuple.hpp>

#include <hatn/dataunit/fields/fieldtraits.h>
#include <hatn/dataunit/fields/scalar.h>
#include <hatn/dataunit/fields/bytes.h>
#include <hatn/dataunit/fields/subunit.h>
#include <hatn/dataunit/fields/repeated.h>
#include <hatn/dataunit/fields/datetime.h>
#include <hatn/dataunit/fields/date.h>
#include <hatn/dataunit/fields/time.h>
#include <hatn/dataunit/fields/daterange.h>
#include <hatn/dataunit/objectid.h>

#include <hatn/dataunit/unit.h>

#include <hatn/dataunit/readunitfieldatpath.h>
#include <hatn/dataunit/updateunitfieldatpath.h>

HATN_DATAUNIT_NAMESPACE_BEGIN

namespace hana=boost::hana;

template <typename ...Ts>
auto path(Ts&& ...keys)
{
    return HATN_VALIDATOR_NAMESPACE::path(std::forward<Ts>(keys)...);
}

/**  @brief Base template for unit types. */
template <typename ...Fields>
class UnitImpl
{
    public:

        /**  Ctor */
        UnitImpl(Unit* self);

        /**  Get field count */
        constexpr static int count() noexcept
        {
            return sizeof ...(Fields);
        }

        template <typename PredicateT, typename HandlerT>
        auto each(const PredicateT& pred, const HandlerT& handler) -> decltype(auto)
        {
            return HATN_VALIDATOR_NAMESPACE::foreach_if(m_fields,pred,handler);
        }

        template <typename PredicateT, typename HandlerT>
        auto each(const PredicateT& pred, const HandlerT& handler) const -> decltype(auto)
        {
            return HATN_VALIDATOR_NAMESPACE::foreach_if(m_fields,pred,handler);
        }

        template <typename PredicateT, typename HandlerT, typename DefaultT>
        auto each(const PredicateT& pred, DefaultT&& defaultRet, const HandlerT& handler) -> decltype(auto)
        {
            return HATN_VALIDATOR_NAMESPACE::foreach_if(m_fields,pred,std::forward<DefaultT>(defaultRet),handler);
        }

        template <typename PredicateT, typename HandlerT, typename DefaultT>
        auto each(const PredicateT& pred, DefaultT&& defaultRet, const HandlerT& handler) const -> decltype(auto)
        {
            return HATN_VALIDATOR_NAMESPACE::foreach_if(m_fields,pred,std::forward<DefaultT>(defaultRet),handler);
        }

        static const Field* findField(const UnitImpl* unit,int id);
        static Field* findField(UnitImpl* unit,int id);

        static const Field* findField(const UnitImpl* unit,common::lib::string_view name);
        static Field* findField(UnitImpl* unit,common::lib::string_view name);

        template <typename BufferT>
        struct FieldParser
        {
            std::function<bool(UnitImpl<Fields...>&, BufferT&, const AllocatorFactory*)> fn;
            WireType wireType;
            const char* fieldName;
            int fieldId;
        };

        template <typename BufferT>
        static const FieldParser<BufferT>* fieldParser(int id)
        {
            auto&& map=fieldParsers<BufferT>();
            const auto it=map.find(id);
            if (it==map.end())
            {
                return nullptr;
            }
            return &it->second;
        }

        template <typename BufferT, typename T>
        static const FieldParser<BufferT>* fieldParser(T&& /*field*/)
        {
            return fieldParser<BufferT>(std::decay_t<T>::ID);
        }

        /**  Iterate fields applying visitor handler */
        template <typename T>
        bool iterate(const T& visitor);

        /**  Iterate fields applying const visitor handler */
        template <typename T>
        bool iterateConst(const T& visitor) const;

        /**
         * @brief Check if field with some ID is present in the unit
         * @param id Integral constant holding ID number
         */
        template <typename Id>
        constexpr static auto hasId(Id id)
        {
            auto ts=hana::tuple_t<Fields...>;
            auto ret=hana::find_if(
                ts,
                [id](auto field)
                {
                    using fieldType=typename decltype(field)::type;
                    return hana::equal(id,hana::size_c<fieldType::ID>);
                }
            );
            return hana::not_equal(ret,hana::nothing);
        }

    protected:

        template <typename Index>
        auto get(Index&& idx) const -> decltype(auto)
        {
            return hana::at(m_fields,std::forward<Index>(idx));
        }

        template <typename Index>
        auto get(Index&& idx) -> decltype(auto)
        {
            return hana::at(m_fields,std::forward<Index>(idx));
        }

        hana::tuple<Fields...> m_fields;

    private:

        static const common::FlatMap<int,uintptr_t>& fieldsMap();
        static const common::FlatMap<common::lib::string_view,uintptr_t>& fieldsNameMap();

        template <typename BufferT>
        static const common::FlatMap<int,FieldParser<BufferT>>& fieldParsers();        
};

struct makeIndexMapT
{
    template <typename TypesC>
    constexpr auto operator()(TypesC typesC) const
    {
        constexpr auto indexes=hana::make_range(hana::int_c<0>,hana::size(typesC));
        constexpr auto pairs=hana::zip_with(hana::make_pair,typesC,hana::unpack(indexes,hana::make_tuple));
        return hana::unpack(pairs,hana::make_map);
    }
};
constexpr makeIndexMapT makeIndexMap{};

template <typename ...Fields>
struct fieldsMapT
{
    constexpr static auto f() noexcept
    {
        auto to_field_traits_c=[](auto x)
        {
            using field_c=typename decltype(x)::type;
            using field_type=typename field_c::traits;
            return hana::type_c<field_type>;
        };

        auto field_traits_c=hana::transform(hana::tuple_t<Fields...>,to_field_traits_c);
        return makeIndexMap(field_traits_c);
    }

    using type=decltype(f());
};

template <typename Conf, typename ...Fields>
struct makeUnitImpl
{
    static auto f()
    {
        auto fields=hana::tuple<Fields...>{};
        return hana::unpack(
                hana::transform(fields,typename Conf::to_field_c{}),
                hana::template_<UnitImpl>
            );
    }

    using type=typename decltype(f())::type;
};

/** @brief Base DataUnit template for concatenation with unit config. **/
template <typename Conf, typename ...Fields>
class UnitConcat : public Unit, public makeUnitImpl<Conf,Fields...>::type
{        
    public:

        using selfType=UnitConcat<Conf,Fields...>;
        using baseType=typename makeUnitImpl<Conf,Fields...>::type;
        using conf=Conf;

        using mapType=typename fieldsMapT<Fields...>::type;
        constexpr static mapType fieldsMap{};

        ~UnitConcat()=default;

        UnitConcat(
                const AllocatorFactory* factory=AllocatorFactory::getDefault()
            ) : Unit(factory),
                baseType(this)
        {}

        UnitConcat(
            UnitConcat&& other
            ) : Unit(std::move(other)),
            baseType(std::move(other))
        {
            setFieldsParent();
        }

        UnitConcat(
            const UnitConcat& other
            ) : Unit(other),
            baseType(other)
        {
            setFieldsParent();
        }

        UnitConcat& operator= (UnitConcat&& other)
        {
            if (&other==this)
            {
                return *this;
            }

            Unit::operator= (std::move(other));
            baseType::operator= (std::move(other));

            setFieldsParent();
            return *this;
        }

        UnitConcat& operator= (const UnitConcat& other)
        {
            if (&other==this)
            {
                return *this;
            }

            Unit::operator= (other);
            baseType::operator= (other);

            setFieldsParent();
            return *this;
        }

        /** Stub for compilation compatibility with SharedPtr */
        constexpr static bool isNull() noexcept
        {
            return false;
        }

        /**  Check if unit has a field. */
        template <typename T>
        constexpr static bool hasField(T&&) noexcept
        {
            constexpr auto idx=hana::type_c<std::decay_t<T>>;
            return hana::contains(fieldsMap,idx);
        }

        template <typename T>
        constexpr static bool hasField() noexcept
        {
            constexpr auto idx=hana::type_c<std::decay_t<T>>;
            return hana::contains(fieldsMap,idx);
        }

        /**  Get position of field */
        template <typename T>
        constexpr static auto fieldPos() noexcept
        {
            return fieldPos(T{});
        }

        /**  Get position of field */
        template <typename T>
        constexpr static auto fieldPos(T&& fieldName) noexcept
        {
            return hana::value(fieldIndex(std::forward<T>(fieldName)));
        }

        /**  Get field reference by index */
        template <int Index>
        auto field() noexcept -> decltype(auto)
        {
            return this->get(hana::int_c<Index>);
        }

        /**  Get const field reference by index */
        template <int Index>
        auto field() const noexcept -> decltype(auto)
        {
            return this->get(hana::int_c<Index>);
        }

        /**  Get field by type. */
        template <typename T>
        auto field() const noexcept -> decltype(auto)
        {
            return field(T{});
        }

        /**  Get field by type. */
        template <typename T>
        auto field() noexcept -> decltype(auto)
        {
            return field(T{});
        }

        /**  Get field. */
        template <typename T>
        auto field(T&& fieldName) const noexcept -> decltype(auto)
        {
            return this->get(fieldIndex(std::forward<T>(fieldName)));
        }

        /**  Get field. */
        template <typename T>
        auto field(T&& fieldName) noexcept -> decltype(auto)
        {
            return this->get(fieldIndex(std::forward<T>(fieldName)));
        }

        /**  Check if field is set. */
        template <typename T>
        bool isSet(T&& fieldName) const noexcept
        {
            constexpr auto idx=hana::type_c<std::decay_t<T>>;
            auto self=this;
            return hana::eval_if(
                hana::contains(fieldsMap,idx),
                [&](auto _)
                {
                    return _(self)->field(_(fieldName)).isSet();
                },
                [&](auto)
                {
                    return false;
                }
            );
        }

        /**
         * @brief Set field.
         * @param fieldName Field.
         * @param val Value.
         */
        template <typename T, typename ...Args>
        void setFieldValue(T&& fieldName, Args&&... val)
        {
            field(std::forward<T>(fieldName)).set(std::forward<Args>(val)...);
        }

        /** Get field's value. */
        template <typename T>
        auto fieldValue(T&& fieldName) const noexcept -> decltype(auto)
        {
            return field(std::forward<T>(fieldName)).value();
        }

        /**  Get field value using brackets. */
        template <typename T>
        auto operator [] (T&& fieldName) const -> decltype(auto)
        {
            return fieldValue(std::forward<T>(fieldName));
        }

        /** Clear field. */
        template <typename T>
        void clearField(T&& fieldName) noexcept
        {
            field(std::forward<T>(fieldName)).clear();
        }

        /** Reset field. */
        template <typename T>
        void resetField(T&& fieldName) noexcept
        {
            field(std::forward<T>(fieldName)).reset();
        }

        /**
          @brief Get field at given path.
          @param path Path to the field in format _[level1][level2]...[levelN].
          @return Field at given path.
         **/
        template <typename PathT>
        const auto& fieldAtPath(PathT&& path) const
        {
            return UnitFieldUpdater::fieldAtPath(*this,path);
        }

        /**
          @brief Get field at given path.
          @param path Path to the field in format _[level1][level2]...[levelN].
          @return Field at given path.
         **/
        template <typename PathT>
        auto& fieldAtPath(PathT&& path)
        {
            return UnitFieldUpdater::fieldAtPath(*this,path);
        }

        /**
          @brief Get data from the field at given path.
          @param path Path to the field in format _[level1][level2]...[levelN].
          @return Value form the field at given path.
         **/
        template <typename PathT>
        auto getAtPath(PathT&& path) const -> decltype(auto)
        {
            return getUnitFieldAtPath(*this,path);
        }

        /**
          @brief Get size of content of the field at given path.
          @param path Path to the field in format _[level1][level2]...[levelN].
          @return Size of content of field at given path.
         **/
        template <typename PathT>
        auto sizeAtPath(PathT&& path) const
        {
            return sizeUnitFieldAtPath(*this,path);
        }

        /**
          @brief Write data to content of the field at given path.
          @param path Path to the field in format _[level1][level2]...[levelN].
          @param args Data to write.
         **/
        template <typename PathT, typename ...Args>
        void setAtPath(PathT&& path, Args&&... args)
        {
            setUnitFieldAtPath(*this,std::forward<PathT>(path),std::forward<Args>(args)...);
        }

        /**
          @brief Unset field at given path.
          @param path Path to the field in format _[level1][level2]...[levelN].
         **/
        template <typename PathT>
        void unsetAtPath(PathT&& path)
        {
            unsetUnitFieldAtPath(*this,std::forward<PathT>(path));
        }

        /**
          @brief Clear content of the field at given path if applicable to fields of this types.
          @param path Path to the field in format _[level1][level2]...[levelN].

          Applicable only to byte, string, repeatable and subunit fields.
          If operation is not applicable to the field then it is silently ignored.
         **/
        template <typename PathT>
        void clearAtPath(PathT&& path)
        {
            clearUnitFieldAtPath(*this,std::forward<PathT>(path));
        }

        /**
          @brief Resize content of the field at given path if applicable to fields of this types.
          @param path Path to the field in format _[level1][level2]...[levelN].
          @param size New size.

          Applicable only to byte, string and repeatable fields.
          If operation is not applicable to the field then it is silently ignored.
         **/
        template <typename PathT>
        void resizeAtPath(PathT&& path, size_t size)
        {
            resizeUnitFieldAtPath(*this,std::forward<PathT>(path),size);
        }

        /**
          @brief Reserve space for content of the field at given path if applicable to fields of this types.
          @param path Path to the field in format _[level1][level2]...[levelN].
          @param size Size to reserve.

          Applicable only to byte, string and repeatable fields.
          If operation is not applicable to the field then it is silently ignored.
         **/
        template <typename PathT>
        void reserveAtPath(PathT&& path, size_t size)
        {
            reserveUnitFieldAtPath(*this,std::forward<PathT>(path),size);
        }

        /**
          @brief Append data to content of the field at given path if applicable to fields of this types.
          @param path Path to the field in format _[level1][level2]...[levelN].
          @param args Data to append.

          Applicable only to byte, string and repeatable fields.
          If operation is not applicable to the field then it is silently ignored.
         **/
        template <typename PathT, typename ...Args>
        void appendAtPath(PathT&& path, Args&&... args)
        {
            appendUnitFieldAtPath(*this,std::forward<PathT>(path),std::forward<Args>(args)...);
        }

        /**
          @brief Automatically append items to repeatable field at given path.
          @param path Path to the field in format _[level1][level2]...[levelN].
          @param size Number of items to append.

          Auto appending means that each item is appended with default initialization as
          defined in createAndAppendValue() method for the item's type. For example, shared byte arrays or shared subunits
          will be allocated appropriately. In contrast, when using ordinary resize methods the items are not initialized
          and must be initialized manually before use.

          Applicable only to repeatable fields.
          If operation is not applicable to the field then it is silently ignored.
         **/
        template <typename PathT>
        void autoAppendAtPath(PathT&& path, size_t size)
        {
            autoAppendUnitFieldAtPath(*this,std::forward<PathT>(path),size);
        }

        /**
         * @brief Iterate over subunits of self type in a tree invoking handler on each subunit.
         * @param handler Handler to invoke on each subunit. Signature bool(const T*,const T*,size_t).
         * @param self Self object.
         * @param fieldName Field with subunits.
         * @param parent Parent object.
         * @param level Current level.
         * @return Iteration status.
         */
        template <typename T,typename HandlerT,typename Field>
        bool iterateUnitTree(
            const HandlerT& handler,
            const T* self,
            const Field& fieldName,
            const T* parent,
            size_t level=0
        ) const;

        /**
         * @brief Iterate over subunits of self type in a tree invoking handler on each subunit.
         * @param handler Handler to invoke on each subunit. Signature bool(const T*,const T*,size_t).
         * @param self Self object.
         * @param fieldName Field with subunits.
         * @return Iteration status.
         */
        template <typename T,typename HandlerT,typename Field>
        bool iterateUnitTree(
            const HandlerT& handler,
            const T* self,
            const Field& fieldName
        ) const
        {
            T* dummy=nullptr;
            return iterateUnitTree(handler,self,fieldName,dummy);
        }

        /**  Get name */
        constexpr static const char* unitName() noexcept
        {
            return Conf::name;
        }

    private:

        void setFieldsParent() noexcept
        {
            hana::for_each(
                this->m_fields,
                [this](auto& field)
                {
                    this->setFieldParent(field);
                }
                );
        }

        template <typename T>
        constexpr static auto fieldIndex(T&&) noexcept
        {
            constexpr auto idx=hana::type_c<std::decay_t<T>>;
            static_assert(hana::value(hana::contains(fieldsMap,idx)),"Field not found");
            return fieldsMap[idx];
        }
};

/**   template */
template <typename Conf,typename ...Fields> using DataUnit=UnitConcat<Conf,Fields...>;

/**  Managed variant of the DataUnit */
template <typename UnitType>
class ManagedUnit : public common::EnableSharedFromThis<ManagedUnit<UnitType>>, public UnitType
{
    public:

        using UnitType::UnitType;

        /**
         * @brief Create dynamically allocated DataUnit of self type
         *
         * Only managed versions of units can be created
         */
        virtual common::SharedPtr<Unit> createManagedUnit() const override
        {
            return this->factory()->template createObject<ManagedUnit<UnitType>>(this->factory()).template staticCast<Unit>();
        }

        virtual common::SharedPtr<Unit> toManagedUnit() const override
        {
            return common::SharedPtr<Unit>{this->sharedFromThis()};
        }

        virtual bool isManagedUnit() const noexcept override
        {
            return true;
        }
};

/**  Empty DataUnit template */
template <typename Conf>
class EmptyUnit : public Unit
{
    public:

        using Unit::Unit;

        using conf=Conf;

        /** Get reference to self **/
        const EmptyUnit& value() const noexcept
        {
            return *this;
        }

        /** Get mutable pointer to self **/
        EmptyUnit* mutableValue() noexcept
        {
            return this;
        }

        /** Stub for compilation compatibility with SharedPtr */
        constexpr static bool isNull() noexcept
        {
            return false;
        }

        /**  Get name */
        constexpr static const char* unitName() noexcept
        {
            return Conf::name;
        }

        template <typename PredicateT, typename HandlerT, typename InitT>
        auto each(const PredicateT&, InitT&& init, const HandlerT&) -> decltype(auto)
        {
            return hana::id(std::forward<InitT>(init));
        }

        template <typename PredicateT, typename HandlerT, typename InitT>
        auto each(const PredicateT&, InitT&& init, const HandlerT&) const -> decltype(auto)
        {
            return hana::id(std::forward<InitT>(init));
        }

        template <typename BufferT>
        struct FieldParser
        {
            std::function<bool(EmptyUnit&, BufferT&, const AllocatorFactory*)> fn;
            WireType wireType;
            const char* fieldName;
            int fieldId;
        };

        template <typename BufferT>
        static const FieldParser<BufferT>* fieldParser(int)
        {
            return nullptr;
        }

        template <typename BufferT, typename T>
        static const FieldParser<BufferT>* fieldParser(T&& /*field*/)
        {
            return nullptr;
        }

        /**  Iterate fields applying visitor handler */
        template <typename T>
        bool iterate(const T&)
        {
            return true;
        }

        /**  Iterate fields applying const visitor handler */
        template <typename T>
        bool iterateConst(const T&) const
        {
            return true;
        }

        virtual int serialize(WireData&,bool) const override
        {
            return 0;
        }

        virtual bool parse(WireData&,bool =true) override
        {
            return true;
        }

        virtual bool parse(WireBufSolid&,bool =true) override
        {
            return true;
        }

        virtual bool iterateFields(const Unit::FieldVisitor&) override
        {
            return true;
        }

        virtual bool iterateFieldsConst(const Unit::FieldVisitorConst&) const override
        {
            return true;
        }

        virtual const Field* fieldById(int) const override
        {
            return nullptr;
        }

        virtual Field* fieldById(int) override
        {
            return nullptr;
        }

        virtual const Field* fieldByName(common::lib::string_view) const override
        {
            return nullptr;
        }
        virtual Field* fieldByName(common::lib::string_view) override
        {
            return nullptr;
        }

#if 0
        virtual int serialize(WireBufSolid& wired,bool topLevel=true) const
        {
            return 0;
        }

        virtual int serialize(WireBufSolidShared& wired,bool topLevel=true) const
        {
            return 0;
        }

        virtual int serialize(WireBufChained& wired,bool topLevel=true) const
        {
            return 0;
        }

        virtual bool parse(WireBufSolidShared& wired,bool topLevel=true) override
        {
            return true;
        }
#endif
};

/**  Managed variant of empty DataUnit */
template <typename Conf>
class EmptyManagedUnit : public EmptyUnit<Conf>, public common::EnableManaged<EmptyManagedUnit<Conf>>
{
    public:

        using selfType=EmptyManagedUnit<Conf>;

        using EmptyUnit<Conf>::EmptyUnit;

        /**
         * @brief Create dynamically allocated DataUnit of self type
         *
         * Only managed versions of units can be created
         */
        virtual common::SharedPtr<Unit> createManagedUnit() const override
        {
            return this->factory()->template createObject<EmptyManagedUnit<Conf>>(this->factory());
        }
};

//---------------------------------------------------------------
template <typename Conf, typename ...Fields>
template <typename T, typename HandlerT, typename Field>
bool UnitConcat<Conf,Fields...>::iterateUnitTree(
        const HandlerT& handler,
        const T* self,
        const Field& fieldName,
        const T* parent,
        size_t level
    ) const
{
    static_assert(Field::type::isRepeatedType::value,"Only repeated fields can be iterated as tree");
    static_assert(Field::type::fieldType::isUnitType::value,"Only dataunit fields can be iterated as tree");
    static_assert(
                  std::is_same<::hatn::common::SharedPtr<Unit>,typename Field::type::type>::value,
                  "Only shared dataunit fields can be iterated as tree"
                );

    // process self
    if (!handler(self,parent,level))
    {
        return false;
    }

    // find field
    const auto& field=this->field(fieldName);

    // process all units in field array
    for (auto&& it:field.vector)
    {
        auto unit=self->castToUnit(it.get());
        if (!unit->iterateUnitTree(handler,static_cast<const T*>(unit),fieldName,self,level+1))
        {
            return false;
        }
    }

    // complete
    return true;
}

//---------------------------------------------------------------
template <typename ...Fields>
Field* UnitImpl<Fields...>::findField(UnitImpl* unit,int id)
{
    const auto& m=fieldsMap();
    const auto it=m.find(id);
    if (it!=m.end())
    {
        return reinterpret_cast<Field*>(reinterpret_cast<uintptr_t>(unit)+it->second);
    }
    return nullptr;
}

template <typename ...Fields>
const Field* UnitImpl<Fields...>::findField(const UnitImpl* unit,int id)
{
    const auto& m=fieldsMap();
    const auto it=m.find(id);
    if (it!=m.end())
    {
        return reinterpret_cast<const Field*>(reinterpret_cast<uintptr_t>(unit)+it->second);
    }
    return nullptr;
}

template <typename ...Fields>
Field* UnitImpl<Fields...>::findField(UnitImpl* unit,common::lib::string_view name)
{
    const auto& m=fieldsNameMap();
    const auto it=m.find(name);
    if (it!=m.end())
    {
        return reinterpret_cast<Field*>(reinterpret_cast<uintptr_t>(unit)+it->second);
    }
    return nullptr;
}

template <typename ...Fields>
const Field* UnitImpl<Fields...>::findField(const UnitImpl* unit,common::lib::string_view name)
{
    const auto& m=fieldsNameMap();
    const auto it=m.find(name);
    if (it!=m.end())
    {
        return reinterpret_cast<const Field*>(reinterpret_cast<uintptr_t>(unit)+it->second);
    }
    return nullptr;
}

//---------------------------------------------------------------
template <typename ...Fields>
template <typename T>
bool UnitImpl<Fields...>::iterate(const T& visitor)
{
    auto predicate=[](bool ok)
    {
        return ok;
    };

    auto handler=[&visitor](auto& field, auto&&)
    {
        return visitor(field);
    };

    return each(predicate,handler);
}

//---------------------------------------------------------------
template <typename ...Fields>
template <typename T>
bool UnitImpl<Fields...>::iterateConst(const T& visitor) const
{
    auto predicate=[](bool ok)
    {
        return ok;
    };

    auto handler=[&visitor](const auto& field, auto&&)
    {
        return visitor(field);
    };

    return each(predicate,handler);
}

//---------------------------------------------------------------

template <typename ...Fields>
template <typename BufferT>
const common::FlatMap<
    int,
    typename UnitImpl<Fields...>::template FieldParser<BufferT>
    >&
UnitImpl<Fields...>::fieldParsers()
{
    using unitT=UnitImpl<Fields...>;
    using itemT=typename UnitImpl<Fields...>::template FieldParser<BufferT>;

    auto f=[](auto&& state, auto fieldTypeC) {

        using type=typename decltype(fieldTypeC)::type;

        auto index=hana::first(state);
        auto map=hana::second(state);

        auto handler=[&index](unitT& unit, BufferT& buf, const AllocatorFactory* factory)
        {
            auto& field=hana::at(unit.m_fields,index);
            return field.deserialize(buf,factory);
        };
        auto item=itemT{
            handler,
            type::fieldWireType(),
            type::fieldName(),
            type::fieldId()
        };
        map[type::fieldId()]=item;

        return hana::make_pair(hana::plus(index,hana::int_c<1>),std::move(map));
    };

    static const auto result=hana::fold(
        hana::tuple_t<Fields...>,
        hana::make_pair(
            hana::int_c<0>,
            common::FlatMap<int,itemT>{}
            ),
        f
        );
    static const auto map=hana::second(result);
    return map;
}

//---------------------------------------------------------------
HATN_DATAUNIT_NAMESPACE_END

#endif // HATNDATAUNITS_H
