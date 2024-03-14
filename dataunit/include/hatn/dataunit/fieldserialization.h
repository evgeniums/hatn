/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.
    
    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)
      
*/

/****************************************************************************/
/*
    
*/
/** \file dataunit/fieldserialization.h
  *
  *      Classes for serializing and deserializing dataunit fields
  *
  */

/****************************************************************************/

#ifndef HATNFIELDSERIALIZATON_H
#define HATNFIELDSERIALIZATON_H

#include <hatn/dataunit/dataunit.h>

HATN_DATAUNIT_NAMESPACE_BEGIN

class Unit;
class WireData;

//! Setrializer/deserializer of fixed size fields
template <typename T> class FixedSer
{
    public:

        //! Serialize to wire
        static bool serialize(const T& value, WireData& wired);

        //! Deserialize from wire
        static bool deserialize(T& value, WireData& wired);
};

//! Setrializer/deserializer of variable size fields on
template <typename T> class VariableSer
{
    public:

        //! Serialize to wire
        static bool serialize(const T& value, WireData& wired);

        //! Deserialize from wire
        static bool deserialize(T& value, WireData& wired);
};

//! Setrializer/deserializer of byte fields
template <typename onstackT,typename sharedT> class BytesSer
{
    public:

        //! Serialize to wire
        static bool serialize(
                                WireData& wired,
                                const onstackT* buf,
                                const sharedT& shared,
                                bool canChainBlocks
                              );

        //! Deserialize from wire
        static bool deserialize(
                                WireData& wired,
                                onstackT* buf,
                                sharedT& shared,
                                AllocatorFactory *factory,
                                int maxSize=0,
                                bool canChainBlocks=true
                                );
};

//! Setrializer/deserializer of subdataunit fields
class HATN_DATAUNIT_EXPORT UnitSer
{
    public:

        //! Serialize to wire
        static bool serialize(const Unit* value, WireData& wired);

        //! Deserialize from wire
        static bool deserialize(Unit* value, WireData& wired);
};

HATN_DATAUNIT_NAMESPACE_END
#endif // HATNFIELDSERIALIZATON_H
