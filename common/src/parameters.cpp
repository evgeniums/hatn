/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.
    
    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)
      
*/

/****************************************************************************/
/** @file common/parameters.h
  *
  *     Container of generic parameters.
  *
  */

#include <boost/thread/locks.hpp>

#include <hatn/common/parameters.h>

#include <hatn/common/loggermoduleimp.h>
INIT_LOG_MODULE(parameters,HATN_COMMON_EXPORT)

HATN_COMMON_NAMESPACE_BEGIN

/********************** Parameters **************************/

class Parameters_p
{
    public:

        boost::shared_mutex mutex;
        ParametersMap parameters;
        bool warningEnabled;
        std::string id;
};

//---------------------------------------------------------------
Parameters::Parameters(
        const std::string& id,
        const ParametersMap &parameters
    ) : d(std::make_unique<Parameters_p>())
{
    logModule=HATN_Log<HATN_Log_parameters>::i();
    d->id=id;
    d->parameters=parameters;
    d->warningEnabled=false;
}

//---------------------------------------------------------------
Parameters::~Parameters()
{
}

//---------------------------------------------------------------
Parameters::Parameters(Parameters&& other) noexcept
    : logModule(other.logModule),
      d(std::move(other.d))
{
}

//---------------------------------------------------------------
Parameters& Parameters::operator=(Parameters&& other) noexcept
{
    if (&other!=this)
    {
        logModule=other.logModule;
        d=std::move(other.d);
    }
    return *this;
}

//---------------------------------------------------------------
void Parameters::setID(
        const std::string &id
    )
{
    d->id=id;
}

//---------------------------------------------------------------
std::string Parameters::id() const
{
    return d->id;
}

//---------------------------------------------------------------
void Parameters::setValue(
        const std::string &name,
        const boost::any &value
    )
{
    boost::unique_lock<boost::shared_mutex> lock(d->mutex);
    d->parameters[name]=value;
}

//---------------------------------------------------------------
void Parameters::removeValue(
        const std::string &name
    )
{
    boost::unique_lock<boost::shared_mutex> lock(d->mutex);
    d->parameters.erase(name);
}

//---------------------------------------------------------------
void Parameters::clear()
{
    boost::unique_lock<boost::shared_mutex> lock(d->mutex);
    d->parameters.clear();
}

//---------------------------------------------------------------
bool Parameters::exists(
        const std::string &name
    ) const
{
    boost::shared_lock<boost::shared_mutex> lock(d->mutex);
    auto it=d->parameters.find(name);
    return it!=d->parameters.end();
}

//---------------------------------------------------------------
boost::any Parameters::value(
        const std::string &name,
        const boost::any &defaultValue
    ) const
{
    boost::shared_lock<boost::shared_mutex> lock(d->mutex);
    auto it=d->parameters.find(name);
    if (it!=d->parameters.end())
    {
        return it->second;
    }
    return defaultValue;
}

//---------------------------------------------------------------
std::set<std::string> Parameters::keys() const
{
    boost::shared_lock<boost::shared_mutex> lock(d->mutex);
    std::set<std::string> result;
    for (auto&& it:d->parameters)
    {
        result.insert(it.first);
    }
    return result;
}

//---------------------------------------------------------------
const ParametersMap& Parameters::map() const
{
    boost::shared_lock<boost::shared_mutex> lock(d->mutex);
    return d->parameters;
}

//---------------------------------------------------------------
void Parameters::setWarningMode(
        bool enable,
        LogModule* loggerModule
    )
{
    d->warningEnabled=enable;
    logModule=loggerModule;
}

//---------------------------------------------------------------
bool Parameters::isWarningEnabled() const
{
    return d->warningEnabled;
}

//---------------------------------------------------------------
LogModule* Parameters::loggerModule() const
{
    return logModule;
}

//---------------------------------------------------------------
boost::shared_mutex& Parameters::mutex() const
{
    return d->mutex;
}

//---------------------------------------------------------------
void Parameters::copyParameters(
        Parameters &target
    ) const
{
    boost::shared_lock<boost::shared_mutex> lockSelf(d->mutex);
    boost::unique_lock<boost::shared_mutex> lockTarget(target.d->mutex);
    target.d->parameters=d->parameters;
}

//---------------------------------------------------------------
HATN_COMMON_NAMESPACE_END
