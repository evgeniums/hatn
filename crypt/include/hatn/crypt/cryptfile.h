/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.
    
    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/****************************************************************************/
/** @file common/cryptfile.h
  *
  *      Class for writing and reading encrypted files
  *
  */

/****************************************************************************/

#ifndef HATNCRYPTFILE_H
#define HATNCRYPTFILE_H

#include <hatn/common/utils.h>
#include <hatn/common/plainfile.h>
#include <hatn/common/cachelru.h>

#include <hatn/crypt/crypt.h>
#include <hatn/crypt/cryptcontainer.h>

#include <hatn/crypt/cryptdataunits.h>

HATN_CRYPT_NAMESPACE_BEGIN

//! Class for writing and reading encrypted files
class HATN_CRYPT_EXPORT CryptFile : public common::File
{
    public:

        using common::File::open;
        using common::File::close;
        using common::File::pos;
        using common::File::size;
        using common::File::write;
        using common::File::read;
        using common::File::storageSize;

        constexpr static const size_t MAX_CACHED_CHUNKS=8;
        constexpr static const size_t MAX_PROCESSING_SIZE=0x100000;

        /**
         * @brief Ctor
         * @param factory Allocator factory
         */
        CryptFile(
                common::pmr::AllocatorFactory* factory=common::pmr::AllocatorFactory::getDefault()
            ) : CryptFile(nullptr,nullptr,factory)
        {}

        /**
         * @brief Ctor
         * @param suite Cipher suite
         * @param factory Allocator factory
         */
        CryptFile(
                const CipherSuite *suite,
                common::pmr::AllocatorFactory* factory=common::pmr::AllocatorFactory::getDefault()
            ) : CryptFile(nullptr,suite,factory)
        {}

        /**
         * @brief Ctor
         * @param masterKey Master key used to encrypt/decrypt container
         * @param suite Cipher suite
         * @param factory Allocator factory
         */
        CryptFile(
            const SymmetricKey* masterKey,
            const CipherSuite* suite=nullptr,
            common::pmr::AllocatorFactory* factory=common::pmr::AllocatorFactory::getDefault()
        );

        virtual ~CryptFile();

        CryptFile(const CryptFile&)=delete;
        CryptFile(CryptFile&&) =delete;
        CryptFile& operator=(const CryptFile&)=delete;
        CryptFile& operator=(CryptFile&&) =delete;

        /**
         * @brief Open file
         * @param filename File name
         * @param mode Mode to open with, @see boost::beast::file_mode
         * @return Operation status
         */
        virtual common::Error open(const char* filename, Mode mode) override;

        //! Check if the file is open
        virtual bool isOpen() const noexcept override;

        /**
         * @brief Flush buffers to disk.
         * @param deep On some platforms flush command might be very expensive, so invoke it only in deep mode.
         * @return Operation result.
         *
         * Cache chunks will be encrypted and written to disk, then flush() at backend will be called.
         */
        virtual common::Error flush(bool deep=true) noexcept override;

        /**
         * @brief Close file
         * @throws common::ErrorException on common::Error
         */
        virtual void close() override;

        /**
         * @brief Set cursor position in file
         * @param pos Position
         * @return Operation status
         */
        virtual common::Error seek(uint64_t pos) override;

        /**
         * @brief Get current cursor position in file
         * @throws common::ErrorException on common::Error
         */
        virtual uint64_t pos() const override;

        /**
         * @brief Get size of file content
         * @throws common::ErrorException if operation failed
         */
        virtual uint64_t size() const override;

        virtual uint64_t size(Error& ec) const override;

        /**
         * @brief Get size of file on disk
         * @throws ErrorException if operation failed
         */
        virtual uint64_t storageSize() const override;

        /**
         * @brief Get size of used data
         * @throws ErrorException if operation failed
         *
         * Used size inlcudes header, descriptor and packed cipher text
         */
        uint64_t usedSize() const;

