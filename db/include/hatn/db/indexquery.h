/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/****************************************************************************/
/*

*/
/** @file db/indexquery.h
  *
  * Contains declaration of db index queries.
  *
  */

/****************************************************************************/

#ifndef HATNDBINDEXQUERY_H
#define HATNDBINDEXQUERY_H

#include <boost/hana.hpp>

#include <hatn/common/objectid.h>
#include <hatn/common/pmr/pmrtypes.h>
#include <hatn/common/pmr/allocatorfactory.h>

#include <hatn/db/db.h>
#include <hatn/db/topic.h>
#include <hatn/db/index.h>
#include <hatn/db/model.h>
#include <hatn/db/query.h>
#include <hatn/db/timepointfilter.h>

HATN_DB_NAMESPACE_BEGIN

class HATN_DB_EXPORT IndexQuery : public TimePointFilter
{
    public:

        constexpr static const size_t PreallocatedFieldsCount=4;

        static size_t DefaultLimit; // 100

        template <typename IndexT, typename WhereT, typename ...TopicsT>
        IndexQuery(IndexT&& index, const WhereT& where, const TopicsT&... topics)
            : IndexQuery(index.indexInfo(),int(0),topics...)
        {
            static_assert(hana::is_a<IndexTag,IndexT>, "Invalid type of index for fields query");
            static_assert(!std::decay_t<IndexT>::isDatePartitioned(), "Partition index can not be used in fields query");

            if constexpr (!std::is_same<typename WhereT::partitionT, hana::false_>::value)
            {
                const auto& partitionIdx=hana::at(where._partitions,hana::size_c<0>);

                using partititionIdxType=std::decay_t<decltype(partitionIdx.get())>;
                static_assert(hana::is_a<IndexTag,partititionIdxType>, "Invalid type of index for partitions query");
                static_assert(partititionIdxType::isDatePartitioned(), "Ordinary index can not be used in partitions query");

                const auto& field=hana::at(where._partitions,hana::size_c<1>);
                m_partitions=query::Field{
                     partitionIdx.get().fieldInfoPos(field.get(),hana::size_c<0>),
                     hana::at(where._partitions,hana::size_c<2>),
                     hana::at(where._partitions,hana::size_c<3>),
                     hana::at(where._partitions,hana::size_c<4>)
                };
            }

            m_fields.reserve(where.size());
            auto self=this;
            hana::fold(
                where._conditions,
                hana::size_c<0>,
                [self,&index](auto pos,const auto& cond)
                {
                    const auto& field=hana::at(cond,hana::size_c<0>);
                    self->m_fields.emplace_back(
                        index.fieldInfoPos(field.get(),pos),
                        hana::at(cond,hana::size_c<1>),
                        hana::at(cond,hana::size_c<2>),
                        hana::at(cond,hana::size_c<3>)
                        );
                    return hana::plus(pos,hana::size_c<1>);
                }
            );
        }

        const IndexInfo* index() const noexcept
        {
            return m_index;
        }

        const auto& fields() const noexcept
        {
            return m_fields;
        }

        const auto& field(size_t pos) const
        {
            return m_fields[pos];
        }

        const auto& partitions() const
        {
            return m_partitions;
        }

        void setLimit(size_t limit) noexcept
        {
            m_limit=limit;
        }

        size_t limit() const noexcept
        {
            return m_limit;
        }

        void setOffset(size_t offset) noexcept
        {
            m_offset=offset;
        }

        size_t offset() const noexcept
        {
            return m_offset;
        }

        const Topics& topics() const noexcept
        {
            return m_topics;
        }

        Topics& topics() noexcept
        {
            return m_topics;
        }

        static size_t defaultLimit() noexcept
        {
            return DefaultLimit;
        }

        static void setDefaultLimit(size_t limit) noexcept
        {
            DefaultLimit=limit;
        }

    private:

        IndexQuery(
            const IndexInfo* index,
            int,
            Topic topic
            ) : m_index(index),
            m_topics({std::move(topic)}),
            m_limit(DefaultLimit),
            m_offset(0)
        {}

