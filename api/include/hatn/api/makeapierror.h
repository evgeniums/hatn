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

struct makeApiErrorT
{
    template <typename ErrCodeT, typename ErrorCatergoryT, typename ApiCodeT, typename ApiCategoryT, typename DataT=std::nullptr_t>
    Error operator() (
                       ErrCodeT code,
                       const ErrorCatergoryT* errCat,
                       ApiCodeT apiCode,
                       const ApiCategoryT* apiCat,
                       std::string description={},
                       DataT dataUnit=nullptr,
                       std::string dataType={},
                       const common::pmr::AllocatorFactory* factory=common::pmr::AllocatorFactory::getDefault()
                       ) const;

    template <typename ApiCodeT, typename ApiCategoryT, typename DataT=std::nullptr_t>
    Error operator() (const Error& ec,
                       ApiCodeT apiCode,
                       const ApiCategoryT* apiCat,
                       std::string description={},
                       DataT dataUnit=nullptr,
                       std::string dataType={},
                       const common::pmr::AllocatorFactory* factory=common::pmr::AllocatorFactory::getDefault()
                       ) const
    {
        Assert(ec.category()!=nullptr,"Can be used only for errors of ErrorCategory category");
        return makeApiErrorT{}(ec.code(),ec,ec.category(),apiCode,apiCat,std::move(description),dataUnit,dataType,factory);
    }
};
constexpr makeApiErrorT makeApiError{};

HATN_API_NAMESPACE_END

#endif // HATNAPIMAKEAPIERROR_H
