/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.
    
    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)
      
*/

/****************************************************************************/
/*
    
*/
/** @file dataunit/wiredata.h
  *
  *      Packed (serialized) DataUnit data for wire.
  *
  */

/****************************************************************************/

#ifndef HATNDATAUNITWIREDATA_H
#define HATNDATAUNITWIREDATA_H

#include <list>

#include <hatn/common/managedobject.h>
#include <hatn/common/bytearray.h>
#include <hatn/common/makeshared.h>
#include <hatn/common/pmr/pmrtypes.h>
#include <hatn/common/sharedptr.h>
#include <hatn/common/weakptr.h>
#include <hatn/common/spanbuffer.h>

#include <hatn/dataunit/allocatorfactory.h>
#include <hatn/dataunit/dataunit.h>

HATN_DATAUNIT_NAMESPACE_BEGIN

class WireDataSingle;

//! Base class for packed (serialized) data units for wire
class HATN_DATAUNIT_EXPORT WireData
{    
    public:

        /**
         * @brief Ctor with allocators
         * @param byteArraysAllocator Allocator for allocation of byte arrays
         * @param byteArraysContentAllocator Allocator for allocation internal contents of byte arrays
         */
        explicit WireData(
            AllocatorFactory* factory=nullptr,
            bool useShareBuffers=false
        ) noexcept
          : m_factory(factory),
            m_offset(0),
            m_size(0),
            m_useShareBuffers(useShareBuffers),
            m_useInlineBuffers(false)
        {}

        explicit WireData(
            size_t size,
            AllocatorFactory* factory=nullptr,
            bool useShareBuffers=false
        ) noexcept
          : m_factory(factory),
            m_offset(0),
            m_size(size),
            m_useShareBuffers(useShareBuffers),
            m_useInlineBuffers(false)
        {}

        virtual ~WireData()=default;
#if __cplusplus < 201703L
        // to use as return in create() method
        WireData(const WireData&)=default;
#else
        WireData(const WireData&)=delete;
#endif
        WireData& operator=(const WireData&)=delete;
        WireData(WireData&&) =default;
        WireData& operator=(WireData&&) =default;

        virtual common::SpanBuffer nextBuffer() const noexcept {return common::SpanBuffer();}
        virtual common::ByteArray* mainContainer() const noexcept {return nullptr;}
        virtual void appendBuffer(common::SpanBuffer&& /*buf*/){}
        virtual void appendBuffer(const common::SpanBuffer& /*buf*/){}
        void appendBuffer(const common::ByteArrayShared& buf)
        {
            appendBuffer(common::SpanBuffer(buf));
        }
        void appendBuffer(common::ByteArrayShared&& buf)
        {
            appendBuffer(common::SpanBuffer(std::move(buf)));
        }
        virtual bool isSingleBuffer() const noexcept {return true;}
        virtual void reserveMetaCapacity(size_t) {}
        inline size_t currentOffset() const noexcept {return m_offset;}
        inline void setCurrentOffset(size_t offs) noexcept {m_offset=offs;}
        inline void incCurrentOffset(int increment) noexcept {m_offset+=increment;}
        virtual void resetState() {m_offset=0;}
        virtual void setCurrentMainContainer(common::ByteArray* /*currentMainContainer*/) noexcept
        {}

        inline void clear()
        {
            doClear();
            resetState();
            m_size=0;
        }

        inline bool isUseSharedBuffers() const noexcept
        {
            return m_useShareBuffers;
        }

        //! Set wired size
        inline void setSize(size_t size) noexcept
        {
            m_size=size;
        }

        //! Get wired size
        inline size_t size() const noexcept
        {
            return m_size;
        }

        //! Increment size
        inline void incSize(int increment) noexcept
        {
            m_size+=increment;
        }

        /**
         * @brief Append meta variable
         * @param varTypeSize Size of variable type
         * @return ByteArray to put the variable to
         *
         */
        virtual common::ByteArray* appendMetaVar(size_t varTypeSize)
        {
            Assert(false,"Invalid operation");
            std::ignore=varTypeSize;
            return nullptr;
        }

        int appendUint32(uint32_t val);

        inline AllocatorFactory* factory() const noexcept
        {
            return m_factory;
        }

        WireDataSingle toSingleWireData() const;

        template <typename ContainerT>
        void copyToContainer(ContainerT& container) const
        {
            const_cast<WireData*>(this)->resetState();
            container.reserve(container.size()+size());
            auto mContainer=mainContainer();
            if (mContainer && !mContainer->isEmpty())
            {
                container.append(*mContainer);
            }
            while (auto buf=nextBuffer())
            {
                container.append(buf.view());
            }
            const_cast<WireData*>(this)->resetState();
        }

