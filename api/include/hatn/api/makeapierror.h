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
#include <hatn/common/apierror.h>

#include <hatn/api/api.h>
#include <hatn/api/genericerror.h>

HATN_API_NAMESPACE_BEGIN

struct makeApiErrorT
{
    template <typename ErrCodeT, typename ErrorCatergoryT, typename ApiCodeT, typename ApiCategoryT, typename DataT=std::nullptr_t>
    Error operator() (
                       ErrCodeT code,
                       const ErrorCatergoryT& errCat,
                       ApiCodeT apiCode,
                       const ApiCategoryT& apiCat,
                       std::string description={},
                       DataT dataUnit=nullptr,
                       std::string dataType={},
                       const common::pmr::AllocatorFactory* factory=common::pmr::AllocatorFactory::getDefault()
                       ) const;

    template <typename ApiCodeT, typename ApiCategoryT, typename DataT=std::nullptr_t>
    Error operator() (
                        ApiCodeT apiCode,
                        const ApiCategoryT& apiCat,
                        std::string description={},
                        DataT dataUnit=nullptr,
                        std::string dataType={},
                        const common::pmr::AllocatorFactory* factory=common::pmr::AllocatorFactory::getDefault()
                    ) const;

    template <typename ApiCodeT, typename ApiCategoryT, typename DataT=std::nullptr_t>
    Error operator() (
                       Error ec,
                       ApiCodeT apiCode,
                       const ApiCategoryT& apiCat,
                       std::string description={},
                       DataT dataUnit=nullptr,
                       std::string dataType={},
                       const common::pmr::AllocatorFactory* factory=common::pmr::AllocatorFactory::getDefault()
                     ) const;
};
constexpr makeApiErrorT makeApiError{};

template <typename ErrCodeT, typename ErrorCatergoryT>
inline Error cloneApiError(common::ApiError apiErr, ErrCodeT code=CommonError::SERVER_API_ERROR, const ErrorCatergoryT* cat=&common::CommonErrorCategory::getCategory())
{
    auto native=std::make_shared<common::NativeError>(std::move(apiErr),cat);
    auto ec=Error{code,std::move(native)};
    return ec;
}

inline Error internalServerError(Error ec)
{
    return makeApiError(std::move(ec),ApiResponseStatus::InternalServerError,ApiGenericErrorCategory::getCategory());
}

HATN_API_NAMESPACE_END

#endif // HATNAPIMAKEAPIERROR_H
