/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/****************************************************************************/
/*

*/
/** @file media/test/testmediautils.h
  *
  *  Helpers shared by the media tests. Header-only on purpose: hatn's ADD_HATN_CTESTS builds one
  *  executable per BOOST_AUTO_TEST_SUITE found in a source file, so shared code cannot live in a
  *  .cpp without being linked into every one of them.
  *
  */

/****************************************************************************/

#ifndef HATNMEDIATESTUTILS_H
#define HATNMEDIATESTUTILS_H

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <vector>

#include <hatn/common/error.h>
#include <hatn/common/file.h>

#include <hatn/media/media.h>
#include <hatn/media/mediaerror.h>
#include <hatn/media/audioformat.h>

#ifdef HATN_MEDIA_HAS_OGG_OPUS
#include <hatn/media/oggopusreader.h>
#include <hatn/media/voicerecorder.h>
#endif

HATN_MEDIA_NAMESPACE_BEGIN

namespace test {

/**
 * A common::File that keeps its bytes in memory.
 *
 * The media library must work through ANY common::File, because a voice message is meant to be
 * written and read through an encrypted crypt::CryptFile as well as a plain file. Running the
 * codec tests on a File that is not a PlainFile checks that nothing in the library assumes one
 * (a real file descriptor, a native handle, in-order access) without dragging crypt and a crypto
 * plugin into the media tests. The CryptFile case itself belongs to the layers that have crypt.
 */
class MemoryFile : public common::File
{
    public:

        using common::File::open;
        using common::File::close;
        using common::File::pos;
        using common::File::size;
        using common::File::write;
        using common::File::read;

        Error open(const char*, Mode mode) override
        {
            if (mode==Mode::write || mode==Mode::write_new)
            {
                m_data.clear();
            }
            m_open=true;
            m_pos=0;
            return OK;
        }

        bool isOpen() const noexcept override
        {
            return m_open;
        }

        Error flush(bool) override
        {
            return OK;
        }

        void close() override
        {
            m_open=false;
        }

        Error seek(uint64_t position) override
        {
            m_pos=position;
            return OK;
        }

        uint64_t pos() const override
        {
            return m_pos;
        }

        uint64_t size() const override
        {
            return m_data.size();
        }

        uint64_t size(Error& ec) const override
        {
            ec.reset();
            return m_data.size();
        }

        size_t write(const char* data, size_t size) override
        {
            if (m_pos+size>m_data.size())
            {
                m_data.resize(static_cast<size_t>(m_pos)+size);
            }
            std::memcpy(m_data.data()+m_pos,data,size);
            m_pos+=size;
            return size;
        }

        size_t read(char* data, size_t maxSize) override
        {
            if (m_pos>=m_data.size())
            {
                return 0;
            }
            const auto n=std::min<size_t>(maxSize,m_data.size()-static_cast<size_t>(m_pos));
            std::memcpy(data,m_data.data()+m_pos,n);
            m_pos+=n;
            return n;
        }

        NativeHandleType nativeHandle() override
        {
            return NativeHandleType{};
        }

        const std::vector<char>& bytes() const noexcept
        {
            return m_data;
        }

        std::vector<char>& bytes() noexcept
        {
            return m_data;
        }

    private:

        std::vector<char> m_data;
        uint64_t m_pos=0;
        bool m_open=false;
};

//! Mono 48 kHz sine, for feeding the recorder and comparing what comes back.
inline std::vector<int16_t> makeSine(size_t frames, double frequency=440.0, double amplitude=10000.0, size_t startFrame=0)
{
    std::vector<int16_t> result(frames);
    const double twoPi=6.283185307179586;
    for (size_t i=0;i<frames;i++)
    {
        const auto t=static_cast<double>(startFrame+i)/static_cast<double>(VoiceSampleRate);
        result[i]=static_cast<int16_t>(std::lround(amplitude*std::sin(twoPi*frequency*t)));
    }
    return result;
}

//! Normalized cross-correlation at lag 0: 1 for identical shape, 0 for unrelated, -1 inverted.
inline double correlation(const int16_t* a, const int16_t* b, size_t frames)
{
    double ab=0.0;
    double aa=0.0;
    double bb=0.0;
    for (size_t i=0;i<frames;i++)
    {
        const auto x=static_cast<double>(a[i]);
        const auto y=static_cast<double>(b[i]);
        ab+=x*y;
        aa+=x*x;
        bb+=y*y;
    }
    if (aa==0.0 || bb==0.0)
    {
        return 0.0;
    }
    return ab/std::sqrt(aa*bb);
}

//! Open a MemoryFile for writing from scratch, as a recorder expects to be handed a file.
inline void openNew(MemoryFile& file)
{
    // MemoryFile::open() cannot fail, so there is no error to look at
    static_cast<void>(file.open("memory",common::File::Mode::write_new));
}

//! Whether `ec` is exactly the given media error.
inline bool isMediaError(const Error& ec, MediaError code)
{
    return ec.code()==static_cast<int>(code)
           && ec.category()!=nullptr
           && std::strcmp(ec.category()->name(),"hatn.media")==0;
}

#ifdef HATN_MEDIA_HAS_OGG_OPUS

/**
 * Record `pcm` through a VoiceRecorder into `file`, feeding it in `chunk`-frame pieces the way a
 * capture callback would and calling process() between them the way a worker would.
 */
inline Error recordPcm(
        MemoryFile& file,
        const std::vector<int16_t>& pcm,
        VoiceRecording& recording,
        size_t chunk=480,
        const VoiceRecorderConfig& config=VoiceRecorderConfig{}
    )
{
    openNew(file);

    VoiceRecorder recorder(config);
    auto ec=recorder.start(file);
    if (ec)
    {
        return ec;
    }

    for (size_t offset=0;offset<pcm.size();offset+=chunk)
    {
        const auto n=std::min(chunk,pcm.size()-offset);
        if (recorder.pushPcm(pcm.data()+offset,n)!=n)
        {
            return mediaError(MediaError::INVALID_STATE);
        }
        ec=recorder.process();
        if (ec)
        {
            return ec;
        }
    }
    return recorder.finish(recording);
}

//! Decode a whole Ogg Opus file to PCM with OggOpusReader.
inline Error decodeAll(common::File& file, std::vector<int16_t>& pcm)
{
    pcm.clear();

    OggOpusReader reader;
    auto ec=reader.open(file);
    if (ec)
    {
        return ec;
    }

    std::vector<int16_t> buffer(4096);
    for (;;)
    {
        size_t got=0;
        ec=reader.read(buffer.data(),buffer.size(),got);
        if (ec)
        {
            return ec;
        }
        if (got==0)
        {
            return OK;
        }
        pcm.insert(pcm.end(),buffer.begin(),buffer.begin()+static_cast<std::ptrdiff_t>(got));
    }
}

#endif // HATN_MEDIA_HAS_OGG_OPUS

} // namespace test

HATN_MEDIA_NAMESPACE_END

#endif // HATNMEDIATESTUTILS_H
