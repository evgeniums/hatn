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

template <typename ErrCodeT, typename ApiCodeT, typename ErrorCatergoryT, typename ApiCategoryT, typename DataT>
Error makeApiError(ErrCodeT code,
               const ErrorCatergoryT* errCat,
               ApiCodeT apiCode,
               const ApiCategoryT* apiCat,
               std::string description,
               const DataT dataUnit,
               std::string dataType,
               const common::pmr::AllocatorFactory* factory
    )
{
    auto nativeError=std::make_shared<common::NativeError>(static_cast<int>(apiCode),apiCat,errCat);
    if (!description.empty())
    {
        nativeError->mutableApiError()->setDescription(std::move(description),true);
    }
    if (dataUnit!=nullptr)
    {
        nativeError->apiError()->setDataType(std::move(dataType));
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
    return Error{code,std::move(nativeError)};
}

HATN_API_NAMESPACE_END

#endif // HATNAPIMAKEAPIERROR_IPP
