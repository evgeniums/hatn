/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/****************************************************************************/
/*

*/
/** @file media/test/testoggopus.cpp
  *
  *  Tests of the Opus codec wrappers and the Ogg Opus writer and reader. All of them need the
  *  codec, so without it they skip, except the ones that check it fails cleanly.
  *
  */

/****************************************************************************/

#include <boost/test/unit_test.hpp>

#include <hatn/media/mediaerror.h>
#include <hatn/media/opuscodec.h>
#include <hatn/media/oggopuswriter.h>
#include <hatn/media/oggopusreader.h>

#include "testmediautils.h"

HATN_MEDIA_USING

BOOST_AUTO_TEST_SUITE(TestOggOpus)

BOOST_AUTO_TEST_CASE(AvailabilityMatchesTheBuild)
{
#ifdef HATN_MEDIA_HAS_OGG_OPUS
    BOOST_CHECK(isOggOpusAvailable());
#else
    BOOST_CHECK(!isOggOpusAvailable());

    // Without the codec every entry point fails the same clean way instead of misbehaving.
    OpusFrameEncoder encoder;
    BOOST_CHECK(test::isMediaError(encoder.init(),MediaError::CODEC_UNAVAILABLE));
    OpusFrameDecoder decoder;
    BOOST_CHECK(test::isMediaError(decoder.init(),MediaError::CODEC_UNAVAILABLE));

    test::MemoryFile file;
    test::openNew(file);
    OggOpusWriter writer;
    BOOST_CHECK(test::isMediaError(writer.open(file,0),MediaError::CODEC_UNAVAILABLE));
    OggOpusReader reader;
    BOOST_CHECK(test::isMediaError(reader.open(file),MediaError::CODEC_UNAVAILABLE));
#endif
}

#ifdef HATN_MEDIA_HAS_OGG_OPUS

BOOST_AUTO_TEST_CASE(EncoderProducesPacketsThatDecode)
{
    OpusFrameEncoder encoder;
    BOOST_REQUIRE(!encoder.init());
    BOOST_CHECK(encoder.isInitialized());
    BOOST_CHECK_GT(encoder.lookahead(),uint32_t{0});

    OpusFrameDecoder decoder;
    BOOST_REQUIRE(!decoder.init());

    const auto pcm=test::makeSine(VoiceFrameSamples);
    std::vector<uint8_t> packet(OpusMaxPacketBytes);
    size_t bytes=0;
    BOOST_REQUIRE(!encoder.encode(pcm.data(),packet.data(),packet.size(),bytes));
    BOOST_CHECK_GT(bytes,size_t{0});
    // 32 kbps for 20 ms is about 80 bytes; a wild value means the settings were not applied
    BOOST_CHECK_LT(bytes,size_t{400});

    std::vector<int16_t> out(OpusMaxFrameSamples);
    size_t frames=0;
    BOOST_REQUIRE(!decoder.decode(packet.data(),bytes,out.data(),out.size(),frames));
    BOOST_CHECK_EQUAL(frames,size_t{VoiceFrameSamples});
}

BOOST_AUTO_TEST_CASE(CodecStateErrors)
{
    OpusFrameEncoder encoder;
    std::vector<uint8_t> packet(OpusMaxPacketBytes);
    size_t bytes=0;
    const auto pcm=test::makeSine(VoiceFrameSamples);

    // not initialized
    BOOST_CHECK(test::isMediaError(encoder.encode(pcm.data(),packet.data(),packet.size(),bytes),MediaError::INVALID_STATE));

    BOOST_REQUIRE(!encoder.init());
    // initialized twice
    BOOST_CHECK(test::isMediaError(encoder.init(),MediaError::INVALID_STATE));
    // bad arguments
    BOOST_CHECK(test::isMediaError(encoder.encode(nullptr,packet.data(),packet.size(),bytes),MediaError::INVALID_ARGUMENT));

    OpusFrameDecoder decoder;
    std::vector<int16_t> out(OpusMaxFrameSamples);
    size_t frames=0;
    BOOST_CHECK(test::isMediaError(decoder.decode(packet.data(),1,out.data(),out.size(),frames),MediaError::INVALID_STATE));
}

BOOST_AUTO_TEST_CASE(RoundTripKeepsLengthAndSignal)
{
    // 120480 frames is 125.5 Opus frames: the last one is partial and has to be padded and then
    // trimmed off again, so this checks the granule-position trimming as well as the pre-skip.
    const auto pcm=test::makeSine(120480);

    test::MemoryFile file;
    VoiceRecording recording;
    BOOST_REQUIRE(!test::recordPcm(file,pcm,recording));

    std::vector<int16_t> decoded;
    BOOST_REQUIRE(!test::decodeAll(file,decoded));

    // exactly what was recorded, not a frame more or less
    BOOST_REQUIRE_EQUAL(decoded.size(),pcm.size());

    // and time-aligned: a pre-skip mistake of even a few hundred samples would wreck this at 440 Hz
    const size_t margin=2000;
    const auto corr=test::correlation(pcm.data()+margin,decoded.data()+margin,pcm.size()-2*margin);
    BOOST_CHECK_GT(corr,0.95);
}

