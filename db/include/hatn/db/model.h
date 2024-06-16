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

#include <hatn/common/classuid.h>

#include <hatn/db/db.h>
#include <hatn/db/object.h>
#include <hatn/db/index.h>
#include <hatn/db/modelregistry.h>

HATN_DB_NAMESPACE_BEGIN

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

template <DatePartitionMode PartitionMode=DatePartitionMode::Month>
class ModelConfig
{
    public:

        ModelConfig()=default;

        ModelConfig(std::string collection) : m_collection(std::move(collection))
        {}

        constexpr static DatePartitionMode datePartitionMode()
        {
            return PartitionMode;
        }

        uint32_t modelId() const noexcept
        {
            return m_modelId;
        }

        const std::string& collection() const noexcept
        {
            return m_collection;
        }

    private:

        void setModelId(uint32_t modelId) noexcept
        {
            m_modelId=modelId;
        }

        uint32_t m_modelId;
        std::string m_collection;

        template <typename UnitType> friend struct unitModelT;
};
using DefaultConfig=ModelConfig<>;

struct ModelTag{};

template <typename ConfigT, typename UnitT, typename ...Indexes>
struct Model : public ConfigT
{
    using hana_tag=ModelTag;

    using UnitType=UnitT;

    hana::tuple<Indexes...> indexes;

    template <typename CfgT,typename Ts>
    Model(
        CfgT&& config,
        Ts&& indexes,
          std::enable_if_t<
              decltype(hana::is_a<hana::tuple_tag,Ts>)::value,
              void*
              > =nullptr) :
                    ConfigT(std::forward<CfgT>(config)),
                    indexes(std::forward<Ts>(indexes))
    {}

    ~Model()=default;
    Model(const Model&)=default;
    Model(Model&&)=default;
    Model& operator=(const Model&)=default;
    Model& operator=(Model&&)=default;

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
            [&](auto _)
            {
                return hana::false_{};
            },
            [&](auto _)
            {
                using type=typename std::decay_t<decltype(_(found).value())>::type;
                return type::datePartitionField();
            }
        );
    }

    private:

        constexpr static auto findPartitionIndex()
        {
            auto pred=[](auto&& index)
            {
                using type=typename std::decay_t<decltype(index)>::type;
                return hana::bool_<type::isDatePartitioned()>{};
            };

            constexpr auto xs=hana::tuple_t<Indexes...>;
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
    template <typename ConfigT, typename ...Indexes>
    auto operator()(ConfigT&& config, Indexes ...indexes) const
    {
        using type=Model<ConfigT,UnitType,Indexes...>;
        auto m=type{std::forward<ConfigT>(config),hana::make_tuple(indexes...)};
        m.setModelId(ModelRegistry::instance().registerModel(type::name()));
        return m;
    }
};
template <typename UnitType> constexpr unitModelT<UnitType> unitModel{};

template <typename UnitT, typename ModelT>
common::DateRange datePartition(const UnitT& unit, const ModelT& model)
{
    return hana::eval_if(
        hana::bool_<std::decay_t<ModelT>::isDatePartitioned()>{},
        [&](auto _)
        {
            const auto& f=getIndexField(_(unit),_(model).datePartitionField());
            Assert(f.isSet(),"Partition field not set");
            return common::DateRange{f.value(),_(model).datePartitionMode()};
        },
        [](auto)
        {
            return common::DateRange{};
        }
    );
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
            : m_collection(std::decay_t<ModelT>::name()),
              m_datePartitioned(model.isDatePartitioned()),
              m_datePartitionMode(model.datePartitionMode()),
              m_id(model.modelId())
        {
            if (!model.collection().empty())
            {
                m_collection=model.collection();
            }
        }

        ~ModelInfo()=default;
        ModelInfo(const ModelInfo&)=default;
        ModelInfo(ModelInfo&&)=default;
        ModelInfo& operator=(const ModelInfo&)=default;
        ModelInfo& operator=(ModelInfo&&)=default;

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

        bool operator <(const ModelInfo& other) const noexcept
        {
            return m_id<other.m_id;
        }

        bool operator ==(const ModelInfo& other) const noexcept
        {
            return m_id==other.m_id;
        }

    private:

        std::string m_collection;
        bool m_datePartitioned;
        DatePartitionMode m_datePartitionMode;
        uint32_t m_id;
};

template <typename ModelT>
struct ModelWithInfo
{
    using ModelType=ModelT;

    template <typename T>
    ModelWithInfo(T&& model,
                  std::enable_if_t<
                      decltype(hana::is_a<ModelTag,T>)::value,
                      void*
                      > =nullptr
                  )
        : info(model),
          model(std::forward<T>(model))
    {}

    ModelInfo info;
    ModelT model;
};

struct makeModelWithInfoT
{
    template <typename ModelT>
    auto operator()(ModelT&& model) const
    {
        return ModelWithInfo<std::decay_t<ModelT>>{std::forward<ModelT>(model)};
    }
};
constexpr makeModelWithInfoT makeModelWithInfo{};

template <typename UnitType>
struct makeModelT
{
    template <typename ConfigT, typename ...Indexes>
    auto operator()(ConfigT&& config, Indexes ...indexes) const
    {
        auto m=unitModel<UnitType>(std::forward<ConfigT>(config),std::forward<Indexes>(indexes)...);
        return makeModelWithInfo(std::move(m));
    }
};
template <typename UnitType> constexpr makeModelT<UnitType> makeModel{};

HATN_DB_NAMESPACE_END

#endif // HATNDBMODEL_H
