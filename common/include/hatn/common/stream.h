/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.
    
    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)
      
*/

/****************************************************************************/
/** @file common/stream.h
  *
  *     Generic definitions for data streams.
  *
  */

/****************************************************************************/

#ifndef HATNSTREAM_H
#define HATNSTREAM_H

#include <functional>

#include <hatn/common/common.h>
#include <hatn/common/error.h>
#include <hatn/common/objectid.h>
#include <hatn/common/objecttraits.h>
#include <hatn/common/bytearray.h>
#include <hatn/common/pmr/pmrtypes.h>
#include <hatn/common/containerutils.h>
#include <hatn/common/spanbuffer.h>

HATN_COMMON_NAMESPACE_BEGIN

template <typename Traits, typename=void>
struct WithPrepareCloseFlags
{
};
template <typename Traits>
struct WithPrepareCloseFlags<Traits,
                            std::enable_if_t<std::is_move_constructible<Traits>::value>
                            >
{
    WithPrepareCloseFlags()
        : m_closed(false),
          m_destroying(false)
    {}

    ~WithPrepareCloseFlags() = default;
    WithPrepareCloseFlags(const WithPrepareCloseFlags& other) = delete;
    WithPrepareCloseFlags& operator= (const WithPrepareCloseFlags& other) = delete;
    WithPrepareCloseFlags(WithPrepareCloseFlags&& other) noexcept
        : m_closed(std::exchange(other.m_closed, false)),
          m_destroying(std::exchange(other.m_destroying, false))
    {}
    WithPrepareCloseFlags& operator=(WithPrepareCloseFlags&& other) noexcept
    {
        if (&other != this)
        {
            m_closed = std::exchange(other.m_closed, false);
            m_destroying = std::exchange(other.m_destroying, false);
        }
        return *this;
    }

    bool m_closed;
    bool m_destroying;
};
template <typename Traits>
struct WithPrepareCloseFlags<Traits,
        std::enable_if_t<!std::is_move_constructible<Traits>::value>
    >
{
    WithPrepareCloseFlags()
        : m_closed(false),
          m_destroying(false)
    {}

    ~WithPrepareCloseFlags() = default;
    WithPrepareCloseFlags(const WithPrepareCloseFlags& other) = delete;
    WithPrepareCloseFlags& operator= (const WithPrepareCloseFlags& other) = delete;
    WithPrepareCloseFlags(WithPrepareCloseFlags&& other) = delete;
    WithPrepareCloseFlags& operator=(WithPrepareCloseFlags&& other) = delete;

    bool m_closed;
    bool m_destroying;
};

/**
 * @brief Base class for object that can be prepared/opened and closed
 *
 * @note Closing Traits (Traits::close()) must not touch any resources of derived object.
 *
 */
template <typename Traits>
class WithPrepareClose : public WithTraits<Traits>,
                         private WithPrepareCloseFlags<Traits>
{
    public:

        //! Ctor
        template <typename ... Args>
        WithPrepareClose(Args&& ...traitsArgs) noexcept
            : WithTraits<Traits>(std::forward<Args>(traitsArgs)...)
        {}

        //! Dtor is intentionally not virtual
        ~WithPrepareClose()
        {
            this->m_destroying=true;
            //! @note Traits::close() must not touch any resources in derived object!
            close();
        }

        WithPrepareClose(const WithPrepareClose& other) = delete;
        WithPrepareClose& operator= (const WithPrepareClose& other) = delete;
        WithPrepareClose(WithPrepareClose&& other) = default;
        WithPrepareClose& operator=(WithPrepareClose&& other) = default;

        /**
         * @brief Prepare object
         * @param callback Status of stream preparation
         */
        inline void prepare(
            std::function<void (const Error &)> callback
        )
        {
            if (isClosed())
            {
                reset();
            }
            this->traits().prepare(std::move(callback));
        }

        //! Close stream
        inline void close(const std::function<void (const Error &)>& callback=
                                std::function<void (const Error &)>())
        {
            if (this->m_closed)
            {
                if (callback)
                {
                    callback(Error());
                }
                return;
            }
            this->m_closed=true;
            this->traits().close(callback, this->m_destroying);
        }

        inline bool isOpen() const
        {
            return this->traits().isOpen();
        }
        inline bool isClosed() const noexcept
        {
            return this->m_closed;
        }
        inline bool isActive() const noexcept
        {
            return !isClosed() && isOpen();
        }

        inline void reset()
        {
            this->m_closed=false;
            this->traits().reset();
        }
};

