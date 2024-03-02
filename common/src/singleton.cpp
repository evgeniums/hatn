/*
   Copyright (c) 2020 - current, Evgeny Sidorov (dracosha.com), All rights reserved.
    
    
  */

/****************************************************************************/
/*
    
*/
/** @file common/singleton.cpp
 *
 *     Base singleton class and registrar of singletons
 *
 */

#include <hatn/common/singleton.h>

HATN_COMMON_NAMESPACE_BEGIN

/********************** Singleton **************************/

//---------------------------------------------------------------
Singleton::Singleton():m_position(-1)
{
    SingletonRegistrar::instance().registerSingleton(this);
}

//---------------------------------------------------------------
Singleton::~Singleton()
{
    SingletonRegistrar::instance().unregisterSingleton(this);
}

/********************** SingletonRegistrar **************************/

//---------------------------------------------------------------
SingletonRegistrar& SingletonRegistrar::instance()
{
    static SingletonRegistrar Instance;
    return Instance;
}

//---------------------------------------------------------------
void SingletonRegistrar::registerSingleton(Singleton *obj)
{
    m_singletons.insert(std::make_pair(static_cast<int>(m_singletons.size()),obj));
    obj->setRegistrarPosition(static_cast<int>(m_singletons.size())-1);
}

//---------------------------------------------------------------
void SingletonRegistrar::unregisterSingleton(Singleton *obj)
{
    if (obj->registrarPosition()>=0)
    {
        m_singletons.erase(obj->registrarPosition());
    }
}

//---------------------------------------------------------------
void SingletonRegistrar::reset()
{
    auto list=singletons();
    for (auto&& it:list)
    {
        it->resetSingleton();
    }
}

//---------------------------------------------------------------
void SingletonRegistrar::free()
{
    instance().reset();
    instance().m_singletons.clear();
}

//---------------------------------------------------------------
std::vector<Singleton*> SingletonRegistrar::singletons() const
{
    std::vector<Singleton*> vec;
    vec.reserve(m_singletons.size());
    for (auto&& it: m_singletons)
    {
        vec.push_back(it.second);
    }
    return vec;
}

//---------------------------------------------------------------
    HATN_COMMON_NAMESPACE_END
