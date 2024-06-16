/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/** @file db/modelregistry.cpp
  *
  *   Definition of db ModelRegistry.
  *
  */

/****************************************************************************/

#include <atomic>

#include <hatn/db/modelregistry.h>

HATN_DB_NAMESPACE_BEGIN

namespace {

uint32_t ModelCounter{0};

}

/*********************** ModelRegistry **************************/

HATN_SINGLETON_INIT(ModelRegistry)

//---------------------------------------------------------------

ModelRegistry::ModelRegistry()
{}

//---------------------------------------------------------------

ModelRegistry& ModelRegistry::instance()
{
    static ModelRegistry inst;
    return inst;
}

//---------------------------------------------------------------

void ModelRegistry::free()
{
    instance().m_modelIds.clear();
    instance().m_modelNames.clear();
    ModelCounter=0;
}

//---------------------------------------------------------------

uint32_t ModelRegistry::registerModel(std::string name)
{
    auto it=m_modelIds.find(name);
    if (it!=m_modelIds.end())
    {
        return it->second;
    }
    m_modelIds.emplace(name,++ModelCounter);
    m_modelNames.emplace(ModelCounter,std::move(name));
    return ModelCounter;
}

//---------------------------------------------------------------

uint32_t ModelRegistry::modelId(const lib::string_view &name) const
{
    auto it=m_modelIds.find(name);
    if (it!=m_modelIds.end())
    {
        return it->second;
    }
    return 0;
}

//---------------------------------------------------------------

std::string ModelRegistry::modelName(uint32_t id) const
{
    auto it=m_modelNames.find(id);
    if (it!=m_modelNames.end())
    {
        return it->second;
    }
    return std::string{};
}

//---------------------------------------------------------------

HATN_DB_NAMESPACE_END
