/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/****************************************************************************/

/** @file db/model.h
  *
  * Contains helpers for definition of database models.
  *
  */

/****************************************************************************/

#ifndef HATNDBMODEL_H
#define HATNDBMODEL_H

#include <type_traits>
#include <set>

#include <hatn/common/daterange.h>
#include <hatn/common/pmr/pmrtypes.h>

#include <hatn/db/db.h>
#include <hatn/db/objectid.h>
#include <hatn/db/object.h>
#include <hatn/db/index.h>
#include <hatn/db/modelregistry.h>

HATN_DB_NAMESPACE_BEGIN

auto objectIndexes();

using DatePartitionMode=common::DateRange::Type;

HDU_UNIT(model_field,
    HDU_FIELD(id,TYPE_UINT32,1)
    HDU_FIELD(name,TYPE_STRING,2)
    HDU_FIELD(field_type,TYPE_STRING,3)
    HDU_FIELD(repeated,TYPE_BOOL,4)
    HDU_FIELD(default_flag,TYPE_BOOL,5)
    HDU_FIELD(default_value,TYPE_STRING,6)
)

HDU_UNIT_WITH(model,(HDU_BASE(object)),
    HDU_FIELD(name,TYPE_STRING,1)
    HDU_REPEATED_FIELD(model_fields,model_field::TYPE,2)
)

struct ModelConfigTag{};

template <DatePartitionMode PartitionMode=DatePartitionMode::Month>
class ModelConfigT
{
    public:

        using hana_tag=ModelConfigTag;

        ModelConfigT()
            : m_modelId(0),
              m_canBeTopic(false)
        {}

        ModelConfigT(bool canBeTopic)
                  : m_modelId(0),
                    m_canBeTopic(canBeTopic)
        {}

        ModelConfigT(const char* collection, bool canBeTopic=false)
            : ModelConfigT(std::string{collection},canBeTopic)
        {}

        ModelConfigT(std::string collection, bool canBeTopic=false)
                : m_collection(std::move(collection)),
                  m_canBeTopic(canBeTopic)
        {
            updateModelId();
        }

        constexpr static DatePartitionMode datePartitionMode()
        {
            return PartitionMode;
        }

        void setModelId(uint32_t id)
        {
            std::array<char,8> buf;
            fmt::format_to_n(buf.data(),8,"{:08x}",id);
            m_modelIdStr=common::fmtBufToString(buf);
            m_modelId=id;
        }

        uint32_t modelId() const noexcept
        {
            return m_modelId;
        }

        bool canBeTopic() const noexcept
        {
            return m_canBeTopic;
        }

        const std::string& modelIdStr() const noexcept
        {
            return m_modelIdStr;
        }

        const std::string& collection() const noexcept
        {
            return m_collection;
        }

        void setCollection(std::string collection)
        {
            m_collection=std::move(collection);
            updateModelId();
        }

    private:

        uint32_t m_modelId;
        std::string m_modelIdStr;
        std::string m_collection;
        bool m_canBeTopic;

        template <typename UnitType> friend struct unitModelT;

        void updateModelId()
        {
            m_modelId=common::Crc32(m_collection);
            m_modelIdStr=common::Crc32Hex(m_collection);
        }
};

using ModelConfig=ModelConfigT<>;
inline ModelConfig DefaultModelConfig{};

struct ModelTag{};

class UnitWrapper
{
    public:

        UnitWrapper()
        {}

        template <typename T>
        UnitWrapper(const HATN_COMMON_NAMESPACE::SharedPtr<T>& sharedUnit) :
                m_shared(sharedUnit.template staticCast<HATN_DATAUNIT_NAMESPACE::Unit>())
        {}

        template <typename T>
        T* unit()
        {
            static T sample;
            return sample.castToUnit(m_shared.get());
        }

        template <typename T>
        const T* unit() const
        {
            static T sample;
            return sample.castToUnit(m_shared.get());
        }

        template <typename T>
        T* managedUnit()
        {
            static T sample;
            return sample.castToManagedUnit(m_shared.get());
        }

        template <typename T>
        const T* managedUnit() const
        {
            static T sample;
            return sample.castToManagedUnit(m_shared.get());
        }

    private:

        HATN_COMMON_NAMESPACE::SharedPtr<HATN_DATAUNIT_NAMESPACE::Unit> m_shared;
};

template <typename ConfigT, typename UnitT, typename Indexes>
struct Model : public ConfigT
{
    using hana_tag=ModelTag;

    using UnitType=UnitT;

    using Type=typename UnitType::type;
    using ManagedType=typename UnitType::managed;
    using SharedPtr=HATN_COMMON_NAMESPACE::SharedPtr<ManagedType>;

    Indexes indexes;

    using IndexCount=decltype(hana::size(indexes));
    std::array<std::string,IndexCount::value> indexIds;

    HATN_COMMON_NAMESPACE::FlatMap<std::string,IndexInfo> indexMap;

