/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/****************************************************************************/
/*

*/
/** @file mediatests/test/testvoicecryptfile.cpp
  *
  *  The voice message path over a real crypt::CryptFile: record, finish, play, seek, crop, and the
  *  pause that closes the file and reopens it in append mode.
  *
  *  media itself never names crypt and its tests run on an in-memory common::File. What that cannot
  *  show is how a CryptFile behaves under the access pattern of a recorder and a player, which is
  *  what is checked here. The crypto plugin is loaded straight from ./plugins/crypt, not through
  *  hatn/test/pluginlist.h: that filters by HATN_TEST_PLUGINS, which is filled from the plugins of
  *  the module under test, and this module has none. Only a dynamically loaded plugin is supported.
  *
  */

/****************************************************************************/

#include <algorithm>
#include <chrono>
#include <cmath>
#include <iostream>
#include <memory>
#include <string>
#include <tuple>
#include <vector>

#include <boost/filesystem.hpp>
#include <boost/test/unit_test.hpp>

#include <hatn/common/bytearray.h>
#include <hatn/common/format.h>
#include <hatn/common/fileutils.h>
#include <hatn/common/logger.h>
#include <hatn/common/plainfile.h>
#include <hatn/common/plugin.h>

#include <hatn/crypt/cryptplugin.h>
#include <hatn/crypt/ciphersuite.h>
#include <hatn/crypt/cryptfile.h>

#include <hatn/media/media.h>
#include <hatn/media/mediaerror.h>
#include <hatn/media/audioformat.h>
#include <hatn/media/waveformextractor.h>

#ifdef HATN_MEDIA_HAS_OGG_OPUS
#include <hatn/media/oggopusreader.h>
#include <hatn/media/voicecrop.h>
#include <hatn/media/voiceplayer.h>
#include <hatn/media/voicerecorder.h>
#endif

#include "../../media/test/testmediautils.h"

HATN_MEDIA_USING

namespace common=hatn::common;
namespace hcrypt=hatn::crypt;

using hatn::media::test::MemoryFile;

namespace {

// ---------------------------------------------------------------------------------------------
// crypt: plugin, suite and a master key, made once per process

struct CryptContext
{
    std::shared_ptr<void> pluginInfos;
    std::shared_ptr<hcrypt::CryptPlugin> plugin;
    std::shared_ptr<hcrypt::CipherSuite> suite;
    common::SharedPtr<hcrypt::SymmetricKey> masterKey;