//! Base template for asynchronous streams with static polymorphism
template <typename Traits>
class Stream : public WithPrepareClose<Traits>
{
    public:

        template <typename ... Args>
        Stream(Args&& ...traitsArgs) : WithPrepareClose<Traits>(std::forward<Args>(traitsArgs)...)
        {}

        //! Write plain data to stream
        inline void write(
            const char* data,
            std::size_t size,
            std::function<void (const Error&,size_t)> callback
        )
        {
            this->traits().write(data,size,std::move(callback));
        }

        /**
         * @brief Write container to stream
         * @param container Container with data
         * @param callback
         */
        template <typename ContainerT>
        inline void write(
            const ContainerT& container,
            std::function<void (const Error&,size_t)> callback,
            size_t offset=0,
            size_t size=0
        )
        {
            auto span=SpanBuffer::span(container,offset,size);
            if (span.first)
            {
                write(span.second.data(),span.second.size(),std::move(callback));
            }
            else
            {
                callback(commonError(CommonError::INVALID_SIZE),0);
            }
        }

        //! Write managed buffer to stream
        inline void write(
            SpanBuffer buffer,
            std::function<void (const Error&,size_t,SpanBuffer)> callback
        )
        {
            auto span=buffer.span();
            if (!span.first)
            {
                callback(commonError(CommonError::INVALID_SIZE),0,std::move(buffer));
                return;
            }
            auto&& cb=[buffer{std::move(buffer)},callback{std::move(callback)}](const Error& ec,size_t size) mutable
            {
                callback(ec,size,std::move(buffer));
            };
            write(span.second.data(),span.second.size(),std::move(cb));
        }

        //! Write scattered buffers to stream
        inline void write(
            SpanBuffers buffers,
            std::function<void (const Error&,size_t,SpanBuffers)> callback
        )
        {
            this->traits().write(std::move(buffers),std::move(callback));
        }

        //! Read from stream
        inline void read(
            char* data,
            std::size_t maxSize,
            std::function<void (const Error&,size_t)> callback
        )
        {
            this->traits().read(data,maxSize,std::move(callback));
        }

        //! Cancel all pending operations
        inline void cancel()
        {
            this->traits().cancel();
        }
};

/**
 * @brief Extender class for stream traits that do not support scatter/gather bufferring for transfers
 *
 * Implements data gathering for sequential writing to stream.
 */
template <typename Traits>
class StreamGatherTraits : public Traits
{
    public:

        using Traits::Traits;
        using Traits::write;

        //! Write scattered buffers to stream
        inline void write(
            SpanBuffers buffers,
            std::function<void (const Error&,size_t,SpanBuffers)> callback
        )
        {
            writeNext(0,std::move(buffers),std::move(callback),0,0);
        }

        inline void readAll(
            char* data,
            std::size_t expectedSize,
            std::function<void (const Error&,size_t)> callback
            )
        {
            readNext(data,expectedSize,std::move(callback),0);
        }

        void reset()
        {}

    private:

