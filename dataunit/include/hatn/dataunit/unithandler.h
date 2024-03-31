/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.
    
    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)
      
*/

/****************************************************************************/
/*
    
*/
/** @file dataunit/unit.h
  *
  *      Base class for data unit handling.
  *
  *      @todo Not used currently.
  *
  */

/****************************************************************************/

#ifndef HATNDATAUNITHANDLER_H
#define HATNDATAUNITHANDLER_H

#include <functional>

#include <hatn/dataunit/unitcontainer.h>

HATN_DATAUNIT_NAMESPACE_BEGIN

/**
 * @brief Base class for data unit handling
 */
class UnitHandler
{
    public:

        UnitHandler() =default;
        virtual ~UnitHandler()=default;
        UnitHandler(const UnitHandler&)=default;
        UnitHandler(UnitHandler&&) =default;
        UnitHandler& operator=(const UnitHandler&)=default;
        UnitHandler& operator=(UnitHandler&&) =default;

        /**
         * @brief Invoke handler
         * @param lazyUnitParsing If true then unit was not parsed yet and its data is kept in wireDataKeeper()
         *
         * @return Operation status
         */
        virtual common::Error invoke(bool lazyUnitParsing) const =0;

        /**
         * @brief Create unit
         */
        virtual void createUnit() =0;

        /**
         * @brief Reset unit
         */
        virtual void resetUnit() =0;
};

/**
 * @brief Template class for handling data unit of a specific type
 */
template <typename TypeTraits>
class UnitHandlerTmpl : public UnitHandler
{
    public:

        using UnitT=typename TypeTraits::type;
        using ManagedUnitT=typename TypeTraits::managed;

        using HandlerT=std::function<common::Error (const UnitContainer<TypeTraits>&, bool)>;

        /**
         * @brief Ctor
         * @param handler Data unit handler
         * @param factory allocator factor to use for creating data units
         */
        explicit UnitHandlerTmpl(HandlerT handler, AllocatorFactory* factory=AllocatorFactory::getDefault())
            : m_handler(std::move(handler)),
              m_container(factory)
        {}

        /**
         * @brief Ctor
         * @param handler Data unit handler
         * @param unit Data unit
         */
        UnitHandlerTmpl(HandlerT handler, common::SharedPtr<ManagedUnitT> unit)
            : m_handler(std::move(handler)),
              m_container(std::move(unit))
        {}

        /**
         * @brief Ctor
         * @param handler Data unit handler
         * @param unit Data unit container
         */
        UnitHandlerTmpl(HandlerT handler, UnitContainer<TypeTraits> container)
            : m_handler(std::move(handler)),
              m_container(std::move(container))
        {}

        /**
         * @brief Invoke handler
         * @param lazyUnitParsing If true then unit was not parsed yet and its data is kept in wireDataKeeper()
         * @return Operation status
         */
        virtual common::Error invoke(bool lazyUnitParsing) const override
        {
            return m_handler(m_container,lazyUnitParsing);
        }

        /**
         * @brief Create unit
         */
        virtual void createUnit() override
        {
            m_container.createUnit();
        }

        /**
         * @brief Reset unit
         */
        virtual void resetUnit() override
        {
            m_container.resetUnit();
        }

    private:

        HandlerT m_handler;
        UnitContainer<TypeTraits> m_container;
};

HATN_DATAUNIT_NAMESPACE_END

#endif // HATNDATAUNITHANDLER_H