        /**
         * @brief Get size of used data
         * @param ec Error status
         *
         * Used size inlcudes header, descriptor and packed cipher text
         */
        uint64_t usedSize(common::Error& ec) const noexcept
        {
            ec.reset();
            try
            {
                return usedSize();
            }
            catch (const common::ErrorException& e)
            {
                ec=e.error();
            }
            return 0;
        }

        /**
         * @brief Write data to file
         * @param data Data buffer
         * @param size Data size
         * @return Written size
         * @throws common::ErrorException if operation failed
         */
        virtual size_t write(const char* data, size_t size) override;

        size_t writeImpl(const char* data, size_t size, Error& ec);

        /**
         * @brief Read data from file
         * @param data Target data buffer
         * @param maxSize Max size to read
         * @return Read size
         * @throws common::ErrorException if operation failed
         */
        virtual size_t read(char* data, size_t maxSize) override;

        NativeHandleType nativeHandle() override
        {
            return m_file->nativeHandle();
        }

        virtual void setShareMode(ShareMode mode) override
        {
            m_file->setShareMode(mode);
        }

        /**
         * @brief Get cryptographic module
         * @return Cryptographic module
         */
        inline CryptContainer& processor() noexcept
        {
            return m_proc;
        }
        /**
         * @brief Get cryptographic module
         * @return Cryptographic module
         */
        inline const CryptContainer& processor() const noexcept
        {
            return m_proc;
        }

        /**
         * @brief Set maximum number of cached chunks
         * @param val Maximum number of cached chunks
         */
        inline void setMaxCachedChunks(size_t val)
        {
            m_cache.setCapacity(val);
        }
        //! Get maximum number of cached chunks
        inline size_t maxCachedChunks() const noexcept
        {
            return m_cache.capacity();
        }

        /**
         * @brief Set max size of processing step for digest and mac calculations
         * @param size Maximum size of data per processing block
         */
        inline void setMaxProcessingSize(size_t size) noexcept
        {
            m_maxProcessingSize=size;
        }
        /**
         * @brief Get max size of processing step for digest and mac calculations
         * @return size Maximum size of data per processing block
         */
        inline size_t maxProcessingSize() const noexcept
        {
            return m_maxProcessingSize;
        }

        void setStreamingMode(bool enable) noexcept
        {
            m_proc.setStreamingMode(enable);
        }

        bool isStreamingMode() const noexcept
        {
            return m_proc.isStreamingMode();
        }

        //! Calculate digest
        template <typename ContainerT>
        common::Error digest(ContainerT& container,bool reOpen=true)
        {
            std::function <common::Error (common::SharedPtr<Digest>&)>
                    f=[&container](common::SharedPtr<Digest>& digestProc)
            {
                return digestProc->finalize(container);
            };
            return doProcessFile(
                            [this](common::SharedPtr<Digest>& digestProc)
                            {
                                return createDigest(digestProc);
                            },
                            f,
                            reOpen
                        );
        }

        //! Check digest
        template <typename ContainerT>
        common::Error digestCheck(const ContainerT& container,bool reOpen=true)
        {
            std::function <common::Error (common::SharedPtr<Digest>&)>
                    f=[&container](common::SharedPtr<Digest>& digestProc)
            {
                return digestProc->finalizeAndCheck(container);
            };
            return doProcessFile(
                            [this](common::SharedPtr<Digest>& digestProc)
                            {
                                return createDigest(digestProc);
                            },
                            f,
                            reOpen
                        );
        }

        //! Calculate mac
        template <typename ContainerT>
        common::Error macSign(ContainerT& container,bool reOpen=true)
        {
            std::function <common::Error (common::SharedPtr<MAC>&)>
                f=[&container](common::SharedPtr<MAC>& mac)
                {
                    return mac->finalize(container);
                };
            return doProcessFile(
                            [this](common::SharedPtr<MAC>& mac)
                            {
                                return createMac(mac);
                            },
                            f,
                            reOpen
                        );
        }

