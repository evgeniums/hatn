/*
    Copyright (c) 2024 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    {{LICENSE}}
*/

/****************************************************************************/
/*
    
*/
/** @file adminserver/adminservice.h
  */

/****************************************************************************/

#ifndef HATNADMINMETHOD_H
#define HATNADMINMETHOD_H

#include <hatn/adminserver/adminserver.h>
#include <hatn/adminserver/apiadmincontroller.h>

HATN_ADMIN_SERVER_NAMESPACE_BEGIN

namespace methods {

class BaseTraits
{
    public:

        BaseTraits(std::shared_ptr<ApiAdminController> controller) : m_controller(std::move(controller))
        {}

    protected:

        ApiAdminController* controller() const
        {
            return m_controller.get();
        }

    private:

        std::shared_ptr<ApiAdminController> m_controller;
};

}

HATN_ADMIN_SERVER_NAMESPACE_END

#endif // HATNADMINMETHOD_H
