/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.
    
    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)
      
*/

/****************************************************************************/
/*
    
*/
/** \file dataunit/unitcontainer.h
  *
  *      Container of data unit
  *
  */

/****************************************************************************/

#ifndef HATNDATAUNITCONTAINER_H
#define HATNDATAUNITCONTAINER_H

#include <hatn/dataunit/unit.h>

HATN_DATAUNIT_NAMESPACE_BEGIN

/**
 * @brief Container of data unit
 */
template <typename TypeTraits>
class UnitContainer
{
    public:

        using UnitT=typename TypeTraits::type;
        using ManagedUnitT=typename TypeTraits::managed;

        /**
         * @brief Ctor with unit
         * @param unit Unit to put o container
         */
        explicit UnitContainer(common::SharedPtr<ManagedUnitT> unit)
            : m_unit(std::move(unit)),
              m_factory(m_unit->factory())
        {}

        /**
         * @brief Default ctor
         * @param factory Allocator factory to use for unit creation
         */
        UnitContainer(AllocatorFactory* factory=AllocatorFactory::getDefault())
            : m_factory(factory)
        {}

        //! Get contained unit
        UnitT* unit() noexcept
        {
            return m_unit.get();
        }

        //! Get contained unit
        UnitT* unit() const noexcept
        {
            return m_unit.get();
        }

        //! Get contained unit
        common::SharedPtr<ManagedUnitT> sharedUnit() const noexcept
        {
            return m_unit;
        }

        //! Reset contained unit
        void resetUnit() noexcept
        {
            m_unit.reset();
        }

        //! Create and keep unit
        void createUnit()
        {
            m_unit=m_factory->createObject<ManagedUnitT>(m_factory);
        }

    private:

        common::SharedPtr<ManagedUnitT> m_unit;
        AllocatorFactory* m_factory;
};

HATN_DATAUNIT_NAMESPACE_END

#endif // HATNDATAUNITCONTAINER_H
