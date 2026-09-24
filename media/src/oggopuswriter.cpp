/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/****************************************************************************/
/*

*/
/** @file media/oggopuswriter.cpp
  *
  *      Contains implementation of OggOpusWriter.
  *
  */

#include <cstring>
#include <vector>

#include <hatn/media/mediaerror.h>
#include <hatn/media/oggopuswriter.h>

#ifdef HATN_MEDIA_HAS_OGG_OPUS
#include <ogg/ogg.h>
#endif

HATN_MEDIA_NAMESPACE_BEGIN

#ifdef HATN_MEDIA_HAS_OGG_OPUS

namespace {

void putLe16(std::vector<uint8_t>& out, uint16_t value)
{
    out.push_back(static_cast<uint8_t>(value&0xFF));
    out.push_back(static_cast<uint8_t>((value>>8)&0xFF));
}

void putLe32(std::vector<uint8_t>& out, uint32_t value)
{
    for (int shift=0;shift<32;shift+=8)
    {
        out.push_back(static_cast<uint8_t>((value>>shift)&0xFF));
    }
}

void putBytes(std::vector<uint8_t>& out, const char* text, size_t size)
{
    out.insert(out.end(),reinterpret_cast<const uint8_t*>(text),reinterpret_cast<const uint8_t*>(text)+size);
}

}

/********************** OggOpusWriter **************************/

class OggOpusWriter_p
{
    public:

        ~OggOpusWriter_p()
        {
            clear();
        }

        void clear() noexcept
        {
            if (streamInitialized)
            {
                ogg_stream_clear(&stream);
                streamInitialized=false;
            }
            file=nullptr;
            hasPending=false;
        }

        //! Hand one packet to libogg.
        void submit(const uint8_t* data, size_t bytes, bool eos, int64_t granule)
        {
            ogg_packet packet;
            packet.packet=const_cast<unsigned char*>(reinterpret_cast<const unsigned char*>(data));
            packet.bytes=static_cast<long>(bytes);
            packet.b_o_s=(packetNo==0)?1:0;
            packet.e_o_s=eos?1:0;
            packet.granulepos=granule;
            packet.packetno=static_cast<ogg_int64_t>(packetNo++);
            ogg_stream_packetin(&stream,&packet);
        }

        Error writePage(const ogg_page& page)
        {
            Error ec;
            auto written=file->write(reinterpret_cast<const char*>(page.header),static_cast<size_t>(page.header_len),ec);
            if (ec || written!=static_cast<size_t>(page.header_len))
            {
                return mediaError(MediaError::FILE_WRITE_FAILED,ec);
            }
            written=file->write(reinterpret_cast<const char*>(page.body),static_cast<size_t>(page.body_len),ec);
            if (ec || written!=static_cast<size_t>(page.body_len))
            {
                return mediaError(MediaError::FILE_WRITE_FAILED,ec);
            }
            bytesWritten+=static_cast<uint64_t>(page.header_len)+static_cast<uint64_t>(page.body_len);
            return OK;
        }

        //! Write every page libogg considers complete.
        Error drainPages()
        {
            ogg_page page;
            while (ogg_stream_pageout(&stream,&page)>0)
            {
                auto ec=writePage(page);
                if (ec)
                {
                    return ec;
                }
            }
            return OK;
        }

        //! Write every queued packet regardless of page fullness.
        Error flushPages()
        {
            ogg_page page;
            while (ogg_stream_flush(&stream,&page)>0)
            {
                auto ec=writePage(page);
                if (ec)
                {
                    return ec;
                }
            }
            return OK;
        }

        common::File* file=nullptr;
        ogg_stream_state stream;
        bool streamInitialized=false;

        uint16_t preSkip=0;
        uint64_t packetNo=0;
        uint64_t bytesWritten=0;

        //! PCM frames of all packets submitted to libogg, excluding the held-back one.
        uint64_t submittedFrames=0;

        //! The most recent packet, held until it is known whether it is the last.
        std::vector<uint8_t> pending;
        uint32_t pendingFrames=0;
        bool hasPending=false;

        bool finished=false;
};

//---------------------------------------------------------------
OggOpusWriter::OggOpusWriter() : d(std::make_unique<OggOpusWriter_p>())
{}

//---------------------------------------------------------------
OggOpusWriter::~OggOpusWriter()=default;

//---------------------------------------------------------------
Error OggOpusWriter::open(common::File& file, uint16_t preSkip, const std::string& vendor)
{
    if (d->file!=nullptr)
    {
        return mediaError(MediaError::INVALID_STATE);
    }
    if (!file.isOpen())
    {
        return mediaError(MediaError::FILE_NOT_OPEN);
    }

    d->clear();
    d->file=&file;
    d->preSkip=preSkip;
    d->packetNo=0;
    d->bytesWritten=0;
    d->submittedFrames=0;
    d->pending.clear();
    d->pendingFrames=0;
    d->finished=false;

    // Serial number only has to differ between streams multiplexed in one file, which never
    // happens here, so a constant is fine and keeps the output reproducible.
    if (ogg_stream_init(&d->stream,0x48544E31)!=0)
    {
        d->clear();
        return mediaError(MediaError::INVALID_OGG_STREAM);
    }
    d->streamInitialized=true;

    // OpusHead, RFC 7845 section 5.1. Version 1, mono, mapping family 0, no output gain.
    std::vector<uint8_t> head;
    putBytes(head,"OpusHead",8);
    head.push_back(1);
    head.push_back(static_cast<uint8_t>(VoiceChannels));
    putLe16(head,preSkip);
    putLe32(head,VoiceSampleRate);
    putLe16(head,0);
    head.push_back(0);
    d->submit(head.data(),head.size(),false,0);

    // The Opus mapping requires the header packet alone on the first page and the tags packet
    // alone on the second, hence the flush after each.
    auto ec=d->flushPages();
    if (ec)
    {
        d->clear();
        return ec;
    }

    // OpusTags, RFC 7845 section 5.2. Vendor string and no user comments.
    std::vector<uint8_t> tags;
    putBytes(tags,"OpusTags",8);
    putLe32(tags,static_cast<uint32_t>(vendor.size()));
    putBytes(tags,vendor.data(),vendor.size());
    putLe32(tags,0);
    d->submit(tags.data(),tags.size(),false,0);

    ec=d->flushPages();
    if (ec)
    {
        d->clear();
        return ec;
    }

    return OK;
}

