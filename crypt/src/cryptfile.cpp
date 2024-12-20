/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.
    
    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/****************************************************************************/
/** @file common/plainfile.cpp
 *
 *     Class for writing and reading plain files
 *
 */

#include <boost/endian/conversion.hpp>

#include <hatn/common/runonscopeexit.h>
#include <hatn/common/filesystem.h>
#include <hatn/common/format.h>

#include <hatn/dataunit/objectid.h>
#include <hatn/dataunit/visitors.h>

#include <hatn/crypt/cryptfile.h>

HATN_CRYPT_NAMESPACE_BEGIN
HATN_COMMON_USING

/********************** CryptFile **************************/

//---------------------------------------------------------------
CryptFile::CryptFile(
        const SymmetricKey *masterKey,
        const CipherSuite *suite,
        pmr::AllocatorFactory *factory
    ) : m_proc(masterKey,suite,factory),
        m_cursor(0),
        m_seekCursor(0),
        m_mode(Mode::scan),
        m_singleChunk(factory),
        m_currentChunk(nullptr),
        m_cache(MAX_CACHED_CHUNKS,factory),
        m_file(&m_plainFile),
        m_dataOffset(0),
        m_size(0),
        m_ciphertextSize(0),
        m_readBuffer(factory->dataMemoryResource()),
        m_writeBuffer(factory->dataMemoryResource()),
        m_tmpBuffer(factory->dataMemoryResource()),
        m_sizeDirty(false),
        m_eofSeqnum(0),
        m_maxProcessingSize(MAX_PROCESSING_SIZE),
        m_enableCache(true)
{
}

//---------------------------------------------------------------
CryptFile::~CryptFile()
{
    Error ec;
    doClose(ec);
}

//---------------------------------------------------------------
bool CryptFile::useCache() const noexcept
{
    return m_enableCache && !(m_mode==Mode::scan || m_mode==Mode::append || m_mode==Mode::append_existing);
}

//---------------------------------------------------------------
bool CryptFile::isNewFile() const noexcept
{
    return m_mode==Mode::write_new || m_mode==Mode::write || (m_mode==Mode::append && !lib::filesystem::exists(filename()));
}

//---------------------------------------------------------------
bool CryptFile::isWriteMode() const noexcept
{
    return !(m_mode==Mode::read || m_mode==Mode::scan);
}

//---------------------------------------------------------------
bool CryptFile::isRandomWrite() const noexcept
{
    return m_mode==Mode::write || m_mode==Mode::write_new || m_mode==Mode::write_existing;
}

//---------------------------------------------------------------
bool CryptFile::isAppend() const noexcept
{
    return m_mode==Mode::append || m_mode==Mode::append_existing;
}

//---------------------------------------------------------------
bool CryptFile::isScan() const noexcept
{
    return m_mode==Mode::scan;
}

//---------------------------------------------------------------
Error CryptFile::open(const char *fname, Mode mode)
{
    setFilename(fname);

    if (false)
    {
        std::cout << "CryptFile::open " << filename() << " mode="<< int(mode) << " cached="<<m_enableCache
                  << " maxChunkSize=" << processor().maxPlainChunkSize(1) <<
            " maxFirstChunkSize=" << processor().maxPlainChunkSize(0) <<
            " maxPackedChunkSize=" << processor().maxPackedChunkSize(1,200) <<
            " maxFirstPackedChunkSize=" << processor().maxPackedChunkSize(0,100) <<
            " kdfType=" << int(processor().kdfType()) <<
            std::endl;
    }

    return doOpen(mode);
}

