/*
    Copyright (c) 2024 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    {{LICENSE}}
*/

/****************************************************************************/
/*
    
*/
/** @file adminserver/dbmodels.h
  */

/****************************************************************************/

#ifndef HATNSERVERAADMINDBMODELS_H
#define HATNSERVERAADMINDBMODELS_H

#include <hatn/db/modelsprovider.h>

#include <hatn/adminserver/adminserver.h>

HATN_ADMIN_SERVER_NAMESPACE_BEGIN

class HATN_ADMIN_SERVER_EXPORT DbModels : public db::ModelsProvider
{
    public:

        virtual void registerRocksdbModels() override;

        virtual std::vector<std::shared_ptr<db::ModelInfo>> models() const override;
};

HATN_ADMIN_SERVER_NAMESPACE_END

#endif // HATNSERVERAADMINDBMODELS_H
