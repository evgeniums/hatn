/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/****************************************************************************/

/** @file db/modelregistry.h
  *
  * Contains declaration of ModelRegistry.
  *
  */

/****************************************************************************/

#ifndef HATNDBMODELRREGISTRY_H
#define HATNDBMODELRREGISTRY_H

#include <map>

#include <hatn/common/singleton.h>
#include <hatn/common/stdwrappers.h>
#include <hatn/common/flatmap.h>

#include <hatn/db/db.h>

HATN_DB_NAMESPACE_BEGIN

class HATN_DB_EXPORT ModelRegistry : public common::Singleton
{
    HATN_SINGLETON_DECLARE()

    public:

        static ModelRegistry& instance();
        static void free();

        uint32_t registerModel(std::string name);

        uint32_t modelId(const common::lib::string_view& name) const;

        std::string modelName(uint32_t id) const;

    private:

        ModelRegistry();

        common::FlatMap<std::string,uint32_t,std::less<>> m_modelIds;
        common::FlatMap<uint32_t,std::string,std::less<>> m_modelNames;
};

HATN_DB_NAMESPACE_END

#endif // HATNDBMODELRREGISTRY_H
