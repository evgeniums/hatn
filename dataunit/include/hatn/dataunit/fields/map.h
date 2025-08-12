/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/****************************************************************************/
/*

*/
/** @file dataunit/map.h
  *
  *      DataUnit fields of map types
  *
  */

/****************************************************************************/

#ifndef HATNDATAUNITMAPFIELDS_H
#define HATNDATAUNITMAPFIELDS_H

#include <type_traits>

#include <boost/hana.hpp>

#include <hatn/dataunit/fields/subunit.h>
#include <hatn/dataunit/fields/fieldtraits.h>

HATN_DATAUNIT_NAMESPACE_BEGIN

namespace detail
{
    struct MapItemKey
    {
        constexpr static const auto index=hana::size_c<0>;
        constexpr static const int id=1;
        constexpr static const char* name="key";
    };

    struct MapItemValue
    {
        constexpr static const auto index=hana::size_c<1>;
        constexpr static const int id=2;
        constexpr static const char* name="value";
    };

    template <ValueType Type, typename =hana::when<true>>
    struct MapItemFieldHelper
    {
        constexpr static auto fieldWireType() noexcept
        {
            return WireType::VarInt;
        }

        constexpr static bool fieldCanChainBlocks() noexcept
        {
            return false;
        }

        template <typename T, typename BufferT>
        static bool serialize(const T& /*val*/, BufferT& /*buf*/)
        {
            return false;
        }

        template <typename T, typename BufferT>
        static bool deserialize(T& /*val*/, BufferT& /*buf*/, const AllocatorFactory* /*factory*/)
        {
            return false;
        }
    };

    template <ValueType Type>
    struct MapItemFieldHelper<Type, hana::when< IsInt<Type> || IsBool<Type> >>
    {
        constexpr static auto fieldWireType() noexcept
        {
            return WireType::VarInt;
        }

        constexpr static bool fieldCanChainBlocks() noexcept
        {
            return false;
        }

        template <typename T, typename BufferT>
        static bool serialize(const T& val, BufferT& buf)
        {
            return VariableSer<std::decay_t<T>>::serialize(val,buf);
        }

        template <typename T, typename BufferT>
        static bool deserialize(T& val, BufferT& buf, const AllocatorFactory* /*factory*/)
        {
            return VariableSer<std::decay_t<T>>::deserialize(val,buf);
        }

        template <typename T>
        constexpr static size_t fieldSize(const T&) noexcept
        {
            // varint can take +1 byte more than size of value
            return sizeof(std::decay_t<T>)+1;
        }
    };

    template <ValueType Type>
    struct MapItemFieldHelper<Type, hana::when< IsString<Type> >>
    {
        struct StringWrapper
        {
            size_t size() const noexcept
            {
                return ref.size();
            }

            const char* data() const noexcept
            {
                return ref.data();
            }

            void loadInline(
                const char* data,
                size_t size
                )
            {
                ref.assign(data,size);
            }

            void load(
                const char* data,
                size_t size
                )
            {
                ref.assign(data,size);
            }

            void clear()
            {
                ref.clear();
            }

            std::string& ref;
        };

        template <typename T>
        static size_t fieldSize(const T& value)
        {
            return value.size()+sizeof(uint32_t);
        }

        constexpr static auto fieldWireType() noexcept
        {
            return WireType::WithLength;
        }

        constexpr static bool fieldCanChainBlocks() noexcept
        {
            return false;
        }

        template <typename T, typename BufferT>
        static bool serialize(const T& val, BufferT& buf)
        {
            using type=std::decay_t<T>;
            return BytesSer<type,common::SharedPtr<type>>::serialize(buf,&val,common::SharedPtr<type>{},fieldCanChainBlocks());
        }

