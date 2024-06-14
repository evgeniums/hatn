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

HATN_DB_NAMESPACE_BEGIN

/*********************** Client **************************/

//---------------------------------------------------------------

Client::Client(common::STR_ID_TYPE id)
    : WithID(std::move(id)),
      m_opened(false)
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
#if __cplusplus >= 201703L
            ranges.merge(r);
#else
            ranges.insert(r.begin(), r.end());
#endif
        }
    }
    return ranges;
}

//---------------------------------------------------------------

HATN_DB_NAMESPACE_END
