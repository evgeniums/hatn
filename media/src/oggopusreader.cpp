/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/****************************************************************************/
/*

*/
/** @file media/oggopusreader.cpp
  *
  *      Contains implementation of OggOpusReader.
  *
  */

#include <cstring>
#include <algorithm>
#include <vector>

#include <hatn/media/mediaerror.h>
#include <hatn/media/opuscodec.h>
#include <hatn/media/oggopusreader.h>

#ifdef HATN_MEDIA_HAS_OGG_OPUS
#include <ogg/ogg.h>
#endif

HATN_MEDIA_NAMESPACE_BEGIN

#ifdef HATN_MEDIA_HAS_OGG_OPUS

namespace {

//! Bytes requested from the file per read.
constexpr const size_t ReadChunk=4096;

//! The largest Ogg page is 65307 bytes, so a window twice as big always contains a whole last page.
constexpr const uint64_t TailWindow=2*65536;

//! RFC 7845 section 4.6: decode this much audio before the seek target so the decoder converges.
constexpr const uint64_t SeekPreroll=voiceMsToFrames(80);

uint16_t getLe16(const unsigned char* p)
{
    return static_cast<uint16_t>(p[0] | (p[1]<<8));
}

}

/********************** OggOpusReader **************************/

class OggOpusReader_p
{
    public:

        //! One audio page, for seeking.
        struct PageInfo
        {
            uint64_t offset=0;      //!< file offset of the page start
            int64_t granule=-1;     //!< granule position, -1 if no packet ends on the page
            bool continued=false;   //!< first packet is a continuation of the previous page's
        };

        ~OggOpusReader_p()
        {
            clear();
        }

        void clear() noexcept
        {
            if (streamInit)
            {
                ogg_stream_clear(&stream);
                streamInit=false;
            }
            if (syncInit)
            {
                ogg_sync_clear(&sync);
                syncInit=false;
            }
            // a decoder that is kept makes the next open() of this reader fail in init()
            decoder.release();
            file=nullptr;
            opened=false;
            pages.clear();
            indexBuilt=false;
            carryPos=carryEnd=0;
            totalFrames=0;
            position=0;
            nextIndex=0;
            skipUntil=0;
            complete=false;
        }

        //! File offset just past the most recent page handed out by `sync`.
        uint64_t syncPageEnd() const noexcept
        {
            return readBase+bytesRead-static_cast<uint64_t>(sync.fill-sync.returned);
        }

        Error feed(bool& gotData)
        {
            gotData=false;
            Error ec;
            auto* buffer=ogg_sync_buffer(&sync,static_cast<long>(ReadChunk));
            if (buffer==nullptr)
            {
                return mediaError(MediaError::INVALID_OGG_STREAM);
            }
            const auto n=file->read(buffer,ReadChunk,ec);
            if (ec)
            {
                return mediaError(MediaError::FILE_READ_FAILED,ec);
            }
            if (n!=0)
            {
                ogg_sync_wrote(&sync,static_cast<long>(n));
                bytesRead+=n;
                gotData=true;
            }
            return OK;
        }

        //! Position the file and reset the container parser to start reading at `offset`.
        Error restartAt(uint64_t offset)
        {
            auto ec=file->seek(offset);
            if (ec)
            {
                return mediaError(MediaError::SEEK_FAILED,ec);
            }
            ogg_sync_reset(&sync);
            if (streamInit)
            {
                ogg_stream_reset(&stream);
            }
            readBase=offset;
            bytesRead=0;
            return OK;
        }

        /**
         * @brief Next Opus packet of the stream.
         * @param got false when the file ended first.
         *
         * The packet's memory stays valid until the next call.
         */
        Error nextPacket(ogg_packet& packet, bool& got)
        {
            got=false;
            for (;;)
            {
                // <0 means a hole (data was dropped, as right after a seek into the middle of a
                // stream). It is reported once and the next call carries on, so just retry.
                auto r=ogg_stream_packetout(&stream,&packet);
                if (r>0)
                {
                    got=true;
                    return OK;
                }
                if (r<0)
                {
                    continue;
                }

                ogg_page page;
                r=ogg_sync_pageout(&sync,&page);
                if (r>0)
                {
                    // a page of some other logical stream is refused, and that is fine
                    if (ogg_stream_pagein(&stream,&page)==0)
                    {
                        lastPageEnd=syncPageEnd();
                    }
                    continue;
                }
                if (r<0)
                {
                    continue;
                }

                bool more=false;
                auto ec=feed(more);
                if (ec)
                {
                    return ec;
                }
                if (!more)
                {
                    return OK;
                }
            }
        }

