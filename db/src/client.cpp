/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/** @file db/client.cpp
  *
  *   Base class for database client.
  *
  */

/****************************************************************************/

#include <hatn/db/client.h>
#include <hatn/db/asyncclient.h>

HATN_DB_NAMESPACE_BEGIN

/*********************** Client **************************/

//---------------------------------------------------------------

Client::Client(const lib::string_view& id)
    : WithID(id),
      m_open(false)
{}

//---------------------------------------------------------------

Client::~Client()
{}

//---------------------------------------------------------------

std::set<common::DateRange> Client::datePartitionRanges(
        const std::vector<ModelInfo>& models,
        const common::Date& to,
        const common::Date& from
    )
{
    std::set<common::DateRange> ranges;
    for (auto&& model:models)
    {
        if (model.isDatePartitioned())
        {
            auto r=common::DateRange::datesToRanges(to,from,model.datePartitionMode());
            ranges.merge(r);
        }
    }
    return ranges;
}

//---------------------------------------------------------------

AsyncClient::AsyncClient(
    std::shared_ptr<Client> client,
    common::MappedThreadMode threadMode,
    common::ThreadQWithTaskContext* defaultThread
    ) : common::WithMappedThreads(defaultThread),
    m_client(std::move(client))
{
    threads()->setThreadMode(threadMode);
}

//---------------------------------------------------------------

AsyncClient::AsyncClient(
    std::shared_ptr<Client> client,
    std::shared_ptr<common::MappedThreadQWithTaskContext> threads
    ) : common::WithMappedThreads(std::move(threads)),
    m_client(std::move(client))
{}

//---------------------------------------------------------------

HATN_DB_NAMESPACE_END