        //! Verify mac
        template <typename ContainerT>
        common::Error macVerify(const ContainerT& container,bool reOpen=true)
        {
            std::function <common::Error (common::SharedPtr<MAC>&)>
                f=[&container](common::SharedPtr<MAC>& mac)
                {
                    return mac->finalizeAndVerify(container);
                };
            return doProcessFile(
                            [this](common::SharedPtr<MAC>& mac)
                            {
                                return createMac(mac);
                            },
                            f,
                            reOpen
                        );
        }

        //! Add digest to file stamp
        common::Error stampDigest();

        //! Add MAC to file stamp
        common::Error stampMAC();

        //! Verify digest in stamp
        common::Error checkStampDigest();

        //! Verify MAC in stamp
        common::Error verifyStampMAC();

        /**
         * @brief Close file and reset state.
         */
        void reset();

        /**
         * @brief Set underlying file object.
         * @param file Underlying file object.
         */
        void setUnderlying(common::File* file)
        {
            m_file=file;
        }

        /**
         * @brief Invalidate cached data.
         * @return Operation status.
         */
        common::Error invalidateCache();

        /**
         * @brief Truncate file.
         * @param size New size. If new size is greater than current size then noop.
         * @param backupCopy Make a backup copy to restore the file in case of error.
         * @return Operation status.
         *
         * @note File becomes corrupted in case of error. For safe use set a backupCopy flag which is true by default.
         * @note A file stamp is removed if it was present before truncating.
         */
        common::Error truncate(size_t size, bool backupCopy=false) override;

        common::Error truncateImpl(size_t size, bool backupCopy=false, bool testFailuire=false);

        //! Sync buffers to disk
        virtual Error sync() override;

        //! Fsync buffers to disk
        virtual Error fsync() override;

        void setCacheEnabled(bool enable) noexcept
        {
            m_enableCache=enable;
        }

        bool isCacheEnabled() const noexcept
        {
            return m_enableCache;
        }

    private:

        common::Error doOpen(Mode mode, bool headerOnly=false);
        common::Error doSeek(uint64_t pos, size_t overwriteSize=0, bool withCache=true);
        void doClose(bool withThrow=true,bool flush=true);
        void doClose(common::Error& ec,bool flush=true) noexcept;

        common::Error doFlush(bool rawFlush=true, bool deep=true) noexcept;
        common::Error writeSize() noexcept;

        bool useCache() const noexcept;
        bool isNewFile() const noexcept;
        bool isWriteMode() const noexcept;
        bool isRandomWrite() const noexcept;
        bool isAppend() const noexcept;
        bool isScan() const noexcept;

        uint32_t posToSeqnum(uint64_t pos) const noexcept;
        uint64_t chunkOffsetForPos(uint64_t pos) const noexcept;
        uint64_t chunkBeginForPos(uint64_t pos) const noexcept;

        uint64_t seqnumToPos(uint32_t seqnum) const noexcept;
        uint64_t seqnumToRawPos(uint32_t seqnum) const;
        inline uint64_t eofPos() const noexcept
        {
            return m_ciphertextSize+m_dataOffset;
        }

        template <typename FieldT>
        common::Error readStampField(const FieldT& fieldName,file_stamp::type& stamp, common::ByteArray** fieldBuf, common::ByteArray& readBuf);
        common::Error writeStamp(const file_stamp::type& stamp);

        common::Error createDigest(common::SharedPtr<Digest>& digest);
        common::Error createMac(common::SharedPtr<MAC>& mac);

        struct Chunk
        {
            explicit Chunk(common::pmr::AllocatorFactory* factory,uint32_t seqnum=0)
                : seqnum(seqnum),
                  dirty(false),
                  offset(0),
                  maxSize(0),
                  ciphertextSize(0),
                  streamWriteCursor(0),
                  content(factory->dataMemoryResource())
            {}
            uint32_t seqnum;
            bool dirty;
            uint64_t offset;
            uint64_t maxSize;
            size_t ciphertextSize;
            size_t streamWriteCursor;

            common::ByteArray content;

