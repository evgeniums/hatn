/*
    Copyright (c) 2024 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    {{LICENSE}}
*/

/****************************************************************************/
/*
    
*/
/** @file adminserver/appadmincontroller.h
  */

/****************************************************************************/

#ifndef HATNAPPADMINCOTROLLER_H
#define HATNAPPADMINCOTROLLER_H

#include <hatn/app/appenv.h>

#include <hatn/adminserver/adminserver.h>
#include <hatn/adminserver/localadmincontroller.h>

HATN_ADMIN_SERVER_NAMESPACE_BEGIN

class AppContextTraits
{
    public:

        AppContextTraits(
            common::SharedPtr<HATN_APP_NAMESPACE::AppEnv> env,
            std::string adminTopic="admin")
            : m_env(std::move(env)),
              m_adminTopic(std::move(adminTopic))
        {}

        template <typename T>
        const std::shared_ptr<db::AsyncClient>& adminDb(const common::SharedPtr<T>&) const
        {
            const auto& db=m_env->template get<HATN_APP_NAMESPACE::Db>();
            const auto& client=db.dbClient(m_adminTopic);
            return client;
        }

        template <typename T>
        db::Topic adminTopic(const common::SharedPtr<T>& ={}) const
        {
            return m_adminTopic;
        }

    private:

        common::SharedPtr<HATN_APP_NAMESPACE::AppEnv> m_env;
        std::string m_adminTopic;
};

using AppAdminController=LocalAdminControllerT<AppContextTraits>;

HATN_ADMIN_SERVER_NAMESPACE_END

#endif // HATNAPPADMINCOTROLLER_H