        //! Write next buffer to stream or continue writing current buffer from offset
        void writeNext(
            size_t index,
            SpanBuffers buffers,
            std::function<void (const Error&,size_t,SpanBuffers)> callback,
            size_t currentOffset,
            size_t sentSize
        )
        {
            if (index==buffers.size())
            {
                callback(Error(),sentSize,std::move(buffers));
                return;
            }

            const auto& current=buffers[index];
            auto span=current.span();
            if (!span.first)
            {
                callback(commonError(CommonError::INVALID_SIZE),sentSize,std::move(buffers));
                return;
            }

            this->write(span.second.data()+currentOffset,span.second.size()-currentOffset,
                [this,index,currentOffset,sentSize,
                buffers{std::move(buffers)}, callback{std::move(callback)}]
                (const Error& ec,size_t transferred) mutable
                {
                    if (ec)
                    {
                        callback(ec,sentSize,std::move(buffers));
                        return;
                    }

                    auto nextSentSize=sentSize+transferred;
                    auto nextCurrentOffset=currentOffset+transferred;

                    const auto& current=buffers[index];
                    auto span=current.span();
                    bool sameBuffer=nextCurrentOffset<span.second.size();
                    size_t nextIndex=sameBuffer?index:index+1;
                    if (!sameBuffer)
                    {
                        nextCurrentOffset=0;
                    }

                    writeNext(nextIndex,std::move(buffers),std::move(callback),nextCurrentOffset,nextSentSize);
                }
            );
        }

        void readNext(
                char* data,
                std::size_t expectedSize,
                std::function<void (const Error&,size_t)> callback,
                size_t receivedSize
            )
        {
            if (receivedSize==expectedSize)
            {
                callback(Error{},receivedSize);
                return;
            }

            this->read(
                data+receivedSize,
                expectedSize-receivedSize,
                [this,data,expectedSize,receivedSize,callback{std::move(callback)}](const Error& ec, size_t readBytes)
                {
                    if (ec)
                    {
                        callback(ec,receivedSize+readBytes);
                        return;
                    }
                    readNext(data,std::move(callback),receivedSize+readBytes);
                }
            );
        }
};

//! Stream with thread
template <typename Traits>
class StreamWithThread : public WithThread, public Stream<Traits>
{
    public:

        template <typename ...Args>
        StreamWithThread(
            Thread* thread,
            Args&& ...traitsArgs
            ) : WithThread(thread),
                Stream<Traits>(std::forward<Args>(traitsArgs)...)
        {}
};

//! Stream with thread and ID
template <typename Traits>
class StreamWithIDThread :
        public Stream<Traits>,
        public WithIDThread
{
    public:

        template <typename ...Args>
        StreamWithIDThread(
                Thread* thread,
                const lib::string_view& id,
                Args&& ...traitsArgs
        ) : Stream<Traits>(std::forward<Args>(traitsArgs)...),
            WithIDThread(thread,id)
        {}

        template <typename ...Args>
        StreamWithIDThread(
                Thread* thread,
                Args&& ...traitsArgs
            ) : StreamWithIDThread(thread,lib::string_view{},std::forward<Args>(traitsArgs)...)
        {}
};

//! Proxy class used for streams chaining using write and read handlers
class StreamChain
{
    public:

        using ResultCb=std::function<void (const Error&,size_t)>;
        using OpCb=std::function<void (const Error&)>;

        using WriteFn=std::function<
                            void (
                                const char*,
                                std::size_t,
                                ResultCb
                            )
                        >;
        using ReadFn=std::function<
                            void (
                                char*,
                                std::size_t,
                                ResultCb
                            )
                        >;

        //! Ctor
        StreamChain()=default;

        //! Ctor with initializers
        StreamChain(WriteFn writeFn, ReadFn readFn) noexcept : m_writeFn(std::move(writeFn)),m_readFn(std::move(readFn))
        {}

        //! Set write handler
        inline void setWriteNext(WriteFn writeFn) noexcept
        {
            m_writeFn=std::move(writeFn);
        }
        //! Set read handler
        inline void setReadNext(ReadFn readFn) noexcept
        {
            m_readFn=std::move(readFn);
        }

        //! Reset handlers
        inline void resetNextHandlers() noexcept
        {
            m_writeFn=WriteFn();
            m_readFn=ReadFn();
        }

