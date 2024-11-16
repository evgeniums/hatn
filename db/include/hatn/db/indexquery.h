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
        IndexQuery(IndexT&& index, WhereT&& where, TopicsT&&... topics)
            : IndexQuery(index.indexInfo(),int(0),std::forward<TopicsT>(topics)...)
        {
            m_fields.reserve(where.size());
            auto self=this;
            hana::fold(
                where.conditions,
                hana::size_c<0>,
                [self,&index](auto pos,const auto& cond)
                {
                    const auto& field=hana::at(cond,hana::size_c<0>);
                    self->m_fields.emplace_back(
                        index.fieldInfoPos(field,pos),
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

        void setLimit(size_t limit) noexcept
        {
            m_limit=limit;
        }

        size_t limit() const noexcept
        {
            return m_limit;
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
            m_limit(DefaultLimit)
        {}

        IndexQuery(
            const IndexInfo* index,
            int,
            std::initializer_list<Topic> topics
            ) : m_index(index),
            m_topics(std::move(topics)),
            m_limit(DefaultLimit)
        {}

        IndexQuery(
            const IndexInfo* index,
            int
            ) : m_index(index),
            m_limit(DefaultLimit)
        {}

        template <typename ...Topics>
        IndexQuery(
            const IndexInfo* index,
            int,
            Topics&&... topics
            ) : m_index(index),
                m_limit(DefaultLimit)
        {
            m_topics.beginRawInsert(sizeof...(Topics));
            hana::for_each(
                hana::make_tuple(std::forward<Topics>(topics)...),
                [this](auto&& topic)
                {
                    m_topics.rawEmplace(std::forward<decltype(topic)>(topic));
                }
                );
            m_topics.endRawInsert();
        }

        const IndexInfo* m_index;
        Topics m_topics;
        common::VectorOnStack<query::Field,PreallocatedFieldsCount> m_fields;
        size_t m_limit;
};

struct ModelIndexQuery
{
    const IndexQuery& query;
    const std::string& modelIndexId;
};

template <typename IndexT>
class Query : public IndexQuery
{
    public:

        template <typename ...Args>
        Query(const IndexT& index, Args&&... args)
            : IndexQuery(index, std::forward<Args>(args)...),
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
    auto operator() (const IndexT& index, WhereT&& where, TopicsT&&... topics) const
    {
        return Query<IndexT>(index,std::forward<WhereT>(where),std::forward<TopicsT>(topics)...);
    }
};
constexpr makeQueryT makeQuery{};

HATN_DB_NAMESPACE_END

#endif // HATNDBINDEXQUERY_H