//---------------------------------------------------------------
Error CryptFile::doOpen(Mode mode, bool headerOnly)
{
    try
    {
        // to emulate appending modes use write or write_existing mode because
        // true appending mode can not be used as the size must be written to the file's header
        // to ensure appending mode check condition (position==size) in seek() method
        m_mode=mode;
        auto openMode=mode;
        bool newFile=isNewFile();
        if (openMode==Mode::append)
        {
            openMode=newFile ? Mode::write : Mode::write_existing;
        }
        else if (openMode==Mode::append_existing)
        {
            openMode=Mode::write_existing;
        }        

        // open raw file
        HATN_CHECK_RETURN(m_file->open(filename(),openMode))

        // process header and descriptor
        if (!newFile)
        {
            // existing file

            // read and unpack header
            m_readBuffer.resize(m_proc.headerSize());
            auto readSize=m_file->read(m_readBuffer.data(),m_readBuffer.size());
            if (readSize!=m_proc.headerSize())
            {
                throw ErrorException(cryptError(CryptError::INVALID_CRYPTFILE_FORMAT));
            }
            uint16_t descriptorSize=0;
            HATN_CHECK_THROW(m_proc.unpackHeader(m_readBuffer,m_size,descriptorSize,m_ciphertextSize))

            // read and unpack descriptor
            m_readBuffer.clear();
            m_readBuffer.resize(descriptorSize);
            readSize=m_file->read(m_readBuffer.data(),m_readBuffer.size());
            if (readSize!=descriptorSize)
            {
                throw ErrorException(cryptError(CryptError::INVALID_CRYPTFILE_FORMAT));
            }
            HATN_CHECK_THROW(m_proc.unpackDescriptor(m_readBuffer))

            // set data offset to position after descriptor
            m_dataOffset=static_cast<size_t>(m_file->pos());
            auto actualDataSize=m_file->size()-m_dataOffset;
            if (actualDataSize<m_ciphertextSize)
            {
                throw ErrorException(cryptError(CryptError::INVALID_CRYPTFILE_FORMAT));
            }
            m_eofSeqnum=posToSeqnum(m_size);

            // seek
            if (!headerOnly)
            {
                if (mode==Mode::append_existing || mode==Mode::append)
                {
                    HATN_CHECK_THROW(doSeek(m_size))
                }
                else
                {
                    HATN_CHECK_THROW(doSeek(0))
                }
            }
        }
        else
        {
            // new/overwritten file

            if (headerOnly)
            {
                throw ErrorException(Error(CommonError::FILE_NOT_OPEN));
            }

            // pack and write header and descriptor
            m_writeBuffer.clear();
            HATN_CHECK_THROW(m_proc.packHeaderAndDescriptor(m_writeBuffer,0))
            auto written=m_file->write(m_writeBuffer.data(),m_writeBuffer.size());
            if (written!=m_writeBuffer.size())
            {
                throw ErrorException(Error(CommonError::FILE_WRITE_FAILED));
            }

            // set data offset to position after descriptor
            m_dataOffset=m_writeBuffer.size();
            m_eofSeqnum=0;

            // seek 0
            HATN_CHECK_THROW(doSeek(0))

            // write first empty chunk
            m_currentChunk->dirty=true;
            m_sizeDirty=true;
            HATN_CHECK_THROW(flushChunk(*m_currentChunk,true))
        }
    }
    catch (const ErrorException& e)
    {
        doClose(false,false);
        return e.error();
    }
    m_readBuffer.clear();
    m_writeBuffer.clear();
    return Error();
}

//---------------------------------------------------------------
bool CryptFile::isOpen() const noexcept
{
    return m_file->isOpen();
}

//---------------------------------------------------------------
// codechecker_false_positive [bugprone-exception-escape]
Error CryptFile::flush() noexcept
{
#if 0
    std::cout << "CryptFile::flush " << filename() << std::endl;
#endif
    return doFlush();
}

//---------------------------------------------------------------
void CryptFile::close()
{
    doClose();
}

//---------------------------------------------------------------
Error CryptFile::writeSize() noexcept
{
    if (m_sizeDirty)
    {
        try
        {
#if 0
            std::cout << "CryptFile::writeSize() " << m_size << std::endl;
#endif
            auto writeVal=[this](size_t offset, uint64_t val)
            {
                HATN_CHECK_RETURN(m_file->seek(offset))
                CryptContainerHeader::contentSizeForStorageInplace(val);
                auto written=m_file->write(reinterpret_cast<char*>(&val),sizeof(val));
                if (written!=sizeof(val))
                {
                    return Error(CommonError::FILE_WRITE_FAILED);
                }
                return Error();
            };

            // write plaintext size
            HATN_CHECK_RETURN(writeVal(CryptContainerHeader::plaintextSizeOffset(),m_size))

            // write ciphertext size
            HATN_CHECK_RETURN(writeVal(CryptContainerHeader::ciphertextSizeOffset(),m_ciphertextSize))

            // clear dirty flag
            m_sizeDirty=false;
        }
        catch (const ErrorException& e)
        {
            return e.error();
        }
    }
    return Error();
}

//---------------------------------------------------------------
// codechecker_false_positive [bugprone-exception-escape]
Error CryptFile::doFlush(bool rawFlush) noexcept
{
    // nothing to do for not open file or in read mode
    if (!m_file->isOpen() || !isWriteMode())
    {
        return Error();
    }

    try
    {
        // write chunks
        auto&& handler=[this](CachedChunk& chunk)
        {
            if (chunk.dirty)
            {
#if 0
                std::cout << "CryptFile::doFlush() cache chunk" << std::endl;
#endif
                HATN_CHECK_THROW(flushChunk(chunk))
            }
            return true;
        };
        if (m_cache.each(handler)==0 && m_currentChunk!=nullptr)
        {
#if 0
            std::cout << "CryptFile::doFlush() current chunk" << std::endl;
#endif
            handler(*m_currentChunk);
        }
    }
    catch (const ErrorException& e)
    {
        return e.error();
    }

    // write size
    HATN_CHECK_RETURN(writeSize())

    // flush raw file
    if (rawFlush)
    {
        return m_file->flush();
    }

    // done
    return Error();
}

//---------------------------------------------------------------
Error CryptFile::flushLastChunk()
{
    if (m_currentChunk
            &&
        m_currentChunk->dirty
            &&
        isLastChunk(*m_currentChunk)
        )
    {
        return flushChunk(*m_currentChunk);
    }
    return Error();
}