        /**
         * @brief Call `handler(page, pageStartOffset)` for every page of our stream, from `start`.
         *
         * Uses a private parser, so it does not disturb the read position.
         */
        template <typename HandlerT>
        Error scan(uint64_t start, HandlerT&& handler)
        {
            auto ec=file->seek(start);
            if (ec)
            {
                return mediaError(MediaError::SEEK_FAILED,ec);
            }

            ogg_sync_state local;
            ogg_sync_init(&local);
            uint64_t consumed=0;
            Error result;

            for (;;)
            {
                ogg_page page;
                int r=0;
                while ((r=ogg_sync_pageout(&local,&page))!=0)
                {
                    if (r<0)
                    {
                        continue;
                    }
                    if (ogg_page_serialno(&page)!=serial)
                    {
                        continue;
                    }
                    const auto end=start+consumed-static_cast<uint64_t>(local.fill-local.returned);
                    const auto pageStart=end-static_cast<uint64_t>(page.header_len+page.body_len);
                    handler(page,pageStart);
                }

                auto* buffer=ogg_sync_buffer(&local,static_cast<long>(ReadChunk));
                if (buffer==nullptr)
                {
                    result=mediaError(MediaError::INVALID_OGG_STREAM);
                    break;
                }
                Error readEc;
                const auto n=file->read(buffer,ReadChunk,readEc);
                if (readEc)
                {
                    result=mediaError(MediaError::FILE_READ_FAILED,readEc);
                    break;
                }
                if (n==0)
                {
                    break;
                }
                ogg_sync_wrote(&local,static_cast<long>(n));
                consumed+=n;
            }

            ogg_sync_clear(&local);
            return result;
        }

        Error ensureIndex()
        {
            if (indexBuilt)
            {
                return OK;
            }

            std::vector<PageInfo> found;
            auto ec=scan(dataOffset,[&found](const ogg_page& page, uint64_t pageStart)
            {
                PageInfo info;
                info.offset=pageStart;
                info.granule=static_cast<int64_t>(ogg_page_granulepos(&page));
                info.continued=ogg_page_continued(&page)!=0;
                found.push_back(info);
            });
            if (ec)
            {
                return ec;
            }

            pages=std::move(found);
            indexBuilt=true;
            return OK;
        }

        common::File* file=nullptr;
        bool opened=false;

        ogg_sync_state sync;
        bool syncInit=false;
        ogg_stream_state stream;
        bool streamInit=false;
        int serial=0;

        uint64_t readBase=0;        //!< file offset the current sync stream started at
        uint64_t bytesRead=0;       //!< bytes fed to the sync since readBase
        uint64_t lastPageEnd=0;     //!< end offset of the last page taken into the stream

        uint64_t dataOffset=0;      //!< file offset of the first audio page
        uint16_t preSkip=0;
        uint64_t totalFrames=0;
        bool complete=false;

        OpusFrameDecoder decoder;

        //! Decoder timeline (pre-skip included) index of the next sample to be DECODED.
        uint64_t nextIndex=0;
        //! Decoded samples with a decoder-timeline index below this are discarded.
        uint64_t skipUntil=0;
        //! PCM frame (pre-skip excluded) of the next sample to be RETURNED.
        uint64_t position=0;

        //! Decoded but not yet returned samples; carry[carryPos..carryEnd).
        std::vector<int16_t> carry=std::vector<int16_t>(OpusMaxFrameSamples);
        size_t carryPos=0;
        size_t carryEnd=0;

        std::vector<PageInfo> pages;
        bool indexBuilt=false;
};

//---------------------------------------------------------------
OggOpusReader::OggOpusReader() : d(std::make_unique<OggOpusReader_p>())
{}

//---------------------------------------------------------------
OggOpusReader::~OggOpusReader()=default;