        template <typename T, typename BufferT>
        static bool deserialize(T& val, BufferT& buf, const AllocatorFactory* factory)
        {
            StringWrapper w{val};
            common::SharedPtr<StringWrapper> shared;
            return BytesSer<StringWrapper,common::SharedPtr<StringWrapper>>::deserialize(buf,&w,shared,factory,0,fieldCanChainBlocks());
        }
    };

    template <typename T, bool Const>
    struct MapElementConstWrapper
    {
    };
    template <typename T>
    struct MapElementConstWrapper<T,true>
    {
        using type=std::add_const_t<T>;
    };
    template <typename T>
    struct MapElementConstWrapper<T,false>
    {
        using type=T;
    };

    template <typename T, ValueType Type, typename ItemPart, bool Const>
    struct MapItemFieldWrapper
    {
        using type=std::reference_wrapper<typename MapElementConstWrapper<T,Const>::type>;

        template <typename T1>
        MapItemFieldWrapper(T1&& val) :
            ref(std::forward<T1>(val))
        {}

        constexpr static bool isNoSerialize() noexcept
        {
            return false;
        }

        constexpr static bool fieldIsSet() noexcept
        {
            return true;
        }

        constexpr static bool isSet() noexcept
        {
            return true;
        }

        constexpr static bool fieldRepeatedUnpackedProtoBuf() noexcept
        {
            return false;
        }

        constexpr static int fieldId() noexcept
        {
            return ItemPart::id;
        }

        constexpr static const char* fieldName() noexcept
        {
            return ItemPart::name;
        }

        constexpr static void fieldReset() noexcept
        {}

        constexpr static void fieldClear() noexcept
        {}

        constexpr static bool fieldRequired() noexcept
        {
            return true;
        }

        constexpr static void setParseToSharedArrays(bool /*enable*/, const AllocatorFactory */*factory*/) noexcept
        {}

        constexpr static auto fieldWireType() noexcept
        {
            return MapItemFieldHelper<Type>::fieldWireType();
        }

        constexpr static bool fieldCanChainBlocks() noexcept
        {
            return MapItemFieldHelper<Type>::fieldCanChainBlocks();;
        }

        template <typename BufferT>
        bool serialize(BufferT& buf) const
        {
            return MapItemFieldHelper<Type>::serialize(value(),buf);
        }

        template <typename BufferT>
        bool deserialize(BufferT& buf, const AllocatorFactory* factory)
        {
            return MapItemFieldHelper<Type>::deserialize(value(),buf,factory);
        }

        size_t fieldSize() const
        {
            return MapItemFieldHelper<Type>::fieldSize(value());
        }

        auto value() const -> decltype(auto)
        {
            return ref.get();
        }

        auto value() -> decltype(auto)
        {
            return ref.get();
        }

        type ref;
    };

    template <typename T, bool Const, ValueType Type=valueTypeOf<std::decay_t<T>>::value>
    using MapKeyWrapper=MapItemFieldWrapper<T,Type,MapItemKey,Const>;

    template <typename T, bool Const, ValueType Type=valueTypeOf<std::decay_t<T>>::value>
    using MapValueWrapper=MapItemFieldWrapper<T,Type,MapItemValue,Const>;

    template <typename Type>
    struct MapVariableType
    {
        using type=typename Type::type;
    };

    template <>
    struct MapVariableType<TYPE_STRING>
    {
        using type=std::string;
    };

    template <typename KeyType, typename ValueType, bool Const=true>
    struct MapItemWrapper
    {
        using selfType=MapItemWrapper<KeyType,ValueType,Const>;

        using keyType=MapKeyWrapper<typename MapVariableType<KeyType>::type,Const>;
        using valueType=MapValueWrapper<typename MapVariableType<ValueType>::type,Const>;
        using type=hana::tuple<keyType,valueType>;

        template <typename KeyT, typename ValueT>
        MapItemWrapper(KeyT&& key, ValueT&& value, const AllocatorFactory* factory)
            : m_fields(hana::make_tuple(std::forward<KeyT>(key),std::forward<ValueT>(value))),
              m_factory(factory)
        {}