    //! Why the setup failed; empty when it did not.
    std::string failure;
};

CryptContext makeCryptContext()
{
    CryptContext context;

    if (!common::Logger::isRunning())
    {
        auto handler=[](const common::FmtAllocatedBufferChar& s)
        {
            std::cout<<common::lib::toStringView(s)<<std::endl;
        };
        common::Logger::setDefaultVerbosity(common::LoggerVerbosity::INFO);
        common::Logger::setFatalTracing(false);
        common::Logger::setOutputHandler(handler);
        common::Logger::setFatalLogHandler(handler);
        common::Logger::start(false);
    }

    // The plugins found stay loaded while the list lives.
    auto found=common::PluginLoader::instance().listDynamicPlugins("./plugins/crypt");
    context.pluginInfos=std::make_shared<decltype(found)>(std::move(found));

    const common::PluginInfo* openssl=nullptr;
    const auto infos=common::PluginLoader::instance().listPlugins(hcrypt::CryptPlugin::Type);
    for (auto&& info : infos)
    {
        if (info->name.find("openssl")!=std::string::npos)
        {
            openssl=info;
            break;
        }
    }
    if (openssl==nullptr)
    {
        context.failure="the openssl crypt plugin was not found in ./plugins/crypt";
        return context;
    }

    auto loaded=common::PluginLoader::instance().loadPlugin<hcrypt::CryptPlugin>(openssl);
    if (loaded)
    {
        context.failure="failed to load the openssl crypt plugin: "+loaded.error().message();
        return context;
    }
    context.plugin=loaded.takeValue();

    auto ec=context.plugin->init();
    if (ec)
    {
        context.failure="failed to init the openssl crypt plugin: "+ec.message();
        return context;
    }

    // HKDF from a raw key: no passphrase, so opening a file costs no key stretching, which would
    // otherwise drown the timing of the append modes.
    const std::string suiteJson=
        "{\"id\":\"mediatests-suite\",\"aead\":\"chacha20-poly1305\",\"pbkdf\":\"pbkdf2/sha256\","
        "\"digest\":\"sha512\",\"mac\":\"poly1305\",\"hkdf\":\"sha256\"}";
    context.suite=std::make_shared<hcrypt::CipherSuite>();
    ec=context.suite->loadFromJSON(common::ByteArray(suiteJson.data(),suiteJson.size()));
    if (ec)
    {
        context.failure="bad cipher suite: "+ec.message();
        return context;
    }
    hcrypt::CipherSuitesGlobal::instance().addSuite(context.suite);
    hcrypt::CipherSuitesGlobal::instance().setDefaultEngine(std::make_shared<hcrypt::CryptEngine>(context.plugin.get()));

    const hcrypt::CryptAlgorithm* aead=nullptr;
    ec=context.suite->aeadAlgorithm(aead);
    if (ec || aead==nullptr)
    {
        context.failure="the suite has no AEAD algorithm";
        return context;
    }
    context.masterKey=aead->createSymmetricKey();
    if (!context.masterKey)
    {
        context.failure="failed to create the master key";
        return context;
    }
    common::ByteArray keyData;
    keyData.resize(32);
    for (size_t i=0;i<keyData.size();i++)
    {
        keyData.data()[i]=static_cast<char>(i*7+3);
    }
    ec=context.masterKey->importFromBuf(keyData,hcrypt::ContainerFormat::RAW_PLAIN);
    if (ec)
    {
        context.failure="failed to import the master key: "+ec.message();
    }
    return context;
}

bool& cryptContextMade() noexcept
{
    static bool made=false;
    return made;
}

CryptContext& cryptContext()
{
    static CryptContext context=makeCryptContext();
    cryptContextMade()=true;
    return context;
}

struct CryptGlobal
{
    void setup()
    {
    }

    void teardown()
    {
        if (!cryptContextMade())
        {
            return;
        }

        auto& context=cryptContext();
        hcrypt::CipherSuitesGlobal::instance().reset();
        context.masterKey.reset();
        if (context.plugin)
        {
            std::ignore=context.plugin->cleanup();
        }
        common::Logger::stop();
    }
};

}

BOOST_TEST_GLOBAL_FIXTURE(CryptGlobal);

