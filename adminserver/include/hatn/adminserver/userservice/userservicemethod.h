/*
    Copyright (c) 2024 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    {{LICENSE}}
*/

/****************************************************************************/
/*
    
*/
/** @file adminserver/userservice/userservicemethod.h
  */

/****************************************************************************/

#ifndef HATNUSERSERVICEMETHOD_H
#define HATNUSERSERVICEMETHOD_H

#include <hatn/adminserver/adminserver.h>
#include <hatn/adminserver/userservice/userservicecontroller.h>

HATN_ADMIN_SERVER_NAMESPACE_BEGIN

namespace user_service {

template <typename RequestT=HATN_API_SERVER_NAMESPACE::Request<>>
class BaseMethod
{
    public:

        BaseMethod(std::shared_ptr<Controller<RequestT>> controller) : m_controller(std::move(controller))
        {}

    protected:

        Controller<RequestT>* controller() const
        {
            return m_controller.get();
        }

    private:

        std::shared_ptr<Controller<RequestT>> m_controller;
};

}

HATN_ADMIN_SERVER_NAMESPACE_END

#endif // HATNUSERSERVICEMETHOD_H