        static const common::SharedPtr<WireData>& wireDataKeeper() noexcept
        {
            static common::SharedPtr<WireData> inst{};
            return inst;
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

        template <typename T>
        bool iterate(const T& visitor)
        {
            if (!visitor(hana::at(m_fields,MapItemKey::index)))
            {
                return false;
            }
            return visitor(hana::at(m_fields,MapItemValue::index));
        }

        size_t maxPackedSize() const
        {
            return io::size(*this);
        }

        template <typename BufferT>
        struct FieldParser
        {
            std::function<bool(selfType&, BufferT&, const AllocatorFactory*)> fn;

            WireType wireType;
            const char* fieldName;
            int fieldId;
        };

        template <typename BufferT>
        static const auto* fieldParser(int id)
        {
            static FieldParser<BufferT> keyParser{
                [](selfType& wrapper, BufferT& buf, const AllocatorFactory* factory)
                {
                    auto& field=hana::at(wrapper.m_fields,MapItemKey::index);
                    return field.deserialize(buf,factory);
                },
                keyType::fieldWireType(),
                keyType::fieldName(),
                keyType::fieldId()
            };
            static FieldParser<BufferT> valueParser{
                [](selfType& wrapper, BufferT& buf, const AllocatorFactory* factory)
                {
                    auto& field=hana::at(wrapper.m_fields,MapItemValue::index);
                    return field.deserialize(buf,factory);
                },
                valueType::fieldWireType(),
                valueType::fieldName(),
                valueType::fieldId()
            };

            if (id==MapItemKey::id)
            {
                return &keyParser;
            }

            return &valueParser;
        }

        constexpr static void setClean(bool) noexcept
        {}

        constexpr static bool isClean() noexcept
        {
            return true;
        }

        constexpr static void resetWireDataKeeper() noexcept
        {}

        const AllocatorFactory* factory() const noexcept
        {
            return m_factory;
        }

        type m_fields;
        const AllocatorFactory* m_factory;
    };
}

struct map_tag{};

struct map_config
{
    using hana_tag=map_tag;
};

struct MapType{};

/**  Template class for Map dataunit field */
template <typename Type, int Id>
struct MapFieldTmpl : public Field, public MapType
{
    using pseudoSubunitType=typename Type::type;

    using keyFieldType=typename pseudoSubunitType::template fieldType<detail::MapItemKey::index.value>;
    using valueFieldType=typename pseudoSubunitType::template fieldType<detail::MapItemValue::index.value>;

    using keyTypeId=typename keyFieldType::Type;
    using valueTypeId=typename valueFieldType::Type;

    using keyType=typename detail::MapVariableType<keyTypeId>::type;
    using valueType=typename detail::MapVariableType<valueTypeId>::type;

    using type=common::pmr::map<keyType,valueType>;

    using isMapType=std::true_type;
    using selfType=MapFieldTmpl<Type,Id>;

    constexpr static const ValueType typeId=Type::typeId;

    constexpr static int fieldId()
    {
        return Id;
    }

    /**  Ctor */
    explicit MapFieldTmpl(
        Unit* parentUnit
        ): Field(Type::typeId,parentUnit,true),
           m_map(parentUnit->factory()->dataAllocator<typename type::value_type>()),
           m_parseToSharedArrays(false)
    {}

    constexpr static bool fieldRepeatedUnpackedProtoBuf() noexcept
    {
        return true;
    }

    constexpr static WireType fieldWireType() noexcept
    {
        return WireType::WithLength;
    }
    //! Get wire type of the field
    virtual WireType wireType() const noexcept override
    {
        return fieldWireType();
    }

    /**  Get value by index */
    inline auto& value(const keyType& key)
    {
        return m_map.at(key);
    }

    /**  Get const value by index */
    inline const auto& value(const keyType& key) const
    {
        return m_map.at(key);
    }