namespace {

#define REQUIRE_CRYPT() BOOST_REQUIRE_MESSAGE(cryptContext().failure.empty(),cryptContext().failure)

std::unique_ptr<hcrypt::CryptFile> makeCryptFile(bool forWriting=false)
{
    auto& context=cryptContext();
    auto file=std::make_unique<hcrypt::CryptFile>(context.masterKey.get(),context.suite.get());
    if (forWriting)
    {
        auto& container=file->processor();
        container.setKdfType(hcrypt::container_descriptor::KdfType::HKDF);
        container.setSalt(std::string("mediatests"));
    }
    return file;
}

std::string testPath(const std::string& name)
{
    auto dir=boost::filesystem::temp_directory_path()/"hatn-mediatests";
    boost::filesystem::create_directories(dir);
    auto path=(dir/name).string();
    std::ignore=common::FileUtils::remove(path);
    return path;
}

common::Error readWhole(common::File& file, std::vector<char>& bytes)
{
    common::Error ec;
    const auto size=file.size(ec);
    if (ec)
    {
        return ec;
    }
    bytes.assign(static_cast<size_t>(size),0);
    size_t done=0;
    while (done<bytes.size())
    {
        const auto got=file.read(bytes.data()+done,bytes.size()-done,ec);
        if (ec)
        {
            return ec;
        }
        if (got==0)
        {
            break;
        }
        done+=got;
    }
    bytes.resize(done);
    return common::Error{};
}

//! Open `path` on `file` for reading and return everything in it.
common::Error readFileBytes(hcrypt::CryptFile& file, const std::string& path, std::vector<char>& bytes)
{
    auto ec=file.open(path,common::File::Mode::read);
    if (ec)
    {
        return ec;
    }
    ec=readWhole(file,bytes);
    common::Error closeError;
    file.close(closeError);
    return ec ? ec : closeError;
}

#ifdef HATN_MEDIA_HAS_OGG_OPUS

//! A tone whose frequency climbs from 200 to 3000 Hz over `frames`: every position is different, which
//! a sine is not, so a seek that lands a few samples off cannot pass for a right one.
std::vector<int16_t> makeChirp(size_t frames)
{
    std::vector<int16_t> pcm(frames);
    const double twoPi=6.283185307179586;
    const double seconds=static_cast<double>(frames)/static_cast<double>(VoiceSampleRate);
    const double f0=200.0;
    const double k=(3000.0-f0)/seconds;
    for (size_t i=0;i<frames;i++)
    {
        const double t=static_cast<double>(i)/static_cast<double>(VoiceSampleRate);
        pcm[i]=static_cast<int16_t>(std::lround(9000.0*std::sin(twoPi*(f0*t+0.5*k*t*t))));
    }
    return pcm;
}

void pushAndProcess(VoiceRecorder& recorder, const std::vector<int16_t>& pcm, size_t chunk=480)
{
    for (size_t offset=0;offset<pcm.size();offset+=chunk)
    {
        const auto n=std::min(chunk,pcm.size()-offset);
        BOOST_REQUIRE_EQUAL(recorder.pushPcm(pcm.data()+offset,n),n);
        auto ec=recorder.process();
        BOOST_REQUIRE_MESSAGE(!ec,ec.message());
    }
}

//! Record `pcm` into a file that is already open, as the recorder glue does.
common::Error recordInto(common::File& file, const std::vector<int16_t>& pcm, VoiceRecording& recording)
{
    VoiceRecorder recorder;
    auto ec=recorder.start(file);
    if (ec)
    {
        return ec;
    }
    pushAndProcess(recorder,pcm);
    return recorder.finish(recording);
}

//! Record `pcm` into a new encrypted file at `path` and close it.
void recordEncrypted(const std::string& path, const std::vector<int16_t>& pcm, VoiceRecording& recording)
{
    auto file=makeCryptFile(true);
    auto ec=file->open(path,common::File::Mode::write_new);
    BOOST_REQUIRE_MESSAGE(!ec,ec.message());
    ec=recordInto(*file,pcm,recording);
    BOOST_REQUIRE_MESSAGE(!ec,ec.message());
    file->close(ec);
    BOOST_REQUIRE_MESSAGE(!ec,ec.message());
}

//! Play a whole message the way the engine does: fill() on one side, pull() on the other.
common::Error playAll(common::File& file, std::vector<int16_t>& pcm, uint64_t& durationMs)
{
    pcm.clear();
    VoicePlayer player(1000);
    auto ec=player.open(file);
    if (ec)
    {
        return ec;
    }
    durationMs=player.durationMs();
    player.play();

    std::vector<int16_t> buffer(960);
    for (size_t guard=0;guard<200000;guard++)
    {
        ec=player.fill();
        if (ec)
        {
            player.close();
            return ec;
        }
        const auto got=player.pull(buffer.data(),buffer.size());
        pcm.insert(pcm.end(),buffer.begin(),buffer.begin()+static_cast<std::ptrdiff_t>(got));
        if (got==0 && player.state()==PlayerState::Ended)
        {
            break;
        }
    }
    player.close();
    return common::Error{};
}

#endif

}

BOOST_AUTO_TEST_SUITE(TestVoiceCryptFile)

#ifdef HATN_MEDIA_HAS_OGG_OPUS