//---------------------------------------------------------------
Error CryptFile::flushChunk(CachedChunk &chunk, bool withSize)
{
    try
    {
        // enctypt chunk
        m_writeBuffer.clear();
        HATN_CHECK_RETURN(m_proc.packChunk(common::SpanBuffer(chunk.content),m_writeBuffer,chunk.seqnum))
        if (!m_writeBuffer.isEmpty())
        {
            // seek to chunk beginning offset
            HATN_CHECK_RETURN(seekReadRawChunk(chunk))

            // write encrypted data
            auto size=m_writeBuffer.size();
            auto written=m_file->write(m_writeBuffer.data(),size);
            m_writeBuffer.clear();
            if (written!=size)
            {
                //! @todo Write by parts in cycle
                return Error(CommonError::FILE_WRITE_FAILED);
            }

            m_ciphertextSize+=size-chunk.ciphertextSize;
            chunk.ciphertextSize=size;
        }
        // unset dirty flag
        chunk.dirty=false;
        if (withSize)
        {
            // update size in file
            return writeSize();
        }
    }
    catch (const ErrorException& e)
    {
        m_writeBuffer.clear();
        return e.error();
    }
    return Error();
}

//---------------------------------------------------------------
void CryptFile::doClose(bool withThrow, bool flush)
{
    Error ec;
    doClose(ec,flush);

    // throw exception on error
    if (ec && withThrow)
    {
        throw ErrorException(ec);
    }
}

//---------------------------------------------------------------
void CryptFile::doClose(Error &ec, bool flush) noexcept
{
    if (m_file->isOpen())
    {
        // flush chunks
        if (flush)
        {
            ec=doFlush();
        }

        // close raw file
        if (!ec)
        {
            m_file->close(ec);
        }
        else
        {
            Error ec1;
            m_file->close(ec1);
        }
    }

    // reset state
    m_cache.clear();
    m_currentChunk=nullptr;
    m_singleChunk.reset();
    m_size=0;
    m_dataOffset=0;
    m_mode=Mode::scan;
    m_readBuffer.clear();
    m_writeBuffer.clear();
    m_cursor=0;
    m_ciphertextSize=0;
    m_eofSeqnum=0;
    m_seekCursor=0;
    m_proc.reset(false);
}

//---------------------------------------------------------------
void CryptFile::reset()
{
    Error ec;
    close(ec);
    m_proc.hardReset(true);
}

//---------------------------------------------------------------
Error CryptFile::seek(uint64_t pos)
{
    if (pos==m_cursor)
    {
        return OK;
    }
    m_seekCursor=pos;
    if (pos>m_size)
    {
        return OK;
    }
    return doSeek(pos);
}

//---------------------------------------------------------------
Error CryptFile::doSeek(uint64_t pos, size_t overwriteSize, bool withCache)
{
    try
    {
        // check state and argument
        if (!isOpen())
        {
            return Error(CommonError::FILE_NOT_OPEN);
        }
        if (isAppend()?(pos!=m_size):(pos>m_size))
        {
            return makeSystemError(std::errc::invalid_seek);
        }

        // skip if hit to current chunk
        auto seqnum=posToSeqnum(pos);
        if (m_currentChunk!=nullptr && m_currentChunk->seqnum==seqnum)
        {
            // update positions
            m_cursor=pos;
            m_currentChunk->offset=chunkOffsetForPos(pos);
            return Error();
        }

        // flush last chunk
        HATN_CHECK_RETURN(flushLastChunk());

        // setup chunk
        m_currentChunk=nullptr;
        CachedChunk* nextChunk=nullptr;
        bool readChunk=false;
        if (withCache && useCache())
        {
            if (m_cache.hasItem(seqnum))
            {
                // chunk is in the cache
                nextChunk=&m_cache.item(seqnum);
                // revalidate the chunk as MRU
                m_cache.touchItem(*nextChunk);
            }
            else
            {
                // if cache is full then flush chunk displaced from the cache
                if (m_cache.isFull())
                {
                    auto& displacedChunk=m_cache.mruItem();
                    if (displacedChunk.dirty)
                    {
                        HATN_CHECK_RETURN(flushChunk(displacedChunk))
                    }
                }

                // add new chunk to cache
                nextChunk=&m_cache.emplaceItem(seqnum,m_proc.factory(),seqnum);
                readChunk=true;
            }
        }
        else
        {
            // reset single chunk
            m_singleChunk.reset(seqnum);
            readChunk=true;
            nextChunk=&m_singleChunk;
        }
        nextChunk->maxSize=m_proc.maxPlainChunkSize(seqnum);
        nextChunk->offset=chunkOffsetForPos(pos);

        // read chunk from file
        if (readChunk)
        {
            // check if chunk is going to be totally overwritten
            auto chunkTotalOverwriteSize=(nextChunk->offset==0&&overwriteSize>=nextChunk->maxSize)
                    ? nextChunk->maxSize : 0;
            HATN_CHECK_RETURN(seekReadRawChunk(*nextChunk,true,static_cast<size_t>(chunkTotalOverwriteSize)))
        }

        // update state
        m_cursor=pos;
        m_currentChunk=nextChunk;
        m_eofSeqnum=(std::max)(m_eofSeqnum,seqnum);
    }
    catch (const ErrorException& e)
    {
        return e.error();
    }

    return Error();
}

