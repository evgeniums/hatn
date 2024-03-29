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

#include <system_error>

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
    /**
     * @brief Serialize data unit without invokation of virtual methods.
     * @param obj Object to serialize.
     * @param wired Buffer containing data.
     * @param top Is it top level object or subunit.
     * @return Serialized size or -1 in case of error.
     *
     * @todo Use descriptive error or maybe just throw on missing required fields?
     */
    template <typename UnitT, typename BufferT>
    int static serialize(const UnitT& obj, BufferT& wired, bool top=true)
    {
        return hana::eval_if(
            std::is_same<Unit,UnitT>{},
            [&](auto _)
            {
                // invoke virtual searilize() of unit object
                return _(obj).serialize(_(wired),_(top));
            },
            [&](auto _)
            {
                // invoke searilization of each field

                auto&& topLevel=_(top);
                auto&& unit=_(obj);
                auto&& buf=_(wired);

                if (topLevel)
                {
                    buf.clear();
                }

                if (!unit.wireDataPack().isNull())
                {
                    // use already serialized data
                    auto addedBytes=buf.append(unit.wireDataPack()->wireData());
                    return addedBytes;
                }

                // remember previous buffer size
                auto prevSize=buf.size();

                // predicate with Error
                auto predicate=[](auto&& ok)
                {
                    return ok;
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
                            return false;
                        }
                        return true;
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
                        return false;
                    }

                    return true;
                };


                auto ok=_(unit).each(predicate,true,handler);
                if (!ok)
                {
                    return -1;
                }

                // return added size
                auto addedBytes=static_cast<int>(buf.size()-prevSize);
                return addedBytes;
            }
        );
    }
};

//---------------------------------------------------------------

HATN_DATAUNIT_NAMESPACE_END

#endif // HATNDATAUNITSERIALIZE_H