//---------------------------------------------------------------
bool OggOpusWriter::isOpen() const noexcept
{
    return d->file!=nullptr && !d->finished;
}

//---------------------------------------------------------------
Error OggOpusWriter::writePacket(const uint8_t* data, size_t bytes, uint32_t frames)
{
    if (!isOpen())
    {
        return mediaError(MediaError::INVALID_STATE);
    }
    if (data==nullptr || bytes==0 || frames==0)
    {
        return mediaError(MediaError::INVALID_ARGUMENT);
    }

    // The previous packet is now known not to be the last one, so it can go to libogg. Its
    // granule position (RFC 7845 section 4) is the number of samples the decoder has produced
    // once it has decoded this packet, counted from the very start of the stream. That count
    // INCLUDES the pre-skip samples, which a player discards; it is the reader that subtracts
    // the pre-skip to get the playable length. So it is just the running total, with nothing
    // added: adding the pre-skip here as well would count it twice.
    if (d->hasPending)
    {
        d->submittedFrames+=d->pendingFrames;
        d->submit(d->pending.data(),d->pending.size(),false,static_cast<int64_t>(d->submittedFrames));
    }

    d->pending.assign(data,data+bytes);
    d->pendingFrames=frames;
    d->hasPending=true;

    return d->drainPages();
}

//---------------------------------------------------------------
Error OggOpusWriter::flush()
{
    if (!isOpen())
    {
        return mediaError(MediaError::INVALID_STATE);
    }

    auto ec=d->flushPages();
    if (ec)
    {
        return ec;
    }

    ec=d->file->flush(false);
    if (ec)
    {
        return mediaError(MediaError::FILE_WRITE_FAILED,ec);
    }
    return OK;
}

//---------------------------------------------------------------
Error OggOpusWriter::finish(uint32_t trimFrames)
{
    if (!isOpen())
    {
        return mediaError(MediaError::INVALID_STATE);
    }

    if (d->hasPending)
    {
        d->submittedFrames+=d->pendingFrames;

        // RFC 7845 section 4.4: the granule position of the last page may be LESS than the audio
        // it carries, and a decoder discards the difference. That is how the padding after the
        // last real sample is cut off. Never move it before the previous packet's end.
        auto trim=static_cast<uint64_t>(trimFrames);
        if (trim>d->pendingFrames)
        {
            trim=d->pendingFrames;
        }
        const auto granule=d->submittedFrames-trim;
        d->submit(d->pending.data(),d->pending.size(),true,static_cast<int64_t>(granule));
        d->hasPending=false;
    }

    auto ec=d->flushPages();
    if (!ec)
    {
        ec=d->file->flush(true);
        if (ec)
        {
            ec=mediaError(MediaError::FILE_WRITE_FAILED,ec);
        }
    }

    d->finished=true;
    d->clear();
    return ec;
}

//---------------------------------------------------------------
void OggOpusWriter::abort() noexcept
{
    d->finished=true;
    d->clear();
}

//---------------------------------------------------------------
uint64_t OggOpusWriter::bytesWritten() const noexcept
{
    return d->bytesWritten;
}

//---------------------------------------------------------------
uint64_t OggOpusWriter::framesWritten() const noexcept
{
    return d->submittedFrames+(d->hasPending?d->pendingFrames:0);
}

#else // HATN_MEDIA_HAS_OGG_OPUS

/********************** stubs: no codec in this build **************************/

class OggOpusWriter_p
{};

OggOpusWriter::OggOpusWriter() : d(std::make_unique<OggOpusWriter_p>()) {}
OggOpusWriter::~OggOpusWriter()=default;
Error OggOpusWriter::open(common::File&, uint16_t, const std::string&) { return mediaError(MediaError::CODEC_UNAVAILABLE); }
bool OggOpusWriter::isOpen() const noexcept { return false; }
Error OggOpusWriter::writePacket(const uint8_t*, size_t, uint32_t) { return mediaError(MediaError::CODEC_UNAVAILABLE); }
Error OggOpusWriter::flush() { return mediaError(MediaError::CODEC_UNAVAILABLE); }
Error OggOpusWriter::finish(uint32_t) { return mediaError(MediaError::CODEC_UNAVAILABLE); }
void OggOpusWriter::abort() noexcept {}
uint64_t OggOpusWriter::bytesWritten() const noexcept { return 0; }
uint64_t OggOpusWriter::framesWritten() const noexcept { return 0; }

#endif // HATN_MEDIA_HAS_OGG_OPUS

//---------------------------------------------------------------

HATN_MEDIA_NAMESPACE_END