//---------------------------------------------------------------
Error CryptFile::seekReadRawChunk(CachedChunk &chunk, bool read, size_t overwriteSize)
{
    // check file size
    if (m_file->size()<eofPos())
    {
        return Error(CommonError::INVALID_SIZE);
    }

    // check beginning position of the chunk
    auto chunkRawPos=seqnumToRawPos(chunk.seqnum);
    if (chunkRawPos>eofPos())
    {
        // chunk is beyond EOF
        return makeSystemError(std::errc::invalid_seek);
    }

    // seek in raw file
    HATN_CHECK_RETURN(m_file->seek(chunkRawPos));

    // read chunk if needed
    if (read)
    {
        // check chunk/file size
        auto chunkSize=m_proc.maxPackedChunkSize(chunk.seqnum,static_cast<uint32_t>(m_ciphertextSize));
        if ((chunkRawPos+chunkSize)>eofPos())
        {
            chunkSize=static_cast<uint32_t>(eofPos()-chunkRawPos);
        }

        if (chunkSize!=0)
        {
            if (overwriteSize==0)
            {
                // read chunk

                // read data from file
                m_readBuffer.clear();
                m_readBuffer.resize(chunkSize);
                Error ec;
                auto readSize=m_file->read(m_readBuffer.data(),chunkSize,ec);
                HATN_CHECK_EC(ec);
                if (readSize!=chunkSize)
                {
                    //! @todo Read by parts in cycle
                    return Error(CommonError::FILE_READ_FAILED);
                }

                // decrypt data
                HATN_CHECK_RETURN(m_proc.unpackChunk(SpanBuffer(m_readBuffer),chunk.content,chunk.seqnum));

                // clear buffer
                m_readBuffer.clear();
            }
            else
            {
                // no need to read and decrypt chunk that will be totally overwritten
                chunk.content.clear();
            }
            chunk.ciphertextSize=chunkSize;
        }
    }
    return Error();
}

//---------------------------------------------------------------
uint64_t CryptFile::seqnumToPos(uint32_t seqnum) const noexcept
{
    if (seqnum==0)
    {
        return 0;
    }
    return m_proc.maxPlainChunkSize(0)+(seqnum-1)*m_proc.maxPlainChunkSize(seqnum);
}

//---------------------------------------------------------------
uint64_t CryptFile::seqnumToRawPos(uint32_t seqnum) const
{
    if (seqnum==0)
    {
        return m_dataOffset;
    }
    auto pos=m_proc.maxPackedChunkSize(0)+(seqnum-1)*m_proc.maxPackedChunkSize(seqnum)+m_dataOffset;
    return pos;
}

//---------------------------------------------------------------
uint32_t CryptFile::posToSeqnum(uint64_t pos) const noexcept
{
    auto firstChunkSize=m_proc.firstChunkMaxSize();
    auto chunkSize=m_proc.chunkMaxSize();
    if (firstChunkSize==0)
    {
        firstChunkSize=chunkSize;
    }

    if (pos<firstChunkSize || firstChunkSize==0)
    {
        return 0u;
    }
    if (pos==firstChunkSize || chunkSize==0)
    {
        return 1u;
    }

    pos=pos-firstChunkSize;
    uint32_t val=1u+static_cast<uint32_t>(pos/chunkSize);
    return val;
}

//---------------------------------------------------------------
uint64_t CryptFile::chunkBeginForPos(uint64_t pos) const noexcept
{
    auto seqnum=posToSeqnum(pos);
    if (seqnum==0)
    {
        return 0;
    }
    return m_proc.maxPlainChunkSize(0)+(seqnum-1)*m_proc.maxPlainChunkSize(1);
}

//---------------------------------------------------------------
uint64_t CryptFile::chunkOffsetForPos(uint64_t pos) const noexcept
{
    return pos-chunkBeginForPos(pos);
}

//---------------------------------------------------------------
uint64_t CryptFile::pos() const
{
    return m_seekCursor;
}

//---------------------------------------------------------------
uint64_t CryptFile::size(Error& ec) const
{
    if (!isOpen())
    {
        // open file and read content size from header
        ec=const_cast<CryptFile*>(this)->doOpen(Mode::scan,true);
        if (ec)
        {
            return 0;
        }
        uint64_t sz=m_size;
        const_cast<CryptFile*>(this)->doClose(false,false);
        return sz;
    }
    return m_size;
}

//---------------------------------------------------------------
uint64_t CryptFile::size() const
{
    if (!isOpen())
    {
        // open file and read content size from header
        HATN_CHECK_THROW(const_cast<CryptFile*>(this)->doOpen(Mode::scan,true))
        uint64_t sz=m_size;
        const_cast<CryptFile*>(this)->doClose(false,false);
        return sz;
    }
    return m_size;
}

//---------------------------------------------------------------
uint64_t CryptFile::usedSize() const
{
    auto self=const_cast<CryptFile*>(this);
    if (!isOpen())
    {
        // open file and read content size from header
        HATN_CHECK_THROW(self->doOpen(Mode::scan,true))
        uint64_t sz=eofPos();
        self->doClose(false,false);
        return sz;
    }
    HATN_CHECK_THROW(self->flushLastChunk());
    return eofPos();
}

//---------------------------------------------------------------
uint64_t CryptFile::storageSize() const
{
    m_file->setFilename(filename());
    return m_file->storageSize();
}

