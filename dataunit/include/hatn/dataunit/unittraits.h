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

#include <hatn/validator/utils/foreach_if.hpp>

#include <hatn/dataunit/fields/fieldtraits.h>
#include <hatn/dataunit/fields/scalar.h>
#include <hatn/dataunit/fields/bytes.h>
#include <hatn/dataunit/fields/subunit.h>
#include <hatn/dataunit/fields/repeated.h>

#include <hatn/dataunit/readunitfieldatpath.h>
#include <hatn/dataunit/updateunitfieldatpath.h>

HATN_DATAUNIT_NAMESPACE_BEGIN

namespace hana=boost::hana;

namespace detail
{
struct FalseFnT
{
    template <typename T>
    constexpr bool operator() (T) const
    {
        return false;
    }
};
constexpr FalseFnT FalseFn{};

template <typename FieldT>
struct CheckEachUnitFieldT
{
    template <typename T>
    constexpr auto operator () (T) const
    {
        using type=typename T::type;
        return hana::bool_<
                    FieldT::ID==type::ID
                    &&
                    hana::is_a<typename FieldT::Type,type>
                >{};
    }
};
template <typename FieldT>
constexpr CheckEachUnitFieldT<FieldT> CheckEachUnitField{};

template <typename ...Fields>
struct UnitHasFieldT
{
    template <typename T>
    constexpr bool operator () (T) const
    {
        return hana::find_if(
                hana::tuple_t<Fields...>,
                CheckEachUnitField<typename T::type>
            ) != hana::nothing;
    }
};
template <typename ...Fields>
constexpr UnitHasFieldT<Fields...> UnitHasField{};
}

/**
 * @brief Helper to check if Unit has a field.
 */
template <typename FieldT, typename ...Fields>
struct UnitHasField
{
    constexpr static const bool value=
                        hana::if_(
                            hana::is_a<FieldTag,FieldT>,
                            dataunit::detail::UnitHasField<Fields...>,
                            dataunit::detail::FalseFn
                        )(hana::type_c<FieldT>);
};

/**  Base DataUnit template */
template <typename ...Fields>
    class UnitImpl : public hatn::common::VInterfacesPack<Fields...>
{
    constexpr static const unsigned short int MaxI = sizeof...(Fields)-1;

    public:

        /**  Ctor */
        UnitImpl(Unit* self);

        virtual ~UnitImpl()=default;
        UnitImpl(const UnitImpl& other)=default;
        UnitImpl& operator= (const UnitImpl& other)=default;
        UnitImpl(UnitImpl&& other) =default;
        UnitImpl& operator= (UnitImpl&& other) =default;

        /**  Get position of field */
        template <typename T>
        constexpr static int fieldPos() noexcept
        {
            return std::decay_t<T>::index;
        }

        /**  Get position of field */
        template <typename T>
        constexpr static int fieldPos(T&&) noexcept
        {
            return fieldPos<std::decay_t<T>>();
        }

        /**  Get field by type. */
        template <typename T>
        auto field() const noexcept -> decltype(auto)
        {
            using type=std::decay_t<T>;
            static_assert(UnitHasField<type,Fields...>::value,"Invalid field");
            return static_cast<const typename std::tuple_element<type::index, std::tuple<Fields...>>::type&>(this->template getInterface<type::index>());
        }

        /**  Get field by type. */
        template <typename T>
        auto field() noexcept -> decltype(auto)
        {
            using type=std::decay_t<T>;
            static_assert(UnitHasField<type,Fields...>::value,"Invalid field");
            return this->template getInterface<type::index>();
        }

        /**  Get field. */
        template <typename T>
        auto field(T&&
            ) const noexcept -> decltype(auto)
        {
            return field<std::decay_t<T>>();
        }

        /**  Get field. */
        template <typename T>
        auto field(T&&
               ) noexcept -> decltype(auto)
        {
            return field<std::decay_t<T>>();
        }

