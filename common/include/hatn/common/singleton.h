/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.
    
    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)
      
*/
/****************************************************************************/
/** @file common/singleton.h
 *
 *     Singletons' registrar and base class for singletons.
 *
 */
/****************************************************************************/

#ifndef HATNSINGLETON_H
#define HATNSINGLETON_H

#include <map>
#include <vector>

#include <hatn/common/common.h>
#include <hatn/common/classuid.h>

HATN_COMMON_NAMESPACE_BEGIN

//! Base class for singletons that can be registered in global registrar of singletons
class HATN_COMMON_EXPORT Singleton : public HumanReadableType
{
    public:

        //! Ctor
        Singleton();

        //! Dtor
        virtual ~Singleton();

        Singleton(const Singleton&)=delete;
        Singleton(Singleton&&) =delete;
        Singleton& operator=(const Singleton&)=delete;
        Singleton& operator=(Singleton&&) =delete;

        //! Get order position of this singleton in the registrar
        int registrarPosition() const noexcept
        {
            return m_position;
        }

    protected:

        /**
         * @brief Reset singleton
         *
         * Usually free() method is called.
         * Depending on the implementation it will actually destroy the object or just reset it.
         * In case of actual destroying the singleton will be unregistered from the registrar.
         */
        virtual void resetSingleton()=0;

    private:

        /**
         * @brief Set position of this singleton in the registrar
         * @param pos Position
         *
         * Only registrar must call this method, so it is a friend class.
         */
        void setRegistrarPosition(int pos) noexcept
        {
            m_position=pos;
        }

        int m_position;
        friend class SingletonRegistrar;
};

//! Registrar of singletons
class HATN_COMMON_EXPORT SingletonRegistrar
{
    public:

        /**
         * @brief Register singleton object
         * @param obj Singleton object
         */
        void registerSingleton(Singleton* obj);

        /**
         * @brief Unreginster singlton object
         * @param obj Singleton object
         */
        void unregisterSingleton(Singleton *obj);

        /**
         * @brief Reset all singletons.
         *
         * Singletons will be reset in reverse order of registration times.
         * In this method the singletons are not removed explicitly from the registrar.
         * Though, they can be removed implicitly if resetSingleton() method of a singleton actually destroys the singleton
         * or unregisters it.
         */
        void reset();

        //! Get registrar instance
        static SingletonRegistrar& instance();
        //! Reset all singletons and remove them from the registrar
        static void free();

        //! Get list of singletons ordered by time of registration
        std::vector<Singleton*> singletons() const;

    private:

        std::map<int,Singleton*> m_singletons;
};

//! Declare singleton, place it in class declaration
#define HATN_SINGLETON_DECLARE() \
    private: \
        static std::string m_humanReadableType; \
    public: \
        virtual std::string humanReadableType() const noexcept override; \
        virtual void setHumanReadableType(std::string type) noexcept override; \
        virtual void resetSingleton() override;

//! Instantiate singleton type, place it in compilation unit source (*.cpp)
#define HATN_SINGLETON_INIT(ClassName) \
    std::string ClassName::m_humanReadableType = #ClassName; \
    std::string ClassName::humanReadableType() const noexcept \
    { \
        return m_humanReadableType; \
    } \
    void ClassName::setHumanReadableType(std::string type) noexcept \
    { \
        m_humanReadableType = std::move(type); \
    } \
    void ClassName::resetSingleton() \
    { \
        free(); \
    }


HATN_COMMON_NAMESPACE_END

#endif // HATNSINGLETON_H