    template <typename CfgT,typename Ts>
    Model(
        CfgT&& config,
        Ts&& indices,
          std::enable_if_t<
              decltype(hana::is_a<hana::tuple_tag,Ts>)::value,
              void*
              > =nullptr) :
                    ConfigT(std::forward<CfgT>(config)),
                    indexes(std::forward<Ts>(indices))
    {
        size_t i=0;
        if (this->collection().empty())
        {
            this->setCollection(name());
        }

        auto eachIndex=[this,&i](auto& idx)
        {
            idx.setCollection(this->collection());
            indexIds[i]=idx.id();
            i++;

            indexMap.emplace(idx.id(),idx);
        };
        hana::for_each(indexes,eachIndex);
    }

    constexpr static const char* name()
    {
        using type=typename UnitType::type;
        return type::unitName();
    }

    constexpr static bool isDatePartitioned()
    {
        return findPartitionIndex() != hana::nothing;
    }

    constexpr static decltype(auto) datePartitionField()
    {
        auto found=findPartitionIndex();
        return hana::eval_if(
            hana::equal(found,hana::nothing),
            [](auto)
            {
                return hana::false_{};
            },
            [&](auto _)
            {
                using type=typename std::decay_t<decltype(_(found).value())>::type;
                return type::frontField();
            }
        );
    }

    static bool isDatePartitionField(const std::string& fieldName)
    {
        auto field=datePartitionField();
        return field.name()==fieldName;
    }

    constexpr static auto ttlIndexes()
    {
        return hana::filter(common::tupleToTupleCType<Indexes>{},
            [](auto x)
            {
                using typeC=std::decay_t<decltype(x)>;
                using idxT=typename typeC::type;
                return hana::bool_c<idxT::isTtl()>;
            }
        );
    }

    constexpr static auto isTtlEnabled()
    {
        return hana::not_(hana::is_empty(ttlIndexes()));
    }

    template <typename IndexT>
    const std::string& indexId(IndexT) noexcept
    {
        auto pred=[](auto&& index)
        {
            using type1=std::decay_t<decltype(index)>;
            using type2=std::decay_t<IndexT>;
            return std::is_same<type1,type2>{};
        };
        thread_local static const auto idx=hana::find_if(indexes,pred);
        static_assert(!decltype(hana::is_nothing(idx))::value,"No such index in the model");
        return idx.value().id();
    }

    private:

        constexpr static auto findPartitionIndex()
        {
            auto pred=[](auto&& index)
            {
                using type=typename std::decay_t<decltype(index)>::type;
                return hana::bool_<type::isDatePartitioned()>{};
            };

            constexpr auto xs=common::tupleToTupleCType<Indexes>{};
            constexpr auto count=hana::count_if(xs,pred);
            static_assert(hana::less_equal(count,hana::size_c<1>),"Only one date partition index can be specified for a model");

            return hana::find_if(xs,pred);
        }
};

/**
 * @brief Create and register unit model.
 *
 * @note Not thread safe. Create and register models at initial steps and then use those models in operations.
 */
template <typename UnitType>
struct unitModelT
{
    template <typename ConfigT, typename Xs>
    auto make(ConfigT&& cfg, Xs&& xs1) const
    {
        auto args=hana::eval_if(
            hana::is_a<ModelConfigTag,ConfigT>,
            [&](auto _)
            {
                return std::make_pair(_(cfg),_(xs1));
            },
            [&](auto _)
            {
                return std::make_pair(DefaultModelConfig,hana::prepend(_(xs1),_(cfg)));
            }
            );
        auto&& config=args.first;
        using configT=std::decay_t<decltype(config)>;
        auto&& xs=args.second;
        using indexesT=std::decay_t<decltype(xs)>;

        using type=Model<configT,UnitType,indexesT>;
        type m{config,xs};
        ModelRegistry::instance().registerModel(m.collection(),m.modelId());
        return m;
    }

    template <typename ConfigT, typename ...Indexes>
    auto operator()(ConfigT&& cfg, Indexes ...indexes) const
    {
        auto xs1=hana::make_tuple(indexes...);
        return make(std::forward<ConfigT>(cfg),xs1);
    }
};
template <typename UnitType> constexpr unitModelT<UnitType> unitModel{};

template <typename UnitT, typename ModelT>
HATN_COMMON_NAMESPACE::DateRange datePartition(const UnitT& unit, const ModelT& model)
{
    return hana::eval_if(
        hana::bool_<std::decay_t<ModelT>::isDatePartitioned()>{},
        [&](auto _)
        {
            const auto& f=getIndexField(_(unit),_(model).datePartitionField());
            Assert(f.isSet(),"Partition field not set");
            return HATN_COMMON_NAMESPACE::DateRange{f.value(),_(model).datePartitionMode()};
        },
        [](auto)
        {
            return HATN_COMMON_NAMESPACE::DateRange{};
        }
    );
}

template <typename ModelT>
HATN_COMMON_NAMESPACE::DateRange datePartition(const HATN_COMMON_NAMESPACE::Date& date, const ModelT& model)
{
    return hana::eval_if(
        hana::bool_<std::decay_t<ModelT>::isDatePartitioned()>{},
        [&](auto _)
        {
            return common::DateRange{_(date),_(model).datePartitionMode()};
        },
        [](auto)
        {
            return common::DateRange{};
        }
        );
}

