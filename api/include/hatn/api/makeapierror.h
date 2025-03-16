/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/****************************************************************************/
/*

*/
/** @file api/makeapierror.h
  *
  */

/****************************************************************************/

#ifndef HATNAPIMAKEAPIERROR_H
#define HATNAPIMAKEAPIERROR_H

#include <hatn/common/pmr/allocatorfactory.h>

#include <hatn/api/api.h>

HATN_API_NAMESPACE_BEGIN

template <typename ErrCodeT, typename ErrorCatergoryT, typename ApiCodeT, typename ApiCategoryT, typename DataT=std::nullptr_t>
Error makeApiError(ErrCodeT code,
               const ErrorCatergoryT* errCat,
               ApiCodeT apiCode,
               const ApiCategoryT* apiCat,
               std::string description={},
               const DataT dataUnit=nullptr,
               std::string dataType={},
               const common::pmr::AllocatorFactory* factory=common::pmr::AllocatorFactory::getDefault()
);

template <typename ApiCodeT, typename ApiCategoryT, typename DataT=std::nullptr_t>
Error makeApiError(const Error& ec,
    ApiCodeT apiCode,
    const ApiCategoryT* apiCat,
    std::string description={},
    const DataT dataUnit=nullptr,
    std::string dataType={},
    const common::pmr::AllocatorFactory* factory=common::pmr::AllocatorFactory::getDefault()
)
{
    Assert(ec.category()!=nullptr,"Can be used only for errors of ErrorCategory category");
    return makeApiError(ec.code(),ec,ec.category(),apiCode,apiCat,std::move(description),dataUnit,dataType,factory);
}

HATN_API_NAMESPACE_END

#endif // HATNAPIMAKEAPIERROR_H
