/*
   Copyright (c) 2019 - current, Evgeny Sidorov (esid1976@gmail.com), All rights reserved


  */

/****************************************************************************/
/*

*/
/** @file dataunit/visitors/serialize.h
  *
  * Contains visitor for data units serialization.
  *
  */

/****************************************************************************/

#ifndef HATNDATAUNITSERIALIZE_H
#define HATNDATAUNITSERIALIZE_H

#include <boost/hana.hpp>

#include <hatn/common/error.h>
#include <hatn/common/result.h>

#include <hatn/dataunit/dataunit.h>
#include <hatn/dataunit/stream.h>
#include <hatn/dataunit/wirebuf.h>

HATN_DATAUNIT_NAMESPACE_BEGIN

//---------------------------------------------------------------

struct HATN_DATAUNIT_EXPORT io
{

    template <typename UnitT, typename BufferT>
    Result<size_t> static serialize(const UnitT& unit, BufferT& buf, bool topLevel=true)
    {
        if (topLevel)
        {
            buf.clear();
        }

        // remember previous buffer size
        auto prevSize=buf.size();

        // predicate with Error
        auto predicate=[](auto&& ec)
        {
            return !ec;
        };

        // handler for each field
        auto handler=[&buf](auto&& field, auto&&)
        {
            using fieldT=std::decay_t<decltype(field)>;

            // skip optional fields which are not set
            if (!field.fieldIsSet())
            {
                if (fieldT::fieldRequired())
                {
                    //! @todo Make error.
                    return commonError(CommonError::NOT_IMPLEMENTED);
                }
                return Error();
            }

            // append tag to stream
            if (!fieldT::fieldRepeatedUnpackedProtoBuf())
            {
                constexpr uint32_t tag=static_cast<uint32_t>((fieldT::fieldId()<<3)|static_cast<uint32_t>(fieldT::fieldWireType()));
                auto* buffer=buf.mainContainer();
                bool storeTagToMeta=fieldT::fieldCanChainBlocks() && !buf.isSingleBuffer();
                if (storeTagToMeta)
                {
                    buf.appendUint32(tag);
                }
                else
                {
                    buf.incSize(Stream<uint32_t>::packVarInt(buffer,tag));
                }
            }

            if (!field.serialize(buf))
            {
                //! @todo Make error.
                return commonError(CommonError::NOT_IMPLEMENTED);
            }

            return Error();
        };

        // serialize each field
        auto ec=unit.each(predicate,Error(),handler);
        if (ec)
        {
            return ec;
        }

        // return added size
        auto addedBytes=buf.size()-prevSize;
        return addedBytes;
    }
};

//---------------------------------------------------------------

HATN_DATAUNIT_NAMESPACE_END

#endif // HATNDATAUNITSERIALIZE_H