BOOST_AUTO_TEST_CASE(ReaderReportsTheStream)
{
    const auto pcm=test::makeSine(96000);
    test::MemoryFile file;
    VoiceRecording recording;
    BOOST_REQUIRE(!test::recordPcm(file,pcm,recording));

    OggOpusReader reader;
    BOOST_REQUIRE(!reader.open(file));
    BOOST_CHECK(reader.isOpen());
    BOOST_CHECK(reader.isComplete());
    BOOST_CHECK_EQUAL(reader.totalFrames(),uint64_t{96000});
    BOOST_CHECK_EQUAL(reader.durationMs(),uint64_t{2000});
    BOOST_CHECK_EQUAL(reader.position(),uint64_t{0});

    // opening twice is a mistake, not a silent re-open
    BOOST_CHECK(test::isMediaError(reader.open(file),MediaError::INVALID_STATE));

    reader.close();
    BOOST_CHECK(!reader.isOpen());
}

BOOST_AUTO_TEST_CASE(SeekLandsOnTheRequestedFrame)
{
    const auto pcm=test::makeSine(120000);
    test::MemoryFile file;
    VoiceRecording recording;
    BOOST_REQUIRE(!test::recordPcm(file,pcm,recording));

    // the sequential decode is the reference
    std::vector<int16_t> full;
    BOOST_REQUIRE(!test::decodeAll(file,full));
    BOOST_REQUIRE_EQUAL(full.size(),pcm.size());

    OggOpusReader reader;
    BOOST_REQUIRE(!reader.open(file));

    const uint64_t targets[]={0,1,959,960,59232,100000,119000};
    for (auto target : targets)
    {
        BOOST_REQUIRE(!reader.seek(target));
        BOOST_CHECK_EQUAL(reader.position(),target);

        std::vector<int16_t> chunk(4800);
        size_t got=0;
        BOOST_REQUIRE(!reader.read(chunk.data(),chunk.size(),got));
        BOOST_CHECK_EQUAL(reader.position(),target+got);

        const auto expected=std::min<size_t>(chunk.size(),full.size()-static_cast<size_t>(target));
        BOOST_CHECK_EQUAL(got,expected);

        // sample-accurate landing: the same audio as playing straight through
        if (got>=480)
        {
            BOOST_CHECK_GT(test::correlation(chunk.data(),full.data()+target,got),0.99);
        }
    }
}

BOOST_AUTO_TEST_CASE(SeekToTheEndAndBeyond)
{
    const auto pcm=test::makeSine(48000);
    test::MemoryFile file;
    VoiceRecording recording;
    BOOST_REQUIRE(!test::recordPcm(file,pcm,recording));

    OggOpusReader reader;
    BOOST_REQUIRE(!reader.open(file));

    std::vector<int16_t> chunk(1000);
    size_t got=0;

    BOOST_REQUIRE(!reader.seek(reader.totalFrames()));
    BOOST_CHECK_EQUAL(reader.position(),reader.totalFrames());
    BOOST_REQUIRE(!reader.read(chunk.data(),chunk.size(),got));
    BOOST_CHECK_EQUAL(got,size_t{0});

    // past the end is clamped, not an error
    BOOST_REQUIRE(!reader.seek(reader.totalFrames()+50000));
    BOOST_CHECK_EQUAL(reader.position(),reader.totalFrames());

    // and seeking back out of the end works
    BOOST_REQUIRE(!reader.seek(0));
    BOOST_REQUIRE(!reader.read(chunk.data(),chunk.size(),got));
    BOOST_CHECK_EQUAL(got,chunk.size());
}

BOOST_AUTO_TEST_CASE(CrashedRecordingStillPlays)
{
    // A recording that was cut off (crash, kill, power loss) after a flush has no end-of-stream
    // page. It must still open and play up to the last flushed page.
    OpusFrameEncoder encoder;
    BOOST_REQUIRE(!encoder.init());

    test::MemoryFile file;
    test::openNew(file);
    OggOpusWriter writer;
    BOOST_REQUIRE(!writer.open(file,static_cast<uint16_t>(encoder.lookahead())));

    const size_t packets=150;
    const auto pcm=test::makeSine(packets*VoiceFrameSamples);
    std::vector<uint8_t> packet(OpusMaxPacketBytes);
    for (size_t i=0;i<packets;i++)
    {
        size_t bytes=0;
        BOOST_REQUIRE(!encoder.encode(pcm.data()+i*VoiceFrameSamples,packet.data(),packet.size(),bytes));
        BOOST_REQUIRE(!writer.writePacket(packet.data(),bytes,VoiceFrameSamples));
    }
    BOOST_REQUIRE(!writer.flush());
    BOOST_CHECK_EQUAL(writer.framesWritten(),uint64_t{packets*VoiceFrameSamples});

    // deliberately no finish(): the last packet is still held back and there is no EOS page
    OggOpusReader reader;
    BOOST_REQUIRE(!reader.open(file));
    BOOST_CHECK(!reader.isComplete());

    // Everything except the held-back last packet made it to disk. The decoder produced that many
    // samples and the first `lookahead` of them are the pre-skip, which is not audio.
    BOOST_CHECK_EQUAL(reader.totalFrames(),uint64_t{(packets-1)*VoiceFrameSamples}-encoder.lookahead());

    std::vector<int16_t> decoded;
    BOOST_REQUIRE(!test::decodeAll(file,decoded));
    BOOST_CHECK_EQUAL(decoded.size(),static_cast<size_t>(reader.totalFrames()));
}

