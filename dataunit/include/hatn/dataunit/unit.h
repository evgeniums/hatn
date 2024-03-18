/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.
    
    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)
      
*/

/****************************************************************************/
/*
    
*/
/** \file dataunit/unit.h
  *
  *      Base class for data units
  *
  */

/****************************************************************************/

#ifndef HATNDATAUNITBASE_H
#define HATNDATAUNITBASE_H

#include <functional>

#include <hatn/common/format.h>
#include <hatn/common/singleton.h>

#include <hatn/dataunit/dataunit.h>
#include <hatn/dataunit/wiredata.h>
#include <hatn/dataunit/allocatorfactory.h>

#include <hatn/dataunit/field.h>

#ifndef RAPIDJSON_NO_SIZETYPEDEFINE
#define RAPIDJSON_NO_SIZETYPEDEFINE
namespace rapidjson { using SizeType=size_t; }
#endif
#include <rapidjson/reader.h>
#include <rapidjson/writer.h>

HATN_DATAUNIT_NAMESPACE_BEGIN

class RapidJsonStringViewStream;
namespace json {
    class Writer;
}

class Unit;
class Field;
class Unit_p;

/**
 * @brief Wrapper of field name to be used as container's key.
 */
struct FieldNamesKey
{
    const char* name;
    size_t size;

    friend inline bool operator== (const FieldNamesKey& left, const FieldNamesKey& right) noexcept
    {
        return left.size==right.size && memcmp(left.name,right.name,left.size);
    }
    friend inline bool operator< (const FieldNamesKey& left, const FieldNamesKey& right) noexcept
    {
        return left.size<right.size || (left.size==right.size && memcmp(left.name,right.name,left.size)<0);
    }
};

//! Base class for data units
class HATN_DATAUNIT_EXPORT Unit
{
    public:

        using JsonParseHandler=std::function<bool(rapidjson::Reader&,RapidJsonStringViewStream&)>;

        //! Ctor
        Unit(AllocatorFactory* factory=AllocatorFactory::getDefault());

        virtual ~Unit();
        Unit(const Unit&)=default;
        Unit(Unit&&) =default;
        Unit& operator=(const Unit&)=default;
        Unit& operator=(Unit&&) =default;

        //! Get field by ID
        virtual const Field* fieldById(int id) const;

        //! Get field by ID
        virtual Field* fieldById(int id);

        //! Get field by name
        /**
         * @brief Find field by name
         * @param name name
         * @param size Name length or 0 if the name is null-terminated
         * @return Found field
         *
         * This operation is slow as the searching is done iteratively field by field.
         * For fast repetitive lookups use fillFieldNamesTable() and then make lookups
         * in that table.
         *
         */
        Field* fieldByName(const char* name,size_t size=0);

        //! Get field by name with const signature
        const Field* fieldByName(const char* name,size_t size=0) const;

        //! Fill field names table as [name]=>[field]
        void fillFieldNamesTable(common::pmr::map<FieldNamesKey,Field*>& table);

        //! Parse DataUnit from plain data buffer
        //! @todo Use Error with NativeError.
        bool parse(
            const char* data,
            size_t size,
            bool inlineBuffer=true
        );

        //! Parse DataUnit from container
        //! @todo Use Error with NativeError.
        bool parse(
                const common::ByteArray& container,
                bool inlineBuffer=true
            )
        {
            return parse(container.data(),container.size(),inlineBuffer);
        }

        /**
         * @brief Parse DataUnit from wired data unit, only single buffer supported for parsing
         * @param wired DataUnit  parse
         * @param topLevel Is this top level unit
         * @return Parsing status
         *
         * @todo Use Error with NativeError.
         * To see parsing errors the DEBUG logging mode must be enabled for "dataunit" module, context "parse"
         *
         */
        bool parse(
            WireData& wired,
            bool topLevel=true
        );