        /**  Check if field is set. */
        template <typename T>
        auto isSet(T&& fieldName,
                   std::enable_if_t<UnitHasField<std::decay_t<T>,Fields...>::value,void*> =nullptr
                ) const noexcept
        {
            return field(std::forward<T>(fieldName)).isSet();
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

        /**  Get field reference by index */
        template <int Index>
        auto field() noexcept -> decltype(auto)
        {
            return this->template getInterface<Index>();
        }

        /**  Get const field reference by index */
        template <int Index>
        auto field() const noexcept -> decltype(auto)
        {
            return static_cast<const typename std::tuple_element<Index, std::tuple<Fields...>>::type&>(this->template getInterface<Index>());
        }

        /**  Get field count */
        constexpr static int count() noexcept
        {
            return sizeof...(Fields);
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
          defined in createAndAddValue() method for the item's type. For example, shared byte arrays or shared subunits
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

        template <typename PredicateT, typename HandlerT>
        auto each(const PredicateT& pred, const HandlerT& handler) -> decltype(auto)
        {
            return hatn::validator::foreach_if(this->m_interfaces,pred,handler);
        }

        template <typename PredicateT, typename HandlerT>
        auto each(const PredicateT& pred, const HandlerT& handler) const -> decltype(auto)
        {
            return hatn::validator::foreach_if(this->m_interfaces,pred,handler);
        }

        template <typename PredicateT, typename HandlerT, typename InitT>
        auto each(const PredicateT& pred, InitT&& init, const HandlerT& handler) -> decltype(auto)
        {
            return hatn::validator::foreach_if(this->m_interfaces,pred,std::forward<InitT>(init),handler);
        }

        template <typename PredicateT, typename HandlerT, typename InitT>
        auto each(const PredicateT& pred, InitT&& init, const HandlerT& handler) const -> decltype(auto)
        {
            return hatn::validator::foreach_if(this->m_interfaces,pred,std::forward<InitT>(init),handler);
        }

        static const Field* findField(const UnitImpl* unit,int id);
        static Field* findField(UnitImpl* unit,int id);

        template <typename BufferT>
        using ParsingHandlerFnT=std::function<bool(UnitImpl<Fields...>&, BufferT&, AllocatorFactory*)>;

        template <typename BufferT>
        static const ParsingHandlerFnT<BufferT>* fieldParser(int id)
        {
            auto&& map=parsingHandlers<BufferT>();
            const auto it=map.find(id);
            if (it==map.end())
            {
                return nullptr;
            }
            return &it->second;
        }

        template <typename BufferT, typename T>
        static const ParsingHandlerFnT<BufferT>* fieldParser(T&& /*field*/)
        {
            return fieldParser<BufferT>(std::decay_t<T>::ID);
        }

    protected:

        /**  Iterate fields applying visitor handler */
        bool iterate(const Unit::FieldVisitor& visitor);

        /**  Iterate fields applying const visitor handler */
        bool iterateConst(const Unit::FieldVisitorConst& visitor) const;

    private:

        static const common::FlatMap<int,uintptr_t>& fieldsMap();

        template <typename BufferT>
        static const common::FlatMap<int,ParsingHandlerFnT<BufferT>>& parsingHandlers();
};

/** Base DataUnit template for concatenation **/
template <typename Conf, typename ...Fields>
class UnitConcat : public Unit, public UnitImpl<Fields...>
{
    public:

        using selfType=UnitConcat<Conf,Fields...>;
        using baseType=UnitImpl<Fields...>;
        using conf=Conf;

        UnitConcat(AllocatorFactory* factory=AllocatorFactory::getDefault());

        /** Stub for compilation compatibility with SharedPtr */
        constexpr static bool isNull() noexcept
        {
            return false;
        }

        /**  Check if unit has a field. */
        template <typename T>
        constexpr static bool hasField(T&& fieldName) noexcept
        {
            std::ignore=fieldName;
            return UnitHasField<std::decay_t<T>,Fields...>::value;
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

    protected:

        /**  Get field const pointer by ID */
        const Field* doFieldById(int id) const;

        /**  Get field pointer by ID */
        Field* doFieldById(int id);

        /**  Iterate fields applying visitor handler */
        bool doIterateFields(const Unit::FieldVisitor& visitor);

        /**  Iterate fields applying visitor handler */
        bool doIterateFieldsConst(const Unit::FieldVisitorConst& visitor) const;

        /**  Get field count */
        size_t doFieldCount() const noexcept;
};

/**  DataUnit template */
template <typename Conf,typename ...Fields> using DataUnit=UnitConcat<Conf,Fields...>;

/**  Managed variant of the DataUnit */
template <typename UnitType>
class ManagedUnit : public common::EnableManaged<ManagedUnit<UnitType>>, public UnitType
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

    protected:

        /**  Iterate fields applying visitor handler */
        bool doIterateFields(const Unit::FieldVisitor&)
        {
            return true;
        }

        /**  Iterate fields applying visitor handler */
        bool doIterateFieldsConst(const Unit::FieldVisitorConst&) const
        {
            return true;
        }

        /**  Get field count */
        constexpr static size_t doFieldCount() noexcept
        {
            return 0;
        }

        /** Get field by ID */
        const Field* doFieldById(int) const
        {
            return nullptr;
        }

        /** Get field by ID */
        Field* doFieldById(int)
        {
            return nullptr;
        }
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

//---------------------------------------------------------------
HATN_DATAUNIT_NAMESPACE_END

#endif // HATNDATAUNITS_H