BOOST_AUTO_TEST_CASE(RecordPlayRoundTrip)
{
    REQUIRE_CRYPT();
    const auto path=testPath("roundtrip.dat");

    const auto pcm=hatn::media::test::makeSine(3*VoiceSampleRate);

    // the same audio into memory is the reference
    MemoryFile memory;
    VoiceRecording memoryRecording;
    auto ec=hatn::media::test::recordPcm(memory,pcm,memoryRecording);
    BOOST_REQUIRE_MESSAGE(!ec,ec.message());

    VoiceRecording recording;
    recordEncrypted(path,pcm,recording);
    BOOST_CHECK_EQUAL(recording.durationMs,3000u);
    BOOST_CHECK_EQUAL(recording.waveform.size(),static_cast<size_t>(WaveformExtractor::WaveformBuckets));
    BOOST_CHECK_EQUAL(recording.fileBytes,memoryRecording.fileBytes);

    // what is on the disk is not the Ogg stream
    {
        std::vector<char> raw;
        common::PlainFile plain;
        ec=plain.open(path.c_str(),common::File::Mode::read);
        BOOST_REQUIRE_MESSAGE(!ec,ec.message());
        ec=readWhole(plain,raw);
        BOOST_REQUIRE_MESSAGE(!ec,ec.message());
        const std::string haystack(raw.begin(),raw.end());
        BOOST_CHECK(haystack.find("OggS")==std::string::npos);
        BOOST_CHECK(haystack.find("OpusHead")==std::string::npos);
    }

    // decrypted, it is the same stream that went into memory
    {
        std::vector<char> plain;
        auto file=makeCryptFile();
        ec=readFileBytes(*file,path,plain);
        BOOST_REQUIRE_MESSAGE(!ec,ec.message());
        BOOST_CHECK_EQUAL(plain.size(),static_cast<size_t>(recording.fileBytes));
        BOOST_CHECK(plain==memory.bytes());
    }

    // and plays: through a second handle, the way a listener opens a message
    std::vector<int16_t> played;
    uint64_t durationMs=0;
    {
        auto file=makeCryptFile();
        ec=file->open(path,common::File::Mode::read);
        BOOST_REQUIRE_MESSAGE(!ec,ec.message());
        ec=playAll(*file,played,durationMs);
        BOOST_REQUIRE_MESSAGE(!ec,ec.message());
        file->close(ec);
        BOOST_CHECK(!ec);
    }
    BOOST_CHECK_EQUAL(durationMs,3000u);
    BOOST_CHECK_EQUAL(played.size(),pcm.size());

    std::vector<int16_t> reference;
    ec=hatn::media::test::decodeAll(memory,reference);
    BOOST_REQUIRE_MESSAGE(!ec,ec.message());
    BOOST_REQUIRE_EQUAL(played.size(),reference.size());
    BOOST_CHECK_GT(hatn::media::test::correlation(played.data(),reference.data(),played.size()),0.9999);
    BOOST_TEST_MESSAGE("player and reader decode alike"<<(played==reference ? ", bit for bit" : ", but not bit for bit"));
    BOOST_CHECK_CLOSE(hatn::media::test::toneFrequency(played.data(),played.size()),440.0,3.0);
}