//---------------------------------------------------------------
bool CryptFile::isLastChunk(const CachedChunk &chunk) const
{
    return chunk.seqnum==m_eofSeqnum;
}

//---------------------------------------------------------------
size_t CryptFile::write(const char *data, size_t size)
{
#if 0
    std::cout << "CryptFile::write << file=" << filename() << " cursor="<<m_cursor<<" data=\"" <<std::string(data,size) << "\", size="
              << size << std::endl;
#endif
    // check state and arguments
    if (size==0)
    {
        return size;
    }
    if (m_currentChunk==nullptr)
    {
        throw ErrorException(Error(CommonError::FILE_NOT_OPEN));
    }
    if (!isWriteMode())
    {
        throw ErrorException(Error(CommonError::FILE_WRITE_FAILED));
    }

    // prepend zeros to previously truncated position
    if (m_seekCursor>m_size)
    {
        auto newSize=m_seekCursor;
        HATN_CHECK_RETURN(truncateImpl(newSize))
        HATN_CHECK_RETURN(doSeek(newSize))
    }
    Error ec;
    auto ret=writeImpl(data,size,ec);
    if (ec)
    {
        throw ErrorException(ec);
    }
    m_seekCursor+=ret;
    return ret;
}

//---------------------------------------------------------------
size_t CryptFile::writeImpl(const char *data, size_t size, Error& ec)
{
    // write data to chunk(s)
    size_t doneSize=0;
    while (size>doneSize)
    {
        // find space available in the current chunk
        size_t writeSize=size-doneSize;
        if (m_currentChunk->maxSize!=0)
        {
            auto availableChunkSpace=m_currentChunk->maxSize-m_currentChunk->offset;
            if (availableChunkSpace==0)
            {
                // if no space left then seek to the next chunk
                ec=doSeek(seqnumToPos(m_currentChunk->seqnum+1),writeSize);
                if (ec)
                {
                    return 0;
                }
                availableChunkSpace=m_currentChunk->maxSize-m_currentChunk->offset;
                Assert(availableChunkSpace!=0,"Invalid cursor after seeking next chunk");
            }
            if (writeSize>availableChunkSpace)
            {
                writeSize=availableChunkSpace;
            }
        }

        // write data to current chunk taking into account chunk offset and size
        auto oldChunkSize=m_currentChunk->content.size();
        auto newChunkSize=static_cast<size_t>(m_currentChunk->offset)+writeSize;
        if (m_currentChunk->content.size()<newChunkSize)
        {
            m_currentChunk->content.resize(newChunkSize);
        }
        std::copy(data+doneSize,data+doneSize+writeSize,m_currentChunk->content.data()+m_currentChunk->offset);

        // update size if it is the last chunk
        if (isLastChunk(*m_currentChunk) && (newChunkSize>oldChunkSize))
        {
            m_size+=newChunkSize-oldChunkSize;
            m_sizeDirty=true;
        }

        // update positions
        doneSize+=writeSize;
        m_currentChunk->offset+=writeSize;
        m_cursor+=writeSize;

        // mark chunk as dirty
        m_currentChunk->dirty=true;
    }
    Assert(m_cursor<=m_size,"Invalid cursor or size");

    // done
    return doneSize;
}

//---------------------------------------------------------------
size_t CryptFile::read(char *data, size_t maxSize)
{
    // check state and arguments
    if (maxSize==0)
    {
        return 0;
    }
    if (m_currentChunk==nullptr)
    {
        throw ErrorException(Error(CommonError::FILE_NOT_OPEN));
    }
    if (isAppend())
    {
        throw ErrorException(Error(CommonError::FILE_READ_FAILED));
    }

    // return noop if seek is beyond eof
    if (m_seekCursor>=m_size)
    {
        return 0;
    }

    // read data from chunk(s)
    size_t doneSize=0;
    while (doneSize<maxSize)
    {
        // check if EOF is reached
        if (m_cursor==m_size)
        {
            break;
        }

        // find space available in the current chunk
        size_t readSize=maxSize-doneSize;
        auto availableSpace=m_currentChunk->content.size()-m_currentChunk->offset;
        if (availableSpace==0)
        {
            // if no space left then seek to the next chunk
            HATN_CHECK_THROW(doSeek(m_cursor));
            availableSpace=m_currentChunk->content.size()-m_currentChunk->offset;
            if (availableSpace==0)
            {
                break;
            }
        }

        // read data from current chunk taking into account chunk offset and size
        if (readSize>availableSpace)
        {
            readSize=static_cast<size_t>(availableSpace);
        }
        auto srcPtr=m_currentChunk->content.data()+m_currentChunk->offset;
        std::copy(srcPtr,srcPtr+readSize,data+doneSize);

        // update positions
        doneSize+=readSize;
        m_currentChunk->offset+=readSize;
        m_cursor+=readSize;
    }
#if 0
    std::cout << "CryptFile::read << file=" << filename() << " cursor="<<m_cursor<<" data=\"" <<std::string(data,doneSize) << "\", size="
              << doneSize << std::endl;
#endif
    // done
    m_seekCursor+=doneSize;
    return doneSize;
}

