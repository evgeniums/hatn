/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/****************************************************************************/
/*
    
*/
/** @file clientapp/confirmationdescriptor.h
*/

/****************************************************************************/

#ifndef HATNCONFIRMATIONDESCRIPTOR_H
#define HATNCONFIRMATIONDESCRIPTOR_H

#include <string>
#include <hatn/clientapp/clientappdefs.h>

HATN_CLIENTAPP_NAMESPACE_BEGIN

struct ConfirmationDescriptor
{
    // both directions
    std::string confirmationType;
    std::string lastFailedTime;

    // request
    std::string confirmationData;
    bool skipConfirmation=false;
    uint32_t triesCount=0;

    // response
    bool confirmationFailed=false;
    std::string confirmationMessage;
    std::string confirmationTitle;
};

HATN_CLIENTAPP_NAMESPACE_END

#endif // HATNCONFIRMATIONDESCRIPTOR_H