BOOST_AUTO_TEST_CASE(SeekOnCryptFile)
{
    REQUIRE_CRYPT();
    const auto path=testPath("seek.dat");

    // 100 s: the stream is then well over 0x41000 bytes (about 400 KB at 32 kbit/s), so it spans the first
    // chunk (0x1000) and the next one (0x40000) and reaches a third
    const auto pcm=makeChirp(100*VoiceSampleRate);
    VoiceRecording recording;
    recordEncrypted(path,pcm,recording);
    BOOST_CHECK_GT(recording.fileBytes,static_cast<uint64_t>(0x41000));
    BOOST_TEST_MESSAGE("seek test file, plain stream bytes: "<<recording.fileBytes);

    std::vector<int16_t> reference;
    {
        auto file=makeCryptFile();
        auto ec=file->open(path,common::File::Mode::read);
        BOOST_REQUIRE_MESSAGE(!ec,ec.message());
        ec=hatn::media::test::decodeAll(*file,reference);
        BOOST_REQUIRE_MESSAGE(!ec,ec.message());
        file->close(ec);
    }
    BOOST_REQUIRE_EQUAL(reference.size(),pcm.size());

    auto file=makeCryptFile();
    auto ec=file->open(path,common::File::Mode::read);
    BOOST_REQUIRE_MESSAGE(!ec,ec.message());
    OggOpusReader reader;
    ec=reader.open(*file);
    BOOST_REQUIRE_MESSAGE(!ec,ec.message());
    BOOST_CHECK_EQUAL(reader.totalFrames(),static_cast<uint64_t>(pcm.size()));

    const size_t window=4800;
    const std::vector<uint64_t> frames{
        0,1,959,960,48000,77777,500000,1234567,2400000,4000000,static_cast<uint64_t>(pcm.size())-window
    };
    // in a different order from the file's, so that seeks go back as well as forward
    const std::vector<size_t> order{5,0,9,3,10,1,7,2,8,4,6};
    for (auto index : order)
    {
        const auto frame=frames[index];
        ec=reader.seek(frame);
        BOOST_REQUIRE_MESSAGE(!ec,"seek to "<<frame<<": "<<ec.message());

        std::vector<int16_t> got(window);
        size_t gotFrames=0;
        ec=reader.read(got.data(),got.size(),gotFrames);
        BOOST_REQUIRE_MESSAGE(!ec,ec.message());
        BOOST_REQUIRE_EQUAL(gotFrames,window);

        // Not bit for bit: after a seek the decoder is primed by a pre-roll, which brings it close to the
        // state a sequential read has but not exactly there.
        const auto match=hatn::media::test::correlation(got.data(),reference.data()+frame,window);
        BOOST_CHECK_MESSAGE(match>0.999,"the audio after a seek to frame "<<frame
                            <<" is not what a sequential read gives there, correlation "<<match);
    }

    reader.close();
    file->close(ec);
    BOOST_CHECK(!ec);
}

BOOST_AUTO_TEST_CASE(CropOnCryptFile)
{
    REQUIRE_CRYPT();
    const auto source=testPath("crop-source.dat");
    const auto cropped=testPath("crop-result.dat");

    const auto pcm=hatn::media::test::makeSine(3*VoiceSampleRate);
    VoiceRecording recording;
    recordEncrypted(source,pcm,recording);

    VoiceRecording cropRecording;
    {
        auto in=makeCryptFile();
        auto ec=in->open(source,common::File::Mode::read);
        BOOST_REQUIRE_MESSAGE(!ec,ec.message());
        auto out=makeCryptFile(true);
        ec=out->open(cropped,common::File::Mode::write_new);
        BOOST_REQUIRE_MESSAGE(!ec,ec.message());

        ec=cropVoiceMs(*in,*out,1000,2000,cropRecording);
        BOOST_REQUIRE_MESSAGE(!ec,ec.message());
        out->close(ec);
        BOOST_CHECK(!ec);
        in->close(ec);
        BOOST_CHECK(!ec);
    }
    BOOST_CHECK_EQUAL(cropRecording.durationMs,1000u);
    BOOST_CHECK_EQUAL(cropRecording.waveform.size(),static_cast<size_t>(WaveformExtractor::WaveformBuckets));

    std::vector<int16_t> played;
    uint64_t durationMs=0;
    auto file=makeCryptFile();
    auto ec=file->open(cropped,common::File::Mode::read);
    BOOST_REQUIRE_MESSAGE(!ec,ec.message());
    ec=playAll(*file,played,durationMs);
    BOOST_REQUIRE_MESSAGE(!ec,ec.message());
    file->close(ec);

    BOOST_CHECK_EQUAL(durationMs,1000u);
    BOOST_CHECK_EQUAL(played.size(),static_cast<size_t>(VoiceSampleRate));
    BOOST_CHECK_CLOSE(hatn::media::test::toneFrequency(played.data(),played.size()),440.0,3.0);
    BOOST_CHECK_CLOSE(hatn::media::test::rmsLevel(played.data(),played.size()),
                      hatn::media::test::rmsLevel(pcm.data(),pcm.size()),10.0);
}

