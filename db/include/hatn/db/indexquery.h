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
#include <hatn/common/pmr/string.h>

#include <hatn/db/db.h>
#include <hatn/db/index.h>
#include <hatn/db/model.h>
#include <hatn/db/query.h>
#include <hatn/db/timepointfilter.h>

HATN_DB_NAMESPACE_BEGIN

class HATN_DB_EXPORT IndexQuery : public TimePointFilter
{
    public:

        static common::pmr::AllocatorFactory* defaultAllocatorFactory() noexcept
        {
            if (DefaultAllocatorFactory!=nullptr)
            {
                return DefaultAllocatorFactory;
            }
            return common::pmr::AllocatorFactory::getDefault();
        }

        IndexQuery(
                const IndexInfo& index,
                const lib::string_view& topic,
                common::pmr::AllocatorFactory* factory=defaultAllocatorFactory()
            ) : m_index(&index),
                m_topics({Topic{topic,factory->bytesAllocator()}},factory->dataAllocator<Topic>()),
                m_fields(factory->dataAllocator<query::Field>()),
                m_limit(DefaultLimit)
        {}

        IndexQuery(
                const IndexInfo& index,
                const lib::string_view& topic,
                std::initializer_list<query::Field> list,
                common::pmr::AllocatorFactory* factory=defaultAllocatorFactory()
            ) : m_index(&index),
                m_topics({Topic{topic,factory->bytesAllocator()}},factory->dataAllocator<Topic>()),
                m_fields(std::move(list),factory->dataAllocator<query::Field>()),
                m_limit(DefaultLimit)
        {
            checkFields();
        }

        IndexQuery(
                const IndexInfo& index,
                const lib::string_view& topic,
                common::pmr::vector<query::Field> fields,
                common::pmr::AllocatorFactory* factory=defaultAllocatorFactory()
            ) : m_index(&index),
                m_topics({Topic{topic,factory->bytesAllocator()}},factory->dataAllocator<Topic>()),
                m_fields(std::move(fields)),
                m_limit(DefaultLimit)
        {
            checkFields();
        }

        IndexQuery(
                const IndexInfo& index,
                Topics topics,
                common::pmr::AllocatorFactory* factory=defaultAllocatorFactory()
            ) : m_index(&index),
                m_topics(std::move(topics)),
                m_fields(factory->dataAllocator<query::Field>()),
                m_limit(DefaultLimit)
        {}

        IndexQuery(
                const IndexInfo& index,
                Topics topics,
                std::initializer_list<query::Field> list,
                common::pmr::AllocatorFactory* factory=defaultAllocatorFactory()
            ) : m_index(&index),
                m_topics(std::move(topics)),
                m_fields(std::move(list),factory->dataAllocator<query::Field>()),
                m_limit(DefaultLimit)
        {
            checkFields();
        }

        IndexQuery(
                const IndexInfo& index,
                Topics topics,
                common::pmr::vector<query::Field> fields
            ) : m_index(&index),
                m_topics(std::move(topics)),
                m_fields(std::move(fields)),
                m_limit(DefaultLimit)
        {
            checkFields();
        }

        void setFields(common::pmr::vector<query::Field> fields)
        {
            m_fields=(std::move(fields));
            checkFields();
        }

        void setFields(std::initializer_list<query::Field> fields)
        {
            m_fields=(std::move(fields));
            checkFields();
        }

        void setField(query::Field field)
        {
            m_fields.clear();
            m_fields.emplace_back(std::move(field));
            //! @todo Check fields
            // checkFields();
        }

        const IndexInfo& index() const noexcept
        {
            return *m_index;
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

        static void setDefaultAllocatorFactory(common::pmr::AllocatorFactory* factory) noexcept
        {
            DefaultAllocatorFactory=factory;
        }

    private:

        static size_t DefaultLimit;
        static common::pmr::AllocatorFactory* DefaultAllocatorFactory;

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
        common::pmr::vector<query::Field> m_fields;
        size_t m_limit;
};

HATN_DB_NAMESPACE_END

#endif // HATNDBINDEXQUERY_H
