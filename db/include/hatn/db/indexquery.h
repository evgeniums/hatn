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

        IndexQuery(
                const IndexInfo* index,
                Topic topic
            ) : m_index(index),
                m_topics({std::move(topic)}),
                m_limit(DefaultLimit)
        {
            checkFields();
        }

        IndexQuery(
                const IndexInfo* index,
                std::initializer_list<Topic> topics
            ) : m_index(index),
                m_topics(std::move(topics)),
                m_limit(DefaultLimit)
        {
            checkFields();
        }

        IndexQuery(
                const IndexInfo* index
            ) : m_index(index),
                m_limit(DefaultLimit)
        {
            checkFields();
        }

        template <typename IndexT, typename TopicsT, typename WhereT>
        IndexQuery(IndexT&& index, TopicsT&& topics, WhereT&& where)
            : IndexQuery(index.indexInfo(),std::forward<TopicsT>(topics))
        {
            m_fields.reserve(where.size());
            auto self=this;
            hana::for_each(
                where.conditions,
                [self,&index](const auto& cond)
                {
                    self->m_fields.emplace_back(
                        index.fieldInfo(hana::at(cond,hana::size_c<0>)),
                        hana::at(cond,hana::size_c<1>),
                        hana::at(cond,hana::size_c<2>),
                        hana::at(cond,hana::size_c<3>)
                        );
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

        const auto& topics() const noexcept
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

        void checkFields() const
        {
            Assert(m_fields.size()<=m_index->fields().size(),"Number of fields must be less or equal to number of fields in the index");

            for (size_t i=0;i<m_fields.size();i++)
            {
                const auto& qF=m_fields[i];
                const auto& iF=m_index->fields()[i];
                if (iF.nested())
                {
                    Assert(iF.name()==qF.fieldInfo->name(),"Field name mismatches name of corresponding index field");
                }
                else
                {
                    Assert(iF.id()==qF.fieldInfo->id(),"Field ID mismatches ID of corresponding index field");
                }
            }
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
            : IndexQuery(index,std::forward<Args>(args)...),
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
    template <typename TopicsT, typename WhereT, typename IndexT>
    auto operator() (TopicsT&& topics, WhereT&& where, const IndexT& index) const
    {
        return Query<IndexT>{index,std::forward<TopicsT>(topics),std::forward<WhereT>(where)};
    }

    template <typename WhereT, typename IndexT>
    auto operator() (std::initializer_list<Topic> topics, WhereT&& where, const IndexT& index) const
    {
        return Query<IndexT>{index,std::move(topics),std::forward<WhereT>(where)};
    }

    template <typename WhereT, typename IndexT>
    auto operator() (WhereT&& where, const IndexT& index) const
    {
        return Query<IndexT>{index,std::initializer_list<Topic>{},std::forward<WhereT>(where)};
    }
};
constexpr makeQueryT makeQuery{};

HATN_DB_NAMESPACE_END

#endif // HATNDBINDEXQUERY_H