BOOST_AUTO_TEST_CASE(PausedPreListenAndAppend)
{
    REQUIRE_CRYPT();
    const auto path=testPath("paused.dat");

    // 1.5 s, a pause, 1.5 s more; the second part goes on in phase
    const auto part1=hatn::media::test::makeSine(72000);
    const auto part2=hatn::media::test::makeSine(72000,440.0,10000.0,72000);
    std::vector<int16_t> whole(part1);
    whole.insert(whole.end(),part2.begin(),part2.end());

    auto file=makeCryptFile(true);
    auto ec=file->open(path,common::File::Mode::write_new);
    BOOST_REQUIRE_MESSAGE(!ec,ec.message());

    VoiceRecorder recorder;
    ec=recorder.start(*file);
    BOOST_REQUIRE_MESSAGE(!ec,ec.message());
    pushAndProcess(recorder,part1);
    ec=recorder.pause();
    BOOST_REQUIRE_MESSAGE(!ec,ec.message());
    BOOST_CHECK(recorder.state()==RecorderState::Paused);

    // The file is closed at the pause, so that nothing writes and reads an encrypted file at once.
    const auto sizeAtPause=file->size(ec);
    BOOST_REQUIRE_MESSAGE(!ec,ec.message());
    file->close(ec);
    BOOST_REQUIRE_MESSAGE(!ec,ec.message());
    BOOST_CHECK(!file->isOpen());

    // The pre-listen: a second, read-only handle on the closed file.
    {
        auto listen=makeCryptFile();
        ec=listen->open(path,common::File::Mode::read);
        BOOST_REQUIRE_MESSAGE(!ec,ec.message());

        std::vector<int16_t> played;
        uint64_t durationMs=0;
        ec=playAll(*listen,played,durationMs);
        BOOST_REQUIRE_MESSAGE(!ec,ec.message());
        listen->close(ec);
        BOOST_CHECK(!ec);

        // short by the held back packet and a partial frame: up to about 40 ms, allowed 80
        BOOST_TEST_MESSAGE("paused stream plays "<<durationMs<<" ms of 1500");
        BOOST_CHECK_LE(durationMs,1500u);
        BOOST_CHECK_GE(durationMs,1420u);
        BOOST_CHECK_GE(played.size(),static_cast<size_t>(durationMs*VoiceSampleRate/1000));
        BOOST_CHECK_LT(played.size(),static_cast<size_t>((durationMs+1)*VoiceSampleRate/1000));
        BOOST_CHECK_CLOSE(hatn::media::test::toneFrequency(played.data(),played.size()),440.0,3.0);
    }

    // The SAME object, reopened in append mode, must land exactly at the end.
    ec=file->open(path,common::File::Mode::append_existing);
    BOOST_REQUIRE_MESSAGE(!ec,ec.message());
    BOOST_CHECK_EQUAL(file->size(ec),sizeAtPause);
    BOOST_CHECK_EQUAL(file->pos(ec),sizeAtPause);

    ec=recorder.resume();
    BOOST_REQUIRE_MESSAGE(!ec,ec.message());
    pushAndProcess(recorder,part2);
    VoiceRecording recording;
    ec=recorder.finish(recording);
    BOOST_REQUIRE_MESSAGE(!ec,ec.message());
    file->close(ec);
    BOOST_REQUIRE_MESSAGE(!ec,ec.message());

    BOOST_CHECK_EQUAL(recording.durationMs,3000u);

    // Played back it is one message of 3 s, and the same as one recorded without the pause.
    MemoryFile memory;
    VoiceRecording memoryRecording;
    ec=hatn::media::test::recordPcm(memory,whole,memoryRecording);
    BOOST_REQUIRE_MESSAGE(!ec,ec.message());
    std::vector<int16_t> reference;
    ec=hatn::media::test::decodeAll(memory,reference);
    BOOST_REQUIRE_MESSAGE(!ec,ec.message());

    std::vector<int16_t> played;
    uint64_t durationMs=0;
    auto reader=makeCryptFile();
    ec=reader->open(path,common::File::Mode::read);
    BOOST_REQUIRE_MESSAGE(!ec,ec.message());
    ec=playAll(*reader,played,durationMs);
    BOOST_REQUIRE_MESSAGE(!ec,ec.message());
    reader->close(ec);

    BOOST_CHECK_EQUAL(durationMs,3000u);
    BOOST_REQUIRE_EQUAL(played.size(),reference.size());
    const auto match=hatn::media::test::correlation(played.data(),reference.data(),played.size());
    BOOST_TEST_MESSAGE("paused and unpaused recordings decode alike: correlation "<<match<<(played==reference ? ", bit for bit" : ", not bit for bit"));
    BOOST_CHECK_GT(match,0.999);
}

