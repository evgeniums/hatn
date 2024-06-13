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

#include <set>

#include <hatn/db/db.h>
#include <hatn/db/object.h>
#include <hatn/db/index.h>

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
struct ModelConfig
{
    constexpr static DatePartitionMode datePartitionMode()
    {
        return PartitionMode;
    }
};

template <typename ConfigT, typename UnitType, typename ...Indexes>
struct Model : public ConfigT
{
    using Type=UnitType;

    hana::tuple<Indexes...> indexes;

    template <typename ...Args>
    Model(Args&& ...args) : indexes(hana::make_tuple(std::forward<Args>(args)...))
    {}

    constexpr static const char* name()
    {
        return Type::name();
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

template <typename UnitType>
struct makeModelT
{
    template <typename ConfigT, typename ...Indexes>
    auto operator()(ConfigT&&, Indexes ...indexes) const
    {
        return Model<ConfigT,UnitType,Indexes...>{indexes...};
    }
};
template <typename UnitType> constexpr makeModelT<UnitType> makeModel{};

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
        ModelInfo(ModelT&& model, std::string collection=std::decay_t<ModelT>::name())
            : m_collection(std::move(collection)),
              m_datePartitioned(model.isDatePartitioned()),
              m_datePartitionMode(model.datePartitionMode())
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

    private:

        std::string m_collection;
        bool m_datePartitioned;
        DatePartitionMode m_datePartitionMode;
};

HATN_DB_NAMESPACE_END

#endif // HATNDBMODEL_H