//---------------------------------------------------------------
Error OggOpusReader::open(common::File& file)
{
    if (d->opened)
    {
        return mediaError(MediaError::INVALID_STATE);
    }
    if (!file.isOpen())
    {
        return mediaError(MediaError::FILE_NOT_OPEN);
    }

    d->clear();
    d->file=&file;

    // any failure below leaves nothing half-open
    auto fail=[this](Error ec)
    {
        d->clear();
        return ec;
    };

    ogg_sync_init(&d->sync);
    d->syncInit=true;

    auto ec=d->file->seek(0);
    if (ec)
    {
        return fail(mediaError(MediaError::SEEK_FAILED,ec));
    }
    d->readBase=0;
    d->bytesRead=0;

    // The first page names the logical stream; it has to be the beginning of one.
    ogg_page first;
    for (;;)
    {
        const auto r=ogg_sync_pageout(&d->sync,&first);
        if (r>0)
        {
            break;
        }
        if (r<0)
        {
            continue;
        }
        bool more=false;
        ec=d->feed(more);
        if (ec)
        {
            return fail(ec);
        }
        if (!more)
        {
            return fail(mediaError(MediaError::INVALID_OGG_STREAM));
        }
    }
    if (!ogg_page_bos(&first))
    {
        return fail(mediaError(MediaError::INVALID_OGG_STREAM));
    }

    d->serial=ogg_page_serialno(&first);
    if (ogg_stream_init(&d->stream,d->serial)!=0)
    {
        return fail(mediaError(MediaError::INVALID_OGG_STREAM));
    }
    d->streamInit=true;
    if (ogg_stream_pagein(&d->stream,&first)!=0)
    {
        return fail(mediaError(MediaError::INVALID_OGG_STREAM));
    }
    d->lastPageEnd=d->syncPageEnd();

    // OpusHead, RFC 7845 section 5.1
    ogg_packet packet;
    bool got=false;
    ec=d->nextPacket(packet,got);
    if (ec)
    {
        return fail(ec);
    }
    if (!got)
    {
        return fail(mediaError(MediaError::TRUNCATED_STREAM));
    }
    if (packet.bytes<19 || std::memcmp(packet.packet,"OpusHead",8)!=0)
    {
        return fail(mediaError(MediaError::NOT_OPUS_STREAM));
    }
    // the high nibble of the version is the major version, and only major 0 is defined
    if ((packet.packet[8]&0xF0)!=0)
    {
        return fail(mediaError(MediaError::NOT_OPUS_STREAM));
    }
    if (static_cast<uint32_t>(packet.packet[9])!=VoiceChannels)
    {
        return fail(mediaError(MediaError::UNSUPPORTED_FORMAT));
    }
    if (packet.packet[18]!=0)
    {
        return fail(mediaError(MediaError::UNSUPPORTED_OPUS_MAPPING));
    }
    d->preSkip=getLe16(packet.packet+10);
    const auto gain=static_cast<int16_t>(getLe16(packet.packet+16));

    // OpusTags, section 5.2. Its content is of no use here, but it must be there and it marks the
    // end of the headers, which is where the audio starts.
    ec=d->nextPacket(packet,got);
    if (ec)
    {
        return fail(ec);
    }
    if (!got)
    {
        return fail(mediaError(MediaError::TRUNCATED_STREAM));
    }
    if (packet.bytes<8 || std::memcmp(packet.packet,"OpusTags",8)!=0)
    {
        return fail(mediaError(MediaError::NOT_OPUS_STREAM));
    }
    d->dataOffset=d->lastPageEnd;

    ec=d->decoder.init(gain);
    if (ec)
    {
        return fail(ec);
    }

    // Length: the granule position of the last page that finishes a packet. It counts from the
    // start of the decoder's timeline, so the pre-skip comes off it.
    int64_t lastGranule=-1;
    bool sawEos=false;
    const auto size=d->file->size(ec);
    if (ec)
    {
        return fail(mediaError(MediaError::FILE_READ_FAILED,ec));
    }
    const auto tailStart=size>TailWindow?size-TailWindow:d->dataOffset;
    ec=d->scan(std::max(tailStart,d->dataOffset),[&lastGranule,&sawEos](const ogg_page& page, uint64_t)
    {
        const auto granule=static_cast<int64_t>(ogg_page_granulepos(&page));
        if (granule>=0)
        {
            lastGranule=granule;
        }
        if (ogg_page_eos(&page)!=0)
        {
            sawEos=true;
        }
    });
    if (ec)
    {
        return fail(ec);
    }
    d->totalFrames=(lastGranule>static_cast<int64_t>(d->preSkip))?static_cast<uint64_t>(lastGranule)-d->preSkip:0;
    d->complete=sawEos;

    // start of the audio, ready to read
    ec=d->restartAt(d->dataOffset);
    if (ec)
    {
        return fail(ec);
    }
    d->nextIndex=0;
    d->skipUntil=d->preSkip;
    d->position=0;
    d->carryPos=d->carryEnd=0;
    d->opened=true;
    return OK;
}

//---------------------------------------------------------------
bool OggOpusReader::isOpen() const noexcept
{
    return d->opened;
}

//---------------------------------------------------------------
void OggOpusReader::close() noexcept
{
    d->clear();
}

//---------------------------------------------------------------
uint64_t OggOpusReader::totalFrames() const noexcept
{
    return d->totalFrames;
}

//---------------------------------------------------------------
bool OggOpusReader::isComplete() const noexcept
{
    return d->complete;
}

//---------------------------------------------------------------
uint64_t OggOpusReader::position() const noexcept
{
    return d->position;
}