        /**
         * @brief Serialize DataUnit to wired data unit
         * @param wired Control structure
         * @param topLevel This DataUnit is either top level (true) or embedded DataUnit (false)
         * @return Size of serialized data or -1 if failed
         *
         * @todo Use Error with NativeError.
         * If serialization fails then warnings will appear in log describing the problem
         */
        int serialize(
            WireData& wired,
            bool topLevel=true
        ) const;

        /**
         * @brief Serialize DataUnit to plain data buffer
         * @param buf Buffer
         * @param bufSize Size of buffer, must be equal or greater than size()
         * @param checkSize Check if bufSize is enough to store serialized data
         * @return Size of serialized data or -1 if failed
         *
         * @todo Use Error with NativeError.
         * If serialization fails then warnings will appear in log describing the problem
         */
        int serialize(
            char* buf,
            size_t bufSize,
            bool checkSize=true
        ) const;

        /**
         * @brief Serialize to data container
         * @param container Target container
         * @param offsetOut Offest in target container
         * @return Operation status
         *
         * @todo Use Error with NativeError.
         */
        template <typename ContainerT>
        bool serialize(ContainerT& container,
                       size_t offsetOut=0,
                       typename std::enable_if<!std::is_base_of<WireData,ContainerT>::value,void*>::type=nullptr
                )
        {
            auto expectedSize=size();
            container.resize(expectedSize+offsetOut);
            int actualSize=serialize(container.data()+offsetOut,expectedSize,false);
            if (actualSize<0)
            {
                return false;
            }
            container.resize(static_cast<size_t>(actualSize)+offsetOut);
            return true;
        }

        //! Get estimated size of wired data unit
        /**
         * @brief Actual packed size can be less than estimated but will never exceed it
         * @return Estimated size of the packed unit
         */
        size_t size() const;

        //! Hold wired data unit
        inline void keepWireDataPack(
            common::SharedPtr<WireDataPack> wired
        ) noexcept
        {
            m_wireDataPack=std::move(wired);
        }

        //! Get wired data unit
        inline const common::SharedPtr<WireDataPack>& wireDataPack() const noexcept
        {
            return m_wireDataPack;
        }

        //! Reset wire dataunit
        inline void resetWireData() noexcept
        {
            m_wireDataPack.reset();
        }

        //! Handler type for fields iterator
        using FieldVisitor=std::function<bool (Field&)>;

        //! Handler type for fields const iterator
        using FieldVisitorConst=std::function<bool (const Field&)>;

        //! Iterate fields applying visitor handler
        virtual bool iterateFields(const Unit::FieldVisitor& visitor);

        //! Iterate fields applying visitor handler
        virtual bool iterateFieldsConst(const Unit::FieldVisitorConst& visitor) const;

        //! Get field count
        virtual size_t fieldCount() const noexcept;

        //! Get name
        virtual const char* name() const noexcept;

        //! Clear unit
        void clear();

        //! Reset unit
        void reset();

        //! Is DataUnit empty
        inline bool isEmpty() const noexcept
        {
            return m_clean;
        }

        /** Get reference to self **/
        inline const Unit& value() const noexcept
        {
            return *this;
        }

        /** Get mutable pointer to self **/
        inline Unit* mutableValue() noexcept
        {
            return this;
        }

        /**
         * @brief Use shared version of byte arrays when parsing wired data
         * @param enable Enabled on/off
         * @param factory Allocator factory to use for dynamic allocation
         *
         * When enabled then shared byte arrays will be auto allocated in managed shared buffers
         */
        void setParseToSharedArrays(bool enable,::hatn::dataunit::AllocatorFactory* factory=nullptr);

        /**
         * @brief Serialize DataUnit to JSON format into buffer
         * @param buf Destination buffer
         * @param prettyFormat Add line endings and identations
         * @param maxDecimalPlaces Maximum number of decimal places for double output
         * @param errorMessage Error message if JSON parsing failed
         * @return True on success
         *
         * @todo Use Error with NativeError.
         * If serialization fails then warnings will appear in log describing the problem
         *
         */
        bool toJSON(common::FmtAllocatedBufferChar& buf,
                                              bool prettyFormat=false,
                                              int maxDecimalPlaces=0
                                        ) const;