        IndexQuery(
            const IndexInfo* index,
            int,
            std::initializer_list<Topic> topics
            ) : m_index(index),
            m_topics(std::move(topics)),
            m_limit(DefaultLimit),
            m_offset(0)
        {}

        IndexQuery(
            const IndexInfo* index,
            int
            ) : m_index(index),
                m_limit(DefaultLimit),
                m_offset(0)
        {}

        template <typename ...Topics>
        IndexQuery(
            const IndexInfo* index,
            int,
            const Topics&... topics
            ) : m_index(index),
                m_limit(DefaultLimit),
                m_offset(0)
        {
            m_topics.beginRawInsert(sizeof...(Topics));
            hana::for_each(
                HATN_VALIDATOR_NAMESPACE::make_cref_tuple(topics...),
                [this](const auto& topic)
                {
                    m_topics.rawEmplace(topic.get());
                }
                );
            m_topics.endRawInsert();
        }

        IndexQuery(
            const IndexInfo* index,
            int,
            Topics topics
            ) : m_index(index),
                m_topics(std::move(topics)),
                m_limit(DefaultLimit),
                m_offset(0)
        {}

        template <typename ContainerT>
        IndexQuery(
            const IndexInfo* index,
            int,
            int,
            const ContainerT& topics
            ) : m_index(index),
            m_limit(DefaultLimit),
            m_offset(0)
        {
            m_topics.beginRawInsert(topics.size());
            for (auto&& it: topics)
            {
                m_topics.rawEmplace(it);
            }
            m_topics.endRawInsert();
        }

        IndexQuery(
            const IndexInfo* index,
            int,
            const std::vector<Topic>& topics
            ) : IndexQuery(index,0,0,topics)
        {}

        IndexQuery(
            const IndexInfo* index,
            int,
            const common::pmr::vector<Topic>& topics
            ) : IndexQuery(index,0,0,topics)
        {}

        template <size_t Size,typename AllocatorT>
        IndexQuery(
            const IndexInfo* index,
            int,
            const common::VectorOnStackT<Topic,Size,AllocatorT>& topics
            ) : IndexQuery(index,0,0,topics)
        {}

        const IndexInfo* m_index;
        Topics m_topics;
        common::VectorOnStack<query::Field,PreallocatedFieldsCount> m_fields;
        size_t m_limit;
        size_t m_offset;

        query::Field m_partitions;
};

struct ModelIndexQuery
{
    const IndexQuery& query;
    const std::string& modelIndexId;

    ModelIndexQuery(
            const IndexQuery& query,
            const std::string& modelIndexId
        ) : query(query),
            modelIndexId(modelIndexId)
    {
        Assert(query.offset()==0 || query.limit()!=0,"Query offset can be used only if the limit is not zero");
    }

    //! @todo Check and warn if query starts with partition field but partitions query is not set
};

template <typename IndexT>
class Query : public IndexQuery
{
    public:

        template <typename WhereT,typename ...Args>
        Query(const IndexT& index, const WhereT& where, Args&&... args)
            : IndexQuery(index, where, std::forward<Args>(args)...),
              m_indexT(index)
        {}

        const IndexT& indexT() const noexcept
        {
            return m_indexT;
        }

    private:

        const IndexT& m_indexT;
};

struct makeQueryT
{
    template <typename IndexT, typename WhereT, typename ...TopicsT>
    auto operator() (const IndexT& index, const WhereT& where, TopicsT&&... topics) const
    {
        return Query<IndexT>(index,where,std::forward<TopicsT>(topics)...);
    }
};
constexpr makeQueryT makeQuery{};

struct allocateQueryT
{
    template <typename IndexT, typename WhereT, typename ...TopicsT>
    auto operator() (common::pmr::AllocatorFactory* factory, const IndexT& index, const WhereT& where, TopicsT&&... topics) const
    {
        return factory->createObject<Query<IndexT>>(index,where,std::forward<TopicsT>(topics)...);
    }
};
constexpr allocateQueryT allocateQuery{};

HATN_DB_NAMESPACE_END

#endif // HATNDBINDEXQUERY_H