//---------------------------------------------------------------

common::Error CryptFile::invalidateCache()
{
    if (isWriteMode())
    {
        HATN_CHECK_RETURN(doFlush(false))
    }
    m_cache.clear();
    if (!isAppend())
    {
        m_currentChunk=nullptr;
        return doSeek(m_cursor);
    }
    return OK;
}

//---------------------------------------------------------------

common::Error CryptFile::truncate(size_t size, bool backupCopy)
{
    return truncateImpl(size,backupCopy);
}

common::Error CryptFile::truncateImpl(size_t size, bool backupCopy, bool testFailuire)
{
    if (!isOpen())
    {
        HATN_CHECK_RETURN(doOpen(Mode::write_existing))
        HATN_CHECK_RETURN(truncateImpl(size,backupCopy))
        Error ec;
        doClose(ec,true);
        return ec;
    }

    // check if size is already ok
    if (size==this->size())
    {
        return OK;
    }

    // if new size greater than current size then append zeros
    size_t prevPos=m_cursor;
    size_t prevSeekPos=m_seekCursor;
    if (size>m_size)
    {
        size_t appendSize=size-m_size;
        size_t doneSize=0;
        auto writeSize=std::min(m_maxProcessingSize,appendSize-doneSize);
        m_tmpBuffer.resize(writeSize);
        m_tmpBuffer.fill(static_cast<char>(0));
        HATN_CHECK_RETURN(doSeek(m_size))
        Error ec;
        while (doneSize<appendSize)
        {
            auto written=writeImpl(m_tmpBuffer.data(),writeSize,ec);
            HATN_CHECK_EC(ec)
            doneSize+=written;
            writeSize=std::min(writeSize,appendSize-doneSize);
        }
        m_tmpBuffer.clear();
        return doSeek(prevPos);
    }

    // prepare positions
    size_t newPos=m_cursor;
    size_t newLastPos=size;
    if (newPos>newLastPos)
    {
        newPos=newLastPos;
    }
    if (testFailuire)
    {
        newLastPos=m_size+1;
    }
    bool updated=testFailuire;

    // make guard
    Error ec;
    std::string backupCopyName;
    auto onExit=HATN_COMMON_NAMESPACE::makeScopeGuard(
        [this,&ec,&backupCopyName,prevPos,&updated,prevSeekPos]()
        {
            if (updated && !backupCopyName.empty())
            {                
                m_tmpBuffer.clear();
                lib::fs_error_code fec;
                if (ec)
                {
                    Error ec1;
                    doClose(ec1,false);
                    if (ec1)
                    {
                        std::cerr << "CryptFile::truncate: failed to close " << filename() << " before restoring from backup " << backupCopyName
                                  << " after error code=" << ec.code() <<" message=\"" << ec.message() << "\"" << std::endl;
                        throw ErrorException{ec1};
                    }
                    lib::filesystem::remove(filename(),fec);
                    if (fec)
                    {
                        std::cerr << "CryptFile::truncate: failed to remove " << filename() << " before restoring from backup " << backupCopyName
                                  << " after error code=" << ec.code() <<" message=\"" << ec.message() << "\"" << std::endl;
                        throw ErrorException{makeSystemError(fec)};
                    }
                    lib::filesystem::copy_file(backupCopyName,filename(),lib::filesystem::copy_options::overwrite_existing,fec);
                    if (fec)
                    {
                        std::cerr << "CryptFile::truncate: failed to restore " << filename() << " from backup " << backupCopyName
                                  << " after error code=" << ec.code() <<" message=\"" << ec.message() << "\"" << std::endl;
                        throw ErrorException{makeSystemError(fec)};
                    }

                    HATN_CHECK_THROW(doOpen(Mode::write_existing))
                    HATN_CHECK_THROW(doSeek(prevPos))
                    m_seekCursor=prevSeekPos;
                }
                lib::filesystem::remove(backupCopyName,fec);
                if (fec)
                {
                    std::cerr << "CryptFile::truncate: failed to remove backup copy " << backupCopyName << " of " << filename() << std::endl;
                }
            }
        }
        );
    std::ignore=onExit;

    // make a backup copy
    if (backupCopy)
    {
#if 1
// maybe no need to close opened file for copying?
        Error ec1;
        doClose(ec1);
        HATN_CHECK_EC(ec);
#else
        HATN_CHECK_RETURN(doFlush())
#endif
        lib::fs_error_code fec;
        auto oid=du::ObjectId::generateId();
        backupCopyName=fmt::format("{}/hcc_{}.bak",lib::filesystem::temp_directory_path().string(),oid.toString());
        lib::filesystem::copy_file(filename(),backupCopyName,lib::filesystem::copy_options::overwrite_existing,fec);
        if (fec)
        {
            return makeSystemError(fec);
        }

#if 1
        // reopen file
        ec=doOpen(Mode::write_existing);
        HATN_CHECK_EC(ec)
        m_seekCursor=prevSeekPos;
#endif
    }

    // invalidate cache
    HATN_CHECK_RETURN(doFlush(false))
    m_cache.clear();
    m_currentChunk=nullptr;

    // resize to 0
    if (size==0)
    {
        m_size=0;
        m_ciphertextSize=0;
        m_sizeDirty=true;
        updated=true;
        ec=writeSize();
        HATN_CHECK_EC(ec)
        return doSeek(0);
    }

    // init raw file size
    size_t newRawFileSize=m_file->size();
    size_t rawEofPos=eofPos();
    if (newRawFileSize>rawEofPos)
    {
        // drop a file stamp that might reside after eofPos()
        newRawFileSize=rawEofPos;
    }

    // read chunks starting from offset
    auto lastCursor=m_cursor;
    ec=doSeek(newLastPos,0,false);
    if (ec)
    {
        // fallback to last cursor
        std::ignore=doSeek(lastCursor);
    }
    HATN_CHECK_EC(ec)    
    m_tmpBuffer.load(m_currentChunk->content.data(),m_currentChunk->offset);
    size_t newSize=m_size;
    size_t newCipherTextSize=m_ciphertextSize;
    uint32_t newEofSeqnum=m_currentChunk->seqnum;
    for (;;)
    {
        newSize-=m_currentChunk->content.size();
        newCipherTextSize-=m_currentChunk->ciphertextSize;
        if (newRawFileSize<m_currentChunk->ciphertextSize)
        {
            return CommonError::INVALID_SIZE;
        }
        newRawFileSize-=m_currentChunk->ciphertextSize;
        if (isLastChunk(*m_currentChunk))
        {
            break;
        }
        auto pos=seqnumToPos(m_currentChunk->seqnum+1);
        ec=doSeek(pos,0,false);
        HATN_CHECK_EC(ec)
    }
    // write updated sizes to header
    updated=true;
    m_sizeDirty=true;
    m_size=newSize;
    m_ciphertextSize=newCipherTextSize;
    m_eofSeqnum=newEofSeqnum;
    ec=writeSize();
    HATN_CHECK_EC(ec)
    m_singleChunk.reset();
    m_currentChunk=nullptr;

    // truncate underlying file
    ec=m_file->truncate(newRawFileSize,backupCopy);
    HATN_CHECK_EC(ec)

    // seek to end
    ec=doSeek(m_size);
    HATN_CHECK_EC(ec)

    // append tmpbuf
    if (!m_tmpBuffer.isEmpty())
    {
        writeImpl(m_tmpBuffer.data(),m_tmpBuffer.size(),ec);
        HATN_CHECK_EC(ec)
    }

    // check if size is ok
    if (m_size!=size)
    {
        std::cerr << "CryptFile::truncate: wrong size after truncate " << filename() << ": " << m_size << "!=" << size << std::endl;
        ec=CommonError::ABORTED;
        HATN_CHECK_EC(ec)
    }

    // done
    return doSeek(newPos);
}