        /**
         * @brief Serialize DataUnit to JSON format into buffer
         * @param buf Destination buffer
         * @param prettyFormat Add line endings and identations
         * @param maxDecimalPlaces Maximum number of decimal places for double output
         * @return True on success
         *
         * @todo Use Error with NativeError.
         * If serialization fails then warnings will appear in log describing the problem
         *
         */
        bool toJSON(common::ByteArray& buf,
                                      bool prettyFormat=false,
                                      int maxDecimalPlaces=0
                            ) const;

        /**
         * @brief Serialize DataUnit to JSON format to string
         * @param buf Destination buffer
         * @param prettyFormat Add line endings and identations
         * @param maxDecimalPlaces Maximum number of decimal places for double output
         * @return True on success
         *
         * @todo Use Error with NativeError.
         * If serialization fails then warnings will appear in log describing the problem
         *
         */
        bool toJSON(std::string& buf,
                                      bool prettyFormat=false,
                                      int maxDecimalPlaces=0
                            ) const;

        /**
         * @brief Serialize DataUnit to JSON format into buffer
         * @param buf Destination buffer
         * @param prettyFormat Add line endings and identations
         * @param maxDecimalPlaces Maximum number of decimal places for double output
         * @return True on success
         *
         * @todo Use Error with NativeError.
         * If serialization fails then warnings will appear in log describing the problem
         */
        bool toJSON(std::vector<char>& buf,bool prettyFormat=false, int maxDecimalPlaces=0) const;

        bool toJSON(json::Writer* writer) const;

        /**
         * @brief Serialize DataUnit to string in JSON format
         * @param prettyFormat Add line endings and identations
         * @param maxDecimalPlaces Maximum number of decimal places for double output
         * @return JSON string
         *
         * @todo Use Error with NativeError.
         * If serialization fails then warnings will appear in log describing the problem
         */
        std::string toString(bool prettyFormat=false,int maxDecimalPlaces=0) const;

        template <typename ContainerT>
        bool loadFromJSON(const ContainerT& container)
        {
            return loadFromJSON(common::lib::string_view(container.data(),container.size()));
        }

        /**
         * @brief Load DataUnit from JSON
         * @param str JSON formatted string
         * @return Status of parsing
         *
         * @todo Use Error with NativeError.
         * To see parsing errors the DEBUG logging mode must be enabled for "dataunit" module, context "json-parse"
         */
        bool loadFromJSON(const common::lib::string_view& str);
        bool loadFromJSON(const char* buf, size_t size)
        {
            common::lib::string_view view(buf,size);
            return loadFromJSON(view);
        }

        void pushJsonParseHandler(const JsonParseHandler& handler);

        /**
         * @brief Get allocator factory
         * @return allocator factory
         */
        inline AllocatorFactory* factory() const noexcept
        {
            return m_factory;
        }

        /**
         * @brief Create dynamically allocated DataUnit of self type
         *
         * Only managed versions of units can be created
         */
        virtual common::SharedPtr<Unit> createManagedUnit() const;

        /**  Check if unit has a field. */
        template <typename T>
        constexpr static bool hasField(T&& fieldName) noexcept
        {
            std::ignore=fieldName;
            return false;
        }

    private:

        template <typename T>
        bool toJSONImpl(T& buf,
          bool prettyFormat=false,
          int maxDecimalPlaces=0
        ) const;


        common::SharedPtr<WireDataPack> m_wireDataPack;
        bool m_clean;

        AllocatorFactory* m_factory;
        common::pmr::list<JsonParseHandler> m_jsonParseHandlers;
};

HATN_DATAUNIT_NAMESPACE_END

#endif // HATNDATAUNITBASE_H