BOOST_AUTO_TEST_CASE(RejectsFilesThatAreNotOggOpus)
{
    OggOpusReader reader;

    // not opened
    test::MemoryFile closed;
    BOOST_CHECK(test::isMediaError(reader.open(closed),MediaError::FILE_NOT_OPEN));

    // empty
    test::MemoryFile empty;
    test::openNew(empty);
    BOOST_CHECK(reader.open(empty));
    BOOST_CHECK(!reader.isOpen());

    // garbage
    test::MemoryFile garbage;
    test::openNew(garbage);
    const char text[]="this is definitely not an Ogg container, just some text padded out a little";
    garbage.write(text,sizeof(text));
    BOOST_CHECK(reader.open(garbage));
    BOOST_CHECK(!reader.isOpen());
}

BOOST_AUTO_TEST_CASE(WriterStateErrors)
{
    OggOpusWriter writer;
    const uint8_t data[4]={1,2,3,4};

    // not opened
    BOOST_CHECK(!writer.isOpen());
    BOOST_CHECK(test::isMediaError(writer.writePacket(data,4,VoiceFrameSamples),MediaError::INVALID_STATE));
    BOOST_CHECK(test::isMediaError(writer.flush(),MediaError::INVALID_STATE));
    BOOST_CHECK(test::isMediaError(writer.finish(),MediaError::INVALID_STATE));

    // file not open
    test::MemoryFile closed;
    BOOST_CHECK(test::isMediaError(writer.open(closed,0),MediaError::FILE_NOT_OPEN));

    test::MemoryFile file;
    test::openNew(file);
    BOOST_REQUIRE(!writer.open(file,0));
    BOOST_CHECK(writer.isOpen());
    // bad arguments
    BOOST_CHECK(test::isMediaError(writer.writePacket(nullptr,4,VoiceFrameSamples),MediaError::INVALID_ARGUMENT));
    BOOST_CHECK(test::isMediaError(writer.writePacket(data,0,VoiceFrameSamples),MediaError::INVALID_ARGUMENT));
    BOOST_CHECK(test::isMediaError(writer.writePacket(data,4,0),MediaError::INVALID_ARGUMENT));

    // finishing closes it
    BOOST_REQUIRE(!writer.finish());
    BOOST_CHECK(!writer.isOpen());
    BOOST_CHECK(test::isMediaError(writer.writePacket(data,4,VoiceFrameSamples),MediaError::INVALID_STATE));
}

BOOST_AUTO_TEST_CASE(WritesIncrementallyNotAtTheEnd)
{
    // The file must grow while recording, not only when it ends: that is what bounds memory for a
    // long recording and what limits a crash to the last page.
    OpusFrameEncoder encoder;
    BOOST_REQUIRE(!encoder.init());

    test::MemoryFile file;
    test::openNew(file);
    OggOpusWriter writer;
    BOOST_REQUIRE(!writer.open(file,static_cast<uint16_t>(encoder.lookahead())));
    const auto headersOnly=file.bytes().size();
    BOOST_CHECK_GT(headersOnly,size_t{0});

    const auto pcm=test::makeSine(VoiceFrameSamples);
    std::vector<uint8_t> packet(OpusMaxPacketBytes);
    size_t previous=headersOnly;
    size_t growths=0;
    for (int i=0;i<500;i++)
    {
        size_t bytes=0;
        BOOST_REQUIRE(!encoder.encode(pcm.data(),packet.data(),packet.size(),bytes));
        BOOST_REQUIRE(!writer.writePacket(packet.data(),bytes,VoiceFrameSamples));
        if (file.bytes().size()>previous)
        {
            growths++;
            previous=file.bytes().size();
        }
    }
    // 10 seconds of voice is many pages
    BOOST_CHECK_GT(growths,size_t{3});
    BOOST_CHECK_EQUAL(writer.bytesWritten(),static_cast<uint64_t>(file.bytes().size()));
}

#endif // HATN_MEDIA_HAS_OGG_OPUS

BOOST_AUTO_TEST_SUITE_END()
