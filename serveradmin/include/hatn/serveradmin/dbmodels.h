/*
    Copyright (c) 2024 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    {{LICENSE}}
*/

/****************************************************************************/
/*
    
*/
/** @file serveradmin/dbmodels.h
  */

/****************************************************************************/

#ifndef HATNSERVERAADMINDBMODELS_H
#define HATNSERVERAADMINDBMODELS_H

#include <hatn/db/modelsprovider.h>

#include <hatn/serveradmin/serveradmin.h>

HATN_SERVER_ADMIN_NAMESPACE_BEGIN

class HATN_SERVER_ADMIN_EXPORT DbModels : public db::ModelsProvider
{
    public:

        virtual void registerRocksdbModels() override;

        virtual std::vector<std::shared_ptr<db::ModelInfo>> models() const override;
};

HATN_SERVER_ADMIN_NAMESPACE_END

#endif // HATNSERVERAADMINDBMODELS_H