#endif // HATN_MEDIA_HAS_OGG_OPUS

BOOST_AUTO_TEST_CASE(AppendExistingLandsAtEnd)
{
    REQUIRE_CRYPT();
    const auto path=testPath("append.dat");

    // around the ends of the first chunk (0x1000) and of the second (0x1000+0x40000)
    const std::vector<size_t> sizes{1,0x1000-1,0x1000,0x1000+1,100000,0x41000-1,0x41000,0x41000+7};
    auto file=makeCryptFile(true);
    for (auto size : sizes)
    {
        std::ignore=common::FileUtils::remove(path);

        std::vector<char> first(size);
        for (size_t i=0;i<first.size();i++)
        {
            first[i]=static_cast<char>((i*31+7)&0xff);
        }
        std::vector<char> second(50000);
        for (size_t i=0;i<second.size();i++)
        {
            second[i]=static_cast<char>((i*17+size)&0xff);
        }

        common::Error ec;
        ec=file->open(path,common::File::Mode::write_new);
        BOOST_REQUIRE_MESSAGE(!ec,ec.message());
        BOOST_REQUIRE_EQUAL(file->write(first.data(),first.size(),ec),first.size());
        BOOST_REQUIRE_MESSAGE(!ec,ec.message());
        file->close(ec);
        BOOST_REQUIRE_MESSAGE(!ec,ec.message());

        ec=file->open(path,common::File::Mode::append_existing);
        BOOST_REQUIRE_MESSAGE(!ec,"append_existing at "<<size<<": "<<ec.message());
        BOOST_CHECK_EQUAL(file->size(ec),static_cast<uint64_t>(size));
        BOOST_CHECK_EQUAL(file->pos(ec),static_cast<uint64_t>(size));
        BOOST_REQUIRE_EQUAL(file->write(second.data(),second.size(),ec),second.size());
        BOOST_REQUIRE_MESSAGE(!ec,ec.message());
        BOOST_CHECK_EQUAL(file->pos(ec),static_cast<uint64_t>(size)+second.size());
        file->close(ec);
        BOOST_REQUIRE_MESSAGE(!ec,ec.message());

        std::vector<char> expected(first);
        expected.insert(expected.end(),second.begin(),second.end());
        std::vector<char> got;
        auto reader=makeCryptFile();
        ec=readFileBytes(*reader,path,got);
        BOOST_REQUIRE_MESSAGE(!ec,ec.message());
        BOOST_CHECK_MESSAGE(got==expected,"appending "<<second.size()<<" bytes to "<<size<<" did not give the two parts one after the other");
    }
}