//---------------------------------------------------------------

common::Error CryptFile::sync()
{
    HATN_CHECK_RETURN(doFlush(false))
    return m_file->sync();
}

//---------------------------------------------------------------

common::Error CryptFile::fsync()
{
    HATN_CHECK_RETURN(doFlush(false))
    return m_file->fsync();
}

//---------------------------------------------------------------
template <typename FieldT>
Error CryptFile::readStampField(const FieldT& fieldName, file_stamp::type& stamp, common::ByteArray** fieldBuf, common::ByteArray& readBuf)
{
    // extract field
    auto& field=stamp.field(fieldName);
    *fieldBuf=field.buf();

    // check if file is already open
    if (isOpen())
    {
        return common::Error(common::CommonError::FILE_ALREADY_OPEN);
    }

    // open file
    HATN_CHECK_RETURN(doOpen(Mode::read,true))
    common::RunOnScopeExit closeGuard(
                    [this]()
                    {
                        common::Error ec;
                        close(ec);
                    }
                );

    // check size
    auto offset=eofPos();
    Error ec;
    auto size=m_file->size(ec);
    HATN_CHECK_EC(ec)
    Assert(size>=offset,"Invalid offset of file stamp");
    uint16_t stampWireSize=0;
    if ((size-offset)<sizeof(stampWireSize))
    {
        return cryptError(CryptError::FILE_STAMP_FAILED);
    }

    // read size of stamp
    HATN_CHECK_RETURN(m_file->seek(offset))
    auto readBytes=m_file->read(reinterpret_cast<char*>(&stampWireSize),sizeof(stampWireSize),ec);
    HATN_CHECK_EC(ec)
    if (readBytes!=sizeof(stampWireSize))
    {
        return common::Error(common::CommonError::FILE_READ_FAILED);
    }
    boost::endian::little_to_native_inplace(stampWireSize);

    // check stamp size
    offset+=sizeof(stampWireSize);
    if ((size-offset)<stampWireSize || stampWireSize==0)
    {
        return cryptError(CryptError::FILE_STAMP_FAILED);
    }

    // read stamp
    readBuf.resize(stampWireSize);
    readBytes=m_file->read(readBuf.data(),stampWireSize,ec);
    HATN_CHECK_EC(ec)
    if (readBytes!=stampWireSize)
    {
        return common::Error(common::CommonError::FILE_READ_FAILED);
    }

    // parse stamp
    if (!du::io::deserializeInline(stamp,readBuf))
    {
        stamp.clear();
        return cryptError(CryptError::FILE_STAMP_FAILED);
    }

    // done
    return Error();
}