        //! Call write handler of the next stream
        inline void writeNext(
            const char* data,
            std::size_t size,
            ResultCb callback
        )
        {
            if (m_writeFn)
            {
                m_writeFn(data,size,std::move(callback));
            }
            else
            {
                callback(makeSystemError(std::errc::broken_pipe),0);
            }
        }

        //! Call read handler of the next stream
        inline void readNext(
            char* data,
            std::size_t maxSize,
            ResultCb callback
        )
        {
            if (m_readFn)
            {
                m_readFn(data,maxSize,std::move(callback));
            }
            else
            {
                callback(makeSystemError(std::errc::broken_pipe),0);
            }
        }

    private:

        WriteFn m_writeFn;
        ReadFn m_readFn;
};

//! Base class for asynchronous streams with dynamic polymorphism
class StreamV
{
    public:

        //! Ctor
        StreamV()=default;

        //! Dtor
        virtual ~StreamV()=default;

        StreamV(const StreamV&)=delete;
        StreamV(StreamV&&) =default;
        StreamV& operator=(const StreamV&)=delete;
        StreamV& operator=(StreamV&&) =default;

        //! Write to stream
        virtual void write(
            const char* data,
            std::size_t size,
            std::function<void (const Error&,size_t)> callback
        )
        {
            std::ignore=data;
            std::ignore=size;
            callback(commonError(CommonError::UNSUPPORTED),0);
        }

        //! Write scattered buffers to stream
        virtual void write(
            SpanBuffers buffers,
            std::function<void (const Error&,size_t,SpanBuffers)> callback
        )
        {
            callback(commonError(CommonError::UNSUPPORTED),0,std::move(buffers));
        }

        //! Write buffer to stream
        virtual void write(
            SpanBuffer buffer,
            std::function<void (const Error&,size_t,SpanBuffer)> callback
        )
        {
            callback(commonError(CommonError::UNSUPPORTED),0,std::move(buffer));
        }

        //! Read from stream
        virtual void read(
            char* data,
            std::size_t maxSize,
            std::function<void (const Error&,size_t)> callback
        )
        {
            std::ignore=data;
            std::ignore=maxSize;
            callback(commonError(CommonError::UNSUPPORTED),0);
        }

        /**
         * @brief Prepare object
         * @param callback Status of stream preparation
         */
        virtual void prepare(
            std::function<void (const Error &)> callback
        )
        {
            callback(Error());
        }

        //! Close stream
        virtual void close(const std::function<void (const Error &)>& callback=
                                std::function<void (const Error &)>())
        {
            if (callback)
            {
                callback(Error());
            }
        }

        virtual bool isOpen() const noexcept
        {
            return false;
        }
        virtual bool isClosed() const noexcept
        {
            return false;
        }

        //! Set write handler
        virtual void setWriteNext(StreamChain::WriteFn writeFn) noexcept
        {
            std::ignore=writeFn;
        }
        //! Set read handler
        virtual void setReadNext(StreamChain::ReadFn readFn) noexcept
        {
            std::ignore=readFn;
        }

        inline bool isActive() const noexcept
        {
            return !isClosed() && isOpen();
        }

        virtual lib::string_view id() const noexcept
        {
            return lib::string_view{};
        }
};

namespace detail
{

template <typename ImplStreamT, typename=void> struct StreamTmplVTraits
{
};
template <typename ImplStreamT> struct StreamTmplVTraits<ImplStreamT,
                                                         std::enable_if_t<std::is_base_of<StreamChain,ImplStreamT>::value>>
{
    //! Set write handler
    static void setWriteNext(ImplStreamT& pimpl,StreamChain::WriteFn writeFn) noexcept
    {
        pimpl.setWriteNext(std::move(writeFn));
    }
    //! Set read handler
    static void setReadNext(ImplStreamT& pimpl,StreamChain::ReadFn readFn) noexcept
    {
        pimpl.setReadNext(std::move(readFn));
    }
};
template <typename ImplStreamT> struct StreamTmplVTraits<ImplStreamT,
                                                         std::enable_if_t<!std::is_base_of<StreamChain,ImplStreamT>::value>>
{
    //! Set write handler
    static void setWriteNext(ImplStreamT& pimpl,StreamChain::WriteFn writeFn) noexcept
    {
        std::ignore=pimpl;
        std::ignore=writeFn;
    }
    //! Set read handler
    static void setReadNext(ImplStreamT& pimpl,StreamChain::ReadFn readFn) noexcept
    {
        std::ignore=pimpl;
        std::ignore=readFn;
    }
};

}