//---------------------------------------------------------------
Error OggOpusReader::read(int16_t* pcm, size_t maxFrames, size_t& outFrames)
{
    outFrames=0;
    if (!d->opened)
    {
        return mediaError(MediaError::INVALID_STATE);
    }
    if (pcm==nullptr)
    {
        return mediaError(MediaError::INVALID_ARGUMENT);
    }

    while (outFrames<maxFrames)
    {
        // Past the stream's length everything is end padding, which the granule position trims.
        if (d->position>=d->totalFrames)
        {
            break;
        }

        if (d->carryPos<d->carryEnd)
        {
            auto n=std::min(maxFrames-outFrames,d->carryEnd-d->carryPos);
            n=static_cast<size_t>(std::min<uint64_t>(n,d->totalFrames-d->position));
            std::memcpy(pcm+outFrames,d->carry.data()+d->carryPos,n*sizeof(int16_t));
            outFrames+=n;
            d->carryPos+=n;
            d->position+=n;
            continue;
        }

        ogg_packet packet;
        bool got=false;
        auto ec=d->nextPacket(packet,got);
        if (ec)
        {
            return ec;
        }
        if (!got)
        {
            break;
        }
        if (packet.bytes<=0)
        {
            continue;
        }

        size_t decoded=0;
        ec=d->decoder.decode(packet.packet,static_cast<size_t>(packet.bytes),d->carry.data(),d->carry.size(),decoded);
        if (ec)
        {
            return ec;
        }

        const auto startIndex=d->nextIndex;
        d->nextIndex+=decoded;
        if (d->nextIndex<=d->skipUntil)
        {
            // entirely inside the pre-skip or the seek pre-roll
            d->carryPos=d->carryEnd=0;
            continue;
        }

        d->carryPos=(startIndex<d->skipUntil)?static_cast<size_t>(d->skipUntil-startIndex):0;
        d->carryEnd=decoded;
    }

    return OK;
}

//---------------------------------------------------------------
Error OggOpusReader::seek(uint64_t frame)
{
    if (!d->opened)
    {
        return mediaError(MediaError::INVALID_STATE);
    }

    frame=std::min(frame,d->totalFrames);

    auto ec=d->ensureIndex();
    if (ec)
    {
        return ec;
    }

    // In the decoder's timeline the target is after the pre-skip. Start decoding one pre-roll
    // earlier, at the latest page boundary that is not after that point.
    const auto target=frame+d->preSkip;
    const auto wanted=target>SeekPreroll?target-SeekPreroll:0;

    uint64_t offset=d->dataOffset;
    uint64_t startIndex=0;
    for (size_t i=d->pages.size();i-->0;)
    {
        const auto& page=d->pages[i];
        if (page.granule<0 || static_cast<uint64_t>(page.granule)>wanted)
        {
            continue;
        }
        // Resume on the NEXT page, and only if it begins with a whole packet: then the packet
        // starts exactly at the granule position of this page. Otherwise keep looking earlier.
        if (i+1>=d->pages.size() || d->pages[i+1].continued)
        {
            continue;
        }
        offset=d->pages[i+1].offset;
        startIndex=static_cast<uint64_t>(page.granule);
        break;
    }

    ec=d->restartAt(offset);
    if (ec)
    {
        return ec;
    }
    ec=d->decoder.reset();
    if (ec)
    {
        return ec;
    }

    d->nextIndex=startIndex;
    d->skipUntil=target;
    d->position=frame;
    d->carryPos=d->carryEnd=0;
    return OK;
}

#else // HATN_MEDIA_HAS_OGG_OPUS

/********************** stubs: no codec in this build **************************/

class OggOpusReader_p
{};

OggOpusReader::OggOpusReader() : d(std::make_unique<OggOpusReader_p>()) {}
OggOpusReader::~OggOpusReader()=default;
Error OggOpusReader::open(common::File&) { return mediaError(MediaError::CODEC_UNAVAILABLE); }
bool OggOpusReader::isOpen() const noexcept { return false; }
void OggOpusReader::close() noexcept {}
uint64_t OggOpusReader::totalFrames() const noexcept { return 0; }
bool OggOpusReader::isComplete() const noexcept { return false; }
uint64_t OggOpusReader::position() const noexcept { return 0; }
Error OggOpusReader::read(int16_t*, size_t, size_t& outFrames)
{
    outFrames=0;
    return mediaError(MediaError::CODEC_UNAVAILABLE);
}
Error OggOpusReader::seek(uint64_t) { return mediaError(MediaError::CODEC_UNAVAILABLE); }

#endif // HATN_MEDIA_HAS_OGG_OPUS

//---------------------------------------------------------------

HATN_MEDIA_NAMESPACE_END