        int append(WireData* other);

        inline void setUseInlineBuffers(bool enable) noexcept
        {
            m_useInlineBuffers=enable;
        }
        inline bool isUseInlineBuffers() const noexcept
        {
            return m_useInlineBuffers;
        }

    protected:

        virtual void doClear(){}

        virtual void setActualMetaVarSize(size_t actualVarSize)
        {
            Assert(false,"Invalid operation");
            std::ignore=actualVarSize;
        }

    private:

        AllocatorFactory* m_factory;

        size_t m_offset;
        size_t m_size;
        bool m_useShareBuffers;
        bool m_useInlineBuffers;
};

//! Packed DataUnit for wire with single buffer
class HATN_DATAUNIT_EXPORT WireDataSingle : public WireData
{
    public:

        explicit WireDataSingle(
            AllocatorFactory* factory=AllocatorFactory::getDefault()
        ) : WireData(factory),
            m_container(factory->dataMemoryResource())
        {}

        WireDataSingle(
                const char* data,
                size_t size,
                bool inlineBuffer,
                AllocatorFactory* factory=AllocatorFactory::getDefault()
        ) : WireData(size,factory),
            m_container(data,size,inlineBuffer)
        {
            setUseInlineBuffers(inlineBuffer);
        }

        explicit WireDataSingle(
                common::ByteArray container,
                AllocatorFactory* factory=AllocatorFactory::getDefault()
        ) noexcept
            : WireData(container.size(),factory),
              m_container(std::move(container))
        {}

        virtual common::ByteArray* mainContainer() const noexcept override {return const_cast<common::ByteArray*>(&m_container);}

        protected:

            virtual void doClear() override
            {
                m_container.clear();
            }

        private:

            common::ByteArray m_container;
            friend class WireBufSolid;
};

//! Packed DataUnit for wire with single shared buffer
class HATN_DATAUNIT_EXPORT WireDataSingleShared : public WireData
{
    public:

        explicit WireDataSingleShared(
            AllocatorFactory* factory=AllocatorFactory::getDefault()
        ) : WireData(factory,true),
            m_container(factory->createObject<common::ByteArrayManaged>(factory->dataMemoryResource()))
        {}

        explicit WireDataSingleShared(
                common::ByteArrayShared container,
                AllocatorFactory* factory=AllocatorFactory::getDefault()
        ) noexcept
            : WireData(container->size(),factory,true),
              m_container(std::move(container))
        {}

        virtual common::ByteArray* mainContainer() const noexcept override
        {
            return sharedMainContainer();
        }

    protected:

        virtual void doClear() override
        {
            m_container->clear();
        }

        common::ByteArray* sharedMainContainer() const
        {
            return const_cast<common::ByteArrayManaged*>(m_container.get());
        }

    private:

        common::ByteArrayShared m_container;
};

//! Packed DataUnit for wire with chained buffers
class HATN_DATAUNIT_EXPORT WireDataChained : public WireDataSingleShared
{
    public:

        explicit WireDataChained(
            AllocatorFactory* factory=AllocatorFactory::getDefault()
        );

        virtual common::SpanBuffer nextBuffer() const noexcept override;
        virtual bool isSingleBuffer() const noexcept override
        {
            return false;
        }
        virtual void resetState() override;
        virtual void appendBuffer(const common::SpanBuffer& buf) override;
        virtual void appendBuffer(common::SpanBuffer&& buf) override;
        virtual void reserveMetaCapacity(size_t capacity) override;

        virtual common::ByteArray* mainContainer() const noexcept override
        {
            return (m_currentMainContainer==nullptr)?sharedMainContainer():m_currentMainContainer;
        }

        /**
         * @brief Append meta variable
         * @param varTypeSize Size of variable type
         * @return ByteArray to put the variable to
         *
         */
        virtual common::ByteArray* appendMetaVar(size_t varTypeSize) override;

        virtual void setCurrentMainContainer(common::ByteArray* currentMainContainer) noexcept override
        {
            m_currentMainContainer=currentMainContainer;
        }

    protected:

        virtual void doClear() override;
        virtual void setActualMetaVarSize(size_t actualVarSize) override;

    private:

        common::ByteArray m_meta;
        mutable common::ByteArray m_tmpInline;
        common::ByteArray* m_currentMainContainer;

        common::SpanBuffers m_bufferChain;
        mutable decltype(m_bufferChain)::iterator m_cursor;
};

HATN_DATAUNIT_NAMESPACE_END

#ifndef HATN_WIREDATA_SRC
#include <hatn/dataunit/wiredatapack.h>
#endif

#endif // HATNDATAUNITWIREDATA_H
