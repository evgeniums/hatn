/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/****************************************************************************/
/*

*/
/** @file db/objectid.h
  *
  * Contains declaration of ObjectId.
  *
  */

/****************************************************************************/

#ifndef HATNDBOBJECTID_H
#define HATNDBOBJECTID_H

#include <atomic>
#include <chrono>

#include <hatn/common/error.h>
#include <hatn/common/databuf.h>

#include <hatn/db/db.h>

HATN_DB_NAMESPACE_BEGIN

class HATN_DB_EXPORT ObjectId
{
    public:

        constexpr static const uint32_t Length=24;

        static uint64_t timeSinceEpochMs()
        {
            using namespace std::chrono;
            return duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();
        }

        static ObjectId generateId();

        void serialize(common::DataBuf& buf);

        std::string toString() const;

    private:

        uint64_t m_timepoint=0; // 5 bytes: 0xFFFFFFFFFF
        uint32_t m_seq=1; // 3 bytes: 0xFFFFFF
        uint32_t m_rand=0; // 4 bytes: 0xFFFFFFFF

        // hex string: 2*(5+3+4) = 24
};

HATN_DB_NAMESPACE_END

#endif // HATNDBOBJECTID_H