//---------------------------------------------------------------
Error CryptFile::writeStamp(const file_stamp::type &stamp)
{
    // check if file is already open
    if (isOpen())
    {
        return common::Error(common::CommonError::FILE_ALREADY_OPEN);
    }

    // open file
    HATN_CHECK_RETURN(doOpen(Mode::write_existing,true))
    common::RunOnScopeExit closeGuard(
                    [this]()
                    {
                        m_writeBuffer.clear();
                        common::Error ec;
                        close(ec);
                    }
                );

    // serialize stamp
    if (du::io::serializeToBuf(stamp,m_writeBuffer)<0)
    {
        return cryptError(CryptError::FILE_STAMP_FAILED);
    }
    if (m_writeBuffer.size()>0xFFFFu)
    {
        return common::Error(common::CommonError::INVALID_SIZE);
    }

    // append size of stamp
    Error ec;
    auto offset=eofPos();
    HATN_CHECK_RETURN(m_file->seek(offset))
    uint16_t stampWireSize=static_cast<uint16_t>(m_writeBuffer.size());
    boost::endian::native_to_little_inplace(stampWireSize);
    auto writtenBytes=m_file->write(reinterpret_cast<const char*>(&stampWireSize),sizeof(stampWireSize),ec);
    HATN_CHECK_EC(ec)
    if (writtenBytes!=sizeof(stampWireSize))
    {
        return common::Error(common::CommonError::FILE_WRITE_FAILED);
    }

    // append stamp
    writtenBytes=m_file->write(m_writeBuffer.data(),m_writeBuffer.size(),ec);
    HATN_CHECK_EC(ec)
    if (writtenBytes!=m_writeBuffer.size())
    {
        return common::Error(common::CommonError::FILE_WRITE_FAILED);
    }

    // done
    return Error();
}

//---------------------------------------------------------------
Error CryptFile::stampDigest()
{
    file_stamp::type stamp;
    ByteArray* fieldBuf=nullptr;
    ByteArray readBuf(m_proc.factory()->dataMemoryResource());

    // read existing stamp
    // result does not matter
    std::ignore=readStampField(file_stamp::digest,stamp,&fieldBuf,readBuf);

    // put digest to stamp
    HATN_CHECK_RETURN(digest(*fieldBuf));

    // write stamp to file
    return writeStamp(stamp);
}

//---------------------------------------------------------------
Error CryptFile::stampMAC()
{
    file_stamp::type stamp;
    ByteArray* fieldBuf=nullptr;
    ByteArray readBuf(m_proc.factory()->dataMemoryResource());

    // read existing stamp
    // result does not matter
    std::ignore=readStampField(file_stamp::mac,stamp,&fieldBuf,readBuf);

    // put MAC to stamp
    HATN_CHECK_RETURN(macSign(*fieldBuf));

    // write stamp to file
    return writeStamp(stamp);
}

//---------------------------------------------------------------
Error CryptFile::checkStampDigest()
{
    file_stamp::type stamp;
    ByteArray* fieldBuf=nullptr;
    ByteArray readBuf(m_proc.factory()->dataMemoryResource());

    // read existing stamp
    HATN_CHECK_RETURN(readStampField(file_stamp::digest,stamp,&fieldBuf,readBuf));

    // check digest
    return digestCheck(*fieldBuf);
}

//---------------------------------------------------------------
Error CryptFile::verifyStampMAC()
{
    file_stamp::type stamp;
    ByteArray* fieldBuf=nullptr;
    ByteArray readBuf(m_proc.factory()->dataMemoryResource());

    // read existing stamp
    HATN_CHECK_RETURN(readStampField(file_stamp::mac,stamp,&fieldBuf,readBuf));

    // verify MAC
    return macVerify(*fieldBuf);
}

//---------------------------------------------------------------
Error CryptFile::createMac(common::SharedPtr<MAC> &mac)
{
    Error ec;
    mac=m_proc.cipherSuite()->createMAC(ec);
    HATN_CHECK_EC(ec)
    if (!mac.isNull())
    {
        auto masterKey=m_proc.masterKey();
        const CryptAlgorithm* alg=nullptr;
        HATN_CHECK_RETURN(m_proc.cipherSuite()->macAlgorithm(alg))
        HATN_CHECK_RETURN(m_proc.deriveKey(masterKey,m_macKey,ConstDataBuf("mac",3),alg));
        mac->setKey(m_macKey.get());
        return Error();
    }
    return cryptError(CryptError::NOT_SUPPORTED_BY_CIPHER_SUITE);
}

//---------------------------------------------------------------
Error CryptFile::createDigest(common::SharedPtr<Digest> &digest)
{
    Error ec;
    digest=m_proc.cipherSuite()->createDigest(ec);
    HATN_CHECK_EC(ec)
    if (digest.isNull())
    {
        return cryptError(CryptError::NOT_SUPPORTED_BY_CIPHER_SUITE);
    }
    return Error();
}

//---------------------------------------------------------------

HATN_CRYPT_NAMESPACE_END
