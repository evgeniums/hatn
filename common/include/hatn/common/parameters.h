/*
   Copyright (c) 2020 - current, Evgeny Sidorov (dracosha.com), All rights reserved.
    
    
  */

/****************************************************************************/
/*
    
*/
/** @file common/parameters.h
  *
  *     Container of generic parameters.
  *
  */

/****************************************************************************/

#ifndef HATNPARAMETERS_H
#define HATNPARAMETERS_H

#include <unordered_map>
#include <set>
#include <string>
#include <memory>

#include <boost/any.hpp>

#ifdef __GNUC__
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wstrict-aliasing"
#endif

#include <boost/thread/shared_mutex.hpp>

#ifdef __GNUC__
#pragma GCC diagnostic pop
#endif

#include <hatn/common/logger.h>
#include <hatn/common/common.h>

DECLARE_LOG_MODULE_EXPORT(parameters,HATN_COMMON_EXPORT)

HATN_COMMON_NAMESPACE_BEGIN

typedef std::unordered_map<std::string,boost::any> ParametersMap;

class Parameters_p;

//! Container of generic parameters
//! \todo Refactor it
class HATN_COMMON_EXPORT Parameters
{
    public:

        //! Constructor
        Parameters(
            const std::string& id=std::string(),
            const ParametersMap& parameters=ParametersMap()
        );

        virtual ~Parameters();
        Parameters(const Parameters&)=delete;
        Parameters(Parameters&&) noexcept;
        Parameters& operator=(const Parameters&)=delete;
        Parameters& operator=(Parameters&&) noexcept;

        //! Copy content to other parameters
        void copyParameters(Parameters& target) const;

        //! Get id
        std::string id() const;
        //! Set id
        void setID(const std::string& id);

        //! Get value by name
        boost::any value(
            const std::string& name,
            const boost::any& defaultValue=boost::any()
        ) const;

        //! Set value
        void setValue(
            const std::string& name,
            const boost::any& value
        );

        //! Check if parameter exists
        bool exists(
            const std::string& name
        ) const;

        //! Remove parameter
        void removeValue(
            const std::string& name
        );

        //! List all parameter keys
        std::set<std::string> keys() const;

        //! Get parameters map
        const ParametersMap& map() const;

        //! Remove all parameters
        void clear();

        //! Set parameters failure warning mode
        void setWarningMode(
            bool enable,
            LogModule* logModule=HATN_Log<HATN_Log_parameters>::i()
        );
        //! Check if warning on missed parameters is enabled
        bool isWarningEnabled() const;
        //! Get logger module
        LogModule* loggerModule() const;

        //! Get parameter value by name with safe cast
        template<class T> T valueTyped(
                const std::string& name,
                const T& defaultValue=T()
            ) const
        {
            T result=defaultValue;
            try
            {
                auto val=value(name,boost::any(defaultValue));
                if (!val.empty())
                {
                    result=boost::any_cast<T>(val);
                }
            }
            catch (...)
            {
                if (isWarningEnabled())
                {
                    if (logModule!=nullptr)
                    {
                        LOGGER_FILTER_AND_LOG(logModule->filter(LoggerVerbosity::WARNING),"Invalid parameter "<<name);
                    }
                }
            }
            return result;
        }

    protected:

        //! Get mutex
        boost::shared_mutex& mutex() const;

    private:

        LogModule* logModule;
        std::unique_ptr<Parameters_p> d;
};

//---------------------------------------------------------------
    HATN_COMMON_NAMESPACE_END
#endif // HATNPARAMETERS_H
