#include <boost/test/unit_test.hpp>
#include <boost/filesystem.hpp>

#include <hatn/test/multithreadfixture.h>

#include <hatn/common/bytearray.h>
#include <hatn/common/plainfile.h>

HATN_USING
HATN_COMMON_USING
HATN_TEST_USING

BOOST_AUTO_TEST_SUITE(TestFile)

static const ByteArray& sample() {static ByteArray b("Hello from hatn! Line 1\nHello from hatn! Line 2\nHello from hatn! Line 3\n"); return b;}
static const ByteArray& sampleLine1() {static ByteArray b("Hello from hatn! Line 1\n"); return b;}
static const ByteArray& sample2() { static ByteArray b("Hello from hatn! Line 2\nHello from hatn! Line 3\n"); return b;}

static void testRead(const std::string& filename)
{
    PlainFile file0;
    file0.setFilename("blabla.txt");
    File& f0=file0;
    BOOST_CHECK(!f0.isOpen());
    auto ec=f0.open(File::Mode::read);
    BOOST_CHECK(ec);
    BOOST_CHECK(!f0.isOpen());

    PlainFile file1;
    file1.setFilename(filename);
    File& f1=file1;
    BOOST_CHECK(!f1.isOpen());
    ec=f1.open(File::Mode::read);
    BOOST_CHECK(!ec);
    BOOST_CHECK(f1.isOpen());
    ByteArray b1;
    auto size= static_cast<size_t>(f1.size());
    BOOST_CHECK_EQUAL(size,sample().size());
    b1.resize(size);
    auto readSize=f1.read(b1.data(),b1.size(),ec);
    BOOST_CHECK(!ec);
    BOOST_CHECK_EQUAL(size,readSize);
    BOOST_CHECK(b1==sample());
    f1.close();
    BOOST_CHECK(!f1.isOpen());

    PlainFile file2;
    File& f2=file2;
    BOOST_CHECK(!f2.isOpen());
    ec=f2.open(filename,File::Mode::read);
    BOOST_CHECK(!ec);
    BOOST_CHECK(f2.isOpen());
    ec=f2.seek(sampleLine1().size());
    BOOST_CHECK(!ec);
    auto pos=f2.pos(ec);
    BOOST_CHECK(!ec);
    BOOST_CHECK_EQUAL(pos,sampleLine1().size());
    ByteArray b2;
    size=static_cast<size_t>(f2.size());
    BOOST_CHECK_EQUAL(size,sample().size());
    b2.resize(size);
    readSize=f2.read(b2.data(),size,ec);
    BOOST_CHECK(!ec);
    BOOST_CHECK_EQUAL(readSize,sample2().size());
    b2.resize(readSize);
    BOOST_CHECK(b2==sample2());
    f2.close();
    BOOST_CHECK(!f2.isOpen());
}

BOOST_AUTO_TEST_CASE(TestFileRead)
{
    std::string filename=fmt::format("{}/common/assets/file.dat",test::MultiThreadFixture::assetsPath());
    testRead(filename);
}

BOOST_AUTO_TEST_CASE(TestFileWrite)
{
    std::string filename=fmt::format("{}/testfilewrite1.dat",test::MultiThreadFixture::tmpPath());

    PlainFile file1;
    file1.setFilename(filename);
    File& f1=file1;
    BOOST_CHECK(!f1.isOpen());
    auto ec=f1.open(File::Mode::write);
    BOOST_CHECK(!ec);
    BOOST_CHECK(f1.isOpen());
    auto writeSize=f1.write(sample().data(),sample().size(),ec);
    BOOST_CHECK(!ec);
    BOOST_CHECK_EQUAL(sample().size(),writeSize);
    BOOST_CHECK_EQUAL(f1.size(),sample().size());
    f1.close();
    BOOST_CHECK(!f1.isOpen());
    testRead(filename);

    PlainFile file2;
    File& f2=file2;
    BOOST_CHECK(!f2.isOpen());
    ec=f2.open(filename,File::Mode::write_existing);
    BOOST_CHECK(!ec);
    BOOST_CHECK(f2.isOpen());
    ec=f2.seek(sampleLine1().size());
    BOOST_CHECK(!ec);
    auto pos=f2.pos(ec);
    BOOST_CHECK(!ec);
    BOOST_CHECK_EQUAL(pos,sampleLine1().size());
    writeSize=f2.write(sample2().data(),sample2().size(),ec);
    BOOST_CHECK(!ec);
    BOOST_CHECK_EQUAL(sample2().size(),writeSize);
    BOOST_CHECK_EQUAL(f2.size(),sample().size());
    f2.close();
    BOOST_CHECK(!f2.isOpen());
    testRead(filename);

    PlainFile file3;
    File& f3=file3;
    BOOST_CHECK(!f3.isOpen());
    ec=f3.open(filename,File::Mode::write);
    BOOST_CHECK(!ec);
    BOOST_CHECK(f3.isOpen());
    BOOST_CHECK_EQUAL(f3.size(),0);
    writeSize=f3.write(sampleLine1().data(),sampleLine1().size(),ec);
    BOOST_CHECK(!ec);
    BOOST_CHECK_EQUAL(sampleLine1().size(),writeSize);
    BOOST_CHECK_EQUAL(f3.size(),sampleLine1().size());
    f3.close();
    BOOST_CHECK(!f3.isOpen());

    PlainFile file4;
    File& f4=file4;
    BOOST_CHECK(!f4.isOpen());
    ec=f4.open(filename,File::Mode::append_existing);
    BOOST_CHECK(!ec);
    BOOST_CHECK(f4.isOpen());
    BOOST_CHECK_EQUAL(f4.size(),sampleLine1().size());
    ec=f4.seek(sampleLine1().size());
    BOOST_CHECK(!ec);
    pos=f4.pos(ec);
    BOOST_CHECK(!ec);
    BOOST_CHECK_EQUAL(pos,sampleLine1().size());
    writeSize=f4.write(sample2().data(),sample2().size(),ec);
    BOOST_CHECK(!ec);
    BOOST_CHECK_EQUAL(sample2().size(),writeSize);
    BOOST_CHECK_EQUAL(f4.size(),sample().size());
    f4.close();
    BOOST_CHECK(!f4.isOpen());
    testRead(filename);

    boost::filesystem::remove(filename);
}

BOOST_AUTO_TEST_SUITE_END()