BOOST_AUTO_TEST_CASE(AppendModeSpeed)
{
    REQUIRE_CRYPT();
    const auto path=testPath("appendspeed.dat");

    // 1.2 MB is about five minutes of voice at the default 32 kbit/s
    std::vector<char> base(1200000);
    for (size_t i=0;i<base.size();i++)
    {
        base[i]=static_cast<char>((i*131+(i>>8))&0xff);
    }
    std::vector<char> extra(4096);
    for (size_t i=0;i<extra.size();i++)
    {
        extra[i]=static_cast<char>(i&0xff);
    }

    auto file=makeCryptFile(true);
    common::Error ec;
    ec=file->open(path,common::File::Mode::write_new);
    BOOST_REQUIRE_MESSAGE(!ec,ec.message());
    BOOST_REQUIRE_EQUAL(file->write(base.data(),base.size(),ec),base.size());
    BOOST_REQUIRE_MESSAGE(!ec,ec.message());
    file->close(ec);
    BOOST_REQUIRE_MESSAGE(!ec,ec.message());

    using clock=std::chrono::steady_clock;
    const auto micros=[](clock::time_point from)
    {
        return std::chrono::duration_cast<std::chrono::microseconds>(clock::now()-from).count();
    };

    // baseline: opening for reading
    auto begin=clock::now();
    ec=file->open(path,common::File::Mode::read);
    BOOST_REQUIRE_MESSAGE(!ec,ec.message());
    file->close(ec);
    BOOST_TEST_MESSAGE("open for reading and close: "<<micros(begin)<<" us");

    // the mode the recorder glue uses
    begin=clock::now();
    ec=file->open(path,common::File::Mode::append_existing);
    BOOST_REQUIRE_MESSAGE(!ec,ec.message());
    const auto openedAfter=micros(begin);
    BOOST_REQUIRE_EQUAL(file->write(extra.data(),extra.size(),ec),extra.size());
    BOOST_REQUIRE_MESSAGE(!ec,ec.message());
    file->close(ec);
    BOOST_REQUIRE_MESSAGE(!ec,ec.message());
    const auto appendTotal=micros(begin);
    BOOST_TEST_MESSAGE("append_existing: open "<<openedAfter<<" us, open + 4 KB + close "<<appendTotal<<" us");
    BOOST_CHECK_LT(appendTotal,2000000);

    // the fallback: write_existing and a seek to the end
    begin=clock::now();
    ec=file->open(path,common::File::Mode::write_existing);
    BOOST_REQUIRE_MESSAGE(!ec,ec.message());
    const auto endPosition=file->size(ec);
    BOOST_CHECK_EQUAL(endPosition,static_cast<uint64_t>(base.size()+extra.size()));
    ec=file->seek(endPosition);
    BOOST_REQUIRE_MESSAGE(!ec,ec.message());
    BOOST_CHECK_EQUAL(file->pos(ec),endPosition);
    BOOST_REQUIRE_EQUAL(file->write(extra.data(),extra.size(),ec),extra.size());
    BOOST_REQUIRE_MESSAGE(!ec,ec.message());
    file->close(ec);
    BOOST_REQUIRE_MESSAGE(!ec,ec.message());
    const auto fallbackTotal=micros(begin);
    BOOST_TEST_MESSAGE("write_existing + seek: open + 4 KB + close "<<fallbackTotal<<" us");
    BOOST_CHECK_LT(fallbackTotal,2000000);

    // both landed where they should
    std::vector<char> expected(base);
    expected.insert(expected.end(),extra.begin(),extra.end());
    expected.insert(expected.end(),extra.begin(),extra.end());
    std::vector<char> got;
    auto reader=makeCryptFile();
    ec=readFileBytes(*reader,path,got);
    BOOST_REQUIRE_MESSAGE(!ec,ec.message());
    BOOST_CHECK(got==expected);
}

BOOST_AUTO_TEST_CASE(ReopenAfterFailedOpen)
{
    REQUIRE_CRYPT();
    const auto missing=testPath("does-not-exist.dat");
    const auto path=testPath("reopen.dat");

    auto file=makeCryptFile(true);

    // what the recorder glue meets if the file was removed while a recording was paused
    auto ec=file->open(missing,common::File::Mode::append_existing);
    BOOST_CHECK(ec);
    BOOST_CHECK(!file->isOpen());

    // the same object is not left unusable
    ec=file->open(path,common::File::Mode::write_new);
    BOOST_REQUIRE_MESSAGE(!ec,ec.message());
    const std::string text="still works";
    BOOST_REQUIRE_EQUAL(file->write(text.data(),text.size(),ec),text.size());
    BOOST_REQUIRE_MESSAGE(!ec,ec.message());
    file->close(ec);
    BOOST_REQUIRE_MESSAGE(!ec,ec.message());

    std::vector<char> got;
    auto reader=makeCryptFile();
    ec=readFileBytes(*reader,path,got);
    BOOST_REQUIRE_MESSAGE(!ec,ec.message());
    BOOST_CHECK_EQUAL(std::string(got.begin(),got.end()),text);
}

BOOST_AUTO_TEST_SUITE_END()
