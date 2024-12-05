/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/****************************************************************************/
/*

*/
/** @file db/transaction.h
  *
  * Contains declaration of db transaction class.
  *
  */

/****************************************************************************/

#ifndef HATNDBTRANSACTION_H
#define HATNDBTRANSACTION_H

#include <functional>

#include <hatn/common/error.h>
#include <hatn/common/meta/dynamiccastwithsample.h>

#include <hatn/db/db.h>

HATN_DB_NAMESPACE_BEGIN

class Transaction
{
    public:

        template <typename T>
        T* derived() noexcept
        {
            T sample;
            return common::dynamicCastWithSample(this,&sample);
        }

        template <typename T>
        const T* derived() const noexcept
        {
            T sample;
            return common::dynamicCastWithSample(this,&sample);
        }
};

using TransactionFn=std::function<Error (Transaction* tx)>;

HATN_DB_NAMESPACE_END

#endif // HATNDBTRANSACTION_H