            void reset(uint32_t seq=0) noexcept
            {
                seqnum=seq;
                dirty=false;
                offset=0;
                ciphertextSize=0;
                streamWriteCursor=0;
                content.clear();
            }
        };
        using CacheType=common::CacheLRU<uint32_t,Chunk>;
        using CachedChunk=CacheType::Item;

        common::Error flushChunk(CachedChunk& chunk, bool withSize=false);
        common::Error seekReadRawChunk(CachedChunk& chunk, bool read=false, size_t overwriteSize=0);
        bool isLastChunk(const CachedChunk& chunk) const;
        common::Error flushLastChunk();

        template <typename InitializerHandlerT, typename ProcT>
        common::Error doProcessFile(
            const InitializerHandlerT& initializer,
            const std::function<common::Error (ProcT&)>& postHandler,
            bool checkOpen=true
        );

        CryptContainer m_proc;

        uint64_t m_cursor;
        uint64_t m_seekCursor;
        Mode m_mode;

        CachedChunk m_singleChunk;
        CachedChunk* m_currentChunk;

        CacheType m_cache;

        common::File* m_file;
        common::PlainFile m_plainFile;

        size_t m_dataOffset;
        uint64_t m_size;
        uint64_t m_ciphertextSize;
        common::ByteArray m_readBuffer;
        common::ByteArray m_writeBuffer;
        common::ByteArray m_tmpBuffer;
        bool m_sizeDirty;

        uint32_t m_eofSeqnum;

        size_t m_maxProcessingSize;
        common::SharedPtr<SymmetricKey> m_macKey;
        bool m_enableCache;
};

//---------------------------------------------------------------
template <typename InitializerHandlerT, typename ProcT>
common::Error CryptFile::doProcessFile(
        const InitializerHandlerT& initializer,
        const std::function<common::Error (ProcT&)>& postHandler,
        bool checkOpen
    )
{
    common::RunOnScopeExit macKeyGuard(
        [this]()
        {
            m_macKey.reset();
        }
    );

    if (checkOpen)
    {
        if (isOpen())
        {
            return common::Error(common::CommonError::FILE_ALREADY_OPEN);
        }
        HATN_CHECK_RETURN(doOpen(Mode::scan,true))
    }

    common::RunOnScopeExit closeGuard(
        [this]()
        {
            m_readBuffer.clear();
            common::Error ec;
            close(ec);
        },
        checkOpen
    );

    ProcT proc;
    HATN_CHECK_RETURN(initializer(proc));
    if (proc.isNull())
    {
        return cryptError(CryptError::NOT_SUPPORTED_BY_CIPHER_SUITE);
    }
    HATN_CHECK_RETURN(proc->init())
    HATN_CHECK_RETURN(m_file->seek(0))

    common::Error ec;
    auto size=eofPos();
    while (size!=0)
    {
        auto readSize=size;
        if (readSize>m_maxProcessingSize && m_maxProcessingSize!=0)
        {
            readSize=m_maxProcessingSize;
        }
        m_readBuffer.resize(static_cast<size_t>(readSize));
        auto readBytes=m_file->read(m_readBuffer.data(),static_cast<size_t>(readSize),ec);
        HATN_CHECK_EC(ec)
        if (readBytes!=static_cast<size_t>(readSize))
        {
            return common::Error(common::CommonError::FILE_READ_FAILED);
        }
        HATN_CHECK_RETURN(proc->process(m_readBuffer))
        size-=readBytes;
    }
    return postHandler(proc);
}

class WithCryptfile
{
    public:

        explicit WithCryptfile(
            const SymmetricKey* masterKey,
            const CipherSuite* suite=nullptr,
            common::pmr::AllocatorFactory* factory=common::pmr::AllocatorFactory::getDefault()
            ) : m_cryptfile(masterKey,suite,factory)
        {}

        CryptFile& cryptFile()
        {
            return m_cryptfile;
        }

    protected:

        CryptFile m_cryptfile;
        common::MutexLock m_mutex;
};

//---------------------------------------------------------------
HATN_CRYPT_NAMESPACE_END
#endif // HATNCRYPTFILE_H
