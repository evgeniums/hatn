/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/****************************************************************************/
/*

*/
/** @file api/makeapierror.ipp
  *
  */

/****************************************************************************/

#ifndef HATNAPIMAKEAPIERROR_IPP
#define HATNAPIMAKEAPIERROR_IPP

#include <hatn/dataunit/visitors.h>

#include <hatn/api/responseunit.h>
#include <hatn/api/makeapierror.h>

HATN_API_NAMESPACE_BEGIN

//---------------------------------------------------------------

template <typename ErrCodeT, typename ErrorCatergoryT, typename ApiCodeT, typename ApiCategoryT, typename DataT>
Error makeApiErrorT::operator() (
               ErrCodeT code,
               const ErrorCatergoryT& errCat,
               ApiCodeT apiCode,
               const ApiCategoryT& apiCat,
               std::string description,
               DataT dataUnit,
               std::string dataType,
               const common::pmr::AllocatorFactory* factory
    ) const
{
    auto nativeError=std::make_shared<common::NativeError>(static_cast<int>(apiCode),&apiCat,&errCat);
    if (!description.empty())
    {
        nativeError->mutableApiError()->setDescription(std::move(description),true);
    }
    if constexpr (!std::is_same_v<DataT,std::nullptr_t>)
    {
        if (dataUnit!=nullptr)
        {
            nativeError->mutableApiError()->setDataType(std::move(dataType));
            du::WireBufSolidShared buf{factory};
            Error ec;
            int r=du::io::serializeAsSubunit(*dataUnit,buf,protocol::response_error_message::data);
            if (r<0)
            {
                std::cerr <<  "Failed to serialize data of API error " << dataType << std::endl;
            }
            else
            {
                nativeError->mutableApiError()->setData(buf.sharedMainContainer());
            }
        }
    }
    return Error{code,std::move(nativeError)};
}

//---------------------------------------------------------------

template <typename ApiCodeT, typename ApiCategoryT, typename DataT>
Error makeApiErrorT::operator() (
    ApiCodeT apiCode,
    const ApiCategoryT& apiCat,
    std::string description,
    DataT dataUnit,
    std::string dataType,
    const common::pmr::AllocatorFactory* factory
    ) const
{
    return makeApiError(CommonError::SERVER_API_ERROR,common::CommonErrorCategory::getCategory(),apiCode,apiCat,std::move(description),std::move(dataUnit),std::move(dataType),factory);
}

//---------------------------------------------------------------

template <typename ApiCodeT, typename ApiCategoryT, typename DataT>
Error makeApiErrorT::operator() (
        Error ec,
        ApiCodeT apiCode,
        const ApiCategoryT& apiCat,
        std::string description,
        DataT dataUnit,
        std::string dataType,
        const common::pmr::AllocatorFactory* factory
    ) const
{
    auto nativeError=std::make_shared<common::NativeError>(static_cast<int>(apiCode),&apiCat);
    if (!description.empty())
    {
        nativeError->mutableApiError()->setDescription(std::move(description),true);
    }
    if constexpr (!std::is_same_v<DataT,std::nullptr_t>)
    {
        if (dataUnit!=nullptr)
        {
            nativeError->mutableApiError()->setDataType(std::move(dataType));
            du::WireBufSolidShared buf{factory};
            Error ec;
            int r=du::io::serializeAsSubunit(*dataUnit,buf,protocol::response_error_message::data);
            if (r<0)
            {
                std::cerr << "Failed to serialize data of API error " << dataType << std::endl;
            }
            else
            {
                nativeError->mutableApiError()->setData(buf.sharedMainContainer());
            }
        }
    }
    return common::chainErrors(std::move(ec),Error{CommonError::SERVER_API_ERROR,std::move(nativeError)});
}

HATN_API_NAMESPACE_END

#endif // HATNAPIMAKEAPIERROR_IPP