    /**  Get value by index */
    inline auto& at(const keyType& key)
    {
        return m_map.at(key);
    }

    /**  Get const value by index */
    inline const auto& at(const keyType& key) const
    {
        return m_map.at(key);
    }

    //! Overload operator []
    inline const auto& operator[] (const keyType& key) const
    {
        return m_map[key];
    }

    //! Overload operator []
    inline auto& operator[] (const keyType& key)
    {
        return m_map[key];
    }

    /**  Get value by index */
    inline auto& field(const keyType& key)
    {
        return m_map.at(key);
    }

    /**  Get const value by index */
    inline const auto& field(const keyType& key) const
    {
        return m_map.at(key);
    }

    /**  Get m_map */
    inline const auto& value() const noexcept
    {
        return m_map;
    }

    /**  Get m_map */
    inline auto* mutableValue() noexcept
    {
        markSet();
        return &m_map;
    }

    /**  Set value by index */
    template <typename KeyT,typename ValueT>
    inline void setValue(KeyT&& key, ValueT&& val)
    {
        markSet();
        m_map.emplace(std::forward<KeyT>(key),std::forward<ValueT>(val));
    }

    /**  Set value by index */
    template <typename KeyT,typename ValueT>
    inline void set(KeyT&& key, ValueT&& val)
    {
        setValue(std::forward<KeyT>(key),std::forward<ValueT>(val));
    }

    /**  Get number of Map fields */
    inline size_t size() const noexcept
    {
        return m_map.size();
    }

    /**  Check if array is empty */
    inline bool empty() const noexcept
    {
        return m_map.empty();
    }

    /**  Get number of Map fields */
    inline size_t dataSize() const noexcept
    {
        return m_map.size();
    }

    /** Get expected field size */
    virtual size_t maxPackedSize() const noexcept override
    {
        return fieldSize();
    }

    size_t fieldSize() const noexcept
    {
        size_t result=0;
        for (const auto& it: m_map)
        {
            detail::MapItemWrapper<keyTypeId,valueTypeId> wrapper{std::cref(it.first),std::cref(it.second),unit()->factory()};
            result+=wrapper.maxPackedSize();
        }
        return result;
    }

    /**  Clear field */
    void fieldClear()
    {
        m_map.clear();
    }

    //! Reset field
    void fieldReset()
    {
        fieldClear();
        markSet(false);
    }

    /**  Clear array */
    virtual void clear() override
    {
        fieldClear();
    }

    /**  Reset field */
    virtual void reset() override
    {
        fieldReset();
    }

    template <typename BufferT>
    bool deserialize(BufferT& wired, const AllocatorFactory* factory)
    {
        if (factory==nullptr)
        {
            factory=unit()->factory();
        }

        keyType key{};
        auto valTypeC=hana::type_c<valueType>;

        auto self=this;
        auto value=hana::eval_if(
            std::is_base_of<UnitType,valueType>{},
            [&](auto _)
            {
                typename std::decay_t<decltype(_(valTypeC))>::type subunit{_(self)};
                subunit.createValue(_(factory),true);
                subunit.fieldSetParseToSharedArrays(_(self).fieldIsParseToSharedArrays());
                return subunit;
            },
            [&](auto _)
            {
                return typename std::decay_t<decltype(_(valTypeC))>::type{};
            }
        );

        detail::MapItemWrapper<keyTypeId,valueTypeId,false> wrapper{std::ref(key),std::ref(value),factory};
        auto ok=UnitSer::deserialize(&wrapper,wired);
        if (!ok)
        {
            return false;
        }

        /* ok */
        m_map.emplace(std::move(key),std::move(value));
        markSet();
        return isSet();
    }

    /**  Load fields from wire */
    virtual bool doLoad(WireData& wired, const AllocatorFactory* factory) override
    {
        return deserialize(wired,factory);
    }