//! Base template for asynchronous streams
template <typename ImplStreamT, typename BaseStreamT>
class StreamTmplV : public WithImpl<ImplStreamT>, public BaseStreamT
{
    public:

        using  WithImpl<ImplStreamT>::WithImpl;

        virtual ~StreamTmplV()=default;

        StreamTmplV(const StreamTmplV&)=delete;
        StreamTmplV(StreamTmplV&&) =default;
        StreamTmplV& operator=(const StreamTmplV&)=delete;
        StreamTmplV& operator=(StreamTmplV&&) =default;

        //! Write to stream
        virtual void write(
            const char* data,
            std::size_t size,
            std::function<void (const Error&,size_t)> callback
        ) override
        {
            this->impl().write(data,size,std::move(callback));
        }

        //! Write scattered buffers to stream
        virtual void write(
            SpanBuffers buffers,
            std::function<void (const Error&,size_t,SpanBuffers)> callback
        ) override
        {
            this->impl().write(std::move(buffers),std::move(callback));
        }

        //! Write buffer to stream
        virtual void write(
            SpanBuffer buffer,
            std::function<void (const Error&,size_t,SpanBuffer)> callback
        ) override
        {
            this->impl().write(std::move(buffer),std::move(callback));
        }

        //! Read from stream
        virtual void read(
            char* data,
            std::size_t maxSize,
            std::function<void (const Error&,size_t)> callback
        ) override
        {
            this->impl().read(data,maxSize,std::move(callback));
        }

        /**
         * @brief Prepare object
         * @param callback Status of stream preparation
         */
        virtual void prepare(
            std::function<void (const Error &)> callback
        ) override
        {
            this->impl().prepare(std::move(callback));
        }

        //! Close stream
        virtual void close(const std::function<void (const Error &)>& callback=
                                std::function<void (const Error &)>()) override
        {
            this->impl().close(callback);
        }

        virtual bool isOpen() const noexcept override
        {
            return this->impl().isOpen();
        }

        virtual bool isClosed() const noexcept override
        {
            return this->impl().isClosed();
        }

        //! Set write handler
        virtual void setWriteNext(StreamChain::WriteFn writeFn) noexcept override
        {
            detail::StreamTmplVTraits<ImplStreamT>::setWriteNext(this->impl(),std::move(writeFn));
        }
        //! Set read handler
        virtual void setReadNext(StreamChain::ReadFn readFn) noexcept override
        {
            detail::StreamTmplVTraits<ImplStreamT>::setReadNext(this->impl(),std::move(readFn));
        }
};

//! Bridge for cross-connecting two stream chains
class StreamBridge
{
    private:

        struct WriteDescr
        {
            const char* data=nullptr;
            std::size_t size=0;
            std::size_t promotedSize=0;
            StreamChain::ResultCb callback;

            inline void reset() noexcept
            {
                data=nullptr;
                size=0;
                promotedSize=0;
                callback=StreamChain::ResultCb();
            }
        };
        struct ReadDescr
        {
            char* data=nullptr;
            std::size_t maxSize=0;
            StreamChain::ResultCb callback;

            inline void reset() noexcept
            {
                data=nullptr;
                maxSize=0;
                callback=StreamChain::ResultCb();
            }
        };

        WriteDescr m_writeLeftToRight;
        ReadDescr m_readLeftToRight;

        WriteDescr m_writeRightToLeft;
        ReadDescr m_readRightToLeft;

        bool m_async;

    public:

        StreamBridge() noexcept : m_async(false)
        {}

