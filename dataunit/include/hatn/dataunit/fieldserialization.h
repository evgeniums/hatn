/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.
    
    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)
      
*/

/****************************************************************************/
/*
    
*/
/** @file dataunit/fieldserialization.h
  *
  *      Classes for serializing and deserializing dataunit fields
  *
  */

/****************************************************************************/

#ifndef HATNFIELDSERIALIZATON_H
#define HATNFIELDSERIALIZATON_H

#include <hatn/dataunit/dataunit.h>
#include <hatn/dataunit/allocatorfactory.h>

HATN_DATAUNIT_NAMESPACE_BEGIN

class Unit;
class WireData;

//! Setrializer/deserializer of fixed size fields
template <typename T>
class FixedSer
{
    public:

        //! Serialize to wire
        template <typename BufferT>
        static bool serialize(const T& value, BufferT& wired);

        //! Deserialize from wire
        template <typename BufferT>
        static bool deserialize(T& value, BufferT& wired);
};

//! Setrializer/deserializer of variable size fields on
template <typename T>
class VariableSer
{
    public:

        //! Serialize to wire
        template <typename BufferT>
        static bool serialize(const T& value, BufferT& wired);

        //! Deserialize from wire
        template <typename BufferT>
        static bool deserialize(T& value, BufferT& wired);
};

//! Setrializer/deserializer of byte fields
template <typename OnStackT,typename SharedT>
class BytesSer
{
    public:

        //! Serialize to wire
        template <typename BufferT>
        static bool serialize(
                                BufferT& wired,
                                const OnStackT* buf,
                                const SharedT& shared,
                                bool canChainBlocks
                              );

        //! Deserialize from wire
        template <typename BufferT>
        static bool deserialize(
                                BufferT& wired,
                                OnStackT* buf,
                                SharedT& shared,
                                const AllocatorFactory *factory,
                                int maxSize=0,
                                bool canChainBlocks=true
                                );
};

//! Serializer/deserializer of subunit fields.
class HATN_DATAUNIT_EXPORT UnitSer
{
    public:

        //! Serialize to wir
        template <typename UnitT, typename BufferT>
        static bool serialize(const UnitT* value, BufferT& wired, common::ByteArrayShared skippedNotParsed={});

        //! Deserialize from wire
        template <typename UnitT, typename BufferT>
        static bool deserialize(UnitT* value, BufferT& wired);
};

//! Setrializer/deserializer of "as bytes" fields.
class HATN_DATAUNIT_EXPORT AsBytesSer
{
    public:

        template <typename BufferT>
        static bool serializeAppend(
            BufferT& wired,
            const char* data,
            size_t size
        );

        template <typename BufferT>
        static common::DataBuf serializePrepare(
            BufferT& wired,
            size_t size
        );

        template <typename BufferT, typename HandlerT>
        static bool deserialize(BufferT& wired, HandlerT fn, size_t maxSize=0);
};

HATN_DATAUNIT_NAMESPACE_END

#endif // HATNFIELDSERIALIZATON_H
