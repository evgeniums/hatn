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
  *      Base class for data units
  *
  */

/****************************************************************************/

#ifndef HATNDATAUNITBASE_H
#define HATNDATAUNITBASE_H

#include <functional>

#include <hatn/common/format.h>
#include <hatn/common/singleton.h>
#include <hatn/common/classuid.h>

#include <hatn/dataunit/dataunit.h>
#include <hatn/dataunit/wiredata.h>
#include <hatn/dataunit/allocatorfactory.h>

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
        Unit(const AllocatorFactory* factory=AllocatorFactory::getDefault());

        virtual ~Unit();
        Unit(const Unit&)=default;
        Unit(Unit&&) =default;
        Unit& operator=(const Unit&)=default;
        Unit& operator=(Unit&&) =default;

        //! Get field by ID
        virtual const Field* fieldById(int id) const;
        virtual Field* fieldById(int id);

        //! Get field by name
        virtual const Field* fieldByName(common::lib::string_view name) const;
        virtual Field* fieldByName(common::lib::string_view name);

        //! Parse DataUnit from plain data buffer
        bool parse(
            const char* data,
            size_t size,
            bool inlineBuffer=true
        );

        //! Parse DataUnit from container
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
         */
        virtual bool parse(
            WireData& wired,
            bool topLevel=true
        );

        virtual bool parse(
            WireBufSolid& wired,
            bool topLevel=true
        );

        virtual bool parse(
            WireBufSolidShared& wired,
            bool topLevel=true
        );

        virtual bool parse(
            WireBufChained& wired,
            bool topLevel=true
        );

        /**
         * @brief Serialize DataUnit to wired data unit
         * @param wired Control structure
         * @param topLevel This DataUnit is either top level (true) or embedded DataUnit (false)
         * @return Size of serialized data or -1 if failed
         */
        virtual int serialize(
            WireData& wired,
            bool topLevel=true
        ) const;

        virtual int serialize(
            WireBufSolid& wired,
            bool topLevel=true
        ) const;

        virtual int serialize(
            WireBufSolidShared& wired,
            bool topLevel=true
        ) const;

        virtual int serialize(
            WireBufSolidRef& wired,
            bool topLevel=true
        ) const;

        virtual int serialize(
            WireBufChained& wired,
            bool topLevel=true
        ) const;

        /**
         * @brief Serialize DataUnit to plain data buffer
         * @param buf Buffer
         * @param bufSize Size of buffer, must be equal or greater than maxPackedSize()
         * @param checkSize Check if bufSize is enough to store serialized data
         * @return Size of serialized data or -1 if failed
         */
        int serialize(
            char* buf,
            size_t bufSize,
            bool checkSize=true
        ) const;

        /**
         * @brief Serialize to data container
         * @param container Target container
         * @param offsetOut Offset in target container
         * @return Operation status
         */
        template <typename ContainerT>
        bool serialize(ContainerT& container,
                       size_t offsetOut=0,
                       typename std::enable_if<
                           !std::is_base_of<WireData,ContainerT>::value
                           &&
                           !std::is_base_of<WireBufBase,ContainerT>::value
                        ,void*>::type=nullptr
                )
        {
            auto expectedSize=maxPackedSize();
            container.resize(expectedSize+offsetOut);
            int actualSize=serialize(container.data()+offsetOut,expectedSize,false);
            if (actualSize<0)
            {
                return false;
            }
            container.resize(static_cast<size_t>(actualSize)+offsetOut);
            return true;
        }

        //! Keep serialized data unit.
        inline void keepWireData(
            common::SharedPtr<WireData> wired
        ) noexcept
        {
            m_wireDataKeeper=std::move(wired);
        }

        //! Get serialized data unit.
        inline const common::SharedPtr<WireData>& wireDataKeeper() const noexcept
        {
            return m_wireDataKeeper;
        }

        //! Reset keeper of serialized data unit.
        inline void resetWireDataKeeper() noexcept
        {
            m_wireDataKeeper.reset();
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
        virtual void clear();

        //! Reset unit
        virtual void reset(bool onlyNonClean=false);

        //! Get estimated size of wired data unit
        /**
         * @brief Actual packed size can be less than estimated but will never exceed it
         * @return Estimated size of the packed unit
         */
        virtual size_t maxPackedSize() const;

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
        void setParseToSharedArrays(bool enable,const AllocatorFactory* factory=nullptr);

        /**
         * @brief Serialize DataUnit to JSON format into buffer
         * @param buf Destination buffer
         * @param prettyFormat Add line endings and identations
         * @param maxDecimalPlaces Maximum number of decimal places for double output
         * @param errorMessage Error message if JSON parsing failed
         * @return True on success
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
         * @todo Refactor json methods with non virtual methods, add mehod with filling error.
         */
        bool toJSON(std::vector<char>& buf,bool prettyFormat=false, int maxDecimalPlaces=0) const;

        bool toJSON(json::Writer* writer) const;

        /**
         * @brief Serialize DataUnit to string in JSON format
         * @param prettyFormat Add line endings and identations
         * @param maxDecimalPlaces Maximum number of decimal places for double output
         * @return JSON string
         */
        std::string toString(bool prettyFormat=false,int maxDecimalPlaces=0) const;

        template <typename ContainerT>
        bool loadFromJSON(const ContainerT& container)
        {
            return loadFromJSON(common::lib::string_view(container.data(),container.size()));
        }

        template <typename ContainerT>
        bool loadFromJSON(const ContainerT& container, Error& ec)
        {
            return loadFromJSON(common::lib::string_view(container.data(),container.size()),ec);
        }

        /**
         * @brief Load DataUnit from JSON
         * @param str JSON formatted string
         * @return Status of parsing
         */
        bool loadFromJSON(const common::lib::string_view& str);

        bool loadFromJSON(const char* buf, size_t size)
        {
            common::lib::string_view view(buf,size);
            return loadFromJSON(view);
        }

        bool loadFromJSON(const char* buf, size_t size, Error& ec)
        {
            common::lib::string_view view(buf,size);
            return loadFromJSON(view,ec);
        }

        // Load from JSON with eerror code
        bool loadFromJSON(const common::lib::string_view& str, Error& ec);

        void pushJsonParseHandler(const JsonParseHandler& handler);

        /**
         * @brief Get allocator factory
         * @return allocator factory
         */
        inline const AllocatorFactory* factory() const noexcept
        {
            return m_factory;
        }

        /**
         * @brief Create dynamically allocated DataUnit of self type
         *
         * Only managed versions of units can be created
         */
        virtual common::SharedPtr<Unit> createManagedUnit() const;

        virtual common::SharedPtr<Unit> toManagedUnit() const;

        virtual bool isManagedUnit() const noexcept;

        /**  Check if unit has a field. */
        template <typename T>
        constexpr static bool hasField(T&& fieldName) noexcept
        {
            std::ignore=fieldName;
            return false;
        }

        bool isClean() const noexcept
        {
            return m_clean;
        }

        bool setClean(bool val) noexcept
        {
            return m_clean=val;
        }

        virtual std::pair<int,const char*> checkRequiredFields() noexcept
        {
            return std::pair<int,const char*>{-1,nullptr};
        }

        virtual common::CUID_TYPE typeID() const noexcept
        {
            return 0;
        }

    protected:

        void setFieldParent(Field& field);

    private:

        template <typename T>
        bool toJSONImpl(T& buf,
          bool prettyFormat=false,
          int maxDecimalPlaces=0
        ) const;

        common::SharedPtr<WireData> m_wireDataKeeper;
        bool m_clean;

        const AllocatorFactory* m_factory;
        common::pmr::list<JsonParseHandler> m_jsonParseHandlers;

        friend struct visitors;
};

using UnitManaged = common::ManagedWrapper<Unit>;

HATN_DATAUNIT_NAMESPACE_END

#endif // HATNDATAUNITBASE_H
