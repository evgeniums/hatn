/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/****************************************************************************/

/** @file db/plugins/rocksdb/detail/rocksdbtransaction.ipp
  *
  *   RocksDB transaction wrapper.
  *
  */

/****************************************************************************/

#ifndef HATNROCKSDBTRANSACTION_IPP
#define HATNROCKSDBTRANSACTION_IPP

#include <rocksdb/db.h>
#include <rocksdb/utilities/transaction_db.h>

#include <hatn/db/dberror.h>
#include <hatn/db/transaction.h>

#include <hatn/db/plugins/rocksdb/rocksdbschemadef.h>
#include <hatn/db/plugins/rocksdb/rocksdbschema.h>

#include <string>
#include <vector>

#include <hatn/db/plugins/rocksdb/rocksdbhandler.h>
#include <hatn/db/plugins/rocksdb/modeltopics.h>
#include <hatn/db/plugins/rocksdb/detail/rocksdbhandler.ipp>

HATN_ROCKSDB_NAMESPACE_BEGIN

/********************** RocksdbTransaction **************************/

//---------------------------------------------------------------

class RocksdbTransaction : public Transaction
{
    public:

        RocksdbTransaction()
        {}

        RocksdbTransaction(RocksdbHandler* handler)
            : m_native(handler->p()->transactionDb->BeginTransaction(handler->p()->writeOptions,handler->p()->transactionOptions))
        {}

        static ROCKSDB_NAMESPACE::Transaction* native(Transaction* tx) noexcept
        {
            return tx->derived<RocksdbTransaction>()->m_native.get();
        }

        /**
         * @brief A change of the registry of topics of a model (ModelTopics) that is made only when the transaction is committed.
         *
         * The registry counts the objects of a topic with merge operands written outside of the transaction, so a change
         * written at once would stay after a rollback and the topic would be one object short (or, when it falls to zero,
         * dropped together with the objects that are still there). A pessimistic transaction cannot merge the counter
         * itself: that would lock the key of the topic until the commit for every other writer of the topic.
         */
        struct TopicUpdate
        {
            std::string modelId;
            std::string topic;
            RocksdbPartition* partition=nullptr;
            ModelTopics::Operator action=ModelTopics::Operator::Add;
        };

        static void deferTopicUpdate(Transaction* tx, TopicUpdate update)
        {
            tx->derived<RocksdbTransaction>()->m_topicUpdates.push_back(std::move(update));
        }

    private:

        std::unique_ptr<ROCKSDB_NAMESPACE::Transaction> m_native;
        std::vector<TopicUpdate> m_topicUpdates;
        friend class RocksdbHandler;
};

//---------------------------------------------------------------

HATN_ROCKSDB_NAMESPACE_END

#endif // HATNROCKSDBTRANSACTION_IPP
