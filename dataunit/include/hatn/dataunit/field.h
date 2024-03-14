/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.
    
    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)
      
*/

/****************************************************************************/
/*
    
*/
/** \file dataunit/field.h
  *
  *      Base class of dataunit fields
  *
  */

/****************************************************************************/

#ifndef HATNFIELDBASE_H
#define HATNFIELDBASE_H

#include <functional>

#include <hatn/dataunit/dataunit.h>
#include <hatn/dataunit/wiredata.h>
#include <hatn/dataunit/allocatorfactory.h>
#include <hatn/dataunit/fieldgetset.h>

HATN_DATAUNIT_NAMESPACE_BEGIN

class Unit;

//! Field types as defined in Google Protocol Buffers
enum class WireType : int
{
    VarInt=0,
    Fixed64=1,
    WithLength=2,
    Fixed32=5
};

namespace json
{
    class Writer;
}

//! Base field class
class HATN_DATAUNIT_EXPORT Field : public FieldGetSet
{
    public:

        constexpr static const bool CanChainBlocks=false;

        //! Ctor
        Field(Unit* unit);

        virtual ~Field();
        Field(const Field& other)=default;
        Field& operator=(const Field& other)=default;
        Field(Field&& other) =default;
        Field& operator =(Field&& other) =default;

        //! Load field from wire
        inline bool load(WireData& wired, AllocatorFactory *factory)
        {
            m_set=doLoad(wired,factory);
            return m_set;
        }

        //! Store field to wire
        inline bool store(WireData& wired) const
        {
            return doStore(wired);
        }

        static inline WireType wireTypeDef() noexcept
        {
            return WireType::VarInt;
        }

        //! Get wire type of the field
        virtual WireType wireType() const noexcept;

        //! Can chain blocks
        virtual bool canChainBlocks() const noexcept
        {
            return CanChainBlocks;
        }

        //! Check if field is set
        inline bool isSet() const noexcept
        {
            return m_set;
        }

        //! Mark field as set
        inline void markSet(bool enable=true) noexcept
        {
            m_set=enable;
        }

        //! Check if field is required
        virtual bool isRequired() const noexcept =0;
        //! Get field ID
        virtual int getID() const noexcept =0;
        //! Get field name
        virtual const char* name() const noexcept {return nullptr;}
        //! Get field name size
        virtual size_t nameSize() const noexcept {return 0;}

        //! Get field description.
        virtual const char* description() const noexcept {return "";}

        //! Get field size
        virtual size_t size() const  noexcept =0;
        //! Clear field
        virtual void clear()=0;

        //! Reset field
        virtual void reset()
        {
            clear();
        }

        //! Check if this field is compatible of repeated unpacked type with Google Protocol Buffers
        virtual bool isRepeatedUnpackedProtoBuf() const noexcept;

        /**
         * @brief Use shared version of byte arrays when parsing wired data
         * @param enable Enabled on/off
         * @param factory Allocator factory to use for dynamic allocation
         *
         * When enabled then shared byte arrays will be auto allocated in managed shared buffers
         */
        virtual void setParseToSharedArrays(bool enable,::hatn::dataunit::AllocatorFactory* /*factory*/=nullptr);

        /**
         * @brief Check if shared byte arrays must be used for parsing
         * @return Boolean flag
         */
        virtual bool isParseToSharedArrays() const noexcept;

        virtual void pushJsonParseHandler(Unit*)=0;

        virtual bool toJSON(json::Writer* writer) const=0;

        virtual bool hasDefaultValue() const noexcept;

        Unit* unit() const noexcept
        {
            return m_unit;
        }
        Unit* unit() noexcept
        {
            return m_unit;
        }

    protected:

        //! Load field from wire
        virtual bool doLoad(WireData&,AllocatorFactory*)=0;

        //! Store field to wire
        virtual bool doStore(WireData&) const = 0;

        bool m_set=false;

    private:

        Unit* m_unit;
};

HATN_DATAUNIT_NAMESPACE_END

#endif // HATNFIELDBASE_H