template <typename ModelT>
HATN_COMMON_NAMESPACE::DateRange datePartition(const ObjectId& id, const ModelT& model)
{
    return datePartition(id.toDate(),model);
}

template <typename ModelT>
HATN_COMMON_NAMESPACE::DateRange datePartition(const HATN_COMMON_NAMESPACE::DateTime& dt, const ModelT& model)
{
    return datePartition(dt.date(),model);
}

class ModelInfo
{
    public:

        template <typename ModelT>
        ModelInfo(ModelT&& model,
                  std::enable_if_t<
                      decltype(hana::is_a<ModelTag,ModelT>)::value,
                      void*
                      > =nullptr
                  )
            : m_collection(model.collection()),
              m_datePartitioned(model.isDatePartitioned()),
              m_datePartitionMode(model.datePartitionMode()),
              m_id(model.modelId()),
              m_idStr(model.modelIdStr()),
              m_nativeModel(nullptr)
        {}

        const std::string& collection() const noexcept
        {
            return m_collection;
        }

        bool isDatePartitioned() const noexcept
        {
            return m_datePartitioned;
        }

        DatePartitionMode datePartitionMode() const noexcept
        {
            return m_datePartitionMode;
        }

        uint32_t modelId() const noexcept
        {
            return m_id;
        }

        const std::string& modelIdStr() const noexcept
        {
            return m_idStr;
        }

        void setModelId(uint32_t id, std::string str)
        {
            m_id=id;
            m_idStr=str;
        }

        bool operator <(const ModelInfo& other) const noexcept
        {
            return m_id<other.m_id;
        }

        bool operator ==(const ModelInfo& other) const noexcept
        {
            return m_id==other.m_id;
        }

        void setNativeModel(void* nativeModel) noexcept
        {
            m_nativeModel=nativeModel;
        }

        void* nativeModelV() const noexcept
        {
            return m_nativeModel;
        }

        template <typename T>
        T* nativeModel() const noexcept
        {
            return reinterpret_cast<T*>(m_nativeModel);
        }

    private:

        std::string m_collection;
        bool m_datePartitioned;
        DatePartitionMode m_datePartitionMode;
        uint32_t m_id;
        std::string m_idStr;

        void* m_nativeModel;
};

template <typename ModelT>
struct ModelWithInfo
{
    using ModelType=ModelT;
    using Type=typename ModelType::Type;
    using ManagedType=typename ModelType::ManagedType;
    using SharedPtr=typename ModelType::SharedPtr;

    template <typename T>
    ModelWithInfo(T&& model,
                  std::enable_if_t<
                      decltype(hana::is_a<ModelTag,T>)::value,
                      void*
                      > =nullptr
                  )
        : info(std::make_shared<ModelInfo>(model)),
          model(std::forward<T>(model))
    {}

    std::shared_ptr<ModelInfo> info;
    ModelT model;
};

struct makeModelWithInfoT
{
    template <typename ModelT>
    auto operator()(ModelT&& model) const
    {
        return std::make_shared<ModelWithInfo<std::decay_t<ModelT>>>(std::forward<ModelT>(model));
    }
};
constexpr makeModelWithInfoT makeModelWithInfo{};

HATN_DB_UNIQUE_INDEX(oidIdx,object::_id)
HATN_DB_INDEX(createdAtIdx,object::created_at)
HATN_DB_INDEX(updatedAtIdx,object::updated_at)

inline auto objectIndexes()
{
    return hana::make_tuple(oidIdx(),createdAtIdx(),updatedAtIdx());
}

template <typename UnitType>
struct makeModelT
{
    template <typename ConfigT, typename ...Indexes>
    auto operator()(ConfigT&& config, Indexes ...indexes) const
    {
        auto m=unitModel<UnitType>.make(std::forward<ConfigT>(config),hana::concat(objectIndexes(),hana::make_tuple(indexes...)));
        return makeModelWithInfo(std::move(m));
    }
};
template <typename UnitType> constexpr makeModelT<UnitType> makeModel{};

template <typename UnitType>
struct makeModelWithIdxT
{
    template <typename ConfigT, typename ...Indexes>
    auto operator()(ConfigT&& config, Indexes ...indexes) const
    {
        auto xs=hana::fold(
            hana::make_tuple(indexes...),
            objectIndexes(),
            [](auto&& ts, auto&& idx)
            {
                return hana::concat(ts,idx);
            }
        );
        auto m=unitModel<UnitType>.make(std::forward<ConfigT>(config),xs);
        return makeModelWithInfo(std::move(m));
    }
};
template <typename UnitType> constexpr makeModelWithIdxT<UnitType> makeModelWithIdx{};

HATN_DB_NAMESPACE_END

#define HATN_DB_MODEL(m,type,...) \
    struct _model_##m { \
        const auto& operator()() const \
        { \
            static auto mm=makeModel< type ::TYPE>(DefaultModelConfig,__VA_ARGS__); \
            return mm; \
        } \
    }; \
    constexpr _model_##m m{};

#endif // HATNDBMODEL_H