    constexpr static const bool CanChainBlocks=false;
    virtual bool canChainBlocks() const noexcept override
    {
        return CanChainBlocks;
    }
    constexpr static bool fieldCanChainBlocks() noexcept
    {
        return CanChainBlocks;
    }

    /**  Serialize field to wire */
    template <typename BufferT>
    bool serialize(BufferT& wired) const
    {
        constexpr auto tagWireType=fieldWireType();
        constexpr auto id=Id;

        /* append each field */
        for (const auto& it: m_map)
        {
            /* append tag to sream */
            constexpr uint32_t tag=(id<<3)|static_cast<int>(tagWireType);
            wired.incSize(Stream<uint32_t>::packVarInt(wired.mainContainer(),tag));

            // append item as subunit
            detail::MapItemWrapper<keyTypeId,valueTypeId> wrapper{std::cref(it.first),std::cref(it.second),unit()->factory()};
            auto ok=UnitSer::serialize(&wrapper,wired);
            if (!ok)
            {
                return false;
            }
        }

        /* ok */
        return true;
    }

    /**  Store field to wire */
    virtual bool doStore(WireData& wired) const override
    {
        return serialize(wired);
    }

    /**
    * @brief Use shared version of byte arrays data when parsing wired data
    * @param enable Enabled on/off
    *
    * When enabled then shared byte arrays will be auto allocated in managed shared buffers
    */
    virtual void setParseToSharedArrays(bool enable,const AllocatorFactory* factory=nullptr) override
    {
        fieldSetParseToSharedArrays(enable,factory);
    }

    /**
    * @brief Check if shared byte arrays must be used for parsing
    * @return Boolean flag
    */
    virtual bool isParseToSharedArrays() const noexcept override
    {
        return fieldIsParseToSharedArrays();
    }

    void fieldSetParseToSharedArrays(bool enable,const AllocatorFactory* =nullptr)
    {
        m_parseToSharedArrays=enable;
    }

    bool fieldIsParseToSharedArrays() const noexcept
    {
        return m_parseToSharedArrays;
    }

    /** Format as JSON element
    */
    inline static bool formatJSON(const MapFieldTmpl& value,json::Writer* writer)
    {
        if (!writer->StartArray())
        {
            return false;
        }

        for (const auto& it: value.m_map)
        {
            if (!writer->StartObject())
            {
                return false;
            }

            if (!writer->Key(detail::MapItemKey::name,common::CStrLength(detail::MapItemKey::name)))
            {
                return false;
            }
            if (!json::Fieldwriter<keyType,keyTypeId>::write(it.first,writer))
            {
                return false;
            }

            if (!writer->Key(detail::MapItemValue::name,common::CStrLength(detail::MapItemValue::name)))
            {
                return false;
            }
            if (!json::Fieldwriter<valueType,valueTypeId>::write(it.second,writer))
            {
                return false;
            }

            if (!writer->EndObject())
            {
                return false;
            }
        }

        return writer->EndArray();
    }

    /** Serialize as JSON element */
    virtual bool toJSON(json::Writer* writer) const override
    {
        return formatJSON(*this,writer);
    }

    virtual void pushJsonParseHandler(Unit */*topUnit*/) override
    {
        //! @todo Implement JSON parsing for map fields
#if 0
        json::pushHandler<selfType,json::FieldReader<Type,selfType>>(topUnit,this);
#endif
    }    

    private:

        type m_map;
        bool m_parseToSharedArrays;
};

template <typename FieldName,typename Type,int Id, bool Required=false>
struct MapField : public FieldConf<MapFieldTmpl<Type,Id>,Id,FieldName,Type,Required>
{
    using FieldConf<MapFieldTmpl<Type,Id>,Id,FieldName,Type,Required>::FieldConf;
};

HATN_DATAUNIT_NAMESPACE_END

#endif // HATNDATAUNITMAPFIELDS_H