        inline void setPromoteAsync(bool enable) noexcept
        {
            m_async=enable;
        }
        inline bool isPromoteAsync() const noexcept
        {
            return m_async;
        }

        inline void writeLeftToRight(
            const char* data,
            std::size_t size,
            StreamChain::ResultCb callback
        )
        {
            HATN_DEBUG_LVL(global,5,HATN_FORMAT("writeLeftToRight {}",size));

            Assert(m_writeLeftToRight.size==0,"Write left to right not ready");
            m_writeLeftToRight.data=data;
            m_writeLeftToRight.size=size;
            m_writeLeftToRight.callback=callback;
            promoteLeftToRight();
        }

        inline void writeRightToLeft(
            const char* data,
            std::size_t size,
            StreamChain::ResultCb callback
        )
        {
            HATN_DEBUG_LVL(global,5,HATN_FORMAT("writeRightToLeft {}",size));

            Assert(m_writeRightToLeft.size==0,"Write right to left not ready");
            m_writeRightToLeft.data=data;
            m_writeRightToLeft.size=size;
            m_writeRightToLeft.callback=callback;
            promoteRightToLeft();
        }

        inline void readLeftToRight(
            char* data,
            std::size_t maxSize,
            StreamChain::ResultCb callback
        )
        {
            HATN_DEBUG_LVL(global,5,HATN_FORMAT("readLeftToRight {}",maxSize));

            Assert(m_readLeftToRight.maxSize==0,"Read left to right not ready");
            m_readLeftToRight.data=data;
            m_readLeftToRight.maxSize=maxSize;
            m_readLeftToRight.callback=callback;
            promoteLeftToRight();
        }

        inline void readRightToLeft(
            char* data,
            std::size_t maxSize,
            StreamChain::ResultCb callback
        )
        {
            HATN_DEBUG_LVL(global,5,HATN_FORMAT("readRightToLeft {}",maxSize));

            Assert(m_readRightToLeft.maxSize==0,"Read right to left not ready");
            m_readRightToLeft.data=data;
            m_readRightToLeft.maxSize=maxSize;
            m_readRightToLeft.callback=callback;
            promoteRightToLeft();
        }

        inline void reset()
        {
            m_writeLeftToRight.reset();
            m_writeRightToLeft.reset();
            m_readLeftToRight.reset();
            m_readRightToLeft.reset();
        }

    private:

        inline void promote(WriteDescr& writeDescr,ReadDescr& readDescr)
        {
            if (writeDescr.size>writeDescr.promotedSize)
            {
                auto notProcesedSize=writeDescr.size-writeDescr.promotedSize;
                auto size=(std::min)(notProcesedSize,readDescr.maxSize);
                if (size!=0)
                {
                    std::copy(writeDescr.data+writeDescr.promotedSize,
                              writeDescr.data+writeDescr.promotedSize+size,
                              readDescr.data);
                    auto readCb=std::move(readDescr.callback);
                    readDescr.reset();
                    writeDescr.promotedSize+=size;
                    if (writeDescr.size==writeDescr.promotedSize)
                    {
                        auto writeCb=std::move(writeDescr.callback);
                        auto writeCbSize=writeDescr.size;
                        writeDescr.reset();
                        writeCb(Error(),writeCbSize);
                    }
                    readCb(Error(),size);
                }
            }
        }

        inline void promoteRightToLeft()
        {
            if (m_async)
            {
                Thread::currentThread()->execAsync([this](){promote(m_writeRightToLeft,m_readRightToLeft);});
            }
            else
            {
                promote(m_writeRightToLeft,m_readRightToLeft);
            }
        }
        inline void promoteLeftToRight()
        {
            if (m_async)
            {
                Thread::currentThread()->execAsync([this](){promote(m_writeLeftToRight,m_readLeftToRight);});
            }
            else
            {
                promote(m_writeLeftToRight,m_readLeftToRight);
            }
        }
};

//---------------------------------------------------------------
HATN_COMMON_NAMESPACE_END

#endif // HATNSTREAM_H
